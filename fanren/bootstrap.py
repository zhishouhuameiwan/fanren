import hashlib
import http.client
import pathlib
import shutil
import subprocess
import sys
import urllib.request
import zipfile
import tarfile

ROOT = pathlib.Path(__file__).resolve().parent

# (archive_name, url, sha256, extracted_dir_name_or_None, kind)
#   kind: "zip"  -> zip archive, extract into vendor/
#         "targz" -> tar.gz archive, extract into vendor/
#         "file" -> plain file, just verify + keep as-is (no extraction)
#
# SDL3 and its three satellites ship prebuilt only for MSVC (the -VC.zip
# devel packages). Everywhere else they are built from the SAME pinned
# versions' source releases -- see SDL_SOURCE_PACKAGES below -- and installed
# into the very same vendor/<name>-<version>/ directories, so CMakeLists.txt
# finds them at one path on every platform.
SDL_VC_PACKAGES = (
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
)

COMMON_PACKAGES = (
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

# Source releases for non-MSVC platforms (macOS first; added 2026-10-15 when
# the project moved to an Intel Mac, where Homebrew has no SDL3 bottles).
# Extracted under vendor/src/, built with CMake + Ninja and installed into
# vendor/<install_dir>/ -- the directory the -VC.zip would have produced.
SDL_SOURCE_PACKAGES = (
    (
        "SDL3-3.4.16.tar.gz",
        "https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz",
        "7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68",
        "SDL3-3.4.16",
        "targz",
    ),
    (
        "SDL3_ttf-3.2.2.tar.gz",
        "https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-3.2.2.tar.gz",
        "63547d58d0185c833213885b635a2c0548201cc8f301e6587c0be1a67e1e045d",
        "SDL3_ttf-3.2.2",
        "targz",
    ),
    (
        "SDL3_image-3.4.6.tar.gz",
        "https://github.com/libsdl-org/SDL_image/releases/download/release-3.4.6/SDL3_image-3.4.6.tar.gz",
        "d2e4637ae700f72e5196b8fbd749850ed2e5e1e09c5a5be8d06ff55aaccf3b01",
        "SDL3_image-3.4.6",
        "targz",
    ),
    (
        "SDL3_mixer-3.2.4.tar.gz",
        "https://github.com/libsdl-org/SDL_mixer/releases/download/release-3.2.4/SDL3_mixer-3.2.4.tar.gz",
        "182a07c745375e113dc740d43964ff21b0be29f29f59876c4dbc4db3d32f6901",
        "SDL3_mixer-3.2.4",
        "targz",
    ),
    # SDL_ttf's source release leaves its git submodules out. These are the
    # exact commits release-3.2.2 pins for external/freetype and
    # external/harfbuzz (the -VC.zip build links both, so text shaping matches).
    (
        "freetype-9973564cfa63763a3e4ac67c09147899539b1e07.tar.gz",
        "https://github.com/libsdl-org/freetype/archive/9973564cfa63763a3e4ac67c09147899539b1e07.tar.gz",
        "026a05a49d114a1235d2926f4c03a9330e4b1a6efe7c217ec9607904c32907d4",
        "freetype-9973564cfa63763a3e4ac67c09147899539b1e07",
        "targz",
    ),
    (
        "harfbuzz-564bf9818a18709776856533829c0c04950773d6.tar.gz",
        "https://github.com/libsdl-org/harfbuzz/archive/564bf9818a18709776856533829c0c04950773d6.tar.gz",
        "a448dd6c22d8e1e1cf39438c662251c1f97f810b8780eed4a6d6ada948c99ddc",
        "harfbuzz-564bf9818a18709776856533829c0c04950773d6",
        "targz",
    ),
)

# (source_dir, install_dir, cmake options). Built in this order: each
# satellite finds SDL3 through CMAKE_PREFIX_PATH. Every optional codec or
# backend that would pull in a system library is switched off, so the build
# depends on nothing outside vendor/ and the platform SDK. PNG goes through
# stb (not ImageIO, which colour-manages; not libpng, which is not vendored),
# Ogg Vorbis through stb_vorbis.
SDL_SOURCE_BUILDS = (
    ("SDL3-3.4.16", "SDL3-3.4.16", (
        "-DSDL_SHARED=ON", "-DSDL_STATIC=OFF", "-DSDL_TEST_LIBRARY=OFF",
        "-DSDL_TESTS=OFF", "-DSDL_EXAMPLES=OFF",
    )),
    ("SDL3_ttf-3.2.2", "SDL3_ttf-3.2.2", (
        "-DBUILD_SHARED_LIBS=ON", "-DSDLTTF_VENDORED=ON", "-DSDLTTF_HARFBUZZ=ON",
        "-DSDLTTF_PLUTOSVG=OFF", "-DSDLTTF_SAMPLES=OFF",
    )),
    ("SDL3_image-3.4.6", "SDL3_image-3.4.6", (
        "-DBUILD_SHARED_LIBS=ON", "-DSDLIMAGE_VENDORED=OFF", "-DSDLIMAGE_BACKEND_STB=ON",
        "-DSDLIMAGE_BACKEND_IMAGEIO=OFF", "-DSDLIMAGE_PNG_LIBPNG=OFF",
        "-DSDLIMAGE_AVIF=OFF", "-DSDLIMAGE_JXL=OFF", "-DSDLIMAGE_TIF=OFF", "-DSDLIMAGE_WEBP=OFF",
        "-DSDLIMAGE_SAMPLES=OFF", "-DSDLIMAGE_TESTS=OFF",
    )),
    ("SDL3_mixer-3.2.4", "SDL3_mixer-3.2.4", (
        "-DBUILD_SHARED_LIBS=ON", "-DSDLMIXER_VENDORED=OFF",
        "-DSDLMIXER_FLAC_LIBFLAC=OFF", "-DSDLMIXER_GME=OFF", "-DSDLMIXER_MOD_XMP=OFF",
        "-DSDLMIXER_MP3_MPG123=OFF", "-DSDLMIXER_MIDI_FLUIDSYNTH=OFF", "-DSDLMIXER_OPUS=OFF",
        "-DSDLMIXER_VORBIS_VORBISFILE=OFF", "-DSDLMIXER_WAVPACK=OFF",
        "-DSDLMIXER_TESTS=OFF", "-DSDLMIXER_EXAMPLES=OFF",
    )),
)

# Where the two SDL_ttf submodule tarballs go inside its source tree.
SDL_TTF_SUBMODULES = (
    ("freetype-9973564cfa63763a3e4ac67c09147899539b1e07", "freetype"),
    ("harfbuzz-564bf9818a18709776856533829c0c04950773d6", "harfbuzz"),
)

# Written into an install dir after a successful install; holds the recipe so
# changing a version or an option rebuilds instead of trusting a stale tree.
BUILT_MARKER = ".fanren-built"

# Written into an extracted top-level directory once extraction finished.
EXTRACTED_MARKER = ".fanren-extracted"

DOWNLOAD_ATTEMPTS = 4

SOL2_EMPLACE_BROKEN = (
    "\t\t\t*this = nullopt;\n"
    "\t\t\tthis->construct(std::forward<Args>(args)...);\n"
    "\t\t}\n"
)
SOL2_EMPLACE_FIXED = (
    "\t\t\t*this = nullopt;\n"
    "\t\t\tnew (static_cast<void*>(this)) optional(std::in_place, std::forward<Args>(args)...);\n"
    "\t\t\treturn **this;\n"
    "\t\t}\n"
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


def fetch_verified(name: str, url: str, checksum: str, vendor: pathlib.Path) -> pathlib.Path:
    """Download to a .download file and only move it into place once its
    SHA256 matches, so a truncated or corrupt download is retried instead of
    being kept as a bad archive that every later run trips over."""
    archive = vendor / name
    if archive.exists() and checksum is not None and sha256_of(archive) != checksum:
        print(f"Discarding {name}: checksum mismatch", flush=True)
        archive.unlink()
    if archive.exists():
        return archive
    temporary = archive.with_suffix(archive.suffix + ".download")
    for attempt in range(1, DOWNLOAD_ATTEMPTS + 1):
        print(f"Downloading {name} (attempt {attempt})", flush=True)
        try:
            with urllib.request.urlopen(url, timeout=60) as response, temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
            digest = sha256_of(temporary)
            if checksum is None or digest == checksum:
                temporary.replace(archive)
                return archive
            message = f"checksum mismatch; expected {checksum}, got {digest}"
        except (OSError, http.client.HTTPException) as error:
            # Dropped TLS handshakes and cut-off chunked responses are routine
            # on some networks.
            message = str(error)
        print(f"Download failed: {message}", flush=True)
        temporary.unlink(missing_ok=True)
    raise RuntimeError(f"Could not download {name} from {url}")


def extract_staged(archive: pathlib.Path, kind: str, directory: str, extract_to: pathlib.Path) -> None:
    """Extract into a scratch directory and move the result into place last,
    then mark it complete. An interrupted extraction therefore never leaves a
    half-written tree that later runs mistake for a finished one."""
    target = extract_to / directory
    staging = extract_to / f".extracting-{directory}"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir()
    if kind == "zip":
        safe_extract_zip(archive, staging)
    else:
        safe_extract_targz(archive, staging)
    if target.exists():
        shutil.rmtree(target)
    (staging / directory).replace(target)
    shutil.rmtree(staging)
    (target / EXTRACTED_MARKER).write_text(archive.name, encoding="utf-8")


def install_packages(packages, vendor: pathlib.Path, extract_to: pathlib.Path) -> None:
    for name, url, checksum, directory, kind in packages:
        archive = fetch_verified(name, url, checksum, vendor)
        if kind in ("zip", "targz") and directory is not None \
                and not (extract_to / directory / EXTRACTED_MARKER).exists():
            print(f"Extracting {name}", flush=True)
            extract_staged(archive, kind, directory, extract_to)
        print(f"Ready: {name}", flush=True)


def patch_sol2(vendor: pathlib.Path) -> None:
    """sol2 3.3.0's optional<T&>::emplace calls a construct() that optional<T&>
    does not have and falls off the end without returning. MSVC never looks
    inside the uninstantiated template; clang 17+ rejects it outright. This is
    the upstream fix (sol2 develop, also carried by vcpkg). Idempotent, and
    applied on every platform so all of them compile the same sol2."""
    header = vendor / "sol2-3.3.0" / "include" / "sol" / "optional_implementation.hpp"
    text = header.read_text(encoding="utf-8")
    if SOL2_EMPLACE_FIXED in text:
        return
    if text.count(SOL2_EMPLACE_BROKEN) != 1:
        raise RuntimeError(f"sol2 patch: expected exactly one broken emplace in {header}")
    header.write_text(text.replace(SOL2_EMPLACE_BROKEN, SOL2_EMPLACE_FIXED), encoding="utf-8", newline="")
    print("Patched: sol2 optional<T&>::emplace", flush=True)


def place_ttf_submodules(sources: pathlib.Path) -> None:
    external = sources / "SDL3_ttf-3.2.2" / "external"
    for extracted, submodule in SDL_TTF_SUBMODULES:
        target = external / submodule
        if (target / EXTRACTED_MARKER).exists():
            continue
        if target.exists():
            shutil.rmtree(target)
        shutil.copytree(sources / extracted, target, ignore=shutil.ignore_patterns(EXTRACTED_MARKER))
        (target / EXTRACTED_MARKER).write_text(extracted, encoding="utf-8")


def run_checked(command) -> None:
    print("+ " + " ".join(str(part) for part in command), flush=True)
    subprocess.run(command, check=True)


def build_sdl_from_source(vendor: pathlib.Path, sources: pathlib.Path) -> None:
    # Each recipe carries the ones built before it (and SDL_ttf its submodule
    # pins), so changing SDL3 or a pinned commit rebuilds everything downstream.
    upstream = ""
    for source_dir, install_dir, options in SDL_SOURCE_BUILDS:
        prefix = vendor / install_dir
        pins = [extracted for extracted, _ in SDL_TTF_SUBMODULES] if source_dir.startswith("SDL3_ttf") else []
        recipe = "\n".join((upstream, source_dir, *pins, *options))
        upstream = recipe
        marker = prefix / BUILT_MARKER
        if marker.exists() and marker.read_text(encoding="utf-8") == recipe:
            print(f"Ready: {install_dir} (built)", flush=True)
            continue
        if prefix.exists():
            shutil.rmtree(prefix)
        build_dir = sources / f"build-{source_dir}"
        if build_dir.exists():
            shutil.rmtree(build_dir)
        print(f"Building {source_dir}", flush=True)
        run_checked([
            "cmake", "-S", sources / source_dir, "-B", build_dir, "-G", "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={prefix}",
            f"-DCMAKE_PREFIX_PATH={vendor / 'SDL3-3.4.16'}",
            *options,
        ])
        run_checked(["cmake", "--build", build_dir, "--parallel"])
        run_checked(["cmake", "--install", build_dir])
        marker.write_text(recipe, encoding="utf-8")
        print(f"Ready: {install_dir}", flush=True)


def main():
    vendor = ROOT / "vendor"
    vendor.mkdir(exist_ok=True)
    install_packages(COMMON_PACKAGES, vendor, vendor)
    patch_sol2(vendor)
    if sys.platform == "win32":
        install_packages(SDL_VC_PACKAGES, vendor, vendor)
        return
    for tool in ("cmake", "ninja"):
        if shutil.which(tool) is None:
            raise RuntimeError(f"{tool} not found on PATH; it is needed to build SDL3 from source")
    sources = vendor / "src"
    sources.mkdir(exist_ok=True)
    install_packages(SDL_SOURCE_PACKAGES, vendor, sources)
    place_ttf_submodules(sources)
    build_sdl_from_source(vendor, sources)


if __name__ == "__main__":
    main()
