#!/usr/bin/env python3
"""gen_luce_expected.py [EXPECTED [OUTPUT]]: writes the oracle's output (expected.txt) as the Luce
fragment tests_css_values_units_expected.lucb of luce-browser-engine (a triple-quoted literal whose
closing delimiter is in column 0; backslashes escaped)."""
import sys, os
here = os.path.dirname(os.path.abspath(__file__))
expected = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'expected.txt')
output = sys.argv[2] if len(sys.argv) > 2 else '/Users/sedov/Dev/luce_dev/luce-browser-engine-r36/src/luce_browser_engine/web/css/tests_css_values_units_expected.lucb'
text = open(expected, encoding='utf-8').read()
assert '"""' not in text and '\t' not in text and '\r' not in text
body = text.replace('\\', '\\\\')
header = '''#==============================================================================================
#
#   tests_css_values_units_expected - what the reference build prints for region r36's dump
#
#   DESCRIPTION:
#       The output of the local oracle luce-browser-tools/oracles/luce-browser-engine/
#       css_values_units (oracle.cpp over the reference build's LibWeb), written by its
#       gen_luce_expected.py; tests_css_values_units_2 compares its own dump with it line by
#       line. Doubles and floats are bits, CSSPixels raw values, colors ARGB.
#
#==============================================================================================

## The oracle's output.
let test_css_values_units_expected: str = """
'''
open(output, 'w', encoding='utf-8').write(header + body + '"""\n')
