"""Predict the pinned Tauri NSIS payload without altering the compiled binary.

Tauri patches this marker for packaging and restores the original afterwards:
https://github.com/tauri-apps/tauri/blob/tauri-cli-v2.11.4/crates/tauri-bundler/src/bundle.rs
Installed-byte verification must compare against that exact packaged variant.
"""
import hashlib
import json
import sys
from pathlib import Path

UNKNOWN = b"__TAURI_BUNDLE_TYPE_VAR_UNK"
NSIS = b"__TAURI_BUNDLE_TYPE_VAR_NSS"


def nsis_payload(compiled: bytes) -> bytes:
    if not compiled.startswith(b"MZ") or compiled.count(UNKNOWN) != 1:
        raise ValueError("Expected one unpatched Tauri bundle marker in the compiled Windows app")
    return compiled.replace(UNKNOWN, NSIS, 1)


if __name__ == "__main__":
    compiled = Path(sys.argv[1]).read_bytes()
    print(json.dumps({
        "compiledApplicationSha256": hashlib.sha256(compiled).hexdigest(),
        "applicationSha256": hashlib.sha256(nsis_payload(compiled)).hexdigest(),
    }))
