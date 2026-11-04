"""Verify the immutable installed payload and its committed source archive."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import zipfile


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate manifest key: {key}")
        result[key] = value
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("install_root", type=Path)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    root = args.install_root.resolve(strict=True)
    raw = (root / "manifest.json").read_bytes()
    manifest = json.loads(raw, object_pairs_hook=unique_object)
    if manifest.get("schema") != "vlc-plus-package/v1":
        raise ValueError("Unsupported package manifest")
    if manifest.get("sourceCommit") != args.source_commit or not re.fullmatch(r"[0-9a-f]{40}", args.source_commit):
        raise ValueError("Installed source identity differs from the tested commit")
    files = manifest.get("files")
    if not isinstance(files, dict) or not files:
        raise ValueError("Manifest has no file map")
    for name, expected in files.items():
        relative = PurePosixPath(name)
        if relative.is_absolute() or ".." in relative.parts or "\\" in name or ":" in name:
            raise ValueError(f"Unsafe manifest path: {name}")
        if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-f]{64}", expected):
            raise ValueError(f"Invalid digest: {name}")
        path = root / relative
        if not path.resolve(strict=True).is_relative_to(root) or path.is_symlink() or path.is_junction():
            raise ValueError(f"Linked or escaped installed file: {name}")
        with path.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        if digest != expected:
            raise ValueError(f"Installed file differs: {name}")
    if "portable/README.txt" not in files or not (root / "portable").is_dir():
        raise ValueError("The isolated VLC+ profile is missing")
    with zipfile.ZipFile(root / "source/vlc-plus-source.zip") as source:
        if source.comment.decode("ascii") != args.source_commit:
            raise ValueError("Bundled source archive commit differs")
        for required in ("build-windows.ps1", "vlc-qt/flubber_panel.hpp", "LICENSE", "scripts/windows-build-lock.json"):
            if required not in source.namelist():
                raise ValueError(f"Bundled source is incomplete: {required}")
    receipt = {
        "schema": "vlc-plus-install-integrity/v1",
        "result": "VERIFIED",
        "sourceCommit": args.source_commit,
        "verifiedFiles": len(files),
        "manifestSha256": hashlib.sha256(raw).hexdigest(),
        "portableProfile": True,
    }
    args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt))


if __name__ == "__main__":
    main()
