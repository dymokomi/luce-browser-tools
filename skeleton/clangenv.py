"""Shared paths and libclang setup for the luce-browser skeleton tools (local only)."""
import json
import os
import shlex
import sys

LLVM = '/opt/homebrew/opt/llvm@21'
sys.path.insert(0, LLVM + '/lib/python3.14/site-packages')
import clang.cindex as ci  # noqa: E402

ci.Config.set_library_file(LLVM + '/lib/libclang.dylib')

DONOR = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin'
BUILD = DONOR + '/Build/release'
TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = TOOLS + '/cache'
PIN = '47c82b38d0'


def compile_commands():
    with open(BUILD + '/compile_commands.json') as f:
        return json.load(f)


def clang_args(entry):
    """The compile command of one TU, minus output/warnings, plus LLVM 21's resource dir."""
    args = shlex.split(entry['command'])[1:]
    out = []
    skip = False
    for a in args:
        if skip:
            skip = False
            continue
        if a in ('-o', '-c'):
            skip = True
            continue
        if a.startswith('-W') or a == '-fcolor-diagnostics':
            continue
        out.append(a)
    out += ['-resource-dir', LLVM + '/lib/clang/21', '-Wno-everything']
    return out


def rel(path):
    """Donor-relative path of a file ('' when outside the donor tree)."""
    if not path:
        return ''
    path = os.path.realpath(path)
    for root, prefix in ((BUILD + '/', 'Build/'), (DONOR + '/', '')):
        if path.startswith(root):
            return prefix + path[len(root):]
    return ''
