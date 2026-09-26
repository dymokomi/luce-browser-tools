#!/usr/bin/env python3
"""Generate es_table_extra.lucb: more ECMAScript conformance rows, printed by upstream libregexp.

usage: tools/oracle_extra.py DRIVER

DRIVER is tools/oracle_driver.c built against QuickJS 2026-06-04:
    cc -O2 -o driver tools/oracle_driver.c QUICKJS/libregexp.c QUICKJS/libunicode.c \
       QUICKJS/cutils.c -IQUICKJS
The rows complement the tables taken from luce-js's libregexp port (tools/oracle_tables.py):
lookbehind, counted and nested quantifiers, capture resets, duplicate names, modifiers,
case folding, surrogates, v-flag class sets and more syntax errors.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src/luce_regex/regex/es_table_extra.lucb"
DRIVER = sys.argv[1]
FLAGS = dict(g=1, i=2, m=4, s=8, u=16, y=32, d=64, v=256)

EXEC = [
    # lookbehind: backward matching, captures, nesting
    ("(?<=(\\w)(\\w))c", "", ["abc"]),
    ("(?<=(a|ab))c", "", ["abc"]),
    ("(?<=(a+?))b", "", ["aaab"]),
    ("(?<=a(?=b)b)c", "", ["abc"]),
    ("(?<=(?<!x)a)b", "", ["ab", "xab"]),
    ("(?<=\\1(a))b", "", ["aab"]),
    ("(?<=(a)\\1)b", "", ["aab"]),
    ("(?<=a{2,3})b", "", ["aaab", "ab"]),
    ("(?<=^a*)b", "", ["aaab", "xab"]),
    ("(?<=\\b\\w+)!", "", ["hey!"]),
    ("(?<=(?:ab)+)c", "", ["ababc"]),
    ("(?<=[\\u{1F600}-\\u{1F64F}]+)x", "u", ["\U0001F600\U0001F601x"]),
    ("(?<=\\uD83D)x", "", ["\ud83dx", "\U0001F600x"]),
    ("(?<=.)x", "", ["\U0001F600x"]),
    ("(?<=(.))x", "u", ["\U0001F600x"]),
    ("(?<=\\p{L})\\d", "u", ["a1", "11"]),
    ("(?<=ab?)c", "", ["ac", "abc"]),
    ("(?<!\\d)\\d{2}(?!\\d)", "", ["1234 56 789"]),
    ("(?<=(?<a>x)|(?<a>y))z", "", ["yz", "xz"]),
    ("(?<=\\k<a>(?<a>.)|(?<a>q))z", "", ["aaz", "qz"]),
    # lookahead captures and atomicity
    ("(?=(a*))\\1b", "", ["aaab", "aab"]),
    ("(?=(a+?))(\\1)", "", ["aaa"]),
    ("(?!a|b)|c", "", ["abc"]),
    ("((?=a))*", "", ["a"]),
    ("(?=(a))?", "", ["a"]),
    ("(?:(?=(a))a)*b", "", ["aab"]),
    ("(?!(a)b)(a)", "", ["ab", "ac"]),
    ("x(?=(y))?", "", ["xy"]),
    ("(?=(a)|b)*", "", ["a"]),
    # quantifiers, counters and resets
    ("(a{2}){3}", "", ["aaaaaa", "aaaaa"]),
    ("(?:(a)|b){3}", "", ["aba", "bab"]),
    ("(?:(a)|b){1,3}?c", "", ["abac"]),
    ("(a{0,2}){2,3}b", "", ["aaab", "b"]),
    ("(?:a{2,3}?){2}", "", ["aaaaaa"]),
    ("(?:(?:a{1,2}){1,2}){2}", "", ["aaaaaaa"]),
    ("(a|){3,5}b", "", ["aab"]),
    ("(?:a|()){2,}", "", ["aaa"]),
    ("(?:(a)|b)*?c", "", ["abac"]),
    ("(?:(a)|(b))+", "", ["abab", "ba"]),
    ("(?:(a)|(b)){2}", "", ["ab"]),
    ("(?:(a)(b)?)+", "", ["aba"]),
    ("(?:()a)+", "", ["aa"]),
    ("((a)|b)+", "", ["ab"]),
    ("(?:(\\w)\\1)+", "", ["aabbc"]),
    ("(?:\\1(a))+", "", ["aaa"]),
    ("(a)\\1{2,}", "", ["aaaa"]),
    ("(?:a*b*)*c", "", ["abac", "c"]),
    ("(?:a?){3}x", "", ["aax"]),
    ("(?:a??){2}x", "", ["ax"]),
    ("(?:(?:)|a){2,4}b", "", ["aab"]),
    ("(?:^|,)(\\w*)", "", [",x"]),
    ("(\\s*)*y", "", ["  y"]),
    ("(a*)+?b", "", ["aab"]),
    ("(?:a{0,3}){2,}b", "", ["aaaaaaab"]),
    ("a{3,}?", "", ["aaaaa"]),
    ("(a{2,}?)(a*)", "", ["aaaaa"]),
    ("(?:x{1,}|y{2,})+z", "", ["xyyxz", "xyz"]),
    ("^(?:a|ab)*c$", "", ["abac", "ababab"]),
    ("(\\2)(a)", "", ["aa"]),
    ("(a)|\\1b", "", ["b"]),
    # duplicate named groups
    ("(?<n>a)|(?<n>b)|(?<n>c)", "", ["c", "b"]),
    ("(?:(?<n>a)|(?<n>b))\\k<n>{2}", "", ["bbb", "aaa"]),
    ("(?:(?<n>a)|b)(?:(?<m>x)|(?<m>y))\\k<m>", "", ["ayy", "bxx"]),
    ("\\k<n>(?:(?<n>a)|(?<n>b))", "", ["aa", "b"]),
    # modifiers
    ("(?i:a(?-i:b)c)", "", ["AbC", "ABC"]),
    ("(?i:(a))\\1", "", ["AA", "Aa"]),
    ("(a)(?i:\\1)", "", ["aA"]),
    ("(?i:[a-c])+", "", ["AbC"]),
    ("(?i:\\bK)", "u", ["\u212a"]),
    ("(?ims:^a.b$)", "", ["x\nA\nB"]),
    ("(?m-i:^B)", "i", ["a\nB", "a\nb"]),
    ("(?s-m:a.$)", "m", ["a\n\n"]),
    ("(?i:\\p{Lu})", "u", ["a"]),
    ("(?i:[^a])", "", ["A"]),
    # case folding and backreferences
    ("(a)\\1", "i", ["aA"]),
    ("(\\u{10400})\\1", "iu", ["\U00010400\U00010428"]),
    ("(ß)\\1", "i", ["ßẞ"]),
    ("(ſ)\\1", "iu", ["ſs", "ſS"]),
    ("(k)\\1", "iu", ["k\u212a"]),
    ("[\\w\\d]", "iu", ["\u212a"]),
    ("[^\\w]", "iu", ["\u212a"]),
    ("\\W", "i", ["\u212a"]),
    ("[k-m]", "i", ["\u212a"]),
    ("[k-m]", "iu", ["\u212a"]),
    ("\\u0130", "iu", ["i", "I"]),
    ("\\u0131", "i", ["I", "i"]),
    ("[\\u00e5]", "i", ["\u212b"]),
    ("[\\u00e5]", "iu", ["\u212b"]),
    ("\\u03c2", "iu", ["\u03a3", "\u03c3"]),
    ("[^\\u03c3]", "iu", ["\u03c2"]),
    ("\\p{Ll}", "iu", ["A", "1"]),
    ("[\\p{Ll}--\\p{ASCII}]", "iv", ["A", "\u00c0"]),
    ("\\p{Lu}", "iv", ["a"]),
    # surrogates and 16-bit subjects
    ("^.", "u", ["\ude00\ud83d"]),
    ("\\S", "u", ["\U0001F600"]),
    ("[^a]", "u", ["\U0001F600"]),
    ("[^a]", "", ["\U0001F600"]),
    ("[\\uD83D\\uDE00]", "", ["\U0001F600"]),
    ("[\\uD83D\\uDE00]", "u", ["\U0001F600", "\ude00"]),
    ("[\\ud83d]", "u", ["\U0001F600", "\ud83d"]),
    ("\\ude00", "u", ["\ude00", "\ud83d\ude00"]),
    ("\\b", "u", ["\U0001F600a"], dict(cindex=[0, 1, 2])),
    ("\\B.", "u", ["\U0001F600a"], dict(cindex=[1])),
    ("$", "u", ["\U0001F600"], dict(cindex=[1])),
    ("x*", "u", ["\U0001F600"], dict(cindex=[1])),
    ("x*", "", ["\U0001F600"], dict(cindex=[1])),
    ("\\uD83D\\uDE00", "y", ["\U0001F600"], dict(cindex=[0, 1])),
    (".", "uy", ["\U0001F600"], dict(cindex=[1])),
    ("(?:)", "u", ["\U0001F600"], dict(cindex=[1])),
    ("\\u{1F600}{2}", "u", ["\U0001F600\U0001F600"]),
    ("\\p{Emoji_Presentation}", "u", ["a\U0001F600"]),
    ("\\p{Script=Han}+", "u", ["x\u4e00\U00020000y"]),
    ("[^\\p{L}]+", "u", ["\U0001F600\U0001F600a"]),
    ("\\P{Any}", "u", ["a"]),
    ("[\\s\\S]{2}", "u", ["\U0001F600a"]),
    # v flag class sets
    ("[\\q{ab|c}--\\q{c}]", "v", ["c", "ab"]),
    ("[\\q{ab|c}&&\\q{ab}]", "v", ["ab", "c"]),
    ("[\\p{RGI_Emoji}--\\q{\U0001F600}]", "v", ["\U0001F600", "\U0001F601"]),
    ("[[a-z]&&[^aeiou]]+", "v", ["bcad"]),
    ("[^[^a]]", "v", ["a", "b"]),
    ("[[^a]--b]", "v", ["b", "c"]),
    ("[\\d--3]+", "v", ["1234"]),
    ("[a-c\\q{xyz}]+", "v", ["xyzab"]),
    ("[\\q{xyz|xy|x}]", "v", ["xyzz"]),
    ("[\\q{KK}]", "vi", ["kk"]),
    ("[\\q{\\u{1F600}a}]", "v", ["\U0001F600a"]),
    ("[\\p{RGI_Emoji}a]+", "v", ["a\U0001F44D\U0001F3FD\U0001F1EB\U0001F1F7"]),
    ("\\p{RGI_Emoji_Flag_Sequence}", "v", ["\U0001F1EB\U0001F1F7"]),
    ("\\p{Basic_Emoji}", "v", ["\u231a"]),
    ("[\\w--\\d]+", "v", ["a1b"]),
    ("[\\p{ASCII}&&\\p{L}]+", "v", ["ab\u00e9"]),
    ("(?<=[\\q{ab}])c", "v", ["abc"]),
    ("[\\q{ab}]{2}", "v", ["abab"]),
    # anchors, dot and multiline
    ("^\\s*$", "m", ["a\n  \nb"]),
    ("$", "m", ["a\u2028b"]),
    ("^b", "m", ["a\u2029b", "a\rb"]),
    (".+", "", ["a\u2028b"]),
    (".+", "s", ["a\u2028b"]),
    ("\\b\\w", "", ["\u00e9a"]),
    ("a\\B", "", ["ab", "a"]),
    # escapes and Annex B atoms
    ("\\c0", "", ["\\c0"]),
    ("[\\c0]", "", ["\u0010"]),
    ("\\a", "", ["a"]),
    ("\\-", "", ["-"]),
    ("[\\d-z]+", "", ["-z1"]),
    ("[z-\\d]+", "", ["-z1"]),
    ("a{2,3", "", ["a{2,3"]),
    ("a{", "", ["a{"]),
    ("\\1\\2(a)(b)", "", ["ab"]),
    ("\\11", "", ["\t"]),
    ("(a)\\11", "", ["a\t"]),
    ("\\012", "", ["\n"]),
    ("\\x4g", "", ["x4g"]),
    ("\\u12", "", ["u12"]),
    ("\\u{12}", "", ["uuuuuuuuuuuu"]),
    ("\\k<a>(?<a>b)", "", ["b"]),
    ("[\\k]", "", ["k"]),
    ("\\p{L}", "", ["p{L}"]),
    # sticky and start positions
    ("(?<=a)b", "y", ["ab"], dict(cindex=[0, 1])),
    ("b|", "y", ["ab"], dict(cindex=[0, 1, 2])),
    ("a*", "y", ["aab"], dict(cindex=[1, 2, 3])),
    ("\\d+", "g", ["a1b22c"], dict(cindex=[0, 2, 3, 5, 6])),
    # performance-shaped patterns
    ("^(\\w+\\s?)*$", "", ["aaaa bbbb cccc dddd"]),
    ("(x+x+)+y", "", ["xxxxxxxxxxy"]),
    ("([a-z]+)@([a-z]+)\\.com", "", ["mail: joe@example.com."]),
]

COMPILE = [
    ("(?ii:a)", "", 0), ("(?i-i:a)", "", 0), ("(?-:a)", "", 0), ("(?:a)", "", 0),
    ("(?i)", "", 0), ("(?i", "", 0), ("(?im-s:a)", "", 0), ("(?x:a)", "", 0),
    ("(?<a>a)(?<a>b)", "", 0), ("(?<a>a)|(?<a>b)", "", 0), ("((?<a>a)|b)(?<a>c)", "", 0),
    ("(?<a>(?<a>b)|c)", "", 0), ("\\k<b>(?<a>a)", "", 0), ("\\k<b>", "u", 0),
    ("\\k<b", "u", 0), ("\\k", "u", 0), ("(?<=a)*", "", 0), ("(?<!a)+", "", 0),
    ("(?=a)*", "u", 0), ("[b-a]", "", 0), ("[\\d-a]", "u", 0), ("[a-\\d]", "u", 0),
    ("\\p{Foo}", "u", 0), ("\\p{Script=Foo}", "u", 0), ("\\p{gc=Foo}", "u", 0),
    ("\\p{L", "u", 0), ("\\p", "u", 0), ("\\pL", "u", 0), ("\\p{Lu=x}", "u", 0),
    ("\\p{RGI_Emoji}", "u", 0), ("\\P{RGI_Emoji}", "v", 0), ("[^\\p{RGI_Emoji}]", "v", 0),
    ("[\\q{a}]", "u", 0), ("\\q{a}", "v", 0), ("[\\q{a]", "v", 0), ("[\\qa]", "v", 0),
    ("[a&&&b]", "v", 0), ("[a&&b--c]", "v", 0), ("[a--b&&c]", "v", 0), ("[a-]", "v", 0),
    ("[(]", "v", 0), ("[a!!b]", "v", 0), ("[[a]", "v", 0), ("[a]]", "v", 0),
    ("[^\\q{ab}]", "v", 0), ("[^[\\q{ab}]]", "v", 0), ("\\c", "u", 0), ("\\c1", "u", 0),
    ("[\\c1]", "u", 0), ("\\-", "u", 0), ("\\a", "u", 0), ("\\1", "u", 0), ("(a)\\2", "u", 0),
    ("\\00", "u", 0), ("\\08", "u", 0), ("a{", "u", 0), ("a{1", "u", 0), ("a{1,", "u", 0),
    ("{", "u", 0), ("}", "u", 0), ("]", "u", 0), ("\\u{110000}", "u", 0), ("\\u{}", "u", 0),
    ("\\x1", "u", 0), ("\\uD83D", "", 0), ("(?<a", "", 0), ("(?<1a>x)", "", 0),
    ("(?<a-b>x)", "", 0), ("(?<>x)", "", 0), ("(?<\\u0061>x)\\k<a>", "", 0),
    ("(?<\\x61>x)", "", 0), ("(?", "", 0), ("(?a)", "", 0), ("a|*", "", 0),
    ("(?<=a", "", 0), ("(?=a", "u", 0), ("[", "", 0), ("[\\", "", 0), ("\\", "", 0),
    ("\\", "u", 0), ("a{99999999999}", "", 0), ("a{2,99999999999}", "", 0),
    ("(" * 255 + ")" * 255, "", 0), ("(" * 254 + ")" * 254, "", 0),
    ("(?:a{2,3})" * 3, "", 0), ("(?:" * 140 + "a{2,3}" + "){2,3}" * 140, "", 0),
    ("(?:" * 130 + "a*" + ")*" * 130, "", 0), ("(?:" * 300 + "a" + ")*" * 300, "", 0),
    ("(?<a>x)(?<b>y)z", "d", 0), ("(?<\U0001d49c>x)", "u", 0), ("(?<\\u{1d49c}>x)", "", 0),
    ("\U0001F600", "", 0), ("\U0001F600", "u", 0), ("[\U0001F600]", "", 0),
    ("a", "gimsuyd", 0), ("a", "v", 0), ("a", "uv", 0),
]


def flags_of(f):
    n = 0
    for ch in f:
        n |= FLAGS[ch]
    return n


def units(subject):
    b = subject.encode("utf-16-be", "surrogatepass")
    return [int.from_bytes(b[i:i + 2], "big") for i in range(0, len(b), 2)]


def luce_str(p):
    out = []
    for ch in p:
        o = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append("\\t")
        elif ch == "\r":
            out.append("\\r")
        elif o == 0:
            out.append("\\0")
        elif 0x20 <= o < 0x7f:
            out.append(ch)
        else:
            out.append("\\u{%x}" % o)
    return '"' + "".join(out) + '"'


def subject_text(us):
    return "".join(chr(u) if 0x20 <= u < 0x7f and chr(u) not in '%"\\' else "%%%04x" % u for u in us)


def run(lines):
    r = subprocess.run([DRIVER], input="\n".join(lines) + "\n", capture_output=True, text=True,
                       check=True)
    res = r.stdout.split("\n")[:-1]
    assert len(res) == len(lines), (len(res), len(lines))
    return res


def summary(res):
    if not res.startswith("ok"):
        return res
    parts = res.split()
    keep = [p for p in parts if p.startswith(("cc=", "flags=", "names="))]
    return " ".join(["ok"] + keep)


def main():
    lines = ["C %d %d %s" % (mode, flags_of(fl), p.encode("utf-8", "surrogatepass").hex())
             for p, fl, mode in COMPILE]
    cres = [summary(r) for r in run(lines)]
    exec_rows = []
    for pat, fl, subjects, *extra in EXEC:
        extra = extra[0] if extra else {}
        for subj in subjects:
            us = units(subj)
            for cindex in extra.get("cindex", [0]):
                for cbuf_type in (0, 1):
                    if cbuf_type == 0 and any(u > 255 for u in us):
                        continue
                    exec_rows.append((pat, fl, us, cbuf_type, cindex))
    lines = []
    for pat, fl, us, cbuf_type, cindex in exec_rows:
        h = bytes(us).hex() if cbuf_type == 0 else "".join("%04x" % u for u in us)
        lines.append("X 0 %d %s %d %d %s" % (flags_of(fl), pat.encode("utf-8", "surrogatepass").hex(),
                                              cbuf_type, cindex, h))
    xres = run(lines)
    text = ["""#==============================================================================================
