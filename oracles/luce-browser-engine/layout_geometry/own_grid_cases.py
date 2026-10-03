# r47's own grid layout cases, as oracle case lines (mode "layout": the three dumps). Appends them to cases_grid.txt
# after the cases taken from Ladybird's tests (it replaces an earlier "# own grid cases" block).
# Usage: python3 own_grid_cases.py; ./oracle cases_grid.txt > expected_grid.txt
def esc(s):
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')
D = '<!DOCTYPE html>'
docs = []
def doc(name, html, css=''):
    docs.append((name, html, css))

B = '.g { display: grid; border: 1px solid; margin: 2px; } .g > div { border: 1px solid; }'

def items(n, text='i'):
    return ''.join('<div>%s%d</div>' % (text, i) for i in range(1, n + 1))

doc('fixed-tracks-and-gaps', D + '''<div class=g style="width:400px;grid-template-columns:50px 100px 70px;grid-template-rows:30px 40px">''' + items(6) + '''</div>
<div class=g style="width:400px;grid-template-columns:50px 100px 70px;gap:5px 11px">''' + items(7) + '''</div>
<div class=g style="width:400px;grid-template-columns:20% 30% calc(10% + 15px);row-gap:3%;column-gap:2%">''' + items(5) + '''</div>
<div class=g style="width:300px;grid-template-columns:repeat(4, 40px);grid-auto-rows:25px 35px">''' + items(11) + '''</div>''', B)

doc('fr-tracks', D + '''<div class=g style="width:500px;grid-template-columns:1fr 2fr 1fr">''' + items(3) + '''</div>
<div class=g style="width:500px;grid-template-columns:100px 1fr 3fr;gap:10px">''' + items(3, 'content ') + '''</div>
<div class=g style="width:500px;grid-template-columns:0.2fr 0.3fr">''' + items(2) + '''</div>
<div class=g style="width:300px;grid-template-columns:1fr 1fr"><div>averyveryveryverylongunbreakablewordthatoverflows</div><div>b</div></div>
<div class=g style="width:300px;grid-template-columns:minmax(0, 1fr) minmax(0, 1fr)"><div>averyveryveryverylongunbreakablewordthatoverflows</div><div>b</div></div>
<div class=g style="width:400px;grid-template-columns:minmax(100px, 1fr) minmax(50px, 2fr) 1fr">''' + items(3) + '''</div>
<div class=g style="height:300px;grid-template-rows:1fr 2fr auto">''' + items(3) + '''</div>''', B)

doc('fr-tracks-indefinite', D + '''<div class=g style="display:inline-grid;grid-template-columns:1fr 2fr"><div>short</div><div>a bit longer text</div></div>
<div class=g style="float:left;grid-template-columns:1fr 1fr 3fr"><div>a</div><div>bbbb</div><div>cc</div></div>
<div style="clear:both"></div>
<div class=g style="position:absolute;top:200px;grid-template-columns:2fr 1fr"><div>abspos grid</div><div>x</div></div>
<div class=g style="width:max-content;grid-template-columns:1fr 1fr"><div style="grid-column:span 2">spanning item across two flexible tracks</div><div>a</div><div>b</div></div>
<div class=g style="width:min-content;grid-template-columns:1fr 3fr;gap:8px"><div>min content grid</div><div>other words</div></div>''', B)

doc('intrinsic-tracks', D + '''<div class=g style="width:500px;grid-template-columns:min-content max-content auto"><div>min content words</div><div>max content words</div><div>auto words</div></div>
<div class=g style="width:500px;grid-template-columns:fit-content(80px) fit-content(300px) 1fr"><div>fit content limited words</div><div>fit content wide</div><div>rest</div></div>
<div class=g style="width:500px;grid-template-columns:fit-content(20%) auto"><div>percent fit content argument words</div><div>auto</div></div>
<div class=g style="display:inline-grid;grid-template-columns:fit-content(50%) auto"><div>indefinite percent fit content</div><div>auto</div></div>
<div class=g style="width:500px;grid-template-columns:minmax(min-content, max-content) minmax(auto, 100px) minmax(50px, max-content)"><div>mm one two</div><div>mm three four five</div><div>six</div></div>
<div class=g style="width:200px;grid-template-columns:auto auto auto"><div>one two three four</div><div>five</div><div>six seven</div></div>''', B)

