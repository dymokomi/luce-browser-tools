#!/usr/bin/env python3
"""gen_luce_expected.py [EXPECTED [OUTPUT]]: writes the oracle's output (expected.txt) as the Luce fragment
web/css/tests_easing_function_expected.lucb of luce-browser-engine (a triple-quoted literal whose closing delimiter is in
column 0)."""
import sys, os
here = os.path.dirname(os.path.abspath(__file__))
expected = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'expected.txt')
output = sys.argv[2] if len(sys.argv) > 2 else '/Users/sedov/Dev/luce_dev/luce-browser-engine-anim/src/web/css/tests_easing_function_expected.lucb'
text = open(expected, encoding='utf-8').read()
assert '"""' not in text and '\t' not in text and '\r' not in text and '\\' not in text
header = '''#==============================================================================================
#
#   tests_easing_function_expected - what the reference build prints for region p4a's easing
#                                    dump
#
#   DESCRIPTION:
#       The output of the local oracle luce-browser-tools/oracles/luce-browser-engine/animations
#       (oracle.cpp over the reference build's LibWeb), written by its gen_luce_expected.py;
#       tests_easing_function compares its own dump with it line by line, doubles within 64
#       units in the last place.
#
#==============================================================================================

## The oracle's output.
let test_easing_function_expected: str = """
'''
open(output, 'w', encoding='utf-8').write(header + text + '"""\n')
