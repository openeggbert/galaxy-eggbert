#!/usr/bin/env python3
"""At which pitch the game played each sound effect, measured in a recording of its output (BD-0031).

The original plays every channel at its own pitch (mobile-eggbert Sound.cpp `tableVolumePitch`,
ported as `kVolumePitch` in src/GalaxyEggbert/Game/Sound.cpp). This measures what the game actually
sent to the sound card, without playing anything aloud: record a playthrough with SDL's disk audio
driver, in real time (the driver writes only what the audio thread mixes as the game runs):

    mkdir -p build-probe/audio-check
    AUTOPLAY_REALTIME=1 AUTOPLAY_RUN=$PWD/build-probe/audio-check QA_SOUND=1 \
      SDL_AUDIODRIVER=disk SDL_AUDIO_DRIVER=disk \
      SDL_AUDIO_DISK_OUTPUT_FILE=$PWD/build-probe/audio-check/out.raw \
      tools/autoplay.sh build-cna-opengl3 tools/autoplay/world012.txt 12
    python3 tools/audit/sound_pitch.py build-probe/audio-check/out.raw

The recording is S16LE stereo 44100 Hz (the game's mixer format; SDL prints it). Each sound file
is resampled to every pitch from 0 to 1 octave in tenths and to its table pitch, and searched for
in the recording by normalised cross-correlation (its first second). A channel is reported heard at
its table pitch when it is there with ncc >= 0.95; "ONLY at pitch" flags one found only elsewhere
(a short sound may also resemble another channel's: check the time against the others).

    usage: sound_pitch.py RECORDING [SOUNDS_DIR] [SOUND_CPP]
"""
import re, sys, wave
import numpy as np

raw = sys.argv[1]
sounds_dir = sys.argv[2] if len(sys.argv) > 2 else 'Content/sounds'
sound_cpp = sys.argv[3] if len(sys.argv) > 3 else 'src/GalaxyEggbert/Game/Sound.cpp'
rec = np.frombuffer(open(raw, 'rb').read(), dtype='<i2').astype(np.float64)
rec = rec.reshape(-1, 2).mean(axis=1)
RATE = 44100
table = [float(v) for v in re.findall(r'(-?\d+\.\d+)f', open(sound_cpp).read().split('kVolumePitch[200] = {')[1].split('};')[0])]
n = len(rec)
size = 1 << int(np.ceil(np.log2(n + RATE * 4)))
spec = np.fft.rfft(rec, size)
energy = np.concatenate([[0.0], np.cumsum(rec * rec)])


def load(path):
    w = wave.open(path)
    rate, ch, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
    data = w.readframes(w.getnframes())
    if width == 2:
        a = np.frombuffer(data, dtype='<i2').astype(np.float64)
    elif width == 1:
        a = np.frombuffer(data, dtype=np.uint8).astype(np.float64) - 128.0
    else:
        return None, rate
    return a.reshape(-1, ch).mean(axis=1), rate


results = []
for chan in range(len(table) // 2):
    try:
        src, rate = load(f'{sounds_dir}/sound{chan:03d}.wav')
    except Exception:
        continue
    if src is None or len(src) < 200 or not np.any(src):
        continue
    want = max(0.0, table[chan * 2 + 1])
    scores = {}
    for pitch in sorted({tenth / 10.0 for tenth in range(0, 11)} | {want}):
        step = (2.0 ** pitch) * rate / RATE          # source samples per output sample
        pos = np.arange(0, len(src) - 1, step)
        t = np.interp(pos, np.arange(len(src)), src)
        t = t[: min(len(t), RATE)]                   # the first second is enough
        m = len(t)
        if m >= n:
            continue
        corr = np.fft.irfft(spec * np.conj(np.fft.rfft(t, size)), size)[: n - m + 1]
        local = np.sqrt(np.maximum(energy[m:] - energy[:-m], 1e-9))
        ncc = corr / (np.linalg.norm(t) * local)
        k = int(np.argmax(ncc))
        scores[pitch] = (float(ncc[k]), k / RATE)
    if want not in scores:
        continue
    best = max(scores, key=lambda q: scores[q][0])
    results.append((chan, want, scores[want], best, scores[best]))

# A channel is heard at its table pitch when the recording holds it there (ncc >= 0.95); a better
# match at another pitch below that is another, similar sound overlapping.
for chan, want, (at_want, t_want), best, (at_best, t_best) in sorted(results, key=lambda r: -r[2][0]):
    if at_want >= 0.95:
        print(f'ch{chan:3d} table pitch {want:.1f}: heard there, ncc {at_want:.2f} at {t_want:7.2f}s'
              + ('' if best == want or at_best - at_want < 0.01 else f' (pitch {best:.1f}: {at_best:.2f})'))
    elif at_best >= 0.95:
        print(f'ch{chan:3d} table pitch {want:.1f}: ONLY at pitch {best:.1f}, ncc {at_best:.2f} at {t_best:7.2f}s '
              f'(table pitch {at_want:.2f})')
