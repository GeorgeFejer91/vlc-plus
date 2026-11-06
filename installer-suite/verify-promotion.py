"""Allow promotion only from this exact successful, same-source main workflow."""
import json
from pathlib import Path
import sys


def verify(run, source, repository):
    assert run['status'] == 'completed' and run['conclusion'] == 'success'
    assert run['event'] == 'workflow_dispatch' and run['head_branch'] == 'main'
    assert run['head_sha'] == source
    assert run['path'] == '.github/workflows/suite-source.yml'
    assert run['head_repository']['full_name'] == repository


if __name__ == '__main__':
    verify(json.loads(Path(sys.argv[1]).read_text()), sys.argv[2], sys.argv[3])
    print('PASS: exact successful main source-suite run is eligible for artifact verification.')
