"""Exercise fail-closed publication using non-executable component fixtures."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
(root / 'build').mkdir(exist_ok=True)
source = '1' * 40


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value), encoding='utf-8')


with tempfile.TemporaryDirectory(prefix='suite-source-check-', dir=root / 'build') as folder:
    release = Path(folder) / 'release'
    evidence = Path(folder) / 'evidence'
    release.mkdir()
    names = ['01_VLCPlus_and_LSL_Recorder_Setup_0.3.3_x64.exe',
             '02_VLCPlus_0.3.3_Setup_x64.exe', '03_Remote_LSL_Recorder_0.1.8_Setup_x64.exe',
             'SuiteSource.zip', 'INSTALL.txt', 'VLC_Plus_Source_0.3.3.zip', 'RecorderSource.zip']
    for name in names:
        (release / name).write_text('fixture, never executed: ' + name)
    lock = {'schema': 'vlc-recorder-suite-components/v1', 'sourceMode': 'same-checkout', 'bundleVersion': '0.3.3'}
    for component, version, name in [('vlc', '0.3.3', names[1]), ('recorder', '0.1.8', names[2])]:
        lock[component] = {'version': version, 'repository': 'GeorgeFejer91/vlc-plus', 'sourceCommit': source,
                           'downloadName': name, 'sha256': digest(release / name)}
        check = {'result': 'VERIFIED', 'sourceCommit': source, 'installerSha256': lock[component]['sha256']}
        receipt = dict(check)
        if component == 'recorder':
            lock[component]['applicationSha256'] = 'a' * 64
            check.update(applicationSha256='a' * 64, safeRunningAppRefusal=True, independentUninstall=True, recordingStarted=False)
            receipt['sourceSha256'] = digest(release / 'RecorderSource.zip')
        else:
            check.update(installExitCode=0, uninstallExitCode=0, userSettingsPreserved=True)
            receipt['sourceBundleSha256'] = digest(release / 'VLC_Plus_Source_0.3.3.zip')
        write(evidence / component / 'installer-check.json', check)
        write(release / f'{component}-build-receipt.json', receipt)
    write(release / 'components.lock.json', lock)
    hashes = {path.name: digest(path) for path in release.iterdir()}
    write(release / 'build-receipt.json', {'sourceCommit': source, 'bundleVersion': '0.3.3', 'components': lock, 'files': hashes})
    sums = {**hashes, 'build-receipt.json': digest(release / 'build-receipt.json')}
    (release / 'SHA256SUMS.txt').write_text(''.join(f'{value}  {name}\n' for name, value in sums.items()))
    smoke = {'sourceCommit': source, 'result': 'VERIFIED', 'installerSha256': hashes[names[0]], 'recordingStarted': False}
    smoke.update({name: True for name in ('bothWindowsOpened', 'duplicateLaunchPrevented', 'openRecorderRefused', 'suiteRemovalPreservedApps', 'independentUninstall')})
    write(evidence / 'smoke.json', smoke)

    def check(expected):
        result = subprocess.run([sys.executable, str(root / 'installer-suite/verify-release.py'),
                                 '--release', str(release), '--evidence', str(evidence), '--source', source, '--check-only'], capture_output=True, text=True)
        assert (result.returncode == 0) == expected, result.stderr

    check(True)
    receipt_path = evidence / 'recorder/installer-check.json'
    original = receipt_path.read_text()
    changed = json.loads(original)
    changed['sourceCommit'] = '2' * 40
    write(receipt_path, changed)
    check(False)
    receipt_path.write_text(original)
    extra = release / 'unexpected.txt'
    extra.write_text('not in the closed inventory')
    check(False)
    extra.unlink()
    (release / names[1]).write_text('corrupt component')
    check(False)
print('PASS: same-checkout release identities and hashes; reject stale standalone proof, unexpected assets and corrupted components.')
