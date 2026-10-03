# r48's own table layout cases, as oracle case lines (mode "layout": the three dumps of a Layout test; mode "table":
# TableGrid::calculate_row_column_grid of every table box). Appends them to cases_table.txt after the cases taken from
# Ladybird's tests (it replaces an earlier "# own table cases" block).
# Usage: python3 own_table_cases.py; ./oracle cases_table.txt > expected_table.txt
def esc(s):
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')
D = '<!DOCTYPE html>'
docs = []
def doc(name, html, css='', mode='layout'):
    docs.append((name, mode, html, css))

B = 'table { margin: 2px; } td, th { border: 1px solid; }'
WORDS = 'lorem ipsum dolor sit amet consectetur adipiscing'

# Column widths: the sizing-guesses and the excess width rules.
doc('auto-layout-text', D + '''<table><tr><td>a</td><td>bb bb</td><td>ccc ccc ccc</td></tr><tr><td>dddd</td><td></td><td>e</td></tr></table>
<table style="width:400px"><tr><td>a</td><td>bb bb</td><td>ccc ccc ccc</td></tr></table>
<table style="width:100%%"><tr><td>a</td><td>bb bb</td><td>ccc ccc ccc</td></tr></table>
<table style="width:60px"><tr><td>%s</td><td>%s</td></tr></table>
<div style="width:120px"><table><tr><td>%s</td><td>%s %s</td></tr></table></div>''' % (WORDS, WORDS, WORDS, WORDS, WORDS), B)
doc('percent-columns', D + '''<table style="width:500px"><tr><td style="width:30%">a</td><td style="width:20%">b</td><td>c</td></tr></table>
<table style="width:500px"><tr><td style="width:60%">a</td><td style="width:70%">b</td><td>c</td></tr></table>
<table><tr><td style="width:25%">auto table</td><td>other</td></tr></table>
<table style="width:400px"><tr><td style="width:10%">a</td><td style="width:15%">b</td></tr></table>
<table style="width:400px"><tr><td style="width:50%;max-width:20%">max</td><td style="width:0%">zero</td><td>x</td></tr></table>
<table style="width:300px"><tr><td style="width:40%" colspan=2>span pct</td><td>c</td></tr><tr><td>a</td><td>b</td><td style="width:30%">d</td></tr></table>''', B)
doc('constrained-columns', D + '''<table style="width:600px"><tr><td style="width:100px">a</td><td style="width:50px">b</td><td>c</td></tr></table>
<table style="width:600px"><tr><td style="width:100px">a</td><td style="width:50px">b</td></tr></table>
<table style="width:600px"><tr><td></td><td></td><td style="width:40px">x</td></tr></table>
<table style="width:200px"><tr><td style="width:150px">a</td><td style="width:150px">b</td></tr></table>
<table style="width:500px"><tr><td style="min-width:120px">min</td><td style="max-width:30px">%s</td><td style="width:20px;min-width:60px">both</td></tr></table>''' % WORDS, B)
doc('columns-without-originating-cells', D + '''<table style="width:500px"><col><col><col><tr><td colspan=3>wide spanning cell</td></tr><tr><td>a</td><td colspan=2>b</td></tr></table>
<table style="width:400px"><tr><td colspan=2>only spanning</td></tr><tr><td colspan=2>also spanning</td></tr></table>
<table style="width:300px"><colgroup><col span=2 style="width:50px"><col></colgroup><tr><td>a</td><td>b</td><td>c</td></tr></table>''', B)
doc('fixed-layout', D + '''<table style="table-layout:fixed;width:500px"><tr><td style="width:100px">a</td><td>b</td><td>c</td></tr><tr><td style="width:300px">ignored second row</td><td>%s</td><td>d</td></tr></table>
<table style="table-layout:fixed;width:500px"><tr><td style="width:100px">a</td><td style="width:50px">b</td></tr></table>
<table style="table-layout:fixed;width:500px"><tr><td style="width:20%%">a</td><td style="width:30%%">b</td></tr></table>
<table style="table-layout:fixed;width:500px"><tr><td style="width:20%%">a</td><td>b</td><td style="width:60px">c</td></tr></table>
<table style="table-layout:fixed;width:50%%"><tr><td>a</td><td>b</td></tr></table>
<table style="table-layout:fixed;width:min-content"><tr><td style="width:40px">a</td><td>%s</td></tr></table>
<table style="table-layout:fixed;width:fit-content"><tr><td style="width:40px">a</td><td>b</td></tr></table>
<table style="table-layout:fixed"><tr><td style="width:40px">auto width is auto mode</td><td>b</td></tr></table>
<table style="table-layout:fixed;width:300px"><col style="width:80px"><col><tr><td>a</td><td>b</td></tr></table>''' % (WORDS, WORDS), B)

