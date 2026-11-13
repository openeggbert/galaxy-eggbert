#!/usr/bin/env python3
"""Check the packaged executable and every bundled DLL's imports, without running Windows."""

from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tempfile
import zipfile


# System libraries stay on Windows; third-party runtimes must be in the package.
SYSTEM_DLLS = {
    "kernel32.dll", "user32.dll", "gdi32.dll", "advapi32.dll", "shell32.dll",
    "ole32.dll", "oleaut32.dll", "ws2_32.dll", "winmm.dll", "imm32.dll",
    "version.dll", "setupapi.dll", "crypt32.dll", "bcrypt.dll", "msvcrt.dll",
    "ucrtbase.dll", "opengl32.dll", "dwmapi.dll", "cfgmgr32.dll", "hid.dll",
    "shlwapi.dll", "iphlpapi.dll", "wldap32.dll", "normaliz.dll", "secur32.dll",
    "dnsapi.dll",
}


def check(archive):
    with zipfile.ZipFile(archive) as bundle, tempfile.TemporaryDirectory() as work:
        binaries = {}
        roots = set()
        for entry in bundle.infolist():
            path = PurePosixPath(entry.filename)
            if path.suffix.lower() not in {".exe", ".dll"}:
                continue
            if len(path.parts) != 2 or path.is_absolute() or ".." in path.parts:
                raise ValueError(f"binary outside the bundle's root: {entry.filename}")
            roots.add(path.parts[0])
            name = path.name.lower()
            if name in binaries:
                raise ValueError(f"duplicate binary: {entry.filename}")
            target = Path(work) / path.name
            target.write_bytes(bundle.read(entry))
            binaries[name] = target
        if len(roots) != 1 or "galaxyeggbert.exe" not in binaries:
            raise ValueError("expected one bundle containing GalaxyEggbert.exe")
        for name, path in sorted(binaries.items()):
            result = subprocess.run(
                ["x86_64-w64-mingw32-objdump", "-p", str(path)],
                check=True, capture_output=True, text=True,
            )
            imports = re.findall(r"DLL Name:\s*(\S+)", result.stdout)
            if not imports:
                raise ValueError(f"no PE imports found in {name}")
            print(f"{name}: {', '.join(imports)}", flush=True)
            for imported in imports:
                dll = imported.lower()
                if dll in binaries or dll in SYSTEM_DLLS:
                    continue
                if re.fullmatch(r"api-ms-win-crt-[a-z0-9-]+\.dll", dll):
                    continue
                raise ValueError(f"{name} imports unbundled non-system DLL {imported}")


if __name__ == "__main__":
    try:
        check(sys.argv[1])
    except (IndexError, OSError, ValueError, zipfile.BadZipFile, subprocess.CalledProcessError) as error:
        sys.exit(f"Windows package check: {error}")
