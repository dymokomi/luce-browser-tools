# r49+r53: writes cases_svg.txt: Ladybird's Layout tests with SVG (those whose elements region r56 has not ported are
# marked "!"), then cases written for r49/r53 in modes "layout" (the three dumps) and "svg" (the SVG paintables'
# transforms, paths and clip path bounds; see oracle.cpp's run_svg).
# Usage: python3 own_svg_cases.py; ./oracle cases_svg.txt > expected_svg.txt; python3 gen_luce_cases.py svg > ...
import subprocess
ladybird = '''flex-container-max-content-width-with-definite-height-and-item-that-has-aspect-ratio
flex/flex-column-item-with-natural-aspect-ratio-and-automatic-cross-size
flex/intrinsic-height-of-flex-container-with-svg-item-that-only-has-natural-aspect-ratio
flex/no-stretch-fit-width-for-item-that-can-resolve-aspect-ratio-through-height
flex/stretch-fit-width-for-column-layout-svg-item-that-only-has-natural-aspect-ratio
flex/svg-flex-item-with-percentage-max-size
grid/svg-width-only-stretches-height
svg-foreignobject-absolute-positioning
svg-preserve-aspect-ratio
svg-text-dominant-baseline
svg-text-with-percentage-values
svg-text-with-viewbox
svg-transforms-and-viewboxes
svg/abspos-svg-polygon
svg/dont-stretch-fit-svg-with-indefinite-containing-block-width
svg/foreignObject-and-mask-with-different-viewbox
svg/foreignObject-simple
svg/foreignObject-with-abspos-descendant
svg/foreignObject-with-mask
svg/natural-width-from-svg-attributes
svg/nested-viewports
svg/objectBoundingBox-mask
svg/rect-percentages
svg/svg-circle-percentage-attr
svg/svg-different-types-of-opacity
svg/svg-fill-with-bogus-url
svg/svg-foreign-object-with-block-element
svg/svg-g-inside-g
svg/svg-g-with-opacity
svg/svg-inside-inline-block
svg/svg-inside-svg-with-xy
svg/svg-inside-svg
svg/svg-intrinsic-size-in-one-dimension
svg/svg-line-with-percentage-values
svg/svg-mask-url-fragment-percent-decoding
svg/svg-negative-elliptical-arg-number
svg/svg-nested-svg-display-block
svg/svg-path-with-implicit-lineto
svg/svg-symbol-with-viewbox
svg/svg-symbol-without-use
svg/svg-text-positioning
svg/svg-use-element-crashtest
svg/svg-viewbox-zero-width
svg/svg-with-css-variable-in-presentation-hint
svg/svg-with-display-block
svg/svg-with-zero-sized-viewBox
svg/text-fill-none
svg/use-honor-outer-viewBox
treat-intrinsic-sizing-keywords-as-auto-when-aspect-ratio-cant-be-resolved
zero-height-replaced-box-with-aspect-ratio
zero-size-replaced-box-with-aspect-ratio'''.split()
# r56 ported the SVG II elements (text, mask, clipPath, pattern, foreignObject, symbol, use, image, gradients, filters,
# a); a case still waits ("!") when it needs a resource fetched (P2): an <image>, or a <use> of another document.
r56 = ['<image', 'href="../']

def waits(line):
    # (A <style> element no longer waits: since the P1 polish the unit harness makes its documents in a Window's
    # realm, whose CSP list a connected style element asks for.)
    return any(tag.lower() in line.lower() for tag in r56)

out = ['# r49+r53 SVG oracle cases: <mode>\t<html>[\t<css>]; "!" = waits for other regions (P2: fetched resources).',
       "# Ladybird's Tests/LibWeb/Layout/input tests with SVG (from_ladybird.py):"]
conv = subprocess.run(['python3', 'from_ladybird.py'] + ladybird, capture_output=True, text=True, check=True).stdout.split('\n')
for l in conv:
    if not l:
        continue
    if l.startswith('# '):
        out.append(l)
    else:
        out.append(('!' if waits(l) else '') + l)

def esc(s):
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')

def case(mode, html, css=''):
    line = mode + '\t' + esc(html) + ('\t' + esc(css) if css else '')
    out.append(('!' if waits(line) else '') + line)

