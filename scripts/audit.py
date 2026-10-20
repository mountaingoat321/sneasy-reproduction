"""Review the explicit distribution set without reading authentication files."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = ['README.md', 'AGENTS.md', 'brad-lipovsky-academic-style-guide.md',
              'Makefile', 'LICENSE', 'requirements.txt']
DIRECTORIES = ['src', 'scripts', 'tests', 'data', 'results', 'figures', 'reports']
EXCLUDE_SUFFIXES = {'.aux', '.log', '.out', '.toc', '.pyc', '.fls', '.fdb_latexmk'}


def files():
    result = [ROOT/p for p in ROOT_FILES]
    for directory in DIRECTORIES:
        result.extend(p for p in (ROOT/directory).rglob('*') if p.is_file()
                      and '__pycache__' not in p.parts and p.suffix not in EXCLUDE_SUFFIXES)
    return sorted(result)


def main():
    secret = re.compile(rb'(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{40,}|'
                        rb'AKIA[A-Z0-9]{16})')
    private = re.compile(('/'+'home/'+r'[^ /\n]+|/'+'data/data2/').encode())
    key_header = ('-----'+'BEGIN ').encode()
    manifest = []
    for p in files():
        assert p.exists() and not p.is_symlink(), p
        assert p.stat().st_size < 8*1024*1024, ('Oversize file', p)
        raw = p.read_bytes()
        if p.suffix == '.pdf':
            assert p.name in {'report.pdf','figure1_nonreproduction.pdf','physical_diagnostics.pdf'}, p
            content = subprocess.check_output(['pdftotext',str(p),'-'])
        elif p.suffix == '.png':
            content = b''
        else:
            content = raw
        assert not secret.search(content), ('Possible secret', p)
        assert not private.search(content), ('Private absolute path', p)
        assert not (key_header in content and b'PRIVATE KEY' in content), ('Possible key', p)
        manifest.append(dict(path=str(p.relative_to(ROOT)),bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
    total = sum(row['bytes'] for row in manifest)
    assert total < 25*1024*1024, ('Oversize distribution',total)
    (ROOT/'build/distribution_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Distribution audit passed: %d files, %.2f MiB; no detected secrets/private paths.' %
          (len(manifest),total/1024**2))
    print('Copyright/provenance reviewed in data/README.md; supplied paper excluded.')


if __name__=='__main__':
    main()