# Spanning cells: the min-content and max-content contributions of cells of span N.
doc('colspan-contributions', D + '''<table><tr><td colspan=2>%s</td></tr><tr><td>a</td><td>bbbbbb</td></tr></table>
<table><tr><td colspan=3>a long spanning cell with words</td></tr><tr><td></td><td></td><td></td></tr></table>
<table style="border-spacing:10px"><tr><td colspan=2 style="width:300px">wide</td><td>c</td></tr><tr><td>a</td><td>b</td><td>c</td></tr></table>
<table><tr><td colspan=2>xx</td></tr><tr><td>%s</td><td>b</td></tr></table>
<table style="width:600px"><tr><td colspan=2 style="width:50%%">half</td><td colspan=2>rest</td></tr><tr><td>a</td><td>b</td><td>c</td><td>d</td></tr></table>''' % (WORDS, WORDS), B)
doc('rowspan-measures', D + '''<table><tr><td rowspan=2 style="height:100px">tall</td><td>a</td></tr><tr><td>b</td></tr></table>
<table><tr><td rowspan=3>%s</td><td>a</td></tr><tr><td>b</td></tr><tr><td style="height:40px">c</td></tr></table>
<table style="border-spacing:4px 12px"><tr><td rowspan=2 style="height:90px">tall</td><td style="height:10px">a</td></tr><tr><td>b</td></tr></table>
<table><tr><td rowspan=2 style="height:30%%">pct</td><td>a</td></tr><tr><td>b</td></tr></table>''' % WORDS, B)

# Heights: the table height and its distribution to the rows.
doc('table-heights', D + '''<table style="height:300px"><tr><td>a</td></tr><tr><td>b</td></tr><tr style="height:50px"><td>c</td></tr></table>
<table style="height:300px"><tr style="height:40px"><td>a</td></tr><tr style="height:60px"><td>b</td></tr></table>
<table style="height:30px"><tr><td>%s</td></tr><tr><td>b</td></tr></table>
<table style="height:200px"><tr style="height:50%%"><td>half</td></tr><tr><td>rest</td></tr></table>
<table style="height:200px"><tr><td style="height:25%%">quarter</td></tr><tr><td>rest</td></tr></table>
<table style="height:200px;box-sizing:border-box;border:10px solid;padding:5px"><tr><td>bb</td></tr></table>
<div style="height:400px"><table style="height:50%%"><tr><td>pct of wrapper cb</td></tr></table></div>''' % WORDS, B)
doc('row-heights', D + '''<table><tr style="height:60px"><td>a</td><td style="height:20px">b</td></tr><tr><td style="height:45px">c</td><td>d</td></tr></table>
<table><tr><td style="min-height:30px">min</td><td style="max-height:5px">%s</td></tr><tr style="min-height:70px;max-height:10px"><td>e</td></tr></table>
<table><tr><td style="height:50px;box-sizing:border-box;padding:10px">bb</td><td style="padding:3%%">pct padding</td></tr></table>''' % WORDS, B)

