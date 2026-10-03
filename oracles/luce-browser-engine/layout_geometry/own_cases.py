# r45's own inline layout cases, as oracle case lines (each document in "layout" and "lines" modes).
def esc(s):
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')
D = '<!DOCTYPE html>'
docs = []
def doc(name, html, css='', modes=('layout', 'lines')):
    docs.append((name, html, css, modes))

doc('text-align', D + '''<div class=a style="text-align:left">left aligned</div><div class=a style="text-align:right">right aligned</div>
<div class=a style="text-align:center">centered text</div><div class=a style="text-align:start">start ltr</div>
<div class=a style="text-align:end">end ltr</div><div class=a style="text-align:start;direction:rtl">start rtl</div>
<div class=a style="text-align:end;direction:rtl">end rtl</div><div class=a style="text-align:right">a much longer line of text that overflows the box</div>
<center><div style="width:300px">in center</div></center><div align=right style="width:300px">align right</div>''', '.a { width: 300px; border: 1px solid; }')
doc('text-align-justify', D + '''<div style="width:200px;text-align:justify">The quick brown fox jumps over the lazy dog and keeps running far away.</div>
<div style="width:200px;text-align:justify;text-justify:none">The quick brown fox jumps over the lazy dog again.</div>
<div style="width:200px;text-align:justify">Forced break<br>after a short line and a much longer one that wraps around.</div>
<div style="width:150px;text-align:justify">Words <span style="padding:0 5px">with spans</span> and more words to justify here.</div>''')
doc('text-indent', D + '''<div style="width:300px;text-indent:20px">Indented first line of a paragraph that wraps onto a second line here.</div>
<div style="width:300px;text-indent:10%">Ten percent<br>second</div>
<div style="width:300px;text-indent:15px each-line">Each line<br>gets the indent<br>after forced breaks but not after soft wraps in a long line of words.</div>
<div style="width:300px;text-indent:15px hanging">Hanging indent applies to every line except the first one in this paragraph.</div>
<div style="width:300px;text-indent:-10px">Negative indent</div>''')
doc('vertical-align-keywords', D + '''<div style="font-size:20px">Base <span style="font-size:10px;vertical-align:top">top</span> <span style="font-size:10px;vertical-align:middle">mid</span>
<span style="font-size:10px;vertical-align:bottom">bot</span> <span style="font-size:10px;vertical-align:text-top">tt</span>
<span style="font-size:10px;vertical-align:text-bottom">tb</span> <span style="vertical-align:sub">sub</span> <span style="vertical-align:super">sup</span>
<span style="font-size:30px">Big</span></div>''')
doc('vertical-align-lengths', D + '''<div>A <span style="vertical-align:5px">up5</span> <span style="vertical-align:-7px">down7</span> <span style="vertical-align:50%">half</span>
<span style="vertical-align:-25%;line-height:30px">quarter</span> B</div>''')
doc('vertical-align-inline-blocks', D + '''<div>Text <div class=ib style="height:40px"></div> <div class=ib style="height:30px;vertical-align:top"></div>
<div class=ib style="height:20px;vertical-align:middle"></div> <div class=ib style="height:25px;vertical-align:bottom"></div>
<div class=ib style="vertical-align:text-top">tt</div> <div class=ib style="vertical-align:text-bottom">tb</div>
<div class=ib style="vertical-align:10px">len</div> <div class=ib style="overflow:hidden">clip</div> <div class=ib>two<br>lines</div> end</div>''',
    '.ib { display: inline-block; width: 30px; border: 2px solid; margin: 3px 4px; padding: 1px; }')
doc('white-space-pre', D + '''<div style="white-space:pre">  two leading spaces
line two    with   runs
	tab line</div><div style="white-space:pre;width:50px">pre does not wrap at all here</div>''')
