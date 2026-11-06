"""Fail closed before promoting the current run's suite artifacts."""
import argparse
import hashlib
import json
import re
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--release', type=Path, required=True)
p.add_argument('--evidence', type=Path, required=True)
p.add_argument('--source', required=True)
p.add_argument('--check-only', action='store_true')
a = p.parse_args()
build = json.loads((a.release / 'build-receipt.json').read_text())
smoke_path = a.evidence / 'smoke.json'
smoke = json.loads(smoke_path.read_text())
lock = json.loads((a.release / 'components.lock.json').read_text())
if lock.get('sourceMode') == 'same-checkout':
    for component in ('vlc', 'recorder'):
        assert re.fullmatch(r'\d+\.\d+\.\d+', lock[component]['version'])
        assert lock[component]['sourceCommit'] == a.source
        assert lock[component]['repository'] == 'GeorgeFejer91/vlc-plus'
    for component in ('vlc', 'recorder'):
        check = json.loads((a.evidence / component / 'installer-check.json').read_text())
        assert check['result'] == 'VERIFIED' and check['sourceCommit'] == a.source
        assert check['installerSha256'] == lock[component]['sha256']
        if component == 'recorder':
            assert check['applicationSha256'] == lock[component]['applicationSha256']
            assert check['safeRunningAppRefusal'] and check['independentUninstall'] and not check['recordingStarted']
        else:
            assert check['installExitCode'] == check['uninstallExitCode'] == 0
            assert check['userSettingsPreserved']
else:
    assert lock == json.loads((Path(__file__).parent / 'components.lock.json').read_text())
assert build['sourceCommit'] == smoke['sourceCommit'] == a.source
assert build['components'] == lock and build['bundleVersion'] == lock['bundleVersion']
expected = {f"01_VLCPlus_and_LSL_Recorder_Setup_{lock['bundleVersion']}_x64.exe",
            lock['vlc']['downloadName'], lock['recorder']['downloadName'],
            'SuiteSource.zip', 'components.lock.json', 'INSTALL.txt'}
if lock.get('sourceMode') == 'same-checkout':
    source_name = f"VLC_Plus_Source_{lock['vlc']['version']}.zip"
    expected |= {source_name, 'RecorderSource.zip', 'vlc-build-receipt.json', 'recorder-build-receipt.json'}
    for component in ('vlc', 'recorder'):
        receipt = json.loads((a.release / f'{component}-build-receipt.json').read_text())
        assert receipt['sourceCommit'] == a.source
        assert receipt['installerSha256'] == lock[component]['sha256']
        source_file = source_name if component == 'vlc' else 'RecorderSource.zip'
        source_hash = receipt['sourceBundleSha256'] if component == 'vlc' else receipt['sourceSha256']
        with (a.release / source_file).open('rb') as f:
            assert hashlib.file_digest(f, 'sha256').hexdigest() == source_hash
assert set(build['files']) == expected
assert {f.name for f in a.release.iterdir()} == expected | {'build-receipt.json', 'SHA256SUMS.txt'}
assert smoke['result'] == 'VERIFIED' and not smoke['recordingStarted']
for field in ('bothWindowsOpened', 'duplicateLaunchPrevented', 'openRecorderRefused',
              'suiteRemovalPreservedApps', 'independentUninstall'):
    assert smoke[field] is True, field
for name, digest in build['files'].items():
    assert Path(name).name == name
    with (a.release / name).open('rb') as f:
        assert hashlib.file_digest(f, 'sha256').hexdigest() == digest, name
joint = f"01_VLCPlus_and_LSL_Recorder_Setup_{lock['bundleVersion']}_x64.exe"
assert build['files'][joint] == smoke['installerSha256']
for name in ('vlc', 'recorder'):
    assert build['files'][lock[name]['downloadName']] == lock[name]['sha256']
checksum_names = set()
for line in (a.release / 'SHA256SUMS.txt').read_text().splitlines():
    digest, name = line.split('  ', 1)
    assert Path(name).name == name
    assert name not in checksum_names
    checksum_names.add(name)
    with (a.release / name).open('rb') as f:
        assert hashlib.file_digest(f, 'sha256').hexdigest() == digest, name
assert checksum_names == expected | {'build-receipt.json'}
if not a.check_only:
    (a.release / 'suite-smoke.json').write_bytes(smoke_path.read_bytes())
print('VERIFIED: exact source, component lock, all release bytes and joint smoke scope')
