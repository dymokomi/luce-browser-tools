# r56 (SVG II): writes cases_r56.txt, the element and attribute cases of svg/tests_svg_ii (oracle.cpp's r56 modes).
# Usage: python3 own_r56_cases.py; ./oracle cases_r56.txt > expected_r56.txt; python3 gen_luce_cases.py r56 > .../web/svg/tests_svg_ii_cases.lucb
out = ['# r56 SVG II oracle cases: <mode>\t<html> (oracle.cpp: grad, pattern, maskclip, fe, use, text, a, image, fo)']
def esc(s):
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')
def case(mode, html):
    out.append(mode + '\t' + esc(html))

# Gradients: defaults, every attribute, the href chain (href, xlink:href, a radial gradient linking a linear one),
# cycles, invalid values, stops found through the chain, offsets clamped and ordered by the painter (the raw offsets here).
case('grad', '<!DOCTYPE html><svg><linearGradient id="a"><stop offset="0"/><stop offset="0.5"/><stop offset="100%"/></linearGradient><radialGradient id="b"/></svg>')
case('grad', '<!DOCTYPE html><svg><linearGradient id="a" gradientUnits="userSpaceOnUse" spreadMethod="reflect" gradientTransform="translate(10 20) scale(2)" x1="10%" y1="5" x2="0.75" y2="80%"><stop offset="-1"/><stop offset="2"/></linearGradient><radialGradient id="b" gradientUnits="objectBoundingBox" spreadMethod="repeat" cx="30%" cy="0.4" r="25%" fx="10" fy="20%" fr="5%"/></svg>')
case('grad', '<!DOCTYPE html><svg><linearGradient id="base" x1="1" y1="2" x2="3" y2="4" spreadMethod="repeat" gradientUnits="userSpaceOnUse" gradientTransform="rotate(45)"><stop offset="0.25"/><stop offset="0.75"/></linearGradient><linearGradient id="child" href="#base"/><linearGradient id="grandchild" xlink:href="#child" x2="50%"/><radialGradient id="radial" href="#base" r="10"/><linearGradient id="other" href="#radial"/></svg>')
case('grad', '<!DOCTYPE html><svg><linearGradient id="p" href="#q" x1="1"/><linearGradient id="q" href="#p" y1="2"/><linearGradient id="self" href="#self"/><radialGradient id="r1" href="#r2" fr="-5"/><radialGradient id="r2" r="-1" fr="3"/></svg>')
case('grad', '<!DOCTYPE html><svg><linearGradient id="bad" gradientUnits="bogus" spreadMethod="nope" gradientTransform="translate(1" x1="x" y1="" x2="1e400"/><linearGradient id="empty" href=""/><linearGradient id="missing" href="#nothing"/><linearGradient id="notgrad" href="#rect"/><rect id="rect"/></svg>')
case('grad', '<!DOCTYPE html><svg><linearGradient id="stops"><stop offset="0.1"/><g><stop offset="0.9"/></g><stop offset="30%"/><stop/><stop offset="junk"/></linearGradient><linearGradient id="noStops" href="#stops"/><radialGradient id="r" href="#noStops"><stop offset="1"/></radialGradient></svg>')

# Patterns: defaults, attributes, the href chain and its content element.
case('pattern', '<!DOCTYPE html><svg><pattern id="a"/><pattern id="b" patternUnits="userSpaceOnUse" patternContentUnits="objectBoundingBox" patternTransform="skewX(30)" x="5" y="10%" width="0.5" height="20" viewBox="0 0 10 20"><rect/></pattern></svg>')
case('pattern', '<!DOCTYPE html><svg><pattern id="base" width="10" height="10" patternUnits="userSpaceOnUse" patternTransform="scale(3)"><rect/></pattern><pattern id="child" href="#base" x="2"/><pattern id="grand" xlink:href="#child" height="50%"/><pattern id="cycle1" href="#cycle2"/><pattern id="cycle2" href="#cycle1"/><pattern id="bad" patternUnits="x" patternTransform="nope" width="-"/></svg>')

# Masks and clip paths.
case('maskclip', '<!DOCTYPE html><svg><mask id="m1"/><mask id="m2" maskUnits="userSpaceOnUse" maskContentUnits="objectBoundingBox"/><mask id="m3" maskUnits="bogus" maskContentUnits="userSpaceOnUse"/><clipPath id="c1"/><clipPath id="c2" clipPathUnits="objectBoundingBox"/><clipPath id="c3" clipPathUnits="userSpaceOnUse"/></svg>')