doc('white-space-pre-wrap', D + '''<div style="white-space:pre-wrap;width:120px">pre-wrap   keeps   spaces and wraps
at the box edge   with trailing spaces   </div><div style="white-space:pre-line;width:120px">pre-line   collapses   spaces
but keeps newlines</div><div style="white-space:break-spaces;width:100px">break   spaces   wrap   inside</div>''')
doc('white-space-nowrap', D + '''<div style="white-space:nowrap;width:100px">nowrap text that overflows the narrow box</div>
<div style="width:100px">wrap <span style="white-space:nowrap">no wrap inside this span</span> wrap again</div>
<div style="width:100px;text-wrap-mode:nowrap">text wrap mode no wrap here</div>''')
doc('tabs', D + '''<div style="white-space:pre">a	b		c
	lead	tab</div><div style="white-space:pre;tab-size:4">x	y</div><div style="white-space:pre;tab-size:20px">x	y	z</div>
<div style="white-space:pre;tab-size:2;letter-spacing:2px;word-spacing:3px">q	r</div>''')
doc('letter-word-spacing', D + '''<div style="letter-spacing:3px">letter spaced words</div><div style="word-spacing:10px">word spaced words here</div>
<div style="letter-spacing:-1px;width:100px">negative letter spacing wraps here</div><span style="letter-spacing:5px">span</span>''')
doc('word-break', D + '''<div style="width:60px">averyveryverylongwordthatoverflows next</div>
<div style="width:60px;word-break:break-all">averyveryverylongword broken</div>
<div style="width:60px;word-break:keep-all">keep all words here</div>
<div style="width:60px;overflow-wrap:anywhere">anotherverylongwordhere ok</div>''')
doc('text-overflow', D + '''<div class=e>This text is too long to fit on one line</div><div class=e><span>First span</span> <span>second span</span> <span>third span</span> <span>fourth</span></div>
<div class=e style="overflow:visible">Visible overflow keeps all its text</div><div class=e>W</div><div class=e><b>Bold</b>Text<i>Italic</i>MoreTextAfterwards</div>
<div class=e style="text-overflow:clip">Clip does not add an ellipsis at all</div>''',
    '.e { width: 100px; overflow: hidden; white-space: nowrap; text-overflow: ellipsis; border: 1px solid; }')
doc('floats-beside-lines', D + '''<div style="width:300px"><div style="float:left;width:100px;height:50px"></div>Text that flows beside the left float and then continues below it once the float ends here.</div>
<div style="width:300px"><div style="float:right;width:80px;height:30px"></div>Text beside a right float that wraps under it after a few lines of words.</div>
<div style="width:300px">Words before <span style="float:left;width:50px;height:20px">f</span> a float in the middle of the line and more words after it.</div>''',
    'div div, span { background: red; }')
