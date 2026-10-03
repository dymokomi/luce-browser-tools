# r46's own flex layout cases, as oracle case lines (mode "layout": the three dumps). Appends them to cases_flex.txt
# after the cases taken from Ladybird's tests (it replaces an earlier "# own flex cases" block).
# Usage: python3 own_flex_cases.py; ./oracle cases_flex.txt > expected_flex.txt
def esc(s):
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')
D = '<!DOCTYPE html>'
docs = []
def doc(name, html, css=''):
    docs.append((name, html, css))

B = '.f { display: flex; border: 1px solid; margin: 2px; } .f > div { border: 1px solid; }'

doc('directions', D + '''<div class=f style="width:300px"><div style="width:40px">a</div><div style="width:60px">bb</div><div>ccc</div></div>
<div class=f style="width:300px;flex-direction:row-reverse"><div style="width:40px">a</div><div style="width:60px">bb</div><div>ccc</div></div>
<div class=f style="height:150px;flex-direction:column"><div style="height:20px">a</div><div style="height:30px">bb</div><div>ccc</div></div>
<div class=f style="height:150px;flex-direction:column-reverse"><div style="height:20px">a</div><div style="height:30px">bb</div><div>ccc</div></div>
<div class=f style="flex-direction:column"><div>auto height column</div><div style="padding:5px">padded</div></div>''', B)