doc('spanning-items', D + '''<div class=g style="width:400px;grid-template-columns:auto auto auto"><div style="grid-column:span 2">a spanning item with a lot of text inside</div><div>x</div><div>y</div><div style="grid-column:2 / span 2">z spans two</div></div>
<div class=g style="width:400px;grid-template-columns:min-content auto max-content;gap:4px"><div style="grid-column:1 / 4">spans all three tracks wide wide wide</div><div>a</div><div>b</div><div>c</div></div>
<div class=g style="width:400px;grid-template-columns:50px auto 1fr"><div style="grid-column:1 / span 2">span fixed and auto with text</div><div style="grid-column:2 / span 2">span auto and flex</div></div>
<div class=g style="width:400px;grid-template-rows:auto auto;grid-auto-flow:column"><div style="grid-row:span 2;height:80px">tall spanning</div><div>r1</div><div>r2</div></div>
<div class=g style="width:400px;grid-template-columns:fit-content(60px) fit-content(60px)"><div style="grid-column:span 2">spanning fit-content tracks with long content</div></div>''', B)

doc('line-placement', D + '''<div class=g style="width:400px;grid-template-columns:repeat(3, 60px);grid-template-rows:repeat(2, 30px)"><div style="grid-column:3;grid-row:2">3/2</div><div style="grid-column:1 / 3">1-3</div><div style="grid-row:1 / 3">rows</div><div>auto</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(3, 60px)"><div style="grid-column:-1">last</div><div style="grid-column:-3 / -2">neg</div><div style="grid-column:2 / -1">to end</div><div style="grid-column:-4 / 2">from start</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(2, 60px)"><div style="grid-column:5">implicit after</div><div style="grid-row:4">row 4</div><div style="grid-column:span 3">span 3</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(3, 60px)"><div style="grid-column:3 / 1">reversed</div><div style="grid-column:2 / 2">same line</div><div style="grid-column:span 2 / 3">span start</div><div style="grid-column:span 2 / span 3">two spans</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(3, 60px)"><div style="grid-column:-5">before explicit</div><div>a</div><div>b</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(3, 60px);grid-template-rows:20px"><div style="grid-row:-3">neg row</div><div style="grid-row:span 2 / 3">span to 3</div><div>c</div></div>''', B)

doc('named-lines', D + '''<div class=g style="width:400px;grid-template-columns:[a] 50px [b c] 80px [d] 60px [e];grid-template-rows:[top] 30px [mid] 30px [bottom]"><div style="grid-column:b / d">b-d</div><div style="grid-column:a;grid-row:mid">a/mid</div><div style="grid-column:c / e;grid-row:top / bottom">c-e</div><div style="grid-column:d">d</div></div>
<div class=g style="width:400px;grid-template-columns:[x] 40px [x] 40px [x] 40px [x]"><div style="grid-column:x 2 / x 4">x2-x4</div><div style="grid-column:x -1">x-1</div><div style="grid-column:x 3">x3</div><div style="grid-column:nope">missing</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(2, [r] 50px) [end]"><div style="grid-column:r 2 / end">r2-end</div><div style="grid-column:r">r</div></div>
<div class=g style="width:400px;grid-template-columns:[side-start] 60px [side-end main-start] 1fr [main-end]"><div style="grid-column:main">main</div><div style="grid-column:side">side</div></div>''', B)

doc('template-areas', D + '''<div class=g style="width:400px;grid-template-areas:'head head head' 'nav main main' 'nav foot foot';grid-template-columns:80px 1fr 1fr;grid-template-rows:30px 60px 20px"><div style="grid-area:main">main</div><div style="grid-area:head">head</div><div style="grid-area:foot">foot</div><div style="grid-area:nav">nav</div></div>
<div class=g style="width:400px;grid-template-areas:'a b' 'c d'"><div style="grid-row:b;grid-column:c">mixed</div><div style="grid-column:a-start / b-end">a-start to b-end</div><div style="grid-area:d">d</div><div style="grid-area:zz">unknown area</div></div>
<div class=g style="width:400px;grid-template:'x y' 40px 'x z' 30px / 100px 1fr"><div style="grid-area:x">x</div><div style="grid-area:z">z</div><div style="grid-area:y">y</div></div>''', B)

