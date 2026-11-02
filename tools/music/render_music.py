#!/usr/bin/env python3
"""Renders the original Speedy Eggbert MIDI music to WAV for Galaxy Eggbert (OR-0014).

The original game's ten background tracks are Standard MIDI files named SOUND/MUSIC000.BLP ..
MUSIC009.BLP (Speedy Eggbert 1, 1998). This renders each through libfluidsynth (its C API via
ctypes: no fluidsynth command is needed, only the library) with a General MIDI soundfont, as
fast as the CPU allows, into 44.1 kHz 16-bit stereo WAV files.

    python3 tools/music/render_music.py <midi...> --out <dir> [--soundfont /usr/share/sounds/sf2/default-GM.sf2] [--ogg]

The output name is music<NNN>.wav from the input's MUSIC<NNN> number (else the input's stem).
With --ogg (needs ffmpeg) the WAV becomes music<NNN>.ogg (Vorbis q4, what the game ships: CNA
decodes it with stb_vorbis): the trailing silence some files carry (music000 ends with three
empty minutes) is cut and the loudness normalised (EBU R128, -18 LUFS, -1.5 dBTP), then the
WAV is removed. A WAV of a track is ~64 MB; its OGG ~3 MB.
"""
import argparse
import ctypes
import subprocess
import ctypes.util
import re
import sys
from pathlib import Path

FLUID_OK = 0
FLUID_PLAYER_PLAYING = 1


def load_library():
    name = ctypes.util.find_library("fluidsynth") or "libfluidsynth.so.3"
    lib = ctypes.CDLL(name)
    vp = ctypes.c_void_p
    lib.new_fluid_settings.restype = vp
    lib.fluid_settings_setstr.argtypes = [vp, ctypes.c_char_p, ctypes.c_char_p]
    lib.fluid_settings_setint.argtypes = [vp, ctypes.c_char_p, ctypes.c_int]
    lib.fluid_settings_setnum.argtypes = [vp, ctypes.c_char_p, ctypes.c_double]
    lib.new_fluid_synth.argtypes = [vp]
    lib.new_fluid_synth.restype = vp
    lib.fluid_synth_sfload.argtypes = [vp, ctypes.c_char_p, ctypes.c_int]
    lib.new_fluid_player.argtypes = [vp]
    lib.new_fluid_player.restype = vp
    lib.fluid_player_add.argtypes = [vp, ctypes.c_char_p]
    lib.fluid_player_play.argtypes = [vp]
    lib.fluid_player_get_status.argtypes = [vp]
    lib.new_fluid_file_renderer.argtypes = [vp]
    lib.new_fluid_file_renderer.restype = vp
    lib.fluid_file_renderer_process_block.argtypes = [vp]
    lib.delete_fluid_file_renderer.argtypes = [vp]
    lib.delete_fluid_player.argtypes = [vp]
    lib.delete_fluid_synth.argtypes = [vp]
    lib.delete_fluid_settings.argtypes = [vp]
    return lib


def render(lib, midi: Path, wav: Path, soundfont: Path) -> None:
    settings = lib.new_fluid_settings()
    lib.fluid_settings_setstr(settings, b"audio.file.name", str(wav).encode())
    lib.fluid_settings_setstr(settings, b"audio.file.type", b"wav")
    lib.fluid_settings_setstr(settings, b"audio.file.format", b"s16")
    lib.fluid_settings_setstr(settings, b"player.timing-source", b"sample")
    lib.fluid_settings_setint(settings, b"synth.lock-memory", 0)
    lib.fluid_settings_setnum(settings, b"synth.sample-rate", 44100.0)
    synth = lib.new_fluid_synth(settings)
    if lib.fluid_synth_sfload(synth, str(soundfont).encode(), 1) < 0:
        raise RuntimeError(f"cannot load soundfont {soundfont}")
    player = lib.new_fluid_player(synth)
    if lib.fluid_player_add(player, str(midi).encode()) != FLUID_OK:
        raise RuntimeError(f"cannot read {midi}")
    lib.fluid_player_play(player)
    renderer = lib.new_fluid_file_renderer(synth)
    if not renderer:
        raise RuntimeError("libfluidsynth has no file renderer (libsndfile support)")
    while lib.fluid_player_get_status(player) == FLUID_PLAYER_PLAYING:
        if lib.fluid_file_renderer_process_block(renderer) != FLUID_OK:
            break
    lib.delete_fluid_file_renderer(renderer)
    lib.delete_fluid_player(player)
    lib.delete_fluid_synth(synth)
    lib.delete_fluid_settings(settings)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("midi", nargs="+", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--soundfont", type=Path, default=Path("/usr/share/sounds/sf2/default-GM.sf2"))
    ap.add_argument("--ogg", action="store_true", help="trim, normalise and encode to OGG Vorbis (ffmpeg)")
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    lib = load_library()
    for midi in args.midi:
        m = re.search(r"MUSIC(\d{3})", midi.name, re.IGNORECASE)
        wav = args.out / (f"music{m.group(1)}.wav" if m else midi.stem + ".wav")
        render(lib, midi, wav, args.soundfont)
        if args.ogg:
            ogg = wav.with_suffix(".ogg")
            trim = "areverse,silenceremove=start_periods=1:start_threshold=-60dB:start_silence=0.5,areverse"
            subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(wav), "-af",
                            f"{trim},loudnorm=I=-18:TP=-1.5:LRA=11", "-ar", "44100", "-c:a", "libvorbis",
                            "-q:a", "4", str(ogg)], check=True)
            wav.unlink()
            wav = ogg
        print(f"{midi} -> {wav} ({wav.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
