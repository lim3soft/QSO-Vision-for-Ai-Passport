#!/usr/bin/env python3
"""Verify the exact preserved merged firmware before publishing a release."""
from pathlib import Path
import hashlib,json,sys
root=Path(__file__).resolve().parent.parent
version=sys.argv[1]
if version not in {'1.0','2.0','2.3'}:raise SystemExit('Unsupported release version')
p=root/'firmware'/version
m=json.loads((p/'manifest.json').read_text());data=(p/'QSO-Vision.bin').read_bytes()
assert m['version']==version and m['flash_offset']=='0x0'
assert data[0]==0xe9 and len(data)<=8*1024*1024
assert len(data)==m['size']
assert hashlib.sha256(data).hexdigest()==m['sha256']
assert ('Build: '+version).encode() in data
assert (p/'SHA256SUMS.txt').read_text()==m['sha256']+'  QSO-Vision.bin\n'
if version=='2.3':
    for name,digest in json.loads((p/'source-sha256.json').read_text()).items():
        source=(root/name).resolve();source.relative_to(root)
        assert hashlib.sha256(source.read_bytes()).hexdigest()==digest,name
print('Verified QSO VISION',version,len(data),'bytes;',m['sha256'])
