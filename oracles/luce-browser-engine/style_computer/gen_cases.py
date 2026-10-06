# The cascade oracle (r39): writes a Text test for the reference build's test-web from cases.txt,
# runs it (test-web --rebaseline), and writes the Luce case table (tests_style_computer_cases.lucb)
# from its output.
#
# Each case: "css <text>" (the author style sheet, "\n" escapes a line break; may be empty),
# "tree <markup>" (div#root's innerHTML; written as the HTML parser keeps it), and "q <id>
# <property>..." lines. The test appends a <style> with the sheet to <head>, sets the markup, and
# prints "<id> <property>: <getComputedStyle(element).getPropertyValue(property)>" per property.
#
# Usage: python3 gen_cases.py OUT.lucb
import json, os, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DONOR = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin'
WORK = tempfile.mkdtemp(prefix='style_computer_')

cases = []
for raw in open(os.path.join(HERE, 'cases.txt'), encoding='utf-8').read().split('\n'):
    if not raw or raw.startswith('#'):
        continue
    kind, _, text = raw.partition(' ')
    if kind == 'css':
        cases.append({'css': text.replace('\\n', '\n'), 'tree': '', 'queries': []})
    elif kind == 'tree':
        cases[-1]['tree'] = text
    elif kind == 'q':
        parts = text.split()
        cases[-1]['queries'].append([parts[0], parts[1:]])
    else:
        raise SystemExit('bad line: ' + raw)

html = '''<!DOCTYPE html>
<script src="include.js"></script>
<script>
test(() => {
    const cases = %s;
    const root = document.createElement("div");
    root.id = "root";
    document.body.insertBefore(root, document.body.firstChild);
    const element = id => id === "html" ? document.documentElement : id === "body" ? document.body : id === "root" ? root : document.getElementById(id);
    for (const c of cases) {
        const style = document.createElement("style");
        style.textContent = c.css;
        document.head.appendChild(style);
        root.innerHTML = c.tree;
        println("case");
        for (const [id, properties] of c.queries) {
            const computed = getComputedStyle(element(id));
            for (const property of properties)
                println(id + " " + property + ": " + computed.getPropertyValue(property));
        }
        style.remove();
    }
});
</script>
''' % json.dumps(cases)

os.makedirs(os.path.join(WORK, 'Text', 'input'))
os.makedirs(os.path.join(WORK, 'Text', 'expected'))
shutil.copy(os.path.join(DONOR, 'Tests/LibWeb/Text/input/include.js'), os.path.join(WORK, 'Text', 'input'))
open(os.path.join(WORK, 'Text', 'input', 'style_computer.html'), 'w').write(html)
open(os.path.join(WORK, 'Text', 'expected', 'style_computer.txt'), 'w').write('')
subprocess.run([os.path.join(DONOR, 'Build/release/bin/test-web'), '--test-path', WORK, '--results-dir', os.path.join(WORK, 'results'), '--rebaseline', '-f', 'Text/input/style_computer.html'],
               cwd=DONOR, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
expected = open(os.path.join(WORK, 'Text', 'expected', 'style_computer.txt'), encoding='utf-8').read()
shutil.copy(os.path.join(WORK, 'Text', 'expected', 'style_computer.txt'), os.path.join(HERE, 'expected.txt'))

results = []
for l in expected.rstrip('\n').split('\n'):
    if l == 'case':
        results.append([])
    else:
        results[-1].append(l)
assert len(results) == len(cases), (len(results), len(cases))

def lit(s):
    r = []
    for ch in s:
        if ch == '\\': r.append('\\\\')
        elif ch == '"': r.append('\\"')
        elif ch == '\n': r.append('\\n')
        elif ord(ch) < 0x20: r.append('\\u{%x}' % ord(ch))
        else: r.append(ch)
    return '"' + ''.join(r) + '"'

out = open(sys.argv[1], 'w')
out.write('''#==============================================================================================
#
#   tests_style_computer_cases - the cascade cases pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's test-web on a Text test computing the same styles
#       (luce-browser-tools, oracles/luce-browser-engine/style_computer: cases.txt,
#       gen_cases.py). tests_style_computer builds each tree with the case's author style
#       sheet, runs the port's cascade and compares the values it answers.
#
#==============================================================================================

## One oracle case: the author style sheet, div#root's markup, the queries ("<id> <property>...",
## one per line) and the reference build's getComputedStyle() values ("<id> <property>: <value>",
## one per line).
struct TestStyleComputerCase:
    let css: str
    let markup: str
    let queries: str
    let expected: str

''')
out.write('## Every oracle case, in cases.txt order.\n')
out.write('let test_style_computer_cases: TestStyleComputerCase[%d] = [\n' % len(cases))
for c, result in zip(cases, results):
    q = ''.join(i + ' ' + ' '.join(p) + '\n' for i, p in c['queries'])
    e = ''.join(l + '\n' for l in result)
    out.write('    TestStyleComputerCase(css = %s, markup = %s, queries = %s, expected = %s),\n' % (lit(c['css']), lit(c['tree']), lit(q), lit(e)))
out.write(']\n')

# test-web wrote its results under WORK; nothing of this run stays behind.
shutil.rmtree(WORK, ignore_errors=True)
