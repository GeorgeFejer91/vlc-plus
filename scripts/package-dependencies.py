"""Preserve exact runtime notices and signed MSYS2/Rust source companions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
import tomllib
from urllib.error import HTTPError, URLError
from urllib.parse import quote
from urllib.request import urlopen
import zipfile


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def download(url, destination):
    if destination.is_file():
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    for attempt in range(3):
        try:
            with urlopen(url, timeout=180) as response, destination.with_suffix(destination.suffix + ".tmp").open("wb") as stream:
                shutil.copyfileobj(response, stream)
            destination.with_suffix(destination.suffix + ".tmp").replace(destination)
            return
        except HTTPError as error:
            if error.code == 404 or attempt == 2:
                raise
        except (URLError, TimeoutError):
            if attempt == 2:
                raise
        time.sleep(attempt + 1)


def output(command, **kwargs):
    return subprocess.check_output(command, text=True, encoding="utf-8", **kwargs).strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--mingw", type=Path, required=True)
    parser.add_argument("--binaries", type=Path, required=True)
    args = parser.parse_args()
    root, stage, mingw = (path.resolve(strict=True) for path in (args.project, args.stage, args.mingw))
    if not stage.is_relative_to(root / "build"):
        raise ValueError("Dependency output must stay in this project's generated build tree")
    msys = mingw.parent.parent
    pacman, bash = msys / "usr/bin/pacman.exe", msys / "usr/bin/bash.exe"
    binaries = json.loads(args.binaries.read_text(encoding="utf-8-sig"))
    owners = {}
    for value in binaries:
        binary = Path(value).resolve(strict=True)
        if not binary.is_relative_to(msys):
            raise ValueError("Runtime dependency is outside the selected MSYS2 installation")
        posix = "/" + binary.relative_to(msys).as_posix()
        owner = output([str(pacman), "-Qqo", posix])
        if not re.fullmatch(r"[a-zA-Z0-9_.+-]+", owner):
            raise ValueError("Unexpected package owner")
        owners.setdefault(owner, []).append(binary.name)
    cache = root / "build/downloads/msys-sources"
    cache.mkdir(parents=True, exist_ok=True)
    packages = []
    rust_packages = []
    out = root / "build/stock-vlc-package/out"
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="corresponding-source-", dir=root / "build") as temporary:
        sources = Path(temporary).resolve()
        if not sources.is_relative_to(root / "build"):
            raise ValueError("Temporary source output escaped the build tree")
        shutil.copyfile(stage / "source/vlc-plus-source.zip", sources / "vlc-plus-source.zip")
        archive_cache = {}
        for owner, names in sorted(owners.items()):
            name, version = output([str(pacman), "-Q", owner]).split(" ", 1)
            desc = (msys / "var/lib/pacman/local" / f"{name}-{version}" / "desc").read_text(encoding="utf-8")
            fields = {}
            for block in desc.strip().split("\n\n"):
                lines = block.splitlines()
                fields[lines[0].strip("%")] = lines[1:]
            base = fields.get("BASE", [owner.replace("mingw-w64-x86_64-", "mingw-w64-")])[0]
            version_file = version.split(":")[-1]
            key = (base, version_file)
            notices = []
            for posix in output([str(pacman), "-Qlq", owner]).splitlines():
                if "/share/" not in posix or not (
                    "/share/licenses/" in posix or re.match(r"^(license|copying|notice)", Path(posix).name, re.I)
                ):
                    continue
                path = msys / posix.lstrip("/")
                if not path.is_file():
                    continue
                relative = Path("licenses/msys2") / owner / posix.split("/share/", 1)[1]
                target = stage / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)
                notices.append(relative.as_posix())
            if not notices:
                raise ValueError(f"No original runtime license files found for {owner}")
            if key not in archive_cache:
                archive = None
                for suffix in ("zst", "xz", "gz"):
                    filename = f"{base}-{version_file}.src.tar.{suffix}"
                    url = "https://repo.msys2.org/mingw/sources/" + quote(filename)
                    try:
                        download(url, cache / filename)
                        download(url + ".sig", cache / (filename + ".sig"))
                        archive = cache / filename
                        break
                    except HTTPError as error:
                        if error.code != 404:
                            raise
                if archive is None:
                    raise ValueError(f"Matching source archive unavailable for {base} {version_file}")
                environment = dict(os.environ, MSYSTEM="MINGW64", CHERE_INVOKING="1",
                                   VLCPLUS_SOURCE=str(archive), VLCPLUS_SIGNATURE=str(archive) + ".sig")
                # Match pacman-key's signature AND trust gates directly. Some MSYS2
                # pacman-key releases require lock-never but omit it from --init.
                verified = subprocess.run([str(bash), "-lc",
                    'gpg --homedir /etc/pacman.d/gnupg --batch --status-fd 1 --verify "$(cygpath -u "$VLCPLUS_SIGNATURE")" "$(cygpath -u "$VLCPLUS_SOURCE")"'],
                    env=environment, check=True, capture_output=True, text=True, encoding="utf-8")
                print(verified.stderr, end="", flush=True)
                if not re.search(r"^\[GNUPG:\] TRUST_(FULLY|ULTIMATE)\b", verified.stdout, re.M):
                    raise ValueError(f"Source signature is not trusted by the selected MSYS2 keyring: {archive.name}")
                target = sources / "msys2" / archive.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(archive, target)
                shutil.copyfile(Path(str(archive) + ".sig"), Path(str(target) + ".sig"))
                archive_cache[key] = {"file": "msys2/" + archive.name, "url": url, "sha256": sha256(archive), "signatureVerified": True}
            packages.append({"name": name, "version": version, "base": base, "binaries": sorted(set(names)),
                             "licenses": notices, "source": archive_cache[key]})
            print(f"Preserved {owner} {version}: licenses and signed sources", flush=True)

        vendor = sources / "rust-vendor"
        subprocess.run(["cargo", "vendor", "--locked", "--versioned-dirs", str(vendor)], cwd=root / "vlc-qt/bridge", check=True)
        lock = tomllib.loads((root / "vlc-qt/bridge/Cargo.lock").read_text(encoding="utf-8"))
        for package in lock["package"]:
            if not package.get("source", "").startswith("registry+"):
                continue
            name, version = package["name"], package["version"]
            folder = vendor / f"{name}-{version}"
            notices = [path for path in folder.iterdir() if path.is_file() and re.match(r"^(license|copying|notice)", path.name, re.I)]
            if not notices and name.startswith("labstream-"):
                workspace = vendor / f"labstream-{version}"
                left = json.loads((folder / ".cargo_vcs_info.json").read_text())["git"]["sha1"]
                right = json.loads((workspace / ".cargo_vcs_info.json").read_text())["git"]["sha1"]
                if left == right:
                    notices = [workspace / "LICENSE"]
            if not notices:
                raise ValueError(f"Original Rust crate license unavailable: {name} {version}")
            kept = []
            for path in notices:
                relative = Path("licenses/rust") / f"{name}-{version}" / path.name
                destination = stage / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, destination)
                kept.append(relative.as_posix())
            rust_packages.append({"name": name, "version": version, "crateSha256": package["checksum"], "licenses": kept})
        vlc = json.loads((root / "scripts/windows-build-lock.json").read_text())["vlcSource"]
        upstream = root / "build/downloads" / vlc["file"]
        download(vlc["url"], upstream)
        if sha256(upstream) != vlc["sha256"]:
            raise ValueError("Upstream VLC source differs from its lock")
        (sources / "upstream").mkdir()
        shutil.copyfile(upstream, sources / "upstream" / vlc["file"])
        manifest = {"schema": "vlc-plus-dependency-sources/v1", "sourceCommit": output(["git", "-C", str(root), "rev-parse", "HEAD"]),
                    "vlcSource": vlc, "msys2": packages, "rust": rust_packages}
        text = json.dumps(manifest, indent=2) + "\n"
        (sources / "dependency-sources.json").write_text(text, encoding="utf-8")
        (stage / "source/dependency-sources.json").write_text(text, encoding="utf-8")
        (sources / "README.txt").write_text(
            "Matching VLC+ project, signed MSYS2 sources and locked Rust crate sources.\n"
            "Project source is vlc-plus-source.zip. Native VLC source is in upstream/.\n"
            "MSYS2 source packages include their build recipes and upstream inputs.\n"
            "Rust crates are in rust-vendor/ with their Cargo checksums and original licenses.\n",
            encoding="utf-8")
        companion = out / "VLC_Plus_Source_0.3.5.zip"
        with zipfile.ZipFile(companion, "w") as archive:
            for path in sorted(sources.rglob("*")):
                if path.is_file():
                    compression = zipfile.ZIP_STORED if path.suffix in {".zip", ".gz", ".xz", ".zst"} else zipfile.ZIP_DEFLATED
                    archive.write(path, path.relative_to(sources).as_posix(), compress_type=compression)
        print(f"Matching source companion: {companion.name} ({companion.stat().st_size} bytes)", flush=True)


if __name__ == "__main__":
    main()
