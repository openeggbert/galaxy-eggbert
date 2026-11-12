"""GDB observer for camera_probe.py: index rendered frames and omit diagnostic PNG exports.

Source with GDB on the Linux x86-64 build. AUTOPLAY_SHOTS=1 forces every update
to draw. At Texture2D::SaveAsPng entry, capture the screenshot tick and the
already-flushed interpolation trace byte offset. Return from this void export
function for unselected screenshots only: simulation, camera, rendering and
backbuffer readback have already run normally. Never patch game source/binary.
CAMERA_KEEP_EVERY and CAMERA_KEEP_TICKS select actual PNG exports.
"""
import os
import csv
import re
import struct
import gdb

_index = open("draw_ticks.csv", "w", buffering=1)
_index.write("tick,trace_bytes,retained\n")
_every = int(os.environ.get("CAMERA_KEEP_EVERY", "600"))
_keep = set(map(int, filter(None, os.environ.get("CAMERA_KEEP_TICKS", "").split(","))))

if os.environ.get("CAMERA_TICK_OBSERVER") == "1":
    _camera = open("camera_ticks.csv", "w", buffering=1)
    _writer = csv.writer(_camera)
    _writer.writerow(("tick", "eye_x", "eye_y", "eye_z", "target_x", "target_y", "target_z", "caller"))
    _shakes = open("shake_ticks.csv", "w", buffering=1)
    _shakes.write("tick,x,y\n")
    _shake_pointer = None
    _state = None
    _tick = -1

    class ShakeAddress(gdb.Breakpoint):
        def stop(self):
            global _shake_pointer
            _shake_pointer = int(gdb.parse_and_eval("$rdi"))
            self.enabled = False
            return False

    ShakeAddress("GalaxyEggbert::Game::CameraShake::Update(float)")

    class CameraTick(gdb.Breakpoint):
        def stop(self):
            global _state, _tick
            if _state is None:
                if not os.path.exists("autoplay_trace.txt"):
                    return False
                _state = open("autoplay_trace.txt")
            for line in _state.readlines():
                match = re.match(r"t=(\d+) ", line)
                if match:
                    _tick = int(match[1])
            if _tick < 0:
                return False
            inferior = gdb.selected_inferior()
            eye = struct.unpack("<3f", inferior.read_memory(int(gdb.parse_and_eval("$rsi")), 12))
            target = struct.unpack("<3f", inferior.read_memory(int(gdb.parse_and_eval("$rdx")), 12))
            caller = gdb.newest_frame().older().name() or "unknown"
            if not caller.startswith("GalaxyEggbert::CNA::GalaxyEggbertGame::UpdateTickCamera("):
                return False
            _writer.writerow((_tick + 1, *eye, *target, caller))
            if _shake_pointer is not None:
                # CameraShake.hpp: enum/int/float then offsetX/offsetY, all 4B,
                # standard-layout on this x86-64 build. Read only, after Update.
                sx, sy = struct.unpack("<2f", inferior.read_memory(_shake_pointer + 12, 8))
                _shakes.write(f"{_tick + 1},{sx},{sy}\n")
            return False

    CameraTick("CnaEngine::Graphics::Camera::SetLookAt(Microsoft::Xna::Framework::Vector3 const&, Microsoft::Xna::Framework::Vector3 const&, Microsoft::Xna::Framework::Vector3 const&)")


class Capture(gdb.Breakpoint):
    def stop(self):
        # SysV x86-64: this in RDI, const std::string& filename in RSI.
        address = int(gdb.parse_and_eval("$rsi"))
        pointer = int.from_bytes(gdb.selected_inferior().read_memory(address, 8), "little")
        filename = gdb.Value(pointer).cast(gdb.lookup_type("char").pointer()).string()
        skip = False
        if filename.startswith("autoplay_shots/"):
            tick = int(os.path.basename(filename).split(".")[0])
            retain = tick in _keep or (_every > 0 and tick % _every == 0)
            trace = os.environ["GALAXY_INTERP_TRACE"]
            size = os.stat(trace).st_size if os.path.exists(trace) else 0
            _index.write(f"{tick},{size},{int(retain)}\n")
            skip = not retain
        gdb.set_convenience_variable("camera_skip", int(skip))
        return True


_break = Capture("_ZNK9Microsoft3Xna9Framework8Graphics9Texture2D9SaveAsPngERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE")
gdb.execute(f"commands {_break.number}\nsilent\nif $camera_skip\nreturn (void)0\nend\ncontinue\nend")