#
#   es_table_extra - More expected compile and exec results
#
#   DESCRIPTION:
#       Generated by tools/oracle_extra.py, do not edit: printed by tools/oracle_driver.c
#       linked with QuickJS libregexp.c, libunicode.c and cutils.c (2026-06-04). The rows
#       have the layout of es_table_compile and es_table_exec1.
#
#==============================================================================================

"""]
    text.append("let es_extra_compile_cases: CompileCase[%d] = [\n" % len(COMPILE))
    for (p, fl, mode), res in zip(COMPILE, cres):
        text.append("    CompileCase(%s, %d, %d, %s),\n" % (luce_str(p), flags_of(fl), mode, luce_str(res)))
    text.append("]\n\n")
    text.append("let es_extra_exec_cases: ExecCase[%d] = [\n" % len(exec_rows))
    for (p, fl, us, cbuf_type, cindex), res in zip(exec_rows, xres):
        text.append("    ExecCase(%s, %d, 0, %s, %d, %d, %s),\n" % (
            luce_str(p), flags_of(fl), luce_str(subject_text(us)), cbuf_type, cindex, luce_str(res)))
    text.append("]\n")
    OUT.write_text("".join(text))
    print(f"{len(COMPILE)} compile rows, {len(exec_rows)} exec rows")


main()