doc('justify-content-row', D + ''.join('<div class=f style="width:400px;justify-content:%s"><div style="width:50px">1</div><div style="width:70px">2</div><div style="width:30px">3</div></div>\n' % j for j in
    ['start', 'end', 'left', 'right', 'flex-start', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly', 'stretch', 'normal']), B)
doc('justify-content-row-reverse', D + ''.join('<div class=f style="width:400px;flex-direction:row-reverse;justify-content:%s;gap:7px"><div style="width:50px">1</div><div style="width:70px">2</div><div style="width:30px">3</div></div>\n' % j for j in
    ['start', 'end', 'left', 'right', 'flex-start', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly', 'normal']), B)
doc('justify-content-column', D + '<div style="display:flex">' + ''.join('<div class=f style="height:200px;width:50px;flex-direction:%s;justify-content:%s"><div style="height:20px">1</div><div style="height:35px">2</div></div>\n' % (d, j) for d in ['column', 'column-reverse'] for j in
    ['start', 'end', 'right', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly']) + '</div>', B)
doc('justify-content-overflow', D + ''.join('<div class=f style="width:100px;justify-content:%s"><div style="width:80px;flex-shrink:0">1</div><div style="width:70px;flex-shrink:0">2</div></div>\n' % j for j in
    ['center', 'space-between', 'space-around', 'space-evenly', 'flex-end']), B)

doc('align-items', D + ''.join('<div class=f style="height:80px;width:300px;align-items:%s"><div style="height:20px;margin-top:3px">a</div><div style="font-size:30px;margin-bottom:5px">Big</div><div>small</div><div style="height:auto;padding:4px 0 9px">pad</div></div>\n' % a for a in
    ['normal', 'stretch', 'start', 'end', 'flex-start', 'flex-end', 'self-start', 'self-end', 'center', 'baseline']), B)
doc('align-self', D + '''<div class=f style="height:100px;width:500px;align-items:center"><div style="align-self:auto">auto</div><div style="align-self:flex-start">fs</div>
<div style="align-self:flex-end">fe</div><div style="align-self:start">s</div><div style="align-self:end">e</div><div style="align-self:self-start">ss</div>
<div style="align-self:self-end">se</div><div style="align-self:stretch">st</div><div style="align-self:normal">n</div><div style="align-self:baseline;font-size:24px">bl</div>
<div style="align-self:baseline;padding-top:10px">bl2</div><div style="align-self:stretch;max-height:40px;min-height:10px">clamped</div><div style="align-self:stretch;margin:auto 0">automargin</div></div>
<div class=f style="flex-direction:column;width:200px;align-items:flex-start"><div style="align-self:center">c</div><div style="align-self:flex-end">fe</div><div style="align-self:stretch">st</div><div>auto</div></div>''', B)
doc('align-items-wrap-reverse', D + ''.join('<div class=f style="width:150px;height:120px;flex-wrap:wrap-reverse;align-items:%s"><div style="width:60px;height:20px">a</div><div style="width:60px">b</div><div style="width:60px;height:30px">c</div></div>\n' % a for a in
    ['normal', 'flex-start', 'flex-end', 'start', 'end', 'center', 'stretch']), B)

for d in ['row', 'column']:
    for w in ['wrap', 'wrap-reverse']:
        size = 'width:180px;height:200px' if d == 'row' else 'width:200px;height:120px'
        item = 'width:70px;height:30px'
        doc('align-content-%s-%s' % (d, w), D + ''.join('<div class=f style="%s;flex-direction:%s;flex-wrap:%s;align-content:%s;row-gap:4px;column-gap:6px"><div style="%s">1</div><div style="%s">2</div><div style="%s">3</div><div style="%s">4</div><div style="%s">5</div></div>\n' % (size, d, w, a, item, item, item, item, item) for a in
            ['normal', 'start', 'end', 'flex-start', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly', 'stretch']), B)
doc('align-content-negative-free-space', D + ''.join('<div class=f style="width:100px;height:40px;flex-wrap:wrap;align-content:%s"><div style="width:60px;height:30px">1</div><div style="width:60px;height:30px">2</div></div>\n' % a for a in
    ['space-between', 'space-around', 'space-evenly', 'center', 'stretch']), B)

doc('flex-grow', D + '''<div class=f style="width:500px"><div style="flex-grow:1">a</div><div style="flex-grow:2">b</div><div style="flex-grow:0;width:50px">c</div></div>
<div class=f style="width:500px"><div style="flex-grow:0.2;width:20px">a</div><div style="flex-grow:0.3;width:20px">b</div></div>
<div class=f style="width:500px"><div style="flex:1 1 100px;max-width:120px">max</div><div style="flex:1 1 100px">b</div><div style="flex:3 1 0;min-width:150px">min</div></div>
<div class=f style="width:500px"><div style="flex:1">equal</div><div style="flex:1">equal but much longer content here</div></div>
<div class=f style="width:500px"><div style="flex:auto">auto basis</div><div style="flex:auto">auto basis with more text</div></div>
<div class=f style="width:500px;gap:10px 20px"><div style="flex:1 0 30%">30%</div><div style="flex:1 0 30%">30%</div><div style="flex:2">rest</div></div>
<div class=f style="width:500px"><div style="flex:none;width:100px">none</div><div style="flex:initial;width:100px">initial</div><div style="flex-grow:1;flex-basis:content">content basis</div></div>''', B)
doc('flex-shrink', D + '''<div class=f style="width:200px"><div style="width:150px">a</div><div style="width:150px;flex-shrink:2">b</div><div style="width:100px;flex-shrink:0">c</div></div>
<div class=f style="width:200px"><div style="flex:0 0.5 200px">a</div><div style="flex:0 0.25 100px">b</div></div>
<div class=f style="width:200px"><div style="width:300px;min-width:180px">min wins</div><div style="width:300px">b</div></div>
<div class=f style="width:200px"><div style="width:300px">auto min content</div><div style="width:300px;overflow:hidden">scroll container</div></div>
<div class=f style="width:200px"><div style="flex-basis:150px;min-width:0">longwordthatdoesnotfit</div><div style="flex-basis:150px">longwordthatdoesnotfit</div></div>
<div class=f style="width:100px;flex-direction:column;height:60px"><div style="height:50px">a</div><div style="height:50px;flex-shrink:3">b</div><div style="height:50px;min-height:40px">c</div></div>''', B)
doc('flex-basis', D + '''<div class=f style="width:400px"><div style="flex-basis:25%">25%</div><div style="flex-basis:calc(10% + 20px)">calc</div><div style="flex-basis:min-content">min content basis</div>
<div style="flex-basis:max-content">max content</div><div style="flex-basis:fit-content">fit content basis</div></div>
<div class=f style="width:fit-content"><div style="flex-basis:50%">indefinite 50%</div><div style="flex-basis:80px">80px</div></div>
<div class=f style="flex-direction:column;width:200px"><div style="flex-basis:40px">40px</div><div style="flex-basis:20%">indefinite</div><div style="flex-basis:min-content">column min-content basis words</div><div style="flex-basis:max-content">column max</div></div>
<div class=f style="flex-direction:column;height:200px;width:200px"><div style="flex-basis:25%">25% of 200</div><div style="flex:1 0 0">grow</div><div style="flex-basis:fit-content">fit</div></div>
<div class=f style="width:300px"><div style="width:100px;box-sizing:border-box;padding:10px;border-width:5px">border-box</div><div style="flex-basis:100px;box-sizing:border-box;padding:0 10px">basis border-box</div></div>''', B)
doc('min-max-violations', D + '''<div class=f style="width:300px"><div style="flex:1;max-width:50px">a</div><div style="flex:1;max-width:60px">b</div><div style="flex:1">c</div></div>
<div class=f style="width:300px"><div style="flex:1;min-width:200px">a</div><div style="flex:1;min-width:150px">b</div></div>
<div class=f style="width:300px"><div style="flex:1 1 400px;min-width:100px">a</div><div style="flex:1 1 400px;max-width:120px">b</div></div>
<div class=f style="height:200px;flex-direction:column;width:100px"><div style="flex:1;max-height:30px">a</div><div style="flex:2;min-height:120px">b</div><div style="flex:1">c</div></div>
<div class=f style="width:300px"><div style="flex:1;max-width:30%">30%</div><div style="flex:1;min-width:calc(50% - 10px)">calc min</div></div>''', B)

doc('auto-margins', D + '''<div class=f style="width:400px;height:100px"><div style="margin-left:auto">right</div></div>
<div class=f style="width:400px;height:100px"><div style="margin:auto">center both</div></div>
<div class=f style="width:400px;height:100px"><div style="margin-right:auto">a</div><div>b</div><div style="margin-left:auto;margin-top:auto">c</div></div>
<div class=f style="width:400px;height:100px"><div style="margin:0 auto">h</div><div style="margin:auto 0;width:50px">v</div><div style="margin-top:auto;margin-bottom:10px">bottom</div><div style="margin-bottom:auto">top</div></div>
<div class=f style="width:100px;height:30px"><div style="margin:auto;width:150px;height:50px;flex-shrink:0">overflowing auto margins</div></div>
<div class=f style="flex-direction:column;height:150px;width:300px"><div style="margin-top:auto">down</div><div style="margin:auto">centered</div><div style="margin-left:auto">right</div></div>
<div class=f style="flex-direction:row-reverse;width:300px"><div style="margin-right:auto">auto right in reverse</div><div>x</div></div>''', B)

doc('gaps', D + '''<div class=f style="width:300px;gap:10px"><div>a</div><div>b</div><div>c</div></div>
<div class=f style="width:300px;column-gap:10%;flex-wrap:wrap;row-gap:5px"><div style="width:100px">a</div><div style="width:100px">b</div><div style="width:100px">c</div><div style="width:100px">d</div></div>
<div class=f style="height:200px;flex-direction:column;flex-wrap:wrap;row-gap:8px;column-gap:12px"><div style="height:80px">a</div><div style="height:80px">b</div><div style="height:80px">c</div></div>
<div class=f style="display:inline-flex;gap:15px"><div>inline</div><div>flex</div><div>gap</div></div>
<div class=f style="width:300px;gap:calc(5px + 2%) 4px;flex-wrap:wrap;height:150px"><div style="width:140px">a</div><div style="width:140px">b</div><div style="width:140px">c</div></div>''', B)

doc('order', D + '''<div class=f style="width:400px"><div style="order:3">o3</div><div style="order:-1">o-1</div><div>o0 first</div><div style="order:1">o1</div><div>o0 second</div><div style="order:-1">o-1 b</div></div>
<div class=f style="width:400px;flex-direction:row-reverse"><div style="order:2">o2</div><div>o0</div><div style="order:1">o1</div></div>
<div class=f style="height:200px;flex-direction:column-reverse"><div style="order:2">o2</div><div>o0</div><div style="order:-5">o-5</div></div>''', B)

doc('intrinsic-inline-flex', D + '''<div>before <div class=f style="display:inline-flex"><div>one</div><div style="flex-grow:1">two words</div></div> after</div>
<div>x <div class=f style="display:inline-flex;flex-direction:column"><div>column</div><div>inline flex items</div></div> y</div>
<div>x <div class=f style="display:inline-flex;flex-wrap:wrap"><div style="width:60px">wrap a</div><div style="width:80px">wrap b</div></div> y</div>
<div>x <div class=f style="display:inline-flex;flex-wrap:wrap;flex-direction:column;height:60px"><div style="height:40px">col wrap a</div><div style="height:40px">b</div></div> y</div>
<div style="float:left" class=f><div>float</div><div style="flex:1">flex container</div></div>
<div style="position:absolute;top:400px;left:50px" class=f><div>abspos</div><div>flex</div></div>''', B)
doc('intrinsic-keywords', D + ''.join('<div class=f style="width:%s;flex-direction:%s;flex-wrap:%s"><div>first item text</div><div style="flex:1 1 50px">second item with words</div><div style="padding:0 6px;margin:0 3px">third</div></div>\n' % (w, d, r) for w in ['min-content', 'max-content', 'fit-content'] for d in ['row', 'column'] for r in ['nowrap', 'wrap']), B)
doc('intrinsic-flex-fractions', D + '''<div class=f style="width:max-content"><div style="flex:2 1 20px">grow two</div><div style="flex:1 1 40px">grow one with text</div></div>
<div class=f style="width:max-content"><div style="flex:0.5 1 10px">half grow</div><div style="flex:0.25 1 10px">quarter</div></div>
<div class=f style="width:max-content"><div style="flex:0 2 200px">shrink basis</div><div style="flex:0 0 30px">fixed</div></div>
<div class=f style="width:min-content"><div style="flex:1 1 100px;min-width:20%">percent min</div><div style="max-width:40%">percent max text</div></div>
<div class=f style="height:max-content;flex-direction:column;width:200px"><div style="flex:1 1 10px">col grow</div><div style="flex:0 1 50px">col basis</div></div>''', B)
doc('column-items-intrinsic-height', D + '''<div class=f style="flex-direction:column;width:150px"><div>Some text that wraps onto several lines inside the item</div><div style="width:80px">narrow item with text wrapping</div>
<div style="width:min-content">min content width item</div><div style="width:max-content">max</div><div style="min-width:120px;max-width:60px">clamped</div><div style="align-self:center">centered text that wraps here</div></div>
<div class=f style="flex-direction:column;width:150px;align-items:flex-start"><div>flex-start item text wraps lines</div><div style="width:50%">half width text</div></div>''', B)

doc('aspect-ratio', D + '''<div class=f style="width:400px"><div style="aspect-ratio:2/1;width:100px">w</div><div style="aspect-ratio:1/2;height:60px">h</div><div style="aspect-ratio:1;flex:1">grow</div></div>
<div class=f style="width:400px;height:100px"><div style="aspect-ratio:2/1">stretched</div><div style="aspect-ratio:3/1;align-self:flex-start">start</div><div style="aspect-ratio:1/1;max-height:50px;align-self:center">max</div></div>
<div class=f style="flex-direction:column;width:200px"><div style="aspect-ratio:4/1">col</div><div style="aspect-ratio:2/1;width:50px">col w</div><div style="aspect-ratio:1/1;min-width:120px;align-self:flex-start">min</div></div>
<div class=f style="width:max-content"><div style="aspect-ratio:2/1;height:40px">intrinsic</div><div style="aspect-ratio:1/3;width:20px">tall</div></div>''', B)

doc('abspos-static-position', D + ''.join('<div class=f style="position:relative;width:200px;height:80px;flex-direction:%s;justify-content:%s;align-items:%s;padding:5px"><div style="position:absolute;width:30px;height:20px">abs</div><div style="width:40px;height:10px"></div></div>\n' % (d, j, a) for d, j, a in [
    ('row', 'flex-start', 'flex-start'), ('row', 'center', 'center'), ('row', 'flex-end', 'flex-end'), ('row', 'space-between', 'stretch'),
    ('row-reverse', 'flex-start', 'start'), ('row-reverse', 'flex-end', 'end'), ('column', 'center', 'self-start'), ('column', 'end', 'self-end'),
    ('column-reverse', 'space-around', 'baseline'), ('row', 'right', 'normal'), ('row', 'left', 'center'), ('column', 'space-evenly', 'flex-end')]) +
    '<div class=f style="position:relative;width:200px;height:80px;flex-wrap:wrap-reverse"><div style="position:absolute;width:30px;height:20px">abs</div></div>', B)
doc('abspos-in-flex', D + '''<div class=f style="position:relative;width:300px;height:100px"><div>a</div><div style="position:absolute;right:5px;bottom:5px">corner</div><div>b</div>
<div style="position:absolute;left:10%;top:20%;width:50%;height:30%">percent</div></div>
<div class=f style="width:300px">text <div style="position:absolute">abs in anonymous context</div> more</div>''', B)

doc('baseline', D + '''<div class=f style="align-items:baseline;width:400px"><div style="font-size:12px">small</div><div style="font-size:30px;padding-top:7px">big</div><div style="margin-top:12px">margin</div><div>two<br>lines</div><div style="height:50px"></div></div>
<div class=f style="align-items:baseline;width:200px;flex-wrap:wrap"><div style="width:90px;font-size:20px">a</div><div style="width:90px">b</div><div style="width:90px;font-size:8px">c</div></div>
<div class=f style="align-items:baseline;flex-direction:column;width:200px"><div>col a</div><div style="font-size:20px">col b</div></div>
<div>Inline <div class=f style="display:inline-flex;align-items:baseline"><div style="font-size:24px">flex</div><div>baseline</div></div> text</div>''', B)

doc('writing-modes', D + '''<div class=f style="writing-mode:vertical-rl;height:200px"><div>vertical</div><div style="flex:1">grow</div></div>
<div class=f style="writing-mode:vertical-lr;height:200px;flex-direction:column;width:150px"><div>col in vlr</div><div>b</div></div>
<div class=f style="direction:rtl;width:300px"><div>rtl a</div><div>rtl b</div><div style="margin-left:auto">c</div></div>
<div class=f style="direction:rtl;width:300px;flex-wrap:wrap-reverse;height:80px"><div style="width:200px">rtl wrap</div><div style="width:200px">b</div></div>
<div class=f style="width:300px"><div style="writing-mode:vertical-rl">orthogonal item</div><div>normal</div></div>''', B)

doc('box-model-and-percentages', D + '''<div class=f style="width:400px;padding:10px 20px;border-width:3px 6px"><div style="margin:5% 10px;padding:2% 4px;border:4px solid">percent margins</div><div style="width:25%;height:50%">pct</div></div>
<div class=f style="width:400px;height:120px"><div style="height:50%">half height</div><div style="height:calc(100% - 20px)">calc height</div><div style="min-height:80%;max-height:30px">min max pct</div></div>
<div class=f style="width:400px"><div style="height:50%">indefinite height pct</div><div style="padding-top:10%">pad pct</div></div>
<div class=f style="flex-direction:column;height:200px;width:300px"><div style="height:25%">col 25%</div><div style="width:50%">col width 50%</div><div style="flex:1;height:10px">grow</div></div>''', B)

doc('nested', D + '''<div class=f style="width:500px"><div class=f style="flex:1;flex-direction:column"><div>nested col a</div><div class=f><div style="flex:1">deep</div><div>x</div></div></div>
<div class=f style="flex:2;flex-wrap:wrap"><div style="width:120px">w1</div><div style="width:120px">w2</div><div style="width:120px">w3</div></div></div>
<div class=f style="flex-direction:column;height:300px;width:300px"><div class=f style="flex:1"><div style="flex:1">fills</div></div><div class=f style="flex:2;flex-direction:column"><div style="flex:1">inner grow</div><div>inner</div></div></div>''', B)

doc('single-line-cross-size', D + '''<div class=f style="width:300px;min-height:60px"><div>min height</div><div style="height:20px">fixed</div></div>
<div class=f style="width:300px;max-height:30px"><div style="height:60px">tall</div><div>auto stretched</div></div>
<div class=f style="width:300px;height:50px;flex-wrap:wrap"><div style="width:200px">a</div><div style="width:200px">b</div></div>
<div class=f style="flex-direction:column;width:200px;max-width:150px"><div style="width:180px">wide</div><div>auto</div></div>
<div class=f style="flex-direction:column;min-width:250px;width:min-content"><div>min-width col</div></div>''', B)

doc('anonymous-items-and-text', D + '''<div class=f style="width:300px">loose text <span>span item</span> more text <b>bold</b></div>
<div class=f style="width:300px;flex-direction:column">  <span>a</span>  text run  <i>b</i></div>
<div class=f style="width:300px"><div style="display:none">hidden</div><div style="float:left">float becomes item</div><div style="display:inline">inline item</div></div>''', B)

doc('flex-items-are-flex-and-blocks', D + '''<div class=f style="width:400px;align-items:flex-start"><div style="display:block">block<div style="float:right;width:30px;height:30px"></div></div><div style="display:inline-block">ib</div>
<div style="display:flex;flex-direction:column"><div>flex in flex</div><div>b</div></div><div style="display:table-caption;margin-left:20px">tc</div></div>''', B)

doc('quirks-and-percentage-heights', '''<div class=f style="height:50%"><div style="height:50%">quirks percent</div></div>
<div class=f style="flex-direction:column;height:200px"><div style="flex:1"><div style="height:50%">inner pct</div></div></div>''', B)

out = []
for name, html, css in docs:
    out.append('# own/' + name)
    out.append('layout\t' + esc(html) + '\t' + esc(css))
text = open('cases_flex.txt').read()
marker = '# own flex cases'
if marker in text:
    text = text[:text.index(marker)]
text = text.rstrip('\n') + '\n' + marker + ' (own_flex_cases.py)\n' + '\n'.join(out) + '\n'
open('cases_flex.txt', 'w').write(text)
print(len(docs), 'cases')
