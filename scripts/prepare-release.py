"""Verify artifacts from the successful installer job before release publication."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil


def one(root, name):
    paths = list(root.rglob(name))
    if len(paths) != 1:
        raise ValueError(f"Expected one {name}, found {len(paths)}")
    return paths[0]


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--diagnostics", type=Path, required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"[0-9a-f]{40}", args.source_commit):
        raise ValueError("Expected exact source commit")
    receipt_path = one(args.candidate, "build-receipt.json")
    receipt = read(receipt_path)
    installer = one(args.candidate, "VLC_Plus_Setup_0.3.2_x64.exe")
    source = one(args.candidate, "VLC_Plus_Source_0.3.2.zip")
    packages = one(args.candidate, "toolchain-packages.txt")
    manifest_path = one(args.candidate, "manifest.json")
    manifest = read(manifest_path)
    check_path = one(args.diagnostics, "installer-check.json")
    check = read(check_path)
    if receipt["sourceCommit"] != args.source_commit or manifest["sourceCommit"] != args.source_commit:
        raise ValueError("Build/package source differs from the workflow commit")
    if sha(installer) != receipt["installerSha256"] or sha(source) != receipt["sourceBundleSha256"]:
        raise ValueError("Build output hash mismatch")
    if sha(packages) != receipt["packagesSha256"]:
        raise ValueError("Toolchain inventory hash mismatch")
    if check["result"] != "VERIFIED" or check["sourceCommit"] != args.source_commit or check["installerSha256"] != sha(installer):
        raise ValueError("Installer was not verified for this source/output")
    if check["installExitCode"] != 0 or check["uninstallExitCode"] != 0:
        raise ValueError("Install/uninstall did not succeed")
    proofs = [one(args.diagnostics, name) for name in ("integrity.json", "post-run-integrity.json")]
    for path in proofs:
        proof = read(path)
        if proof["result"] != "VERIFIED" or proof["sourceCommit"] != args.source_commit or proof["manifestSha256"] != sha(manifest_path):
            raise ValueError("Installed integrity proof mismatch")
    probe_path = one(args.diagnostics, "installed-probe.json")
    probe = read(probe_path)
    if probe["result"] != "VERIFIED" or probe["decodedVideoFrames"] <= 0 or not 28 <= probe["observedLslHz"] <= 32 or probe["shutdownMs"] > 10000:
        raise ValueError("Installed playback/LSL/shutdown proof failed")
    if manifest["files"].get("examples/media/dictator-3-study.mp4") != "b5327e7465ec92a4c93f3236a1ebab4556cdf508e24eafe6c593eac1e13afd49":
        raise ValueError("The permission-scoped sample is missing or changed")
    args.output.mkdir(parents=True, exist_ok=False)
    for path in [installer, source, packages, receipt_path, manifest_path, check_path, probe_path, *proofs]:
        shutil.copyfile(path, args.output / path.name)
    checksums = [f"{sha(path)}  {path.name}" for path in sorted(args.output.iterdir())]
    (args.output / "SHA256SUMS").write_text("\n".join(checksums) + "\n", encoding="ascii")
    print(f"VERIFIED: release assets for {args.source_commit}")


if __name__ == "__main__":
    main()
