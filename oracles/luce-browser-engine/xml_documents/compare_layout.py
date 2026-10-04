# Compares the layout trees of the XML test documents: the reference Ladybird (headless layout-tree)
# against web_test --one (layout mode), "baseline:" values masked (the headless run's fonts differ).
import subprocess, os, re, sys, glob, concurrent.futures
E = '/Users/sedov/Dev/luce_dev/luce-browser-engine-p2x'
L = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/bin/Ladybird.app/Contents/MacOS/Ladybird'
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'compare_out')
os.makedirs(OUT, exist_ok=True)
files = []
for ext in ('xht', 'xhtml', 'svg', 'xml'):
    files += glob.glob(E + '/tests/libweb/**/*.' + ext, recursive=True)
files.sort()
def layout_part(text):
    lines = []
    for line in text.split('\n'):
        if line.startswith('ViewportPaintable') or line.startswith('SC for'):
            break
        if 'mach_msg' in line or line.startswith('web_test:'):
            continue
        lines.append(re.sub(r'baseline: [0-9.]+', 'baseline: *', line))
    while lines and lines[-1] == '':
        lines.pop()
    return '\n'.join(lines)
def run(path):
    try:
        lb = subprocess.run([L, '--headless=layout-tree', '--test-mode', '--disable-scripting', 'file://' + path], capture_output=True, text=True, timeout=30).stdout
    except subprocess.TimeoutExpired:
        lb = 'TIMEOUT'
    try:
        r = subprocess.run([E + '/build/web_test', '--one', path, '--mode', 'layout', '--root', E + '/tests/libweb'], capture_output=True, text=True, timeout=60)
        ours = r.stdout
        trap = [l for l in r.stderr.split('\n') if l.startswith('trap:')]
    except subprocess.TimeoutExpired:
        ours, trap = 'TIMEOUT', ['timeout']
    a, b = layout_part(lb), layout_part(ours)
    name = path[len(E) + 1:]
    if trap:
        return (name, 'trap', trap[0])
    if a == b:
        return (name, 'same', '')
    safe = name.replace('/', '__')
    open(OUT + '/' + safe + '.lb', 'w').write(a)
    open(OUT + '/' + safe + '.ours', 'w').write(b)
    return (name, 'differ', '')
with concurrent.futures.ThreadPoolExecutor(8) as pool:
    for name, status, note in pool.map(run, files):
        print(status, name, note)
