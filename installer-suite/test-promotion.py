"""A passing run cannot substitute another workflow, branch, source or repository."""
from copy import deepcopy
import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location('promotion', Path(__file__).with_name('verify-promotion.py'))
promotion = importlib.util.module_from_spec(spec)
spec.loader.exec_module(promotion)
source = 'a' * 40
repository = 'GeorgeFejer91/vlc-plus'
valid = dict(status='completed', conclusion='success', event='workflow_dispatch', head_branch='main',
             head_sha=source, path='.github/workflows/suite-source.yml',
             head_repository=dict(full_name=repository))
promotion.verify(valid, source, repository)
for key, value in [('status', 'in_progress'), ('conclusion', 'failure'), ('event', 'pull_request'),
                   ('head_branch', 'candidate'), ('head_sha', 'b' * 40),
                   ('path', '.github/workflows/suite-installer.yml'),
                   ('head_repository', dict(full_name='someone/vlc-plus'))]:
    run = deepcopy(valid)
    run[key] = value
    try:
        promotion.verify(run, source, repository)
    except AssertionError:
        continue
    raise AssertionError(f'Unsafe promotion accepted: {key}')
print('PASS: exact-run promotion and seven stale/foreign/incomplete run rejections.')
