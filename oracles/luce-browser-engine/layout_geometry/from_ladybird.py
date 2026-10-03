# Turns Ladybird Layout test inputs into oracle cases: the <style> elements' text becomes the case's author style
# sheet and is removed from the document (the harness attaches the sheet itself).
# Usage: python3 from_ladybird.py NAME... (names under Tests/LibWeb/Layout/input/, without .html)
import re, sys
root = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Tests/LibWeb/Layout/input/'
def esc(s):
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')
for name in sys.argv[1:]:
    text = open(root + name + '.html', encoding='utf-8').read()
    css = ''.join(m.group(1) for m in re.finditer(r'<style[^>]*>(.*?)</style>', text, re.S))
    html = re.sub(r'<style[^>]*>.*?</style>', '', text, flags=re.S)
    print('# ' + name)
    print('layout\t' + esc(html) + '\t' + esc(css))
