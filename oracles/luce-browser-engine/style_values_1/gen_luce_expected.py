#!/usr/bin/env python3
"""gen_luce_expected.py [EXPECTED [OUTPUT]]: writes the oracle's output (expected.txt) as the Luce
fragment css/style_values/tests_style_values_1_expected.lucb of luce-browser-engine (a triple-quoted
literal whose closing delimiter is in column 0; backslashes escaped)."""
import sys, os
here = os.path.dirname(os.path.abspath(__file__))
expected = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'expected.txt')
output = sys.argv[2] if len(sys.argv) > 2 else '/Users/sedov/Dev/luce_dev/luce-browser-engine-r34/src/web/css/style_values/tests_style_values_1_expected.lucb'
text = open(expected, encoding='utf-8').read()
assert '"""' not in text and '\t' not in text and '\r' not in text
body = text.replace('\\', '\\\\')
header = '''#==============================================================================================
#
#   tests_style_values_1_expected - what the reference build prints for region r34's dump
#
#   DESCRIPTION:
#       The output of the local oracle luce-browser-tools/oracles/luce-browser-engine/
#       style_values_1 (oracle.cpp over the reference build's LibWeb), written by its
#       gen_luce_expected.py; tests_style_values_1_2 compares its own dump with it line by line.
#       Colors are ARGB, CSSPixels raw values, floats and doubles bits.
#
#==============================================================================================

## The oracle's output.
let test_style_values_1_expected: str = """
'''
open(output, 'w', encoding='utf-8').write(header + body + '"""\n')