doc('auto-flow', D + '''<div class=g style="width:400px;grid-template-columns:repeat(3, 70px)"><div style="grid-column:span 2">wide</div><div style="grid-column:span 2">wide2</div><div>a</div><div>b</div><div>c</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(3, 70px);grid-auto-flow:row dense"><div style="grid-column:span 2">wide</div><div style="grid-column:span 2">wide2</div><div>a</div><div>b</div><div>c</div></div>
<div class=g style="width:400px;grid-template-rows:repeat(3, 20px);grid-auto-flow:column"><div>1</div><div style="grid-row:span 2">2</div><div style="grid-row:span 2">3</div><div>4</div><div>5</div></div>
<div class=g style="width:400px;grid-template-rows:repeat(3, 20px);grid-auto-flow:column dense"><div>1</div><div style="grid-row:span 2">2</div><div style="grid-row:span 2">3</div><div>4</div><div>5</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(4, 50px)"><div style="grid-column:3">c3</div><div style="grid-column:1">c1</div><div style="grid-row:2">r2</div><div>auto</div><div style="grid-column:2">c2</div><div>auto2</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(4, 50px);grid-auto-flow:dense"><div style="grid-column:3">c3</div><div style="grid-column:1">c1</div><div>auto</div><div style="grid-column:2">c2</div></div>
<div class=g style="width:400px;grid-template-columns:repeat(3, 50px)"><div style="order:2">o2</div><div style="order:-1">o-1</div><div>o0</div><div style="order:2;grid-row:1">o2r1</div></div>''', B)

doc('auto-repeat', D + '''<div class=g style="width:400px;grid-template-columns:repeat(auto-fill, 90px)">''' + items(6) + '''</div>
<div class=g style="width:400px;grid-template-columns:repeat(auto-fill, 90px);gap:10px">''' + items(3) + '''</div>
<div class=g style="width:400px;grid-template-columns:repeat(auto-fit, 90px);gap:10px">''' + items(2) + '''</div>
<div class=g style="width:400px;grid-template-columns:repeat(auto-fit, minmax(90px, 1fr))">''' + items(3) + '''</div>
<div class=g style="width:400px;grid-template-columns:repeat(auto-fill, minmax(90px, 1fr))">''' + items(3) + '''</div>
<div class=g style="width:400px;grid-template-columns:50px repeat(auto-fill, [l] 40px [r]) 60px"><div style="grid-column:l 2">l2</div>''' + items(3) + '''</div>
<div class=g style="width:400px;grid-template-columns:repeat(auto-fill, minmax(min-content, 120px))">''' + items(4) + '''</div>
<div class=g style="display:inline-grid;grid-template-columns:repeat(auto-fill, 50px)">''' + items(3) + '''</div>
<div class=g style="width:400px;height:100px;grid-auto-flow:column;grid-template-rows:repeat(auto-fit, 30px)">''' + items(2) + '''</div>
<div class=g style="width:400px;grid-template-columns:repeat(2, 30px 50px)">''' + items(5) + '''</div>''', B)

doc('content-distribution', D + ''.join('<div class=g style="width:300px;height:110px;grid-template-columns:repeat(3, 50px);grid-template-rows:repeat(2, 30px);gap:4px;justify-content:%s;align-content:%s">%s</div>\n' % (j, a, items(6)) for j, a in
    [('start', 'start'), ('end', 'end'), ('center', 'center'), ('space-between', 'space-between'), ('space-around', 'space-around'), ('space-evenly', 'space-evenly'), ('stretch', 'stretch'),
     ('normal', 'normal'), ('left', 'flex-start'), ('right', 'flex-end'), ('flex-start', 'start'), ('flex-end', 'end')]), B)

doc('content-distribution-auto-tracks', D + ''.join('<div class=g style="width:300px;height:100px;grid-template-columns:auto auto 40px;grid-template-rows:auto 30px;justify-content:%s;align-content:%s">%s</div>\n' % (j, a, items(5)) for j, a in
    [('normal', 'normal'), ('stretch', 'stretch'), ('start', 'start'), ('space-between', 'center')]), B)

doc('item-alignment', D + ''.join('<div class=g style="width:300px;grid-template-columns:100px 100px;grid-auto-rows:50px;justify-items:%s;align-items:%s"><div>a</div><div style="width:30px">b</div><div style="height:20px">c</div><div style="margin:auto">auto margins</div></div>\n' % (j, a) for j, a in
    [('normal', 'normal'), ('stretch', 'stretch'), ('start', 'start'), ('end', 'end'), ('center', 'center'), ('baseline', 'baseline'), ('self-start', 'self-start'), ('self-end', 'self-end'),
     ('flex-start', 'flex-start'), ('flex-end', 'flex-end'), ('left', 'safe center'), ('right', 'unsafe end'), ('legacy', 'normal')]), B)