doc('floats-narrow-lines', D + '''<div style="width:200px"><div style="float:left;width:150px;height:40px"></div><div style="float:right;width:40px;height:60px"></div>Narrow lines push the words down past the floats.</div>
<div style="width:200px"><div style="float:left;width:120px;height:30px"></div>Supercalifragilistic word</div>
<div style="width:200px">Line one<br><div style="float:left;width:180px;height:20px"></div>wide float on line two</div>''')
doc('floats-and-br-clear', D + '''<div style="width:300px"><div style="float:left;width:60px;height:60px"></div>before br<br style="clear:left">after the clearing br</div>
<div style="width:300px"><div style="float:right;width:60px;height:40px"></div>one<br style="clear:both">two<br>three</div>''')
doc('inline-boxes-across-lines', D + '''<div style="width:150px">Start <span style="padding:5px;border:2px solid;margin:0 7px">a span that wraps across several lines of text</span> end.</div>
<div style="width:150px">Nested <span style="padding-left:10px">outer <b style="padding:0 3px;border-right:4px solid">inner bold</b> outer again</span> done</div>
<div>Empty <span style="padding:0 6px;border:1px solid"></span> span and <span style="margin-left:-5px">negative margin</span></div>''')
doc('relative-inline', D + '''<div>Before <span style="position:relative;top:5px;left:10px">relative span <b style="position:relative;left:3px">nested</b></span> after</div>
<div style="position:relative;left:20px">relative block <span style="position:relative;top:-4px">and span</span></div>''')
doc('abspos-static-position', D + '''<div style="position:relative">Text <span style="position:absolute">abs inline</span> more text <div style="position:absolute">abs block</div> tail</div>
<div style="position:relative"><span>prev</span><div style="position:absolute;display:inline-block">abs after span</div></div>''')
doc('collapsible-whitespace', D + '''<div>   leading spaces    and   runs   </div><div style="width:80px">a   b   c   d   e   f   g   h</div>
<div><span> spaced </span><span> spans </span> <b> bold </b></div>''')
doc('line-height', D + '''<div style="line-height:30px">thirty px line height</div><div>normal <span style="line-height:50px">tall span</span> normal</div>
<div style="line-height:0">zero line height</div><div style="line-height:1.5;font-size:20px">factor <small>small</small></div>''')
doc('font-sizes-on-a-line', D + '''<div>small <span style="font-size:32px">BIG</span> <span style="font-size:8px">tiny</span> normal</div>''')
doc('inline-block-text', D + '''<div>Before <span style="display:inline-block">an inline block with text</span> after <span style="display:inline-block;width:50px">narrow inline block wraps</span> end</div>''')
doc('intrinsic-sizes', D + '''<div style="float:left">shrink to fit float text</div><div style="clear:both;width:min-content">min content width</div>
<div style="width:max-content">max content width text</div><div style="width:fit-content">fit content</div>
<div style="display:inline-block;max-width:80px">inline block with a max width</div>''')
doc('br-sequences', D + '''<div>one<br><br>three<br></div><div><br></div><div>a<br>b<span><br>c</span></div>''')
doc('empty-and-whitespace', D + '''<div> </div><div><span></span></div><div>
</div><p>paragraph</p>   <p> second </p>''')
doc('direction-rtl', D + '''<div style="direction:rtl;width:200px">right to left text that wraps across lines</div>
<div style="direction:rtl">abc 123 def</div><div style="direction:rtl"><span>one</span> <span>two</span></div>''')
doc('bidi-hebrew', D + '''<div>abc שלום def</div><div style="direction:rtl">שלום abc עולם</div>''')
doc('writing-mode', D + '''<div style="writing-mode:vertical-rl;height:100px">vertical text here</div><div style="writing-mode:vertical-lr">lr text</div>''')
doc('list-items-text', D + '''<ul><li>first item</li><li>second item that is long enough to wrap around in a narrow list</li></ul><ol style="width:150px"><li>one</li><li>two <b>bold</b></li></ol>''')
doc('mixed-inline-content', D + '''<div style="width:250px">Text with <b>bold</b>, <i>italic</i>, <code>code</code>, <a>an anchor</a>, <sub>sub</sub> and <sup>sup</sup> and a <span style="display:inline-block;width:20px;height:20px;background:red"></span> box.</div>''')
doc('margins-padding-percent', D + '''<div style="width:200px">x <span style="padding:5%;margin:0 10%;border:1px solid">percent</span> y</div>''')
doc('generated-content', D + '''<div class=g>text</div><div class=e></div>''', '.g::before { content: "before "; } .g::after { content: " after"; } .e::before { content: ""; padding: 3px; border: 1px solid; }')
doc('clearance-in-inline', D + '''<div style="width:300px"><div style="float:left;width:50px;height:50px"></div>a<br style="clear:left">b<div style="float:right;width:50px;height:80px"></div>c<br style="clear:right">d</div>''')

for name, html, css, modes in docs:
    print('# r45/' + name)
    for mode in modes:
        print(mode + '\t' + esc(html) + '\t' + esc(css))