# Separated and collapsed borders.
doc('border-spacing', D + '''<table style="border-spacing:10px 5px"><tr><td>a</td><td>b</td></tr><tr><td colspan=2>c</td></tr></table>
<table style="border-spacing:0"><tr><td>a</td><td>b</td></tr></table>
<table style="border-spacing:7px;border:3px solid;padding:4px"><tr><td>a</td></tr></table>
<table style="border-spacing:1em 2em;font-size:10px"><thead><tr><td>h</td></tr></thead><tbody><tr><td>a</td></tr><tr><td>b</td></tr></tbody></table>
<table style="border-spacing:5px"></table>
<table style="border-spacing:5px"><tr></tr></table>''', B)
doc('border-collapse-widths', D + '''<table style="border-collapse:collapse;border:5px solid"><tr><td style="border:1px solid">a</td><td style="border:3px solid">b</td></tr><tr><td style="border:7px solid">c</td><td>d</td></tr></table>
<table style="border-collapse:collapse"><tr style="border:4px solid"><td style="border:2px solid">a</td><td>b</td></tr><tr><td style="border:1px dotted">c</td><td style="border-top:9px double">d</td></tr></table>
<table style="border-collapse:collapse;border-spacing:20px"><tr><td>spacing ignored</td></tr></table>''', 'td { padding: 3px; }')
doc('border-collapse-styles', D + '''<table style="border-collapse:collapse"><tr>%s</tr></table>
<table style="border-collapse:collapse"><tr><td style="border:3px hidden">hidden</td><td style="border:5px solid">solid</td></tr><tr><td style="border:3px none">none</td><td style="border:0">zero</td></tr></table>
<table style="border-collapse:collapse;border:2px solid red"><tr><td style="border:2px solid green">same width</td><td style="border:2px solid blue">x</td></tr></table>''' % ''.join('<td style="border:4px %s">%s</td>' % (s, s) for s in ['double', 'solid', 'dashed', 'dotted', 'ridge', 'outset', 'groove', 'inset']), 'td { padding: 2px; }')
doc('border-collapse-groups', D + '''<table style="border-collapse:collapse;border:1px solid"><colgroup style="border:6px solid"><col style="border:3px solid"><col></colgroup><col style="border:8px solid">
<thead style="border:5px solid"><tr><td>h1</td><td>h2</td><td>h3</td></tr></thead><tbody style="border:2px solid"><tr style="border:4px solid"><td>a</td><td>b</td><td>c</td></tr><tr><td>d</td><td>e</td><td>f</td></tr></tbody>
<tfoot style="border:7px dashed"><tr><td>f1</td><td>f2</td><td>f3</td></tr></tfoot></table>
<table style="border-collapse:collapse"><col span=2 style="border:5px solid"><tr><td>a</td><td>b</td><td>c</td></tr></table>''', 'td { padding: 2px; }')
doc('border-collapse-spans', D + '''<table style="border-collapse:collapse;border:3px solid"><tr><td rowspan=2 style="border:6px solid">tall</td><td style="border:1px solid">a</td><td>b</td></tr>
<tr><td colspan=2 style="border:4px dotted">wide</td></tr><tr><td>c</td><td style="border:9px solid">d</td><td>e</td></tr></table>
<table style="border-collapse:collapse"><tr><td style="border:2px solid">a</td><td>b</td><td style="border:5px solid">c</td><td>d</td></tr></table>''', 'td { padding: 2px; border: 1px solid; }')

# Captions.
doc('captions', D + '''<table><caption>top caption</caption><tr><td>a</td></tr></table>
<table><caption style="caption-side:bottom">bottom caption</caption><tr><td>a</td></tr></table>
<table><caption style="margin:5px 10px;padding:3px;border:2px solid">boxed</caption><caption style="caption-side:bottom;margin-top:7px">second</caption><tr><td>a</td><td>b</td></tr></table>
<table><caption>a caption that is much wider than the table itself</caption><tr><td>x</td></tr></table>
<table><caption style="width:300px">fixed width caption</caption><tr><td>x</td></tr></table>
<table><caption style="width:50%%">percent width caption</caption><tr><td>x</td></tr></table>
<table><caption style="height:40px">tall caption</caption><caption>%s</caption><tr><td>x</td></tr></table>
<table style="width:200px"><caption style="contain:size">contained</caption><tr><td>x</td></tr></table>''' % WORDS, B)

# Vertical alignment of cells.
doc('vertical-align', D + '<table><tr style="height:80px">' + ''.join('<td style="vertical-align:%s">%s</td>' % (v, v) for v in
    ['top', 'middle', 'bottom', 'baseline', 'sub', 'super', 'text-top', 'text-bottom', '10px', '20%']) + '<td style="font-size:30px">Big</td></tr></table>' +
    '''<table><tr><td style="vertical-align:baseline;padding-top:15px">pad</td><td style="vertical-align:baseline;font-size:24px">big</td><td style="vertical-align:middle" rowspan=2>span</td></tr><tr><td>x</td><td style="vertical-align:bottom">y</td></tr></table>
<table><tr><td valign=top style="height:60px">top</td><td valign=bottom>bottom</td><td valign=middle>middle</td></tr></table>''', B)