doc('self-alignment', D + '''<div class=g style="width:400px;grid-template-columns:repeat(4, 90px);grid-auto-rows:40px;justify-items:center;align-items:end">
<div style="justify-self:auto;align-self:auto">auto</div><div style="justify-self:start;align-self:start">start</div><div style="justify-self:end;align-self:end">end</div><div style="justify-self:center;align-self:center">center</div>
<div style="justify-self:stretch;align-self:stretch">stretch</div><div style="justify-self:normal;align-self:normal">normal</div><div style="justify-self:self-start;align-self:self-start">ss</div><div style="justify-self:self-end;align-self:self-end">se</div>
<div style="justify-self:flex-start;align-self:flex-start">fs</div><div style="justify-self:flex-end;align-self:flex-end">fe</div><div style="justify-self:left;align-self:baseline">left</div><div style="justify-self:right;align-self:safe center">right</div>
<div style="justify-self:baseline;align-self:unsafe start">bl</div><div style="margin-left:auto">ml auto</div><div style="margin-right:auto;margin-top:auto">mr mt auto</div><div style="width:50%;height:50%;justify-self:center">pct</div></div>''', B)

doc('item-sizes', D + '''<div class=g style="width:400px;grid-template-columns:150px 150px;grid-auto-rows:60px">
<div style="width:100px;height:30px">fixed</div><div style="width:50%;height:50%">percent</div><div style="width:200px">too wide</div><div style="min-width:180px;max-height:20px">min max</div>
<div style="width:fit-content">fit content</div><div style="width:min-content">min content words</div><div style="width:max-content">max content words here</div><div style="max-width:60px">max width with words</div>
<div style="box-sizing:border-box;width:100px;padding:10px">border-box</div><div style="padding:5%;margin:3%">pct box</div><div style="aspect-ratio:2/1;justify-self:start">aspect</div><div style="aspect-ratio:1;width:40px;align-self:start">aspect w</div></div>''', B)

doc('minimum-contributions', D + '''<div class=g style="width:100px;grid-template-columns:auto auto"><div>averyverylongword</div><div>short</div></div>
<div class=g style="width:100px;grid-template-columns:auto auto"><div style="min-width:0">averyverylongword</div><div>short</div></div>
<div class=g style="width:100px;grid-template-columns:auto auto"><div style="overflow:hidden">averyverylongword</div><div>short</div></div>
<div class=g style="width:100px;grid-template-columns:auto auto"><div style="width:120px">fixed wide</div><div>x</div></div>
<div class=g style="width:100px;grid-template-columns:50px auto"><div style="min-width:min-content">min-content min</div><div style="min-width:max-content">max</div></div>
<div class=g style="width:100px;grid-template-columns:30px 30px"><div style="grid-column:span 2">spanningverylongword</div></div>
<div class=g style="width:100px;grid-template-columns:auto 1fr"><div style="grid-column:span 2">spanningflexverylongword</div></div>
<div class=g style="width:100px;grid-template-columns:auto"><div style="aspect-ratio:1/2;height:80px"></div></div>
<div class=g style="width:100px;grid-template-columns:auto;grid-template-rows:auto"><div style="max-width:40px">capped by max width words</div></div>''', B)

doc('container-sizes', D + '''<div class=g style="width:300px;max-width:200px;grid-template-columns:repeat(3, auto)"><div>max width container</div><div>b</div><div>c</div></div>
<div class=g style="min-height:100px;grid-template-rows:auto auto">''' + items(2) + '''</div>
<div class=g style="height:50px;grid-template-rows:auto auto"><div style="height:40px">tall</div><div>b</div></div>
<div class=g style="max-height:40px;grid-template-rows:1fr 1fr"><div style="height:40px">tall</div><div>b</div></div>
<div class=g style="min-height:90px;grid-template-rows:1fr 2fr">''' + items(2) + '''</div>
<div class=g style="width:fit-content;grid-template-columns:repeat(2, max-content)"><div>fit content container</div><div>b</div></div>
<div class=g style="display:inline-grid;grid-template-columns:auto auto;padding:7px;border-width:3px">inline <span>grid</span></div> text after
<div class=g style="width:300px;padding:10px 20px;box-sizing:border-box;grid-template-columns:1fr 1fr">''' + items(2) + '''</div>
<div class=g></div>
<div class=g style="width:200px;grid-template-columns:100px 100px;height:auto"></div>''', B)