out.append('# Written for r49: preserveAspectRatio, every alignment, meet and slice (scale_and_align_viewbox_content)')
aligns = ['none', 'xMinYMin', 'xMidYMin', 'xMaxYMin', 'xMinYMid', 'xMidYMid', 'xMaxYMid', 'xMinYMax', 'xMidYMax', 'xMaxYMax']
for mos in ['meet', 'slice']:
    svgs = ''.join('<svg width="160" height="70" viewBox="5 10 50 40" preserveAspectRatio="%s %s"><rect x="10" y="15" width="30" height="20"/></svg>' % (a, mos) for a in aligns)
    case('layout', '<!DOCTYPE html>' + svgs, 'svg { display: block; border: 1px solid black; margin: 2px; }')
for mos in ['meet', 'slice']:
    svgs = ''.join('<svg width="70" height="160" viewBox="0 0 30 40" preserveAspectRatio="%s %s"><circle cx="15" cy="20" r="10"/></svg>' % (a, mos) for a in aligns)
    case('layout', '<!DOCTYPE html>' + svgs, 'svg { margin: 1px; }')
out.append('# viewBox edge cases: negative and zero sizes, offsets, fractional scales, viewBox on nested viewports')
case('layout', '<!DOCTYPE html><svg width="100" height="100" viewBox="0 0 -10 50"><rect width="40" height="40"/></svg><svg width="100" height="100" viewBox="0 0 50 0"><rect width="40" height="40"/></svg><svg width="100" height="100" viewBox="-20 -30 70 90"><rect x="-10" y="-20" width="40" height="40"/></svg><svg width="99" height="77" viewBox="0 0 7 3"><rect x="1" y="1" width="3" height="1"/></svg>')
case('layout', '<!DOCTYPE html><svg width="300" height="200"><svg x="10" y="20" width="100" height="50" viewBox="0 0 10 10"><rect width="5" height="5"/></svg><svg x="25%" y="10%" width="50%" height="40%"><rect width="20" height="30"/></svg><svg x="150" y="100"><rect width="10" height="10"/></svg></svg>')
case('layout', '<!DOCTYPE html><svg width="400" height="300" viewBox="0 0 200 150"><svg x="10" y="10" width="100" height="100" viewBox="0 0 50 50" preserveAspectRatio="xMinYMax slice"><rect width="80" height="20"/><svg x="5" y="5" width="20" height="20" viewBox="0 0 2 4"><rect width="1" height="1"/></svg></svg></svg>')
out.append('# Groups and transforms: nested <g>, transform lists, empty groups, display and visibility')
case('layout', '<!DOCTYPE html><svg width="300" height="300"><g transform="translate(10 20)"><rect width="30" height="40"/><g transform="scale(2)"><circle cx="20" cy="20" r="5"/><g></g></g></g><g transform="matrix(1 0 0 1 100 100)"><ellipse cx="10" cy="10" rx="20" ry="5"/></g></svg>')
case('layout', '<!DOCTYPE html><svg width="300" height="300"><rect x="50" y="50" width="60" height="30" transform="rotate(30)"/><rect x="50" y="150" width="60" height="30" transform="skewX(20) translate(5 5)"/><line x1="200" y1="10" x2="250" y2="90" transform="rotate(-45 225 50)" stroke="black" stroke-width="3"/></svg>')
case('layout', '<!DOCTYPE html><svg width="200" height="200"><g><rect x="10" y="10" width="20" height="20" style="display: none"/><rect x="40" y="40" width="20" height="20" visibility="hidden"/><g style="display: none"><rect width="5" height="5"/></g></g></svg>')
out.append('# Shapes with and without strokes, stroke widths in user units, percentages')
case('layout', '<!DOCTYPE html><svg width="400" height="300" viewBox="0 0 200 150"><rect x="5" y="5" width="40" height="30" rx="5" stroke="red" stroke-width="4"/><circle cx="80" cy="30" r="20" stroke="blue" stroke-width="1.5" fill="none"/><ellipse cx="140" cy="30" rx="30" ry="15" stroke="green"/><line x1="10" y1="80" x2="60" y2="140" stroke="black" stroke-width="7"/><polyline points="80,80 100,140 120,90" stroke="black" stroke-width="2" fill="none"/><polygon points="140,80 190,90 170,140" stroke="none"/></svg>')
case('layout', '<!DOCTYPE html><svg width="300" height="200"><path d="M 10 10 L 90 10 L 90 90 Z" stroke="black" stroke-width="10"/><path d="M 120 50 A 40 20 30 1 1 200 60 Q 220 120 250 50 C 260 10 280 190 290 100" fill="none" stroke="black"/><rect x="10%" y="50%" width="20%" height="25%" stroke="black" stroke-width="2%"/></svg>')
case('layout', '<!DOCTYPE html><svg width="200" height="100"><rect width="50" height="50" stroke="black" stroke-width="0"/><rect x="60" width="30" height="30" style="stroke: red; stroke-width: 6px"/><rect x="100" width="30" height="30" stroke="red" stroke-width="6" stroke-opacity="0"/></svg>')
out.append('# The <svg> box: sizing in blocks, inline, flex and grid, attributes against CSS, natural sizes')
case('layout', '<!DOCTYPE html><div class=a><svg width="100" viewBox="0 0 40 20"><rect width="40" height="20"/></svg></div><div class=b><svg viewBox="0 0 30 60"><rect width="30" height="60"/></svg></div><span>text <svg width="20" height="10"><rect width="20" height="10"/></svg> text</span>', '.a { width: 300px; } .b { width: 120px; }')
case('layout', '<!DOCTYPE html><div class=flex><svg width="50" height="40"><rect width="10" height="10"/></svg><svg viewBox="0 0 10 20"><rect width="10" height="10"/></svg></div><div class=grid><svg height="30" viewBox="0 0 20 10"><rect width="20" height="10"/></svg><svg style="width: 50%"><circle cx="10" cy="10" r="10"/></svg></div>', '.flex { display: flex; width: 200px; } .grid { display: grid; grid-template-columns: 100px 80px; }')
case('layout', '<!DOCTYPE html><svg style="width: 120px; height: 60px" width="10" height="10" viewBox="0 0 10 10"><rect width="10" height="10"/></svg><svg width="0" height="50"><rect width="10" height="10"/></svg><svg style="display: block; width: 100%; height: auto" viewBox="0 0 40 10"><rect width="40" height="10"/></svg>')
case('layout', '<!DOCTYPE html><svg width="100" height="100" style="position: absolute; left: 20px; top: 30px"><rect x="10" y="10" width="30" height="30"/></svg><div style="transform: translate(5px, 5px)"><svg width="50" height="50"><circle cx="25" cy="25" r="20" stroke="black"/></svg></div>')
out.append('# Form controls whose user-agent shadow trees hold SVG (number steppers, select chevrons) and the checkbox and radio boxes')
case('layout', '<!DOCTYPE html><input type=number value=5 style="width: 120px"><input type=number style="width: 60px; font-size: 20px"><select><option>one</option><option selected>two</option></select><select style="height: 40px"></select>')
case('layout', '<!DOCTYPE html><input type=checkbox><input type=checkbox checked style="width: 30px; height: 20px"><input type=radio><input type=radio checked style="margin: 10px"><label><input type=checkbox> label</label>')
out.append('# The "svg" mode: computed transforms, paths, clip path bounds and anti-aliasing of the SVG paintables')
out.append('# (without arcs, whose points come from libm and differ in the last digits off macOS)')
case('svg', '<!DOCTYPE html><svg width="200" height="100" viewBox="0 0 100 50"><g transform="translate(10 5)"><rect x="5" y="5" width="20" height="10" stroke="black" stroke-width="2"/><g transform="scale(2 0.5)"><polygon points="16,16 24,16 20,24"/></g></g><path d="M 0 0 L 50 25 H 80 V 40 Z" shape-rendering="crispEdges"/><line x1="0" y1="50" x2="100" y2="0" stroke="red" shape-rendering="optimizeSpeed"/></svg>')
case('svg', '<!DOCTYPE html><div style="padding: 13px"><svg width="150" height="150" viewBox="10 20 30 30" preserveAspectRatio="xMaxYMax slice"><polygon points="10,20 40,20 25,50"/><polyline points="12,22 14,30 30,25" fill="none" stroke="blue"/><svg x="20" y="30" width="10" height="10" viewBox="0 0 4 4"><rect width="2" height="2" style="visibility: hidden"/><rect x="2" y="2" width="2" height="2"/></svg></svg></div>')
case('svg', '<!DOCTYPE html><svg width="120" height="80"><polygon points="10,10 110,10 60,70" transform="matrix(1 0 0 1 3 4)"/><rect x="25%" y="25%" width="50%" height="50%"/><g style="display: none"><rect width="10" height="10"/></g><path d=""/></svg>')
out.append('# Written for r56 (SVG II): masks and clip paths in both unit systems, use and symbol, text, tspan and textPath,')
out.append('# foreignObject, patterns, gradients and filters (layout and the "svg" mode)')
r56_cases = [
    '<!DOCTYPE html><svg width="200" height="150"><mask id="m1"><rect x="10" y="10" width="50" height="40" fill="white"/></mask><mask id="m2" maskUnits="userSpaceOnUse" maskContentUnits="objectBoundingBox" x="0" y="0" width="200" height="150"><rect width="0.5" height="0.5" fill="white"/></mask><rect x="5" y="5" width="80" height="60" mask="url(#m1)"/><rect x="100" y="20" width="60" height="100" mask="url(#m2)"/></svg>',
    '<!DOCTYPE html><svg width="200" height="150" viewBox="0 0 100 75"><clipPath id="c1"><rect x="10" y="10" width="30" height="20"/><polygon points="50,5 90,5 70,40"/></clipPath><clipPath id="c2" clipPathUnits="objectBoundingBox"><rect x="0.25" y="0.25" width="0.5" height="0.5"/></clipPath><g clip-path="url(#c1)"><rect width="100" height="75" fill="green"/></g><rect x="10" y="45" width="40" height="20" clip-path="url(#c2)"/></svg>',
    '<!DOCTYPE html><svg width="300" height="200"><defs><symbol id="s" viewBox="0 0 10 10"><rect x="1" y="1" width="8" height="8"/></symbol><g id="g"><rect width="20" height="10"/><polygon points="25,0 35,0 30,10"/></g></defs><use href="#s" x="10" y="10" width="50" height="50"/><use href="#s" x="100" y="10" width="80" height="40"/><use href="#g" x="10" y="100" transform="scale(2)"/><use xlink:href="#g" x="150" y="150"/></svg>',
    '<!DOCTYPE html><svg width="200" height="100"><g id="a"><use href="#b"/></g><g id="b"><use href="#a"/><rect width="10" height="10"/></g><use href="#missing" x="5"/><use href="#b" x="50" y="20"/></svg>',
    '<!DOCTYPE html><svg width="300" height="150" style="font: 16px SerenitySans"><text x="10" y="30">Hello</text><text x="10" y="60" dx="5" dy="-3">shifted<tspan x="120" dy="10">span</tspan></text><text x="45%" y="40%" text-anchor="middle">middle</text><text x="290" y="140" text-anchor="end">end</text></svg>',
    '<!DOCTYPE html><svg width="300" height="150" style="font: 12px SerenitySans"><path id="p" d="M 10 100 L 150 20 L 290 100" fill="none" stroke="black"/><text><textPath href="#p">along the path</textPath></text></svg>',
    '<!DOCTYPE html><svg width="200" height="200"><foreignObject x="20" y="30" width="150" height="80"><div style="width: 100px; height: 40px; background: red">box</div></foreignObject><g transform="translate(10 10)"><foreignObject width="50" height="50"><span>t</span></foreignObject></g></svg>',
    '<!DOCTYPE html><svg width="200" height="120"><defs><pattern id="pt" width="20" height="20" patternUnits="userSpaceOnUse"><rect width="10" height="10" fill="red"/><polygon points="11,11 19,11 15,19"/></pattern><pattern id="pt2" href="#pt" x="5" patternTransform="rotate(0)"/><linearGradient id="lg"><stop offset="0" stop-color="red"/><stop offset="1" stop-color="blue"/></linearGradient><radialGradient id="rg" href="#lg"/></defs><rect width="90" height="50" fill="url(#pt)"/><rect x="100" width="90" height="50" fill="url(#pt2)" stroke="url(#lg)"/><rect y="60" width="90" height="50" fill="url(#lg)"/><rect x="100" y="60" width="90" height="50" fill="url(#rg)"/></svg>',
    '<!DOCTYPE html><svg width="200" height="100"><filter id="f"><feFlood flood-color="green" result="fl"/><feGaussianBlur in="SourceGraphic" stdDeviation="2"/><feOffset dx="3" dy="4"/><feMerge><feMergeNode in="fl"/><feMergeNode/></feMerge></filter><rect x="10" y="10" width="50" height="50" filter="url(#f)"/><a href="#x"><rect x="100" y="10" width="40" height="40"/></a></svg>',
]
for html in r56_cases:
    case('layout', html)
    case('svg', html)
open('cases_svg.txt', 'w').write('\n'.join(out) + '\n')
print(len(out), 'lines;', sum(1 for l in out if l.startswith('!')), 'waiting')