# Row groups, collapsed rows and anonymous table parts.
doc('row-groups', D + '''<table style="border-spacing:3px"><tfoot><tr><td>foot</td></tr></tfoot><tbody><tr><td>body1</td></tr><tr><td>body2</td></tr></tbody><thead><tr><td>head</td></tr></thead></table>
<table><tr><td>direct row</td></tr><tbody><tr><td>grouped</td></tr></tbody><tr><td>direct again</td></tr></table>
<table><tbody></tbody><tbody><tr><td>after empty group</td></tr></tbody></table>''', B)
doc('visibility-collapse', D + '''<table><tr><td>a</td><td>b</td></tr><tr style="visibility:collapse"><td>collapsed</td><td>row</td></tr><tr><td>c</td><td>d</td></tr></table>
<table style="height:200px"><tbody style="visibility:collapse"><tr><td>collapsed group</td></tr></tbody><tbody><tr><td>visible</td></tr><tr><td>visible2</td></tr></tbody></table>
<table style="height:150px"><tr style="visibility:collapse;height:40px"><td>x</td></tr><tr style="height:30px"><td>y</td></tr></table>''', B)
doc('anonymous-table-parts', D + '''<div style="display:table-cell;border:1px solid">lone cell</div><div style="display:table-cell">second lone cell</div>
<div style="display:table;border-spacing:4px"><div style="display:table-row"><div style="display:table-cell">r1c1</div><div style="display:table-cell">r1c2</div></div><span style="display:table-cell">stray cell</span></div>
<div style="display:table">text directly in a table</div>
<div style="display:table-row"><div>row without table</div></div>
<div style="display:inline-table;border:1px solid"><span style="display:table-cell">inline table</span></div> after''', '')
doc('cells-with-flex', D + '''<table><tr style="height:80px"><td style="display:flex;align-items:center">flex cell</td><td>normal</td></tr></table>
<table><tr><td><div style="display:flex"><div>flex in cell</div><div>b</div></div></td></tr></table>''', B)

# The table's own width: min/max, box-sizing, intrinsic keywords, containers.
doc('table-width-keywords', D + '''<table style="width:300px;box-sizing:border-box;padding:10px;border:5px solid"><tr><td>bb</td></tr></table>
<table style="width:50px;min-width:150px"><tr><td>min wins</td></tr></table>
<table style="width:400px;max-width:200px"><tr><td>max wins</td></tr></table>
<table style="min-width:250px"><tr><td>auto with min-width</td></tr></table>
<table style="width:max-content"><tr><td>%s</td></tr></table>
<table style="width:min-content"><tr><td>%s</td></tr></table>
<table style="width:fit-content"><tr><td>%s</td></tr></table>
<table style="width:10px"><tr><td>%s</td></tr></table>''' % (WORDS, WORDS, WORDS, WORDS), B)
doc('table-in-containers', D + '''<div style="float:left"><table><tr><td>%s</td></tr></table></div><div style="clear:both"></div>
<div style="float:left"><table style="width:50%%"><tr><td>indefinite percentage</td></tr></table></div><div style="clear:both"></div>
<div style="display:inline-block"><table style="width:100%%"><tr><td>a</td><td>b</td></tr></table></div>
<div style="width:min-content"><table><tr><td>%s</td></tr></table></div>
<div style="width:max-content"><table><tr><td>%s</td></tr></table></div>
<span>before <table style="display:inline-table"><tr><td>inline</td></tr></table> after</span>
<table style="float:right"><tr><td>floated table</td></tr></table>
<div style="position:relative;height:60px"><table style="position:absolute;right:0;bottom:0"><tr><td>abspos table</td></tr></table></div>
<div style="display:flex"><table><tr><td>flex item table</td></tr></table><table style="flex:1"><tr><td>growing table</td></tr></table></div>''' % (WORDS, WORDS, WORDS), B)
doc('table-margins-and-align', D + '''<table style="margin:0 auto"><tr><td>centered</td></tr></table>
<table align=center><tr><td>align center</td></tr></table>
<table align=right><tr><td>align right</td></tr></table>
<table style="margin-left:30%;margin-top:10px"><tr><td>pct margin</td></tr></table>''', B)
doc('nested-tables', D + '''<table><tr><td><table><tr><td>inner</td><td>%s</td></tr></table></td><td>outer</td></tr></table>
<table style="width:400px"><tr><td><table style="width:100%%"><tr><td>full width inner</td></tr></table></td><td style="width:100px">x</td></tr></table>''' % WORDS, B)

# Columns: col/colgroup widths, spans and percentages.
doc('columns', D + '''<table><colgroup><col style="width:80px"><col style="width:30px"></colgroup><tr><td>a</td><td>b</td><td>c</td></tr></table>
<table style="width:500px"><col style="width:20%"><col style="width:40%"><tr><td>a</td><td>b</td><td>c</td></tr></table>
<table><col span=3 style="width:40px"><tr><td>a</td><td>b</td><td>c</td><td>d</td></tr></table>
<table><colgroup style="width:100px"></colgroup><colgroup><col style="min-width:70px;max-width:20px"></colgroup><tr><td>a</td><td>b</td></tr></table>
<table style="width:300px"><col style="max-width:50%"><tr><td>a</td><td>b</td></tr></table>''', B)

