"""Check the extracted source, assets and documentation without a sibling repo."""

import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
errors = []


def require(condition, message):
    if not condition:
        errors.append(message)


manifest = json.loads((ROOT / "docs/source-extraction.json").read_text(encoding="utf-8"))
require(manifest["schema"] == "vlc-plus-source-extraction/v1", "Unknown extraction manifest")
adapted = {
    "package-stock-vlc.ps1", "installer-stock-vlc.iss", "vlc-qt/README.md",
    "docs/stock-vlc-integration.md", "docs/third-party.md", "assets/face/NOTICE.md",
}
paths = set()
for entry in manifest["files"]:
    name = entry["path"]
    relative = PurePosixPath(name)
    safe = not relative.is_absolute() and ".." not in relative.parts
    require(safe, f"Unsafe path: {name}")
    if not safe:
        continue
    require(name not in paths, f"Duplicate extraction path: {name}")
    paths.add(name)
    path = ROOT / name
    require(path.is_file() and not path.is_symlink(), f"Missing or linked source: {name}")
    if not path.is_file() or path.is_symlink():
        continue
    require(entry["adapted"] == (name in adapted), f"Unexpected adaptation: {name}")
    if not entry["adapted"]:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        require(digest == entry["upstreamSha256"], f"Upstream content drift: {name}")

fixture_info = manifest["generatedFixture"]
fixture = ROOT / fixture_info["path"]
require(hashlib.sha256(fixture.read_bytes()).hexdigest() == fixture_info["sha256"],
        "Planner-generated fixture drift")
require(json.loads(fixture.read_text())["schema"] == "vlc-flubber-appearance/v1",
        "Fixture is not the standalone Planner appearance format")
video_fixture = ROOT / "test/fixtures/validation.mp4"
require(video_fixture.is_file(), "Synthetic installed-player video fixture is missing")
if video_fixture.is_file():
    require(hashlib.sha256(video_fixture.read_bytes()).hexdigest() ==
            "82fc7904c7967c396122d13eeae89486084db97e168faf12ad0ef34a625720ee",
            "Synthetic video fixture differs from the declared generated input")
for name in paths | {fixture_info["path"]}:
    if name.endswith(".json"):
        try:
            json.loads((ROOT / name).read_text(encoding="utf-8"))
        except (ValueError, UnicodeError) as error:
            errors.append(f"Invalid JSON {name}: {error}")

package = (ROOT / "package-stock-vlc.ps1").read_text(encoding="utf-8")
require("experiments/vlc-flubber" not in package and "..\\..\\LICENSE" not in package,
        "Packaging still references the parent monorepo")
require("git -C $repository archive --format=zip" in package,
        "Package must include the complete committed standalone source")
for name in ("LICENSE", "licenses/AFFECT-RESEARCH-BSD-3-Clause.txt", "vlc-qt/bridge/Cargo.lock"):
    require((ROOT / name).is_file(), f"Required source/license missing: {name}")
installer = (ROOT / "installer-stock-vlc.iss").read_text(encoding="utf-8")
require("LicenseFile=LICENSE" in installer, "Installer license escapes this repository")

documents = [ROOT / "README.md", ROOT / "AGENTS.md"]
documents += list((ROOT / "docs").glob("*.md"))
documents += list((ROOT / "for-ai").glob("*.md"))
documents += [ROOT / "vlc-qt/README.md", ROOT / "assets/face/NOTICE.md"]
for document in documents:
    for match in re.finditer(r"\[[^\]]*\]\(([^)]+)\)", document.read_text(encoding="utf-8")):
        target = match.group(1)
        parsed = urlsplit(target)
        if parsed.scheme or target.startswith("#"):
            continue
        destination = document.parent / unquote(parsed.path)
        require(destination.exists(), f"Broken link in {document.relative_to(ROOT)}: {target}")

if errors:
    for error in errors:
        print(f"FAIL: {error}", file=sys.stderr)
    sys.exit(1)
print(f"PASS: {len(paths)} imported files, unchanged runtime/atlas bytes, Planner fixture, standalone paths and local links")
