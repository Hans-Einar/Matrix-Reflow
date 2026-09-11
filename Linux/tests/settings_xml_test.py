"""Verify actual XML controls against the schema-backed CLI, without a display."""
import math
import os
import re
import shlex
import subprocess
import sys
import xml.etree.ElementTree as ET
exe, xml = sys.argv[1:]
env = dict(os.environ, DISPLAY='')
def run(args):
    p = subprocess.run([exe, '--no-config', *args, '--print-effective-settings'],
                       env=env, text=True, capture_output=True, timeout=5)
    assert p.returncode == 0, (args,p.stderr)
    return dict(line.split('=',1) for line in p.stdout.splitlines() if line and not line.startswith('#'))
defaults=run([])
help=subprocess.check_output([exe,'--help'],env=env,text=True)
schema={m[0]:m[1:] for m in re.findall(r'^  --([\w-]+) (.*?)  (.*?) \[(.*?)\.\.(.*?)\] \(default (.*?)\)$',help,re.M)}
root=ET.parse(xml).getroot()
seen=set()
for node in root.findall('.//number'):
    key=node.attrib['id']; seen.add(key)
    _,_,lo,hi,default=schema[key]
    for attr,value in [('low',lo),('high',hi),('default',default)]:
        assert math.isclose(float(node.attrib[attr]),float(value),abs_tol=1e-7), (key,attr)
        values=run(shlex.split(node.attrib['arg'].replace('%',node.attrib[attr])))
        assert math.isclose(float(values[key]),float(node.attrib[attr]),abs_tol=1e-7),key
for node in root.findall('.//boolean'):
    key=node.attrib['id'];seen.add(key)
    for action in ['arg-set','arg-unset']:
        if action in node.attrib:
            values=run(shlex.split(node.attrib[action]))
            assert values[key]==('1' if action=='arg-set' else '0'),key
            assert defaults[key]==('0' if action=='arg-set' else '1'),key
assert seen == set(schema)
assert [n.attrib['arg'] for n in root.findall('command')] == ['--root', '--profile 0']
print('All XML bounds, defaults, boolean polarity and emitted arguments match CLI')