# Positioned boxes in tables.
doc('positioned-in-tables', D + '''<table><tr><td style="position:relative;height:50px">cell<div style="position:absolute;top:5px;left:5px">abs</div></td></tr></table>
<table style="position:relative"><tr><td>x<span style="position:absolute;bottom:0">abs in table</span></td></tr></table>
<table><tr style="position:relative;top:10px"><td>relative row</td></tr><tr><td style="position:relative;left:20px">relative cell</td></tr></table>''', B)

# Presentational hints and quirks.
doc('presentational-hints', D + '''<table border=2 cellpadding=6 cellspacing=3><tr><td>a</td><td>b</td></tr></table>
<table border=0 cellspacing=0 width=300><tr><td width=50>a</td><td height=40>b</td></tr></table>
<table bgcolor=yellow width=50%%><tr><th>th centered bold</th><td nowrap>%s</td></tr></table>''' % WORDS, '')
doc('quirks-tables', '''<table><tr><td>quirks</td><td style="height:50%">pct</td></tr></table>
<table style="height:100px"><tr><td><span style="display:inline-block;height:20px;width:20px"></span></td></tr></table>
<table><tr><td style="font-size:30px">big</td></tr></table>''', '')

# Edge cases of the grid.
doc('grid-edge-cases', D + '''<table><tr><td rowspan=0>rowspan 0</td><td>a</td></tr><tr><td>b</td></tr></table>
<table><tr><td rowspan=5>beyond the end</td><td>a</td></tr></table>
<table><tr><td colspan=2>a</td></tr><tr><td colspan=4>wider</td></tr></table>
<table><tr><td rowspan=2>a</td><td>b</td></tr><tr><td colspan=2>overlap</td></tr></table>
<table><tr></tr><tr><td>after empty row</td></tr></table>
<table><caption>only caption</caption></table>''', B)

grid_html = D + '''<table><tr><td rowspan=2>a</td><td colspan=3>b</td></tr><tr><td>c</td></tr></table>
<table><col span=2><colgroup><col><col span=3></colgroup><tr><td>a</td></tr></table>
<table><colgroup span=4></colgroup><tr><td>a</td><td>b</td></tr></table>
<table><tr><td rowspan=0>zero</td><td>a</td></tr><tr><td>b</td></tr><tr><td>c</td></tr></table>
<table><tr><td rowspan=4 colspan=2>clipped</td></tr><tr><td>x</td></tr></table>
<table><tr><td rowspan=2>a</td><td>b</td></tr><tr><td colspan=2>overlapping</td></tr></table>
<table><thead><tr><td>h</td></tr></thead><tr style="visibility:collapse"><td>direct</td></tr><tbody style="visibility:collapse"><tr><td>g</td></tr></tbody><tfoot><tr><td>f</td><td>f2</td></tr></tfoot></table>
<div style="display:table"><div style="display:table-row-group"><div style="display:table-row"><div style="display:table-cell">1</div><div style="display:table-cell">2</div></div></div><div style="display:table-row"><div style="display:table-cell">3</div></div></div>
<table><tr><td colspan=1001>huge colspan</td></tr></table>
<table><tr><td>outer<table><tr><td>inner a</td><td>inner b</td></tr></table></td></tr></table>'''
doc('grid', grid_html, '', 'table')
doc('grid-anonymous', D + '''<div style="display:table-cell">a</div><div style="display:table-cell">b</div>
<span style="display:table-row">row</span><span style="display:table-row">row2</span>
<div style="display:table"><div style="display:table-column"></div><div style="display:table-column-group"><div style="display:table-column"></div></div><div style="display:table-cell">c</div></div>''', '', 'table')

# TableFormattingContext::border_is_less_specific over pairs of borders (no document).
doc('border-specificity', '', '', 'border-specificity')

out = []
for name, mode, html, css in docs:
    out.append('# own/' + name)
    out.append(mode + '\t' + esc(html) + '\t' + esc(css))
text = open('cases_table.txt').read()
marker = '# own table cases'
if marker in text:
    text = text[:text.index(marker)]
text = text.rstrip('\n') + '\n' + marker + ' (own_table_cases.py)\n' + '\n'.join(out) + '\n'
open('cases_table.txt', 'w').write(text)
print(len(docs), 'cases')