doc('percent-tracks', D + '''<div class=g style="width:400px;grid-template-columns:25% 50% auto">''' + items(3) + '''</div>
<div class=g style="display:inline-grid;grid-template-columns:25% 50px">''' + items(2) + '''</div>
<div class=g style="width:400px;height:200px;grid-template-rows:20% 30% 1fr">''' + items(3) + '''</div>
<div class=g style="width:400px;grid-template-rows:20% 30%">''' + items(2) + '''</div>
<div class=g style="width:400px;grid-template-columns:calc(50% - 20px) calc(25% + 10px)">''' + items(2) + '''</div>
<div class=g style="width:400px;grid-template-columns:minmax(10%, 100px) minmax(50px, 20%)">''' + items(2) + '''</div>''', B)

doc('abspos-children', D + '''<div class=g style="position:relative;width:300px;height:120px;padding:10px 15px;grid-template-columns:50px 100px 1fr;grid-template-rows:40px 1fr"><div>a</div><div>b</div>
<div style="position:absolute;grid-column:2;grid-row:2;inset:0">area</div>
<div style="position:absolute;grid-column:2 / 4;top:5px;left:5px;width:20px;height:20px">cols</div>
<div style="position:absolute;grid-row:1;right:0;bottom:0">auto col</div>
<div style="position:absolute;grid-column:auto / 2;top:0">mixed start</div>
<div style="position:absolute;grid-column:3 / auto;bottom:0">mixed end</div>
<div style="position:absolute;grid-column:span 2;top:0">span only</div>
<div style="position:absolute">static pos</div>
<div style="position:absolute;grid-column:4;grid-row:3">last line</div></div>
<div class=g style="position:relative;width:300px;grid-template-columns:100px 100px;justify-content:center;align-content:end;height:100px"><div>x</div>
<div style="position:absolute;grid-column:2 / auto;grid-row:auto / 1;width:10px;height:10px">aligned</div></div>
<div class=g style="width:200px;grid-template-columns:100px 100px"><div style="position:relative">rel<div style="position:absolute;top:0;left:30px">nested abspos</div></div><div style="position:relative;top:5px;left:-5px">relpos item</div></div>''', B)

doc('nested-and-mixed', D + '''<div class=g style="width:400px;grid-template-columns:1fr 2fr"><div class=g style="grid-template-columns:auto auto"><div>n1</div><div>n2</div></div><div style="display:flex"><div>flex in grid</div><div>f2</div></div>
<div style="display:inline-block">inline-block item</div><div>block <span>with</span> inline content and more words to wrap the line</div></div>
<div style="display:flex;width:400px"><div class=g style="grid-template-columns:auto auto;flex:1"><div>grid</div><div>in flex</div></div><div>sibling</div></div>
<div class=g style="width:300px">loose text <span>span</span> more <b>bold</b><div>block</div></div>
<div class=g style="width:300px;grid-template-columns:1fr 1fr"><div style="float:left">float item</div><div style="display:none">none</div><div style="display:contents"><div>contents child</div></div></div>''', B)

doc('box-model-items', D + '''<div class=g style="width:400px;grid-template-columns:repeat(2, 1fr);gap:6px"><div style="margin:5px 10px;padding:3px 7px;border-width:4px">box</div><div style="margin:-5px">negative</div>
<div style="padding:10%">pct pad</div><div style="margin:auto 10px">auto v</div><div style="border:none;padding:0;margin:0">plain</div><div style="margin-left:20%">pct margin</div></div>''', B)

doc('large-grids', D + '''<div class=g style="width:500px;grid-template-columns:repeat(7, 1fr);gap:2px">''' + items(30) + '''</div>
<div class=g style="width:500px;grid-template-columns:repeat(3, auto);grid-auto-rows:minmax(20px, auto)">''' + ''.join('<div>%s</div>' % ('w ' * (i % 5 + 1)) for i in range(12)) + '''</div>''', B)

doc('quirks', '''<div class=g style="height:50%;grid-template-rows:1fr"><div style="height:50%">quirks pct</div></div>
<div class=g style="width:300px;grid-template-columns:50% 50%"><div>a</div><div>b</div></div>''', B)

out = []
for name, html, css in docs:
    out.append('# own/' + name)
    out.append('layout\t' + esc(html) + '\t' + esc(css))
text = open('cases_grid.txt').read()
marker = '# own grid cases'
if marker in text:
    text = text[:text.index(marker)]
text = text.rstrip('\n') + '\n' + marker + ' (own_grid_cases.py)\n' + '\n'.join(out) + '\n'
open('cases_grid.txt', 'w').write(text)
print(len(docs), 'cases')