# Filters and filter primitives.
case('fe', '<!DOCTYPE html><svg><filter id="f"><feBlend in="SourceGraphic" in2="a" mode="multiply" result="b"/><feBlend mode="bogus"/><feBlend mode="color-dodge"/><feColorMatrix type="saturate" values="0.5"/><feColorMatrix/><feColorMatrix type="hueRotate"/><feColorMatrix type="LUMINANCEtoALPHA"/><feColorMatrix type="nope"/><feComponentTransfer in="x"><feFuncR/></feComponentTransfer></filter></svg>')
case('fe', '<!DOCTYPE html><svg><filter id="g" filterUnits="userSpaceOnUse" primitiveUnits="objectBoundingBox" x="10" width="50%"><feComposite in="a" in2="b" operator="arithmetic" k1="1" k2="-0.5" k3="2e1" k4="x"/><feComposite operator="xor"/><feComposite operator="lighter"/><feComposite operator="in"/><feComposite/><feDisplacementMap in="a" in2="b" scale="10" xChannelSelector="R" yChannelSelector="G"/><feDisplacementMap xChannelSelector="b" yChannelSelector="A"/><feDropShadow dx="3" dy="-4" stdDeviation="5 6"/><feDropShadow stdDeviation="7"/><feFlood/></filter></svg>')
case('fe', '<!DOCTYPE html><svg><filter><feGaussianBlur in="SourceAlpha" stdDeviation="2.5" edgeMode="wrap"/><feGaussianBlur stdDeviation="1 3"/><feImage result="img"/><feMerge><feMergeNode in="a"/><feMergeNode/></feMerge><feMorphology operator="dilate" radius="2 4"/><feMorphology operator="ERODE" radius="3"/><feMorphology/><feMorphology operator="x"/><feOffset dx="1.5"/><feOffset dy="2" dx="-1"/><feTurbulence baseFrequency="0.05 0.1" numOctaves="3" seed="7.5" stitchTiles="stitch" type="fractalNoise"/><feTurbulence baseFrequency="0.2" stitchTiles="x" type="y"/></filter></svg>')

# Use elements: same-document references (before the use; a later one waits for "completely loaded", which the
# test documents never reach), symbols and svgs get the use's width and height, x and y in the transform, invalid
# and circular references; then the referenced #ref is removed.
case('use', '<!DOCTYPE html><svg><defs><symbol id="ref" viewBox="0 0 10 10"><rect width="5" height="5"/></symbol></defs><use href="#ref" x="10" y="20" width="30"/><use href="#ref" height="7" transform="scale(2)"/></svg>')
case('use', '<!DOCTYPE html><svg><svg id="ref" width="1" height="2"><circle r="1"/></svg><g id="g"><rect id="inner"/></g><use xlink:href="#ref" x="1.5"/><use href="#g"/><use href="#missing"/><use href="#later"/><g id="later"/><use href="#notsvg"/></svg><div id="notsvg"></div>')
case('use', '<!DOCTYPE html><svg><g id="a"><use href="#b"/></g><g id="b"><use href="#a"/></g><use id="self" href="#self"/><g id="ref"><use href="#a"/></g><use href="#ref" x="x" y="-3"/></svg>')

# Text: positions as lengths, percentages and numbers, the reflected lists, text contents.
case('text', '<!DOCTYPE html><svg><text x="10" y="20px" dx="5%" dy="2em" rotate="45">  Hello  <tspan x="1 2" dy="-3">span</tspan></text><text x="bogus" rotate="10%">\u00e9t\u00e9</text><text/></svg>')
case('text', '<!DOCTYPE html><svg><path id="p" d="M0 0 L10 10"/><rect id="r"/><text><textPath href="#p">along</textPath><textPath xlink:href="#r">rect</textPath><textPath href="#none">x</textPath><textPath>y</textPath><textPath href="#t">z</textPath></text><g id="t"/></svg>')

# The SVG a element.
case('a', '<!DOCTYPE html><svg><a href="x" target="_blank" rel="noopener  nofollow"><rect/></a><a xlink:href="y"/><a/></svg>')

# Images and foreignObjects (no image data in phase 1, so no fetch: no href).
case('image', '<!DOCTYPE html><svg><image x="5" y="6" width="70" height="80"/><image width="10"/><image/><image x="bogus" height="-2"/></svg>')
case('fo', '<!DOCTYPE html><svg><foreignObject x="5" width="10"></foreignObject></svg>')
open('cases_r56.txt', 'w').write('\n'.join(out) + '\n')
print(len(out) - 1, 'cases')
