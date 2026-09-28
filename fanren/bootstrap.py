import hashlib
import pathlib
import shutil
import urllib.request
import zipfile
import tarfile

ROOT = pathlib.Path(__file__).resolve().parent
# Reuse already-downloaded, SHA256-verified archives from the P0 probe (or the
# archived fanren-sdl3 prototype) instead of re-downloading them.
REUSE_VENDORS = (
    ROOT.parent / "probe" / "vendor",
    ROOT.parent / "_archive" / "fanren-sdl3" / "vendor",
)

# (archive_name, url, sha256, extracted_dir_name_or_None, kind)
#   kind: "zip"  -> zip archive, extract into vendor/
#         "targz" -> tar.gz archive, extract into vendor/
#         "file" -> plain file, just verify + keep as-is (no extraction)
PACKAGES = (
    (
        "SDL3-devel-3.4.16-VC.zip",
        "https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-devel-3.4.16-VC.zip",
        "1a784cb2a5c64d56fe7a62090fe9d242d9865f235e4ea9678f1a6ba4e693e7de",
        "SDL3-3.4.16",
        "zip",
    ),
    (
        "SDL3_ttf-devel-3.2.2-VC.zip",
        "https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-devel-3.2.2-VC.zip",
        "67805c5babfc49ca0c56882dc9b8cabbcdd1e6f9edde10ddac91ddb38f3afb8c",
        "SDL3_ttf-3.2.2",
        "zip",
    ),
    (
        "SDL3_image-devel-3.4.6-VC.zip",
        "https://github.com/libsdl-org/SDL_image/releases/download/release-3.4.6/SDL3_image-devel-3.4.6-VC.zip",
        "03c6b313623edadf707a7c187e2036a5be5f12e693025c0697833379970bb4c0",
        "SDL3_image-3.4.6",
        "zip",
    ),
    (
        "SDL3_mixer-devel-3.2.4-VC.zip",
        "https://github.com/libsdl-org/SDL_mixer/releases/download/release-3.2.4/SDL3_mixer-devel-3.2.4-VC.zip",
        "f4263ed5082fb7018059d64952017534e26821e9e878ce6b8c924b77cb17c4fb",
        "SDL3_mixer-3.2.4",
        "zip",
    ),
    (
        "lua-5.4.9.tar.gz",
        "https://www.lua.org/ftp/lua-5.4.9.tar.gz",
        "2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6",
        "lua-5.4.9",
        "targz",
    ),
    (
        "sol2-3.3.0.zip",
        "https://github.com/ThePhD/sol2/archive/refs/tags/v3.3.0.zip",
        "a7489629c596c8a67108ad3603cb6a90073ba6647e50441c8c55492254190d67",
        "sol2-3.3.0",
        "zip",
    ),
    (
        "json.hpp",
        "https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp",
        "aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63",
        None,
        "file",
    ),
    (
        "googletest-1.15.2.tar.gz",
        "https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz",
        "7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926",
        "googletest-1.15.2",
        "targz",
    ),
)


def sha256_of(path: pathlib.Path) -> str:
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def safe_extract_zip(archive: pathlib.Path, vendor: pathlib.Path) -> None:
    with zipfile.ZipFile(archive) as package:
        for entry in package.infolist():
            target = (vendor / entry.filename).resolve()
            if not target.is_relative_to(vendor.resolve()):
                raise RuntimeError("Invalid archive entry")
        package.extractall(vendor)


def safe_extract_targz(archive: pathlib.Path, vendor: pathlib.Path) -> None:
    with tarfile.open(archive, "r:gz") as package:
        for entry in package.getmembers():
            target = (vendor / entry.name).resolve()
            if not target.is_relative_to(vendor.resolve()):
                raise RuntimeError("Invalid archive entry")
        package.extractall(vendor)


def main():
    vendor = ROOT / "vendor"
    vendor.mkdir(exist_ok=True)
    for name, url, checksum, directory, kind in PACKAGES:
        archive = vendor / name
        if not archive.exists():
            for reuse_dir in REUSE_VENDORS:
                source_copy = reuse_dir / name
                if source_copy.exists():
                    print(f"Reusing {name} from {reuse_dir}", flush=True)
                    shutil.copyfile(source_copy, archive)
                    break
        if not archive.exists():
            temporary = archive.with_suffix(archive.suffix + ".download")
            print(f"Downloading {name}", flush=True)
            with urllib.request.urlopen(url, timeout=60) as response, temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
            temporary.replace(archive)
        digest = sha256_of(archive)
        if checksum is not None and digest != checksum:
            raise RuntimeError(f"Checksum mismatch: {archive}; expected {checksum}, got {digest}")
        if kind == "zip" and directory is not None and not (vendor / directory / "include").exists():
            print(f"Extracting {name}", flush=True)
            safe_extract_zip(archive, vendor)
        elif kind == "targz" and directory is not None and not (vendor / directory).exists():
            print(f"Extracting {name}", flush=True)
            safe_extract_targz(archive, vendor)
        print(f"Ready: {name}", flush=True)


if __name__ == "__main__":
    main()
