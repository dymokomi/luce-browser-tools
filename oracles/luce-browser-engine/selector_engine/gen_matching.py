# The SelectorEngine matching oracle: writes a Text test for the reference build's test-web from
# matching_cases.txt, runs it (test-web --rebaseline), and writes the Luce case table
# (tests_selector_engine_cases.lucb) from its output.
#
# matching_cases.txt: "tree <markup>" starts a case (the markup becomes div#root's innerHTML; it
# must be what the HTML parser keeps as written: no implied tags, no tables); each following
# "root <selector>" line runs root.querySelectorAll(selector), each "doc <selector>" line
# document.querySelectorAll(selector). The output lists the matched elements by id (or local
# name), or "error" when the selector does not parse.
#
# Usage: python3 gen_matching.py OUT.lucb
import json, os, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DONOR = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin'
WORK = tempfile.mkdtemp(prefix='selector_matching_')

cases = []
for raw in open(os.path.join(HERE, 'matching_cases.txt'), encoding='utf-8').read().split('\n'):
    if not raw or raw.startswith('#'):
        continue
    kind, text = raw.split(' ', 1)
    if kind == 'tree':
        cases.append([text, []])
    else:
        cases[-1][1].append([kind, text])

html = '''<!DOCTYPE html>
<script src="include.js"></script>
<script>
test(() => {
    const cases = %s;
    const root = document.createElement("div");
    root.id = "root";
    document.body.insertBefore(root, document.body.firstChild);
    const name = e => e.id || e.localName;
    const describe = e => name(e) + "(" + [...e.children].map(describe).join(" ") + ")";
    println("document: " + describe(document.documentElement));
    for (const [markup, queries] of cases) {
        root.innerHTML = markup;
        println("tree " + markup);
        for (const [kind, selector] of queries) {
            let result;
            try {
                const scope = kind === "doc" ? document : root;
                result = [...scope.querySelectorAll(selector)].filter(e => e.id !== "out").map(name).join(",");
            } catch (e) {
                result = "error";
            }
            println(kind + " " + selector + " => " + result);
        }
    }
});
</script>
''' % json.dumps(cases)

os.makedirs(os.path.join(WORK, 'Text', 'input'))
os.makedirs(os.path.join(WORK, 'Text', 'expected'))
shutil.copy(os.path.join(DONOR, 'Tests/LibWeb/Text/input/include.js'), os.path.join(WORK, 'Text', 'input'))
open(os.path.join(WORK, 'Text', 'input', 'selector_matching.html'), 'w').write(html)
open(os.path.join(WORK, 'Text', 'expected', 'selector_matching.txt'), 'w').write('')
subprocess.run([os.path.join(DONOR, 'Build/release/bin/test-web'), '--test-path', WORK, '--results-dir', os.path.join(WORK, 'results'), '--rebaseline', '-f', 'Text/input/selector_matching.html'],
               cwd=DONOR, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
expected = open(os.path.join(WORK, 'Text', 'expected', 'selector_matching.txt'), encoding='utf-8').read()
shutil.copy(os.path.join(WORK, 'Text', 'expected', 'selector_matching.txt'), os.path.join(HERE, 'matching_expected.txt'))

lines = expected.rstrip('\n').split('\n')
document_line = lines[0]
results = []
for l in lines[1:]:
    if l.startswith('tree '):
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
#   tests_selector_engine_cases - the SelectorEngine matching cases pinned to the reference
#                                 build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's test-web on a Text test doing the same queries
#       (luce-browser-tools, oracles/luce-browser-engine/selector_engine: matching_cases.txt,
#       gen_matching.py). tests_selector_engine builds each tree and runs each query through the
#       port's querySelectorAll, and compares the matched elements.
#
#==============================================================================================

## The reference document's element tree around div#root, as the oracle describes it.
let test_selector_engine_document: str = %s

## One oracle case: div#root's markup, its queries ("root <selector>" / "doc <selector>", one per
## line) and the reference build's results ("<query> => <ids>", one per line).
struct TestSelectorEngineCase:
    let markup: str
    let queries: str
    let expected: str

''' % lit(document_line))
out.write('## Every oracle case, in matching_cases.txt order.\n')
out.write('let test_selector_engine_cases: TestSelectorEngineCase[%d] = [\n' % len(cases))
for (markup, queries), result in zip(cases, results):
    q = ''.join(k + ' ' + s + '\n' for k, s in queries)
    e = ''.join(l + '\n' for l in result)
    out.write('    TestSelectorEngineCase(markup = %s, queries = %s, expected = %s),\n' % (lit(markup), lit(q), lit(e)))
out.write(']\n')

# test-web wrote its results under WORK; nothing of this run stays behind.
shutil.rmtree(WORK, ignore_errors=True)
