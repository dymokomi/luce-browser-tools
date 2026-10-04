# Writes cases.txt for the CSP oracle: python3 gen_cases.py > cases.txt
import hashlib, base64, itertools

def esc(s):
    if s == "":
        return "\\-"
    out = []
    for ch in s.encode("utf-8"):
        if ch == 0x5c: out.append("\\\\")
        elif ch == 0x0a: out.append("\\n")
        elif ch == 0x09: out.append("\\t")
        elif ch < 0x20 or ch >= 0x7f: out.append("\\x%02X" % ch)
        else: out.append(chr(ch))
    return "".join(out)

lines = []
def case(op, *args, comment=None):
    if comment:
        lines.append("# " + comment)
    lines.append("\t".join([op] + [esc(a) for a in args]))

# mark: parsing serialized policies
lines.append("# Parsing a serialized CSP (Policy::parse_a_serialized_csp): names, values, duplicates, whitespace, non-ASCII.")
for s in [
    "", ";", " ; ; ", "default-src 'self'", "DEFAULT-SRC 'SELF' https://A.test",
    "script-src https://a.test; IMG-src *", "script-src a; script-src b", "script-src a;script-src b",
    "  style-src\t'unsafe-inline'  \n  ;font-src data:", "unknown-directive value1  value2",
    "default-src 'none'; report-uri /csp-report https://r.test/x; report-to group",
    "base-uri 'self'; child-src c; connect-src c; default-src d; font-src f; form-action fa; frame-ancestors 'none'; frame-src fs; img-src i; manifest-src m; media-src me; object-src o; report-to rt; report-uri ru; require-trusted-types-for 'script'; sandbox allow-scripts; script-src-attr sa; script-src s; script-src-elem se; style-src-attr sta; style-src st; style-src-elem ste; trusted-types tt; webrtc 'allow'; worker-src w",
    "script-src 'nonce-abc' 'sha256-xyz='; café-src x; img-src café", "img-src", "img-src ;", "sandbox", "a b\u000bc", "x\u000cy z",
    "script-src a b", ";;script-src a;;", "upgrade-insecure-requests; block-all-mixed-content",
]:
    case("parse", "header", "enforce", s)
case("parse", "meta", "report", "default-src 'self'; frame-ancestors 'none'")
case("parse", "meta", "enforce", "Script-Src 'Unsafe-Inline'")

# mark: response headers
lines.append("# A response's Content-Security-Policy and Content-Security-Policy-Report-Only headers.")
case("headers", "https://example.com/page?q#f")
case("headers", "https://example.com/page", "Content-Security-Policy", "default-src 'self'")
case("headers", "https://example.com:8443/page", "Content-Security-Policy", "default-src 'self', img-src *", "Content-Security-Policy-Report-Only", "script-src 'none'")
case("headers", "http://example.com/", "Content-Security-Policy", "", "Content-Security-Policy", "   ", "Content-Security-Policy", "img-src a")
case("headers", "https://example.com/", "content-security-policy", "script-src a;", "CONTENT-SECURITY-POLICY-REPORT-ONLY", "style-src b")
case("headers", "https://example.com/", "Content-Security-Policy", "script-src \"a,b\"")
case("headers", "https://example.com/", "Content-Security-Policy", "img-src éé")
case("headers", "file:///tmp/x.html", "Content-Security-Policy", "default-src 'self'")
case("headers", "data:text/html,hi", "Content-Security-Policy", "default-src 'self'")

# mark: source expressions
lines.append("# Source expressions: each production over a set of inputs.")
exprs = ["https:", "https", "custom-scheme+1.2-:", "1http:", "*", "*.", "*.example.com", "example.com", "example.com.", "EXAMPLE.com",
         "a.b.c", "a..b", "-a-.b", "*example.com", "https://example.com", "https://*.example.com:12/path/to/file.js", "example.com:*",
         "example.com:", "example.com:80a", "example.com:0080", "example.com/", "example.com/a//b/", "example.com/%2Fa%zz", "example.com/a;b",
         "example.com/a,b", "example.com/a?b", "example.com/a#b", "example.com/!$&'()*+=:@-._~", "ws://x", "https:/x", "http://", "://a",
         "'self'", "'SELF'", "'unsafe-inline'", "'unsafe-eval'", "'strict-dynamic'", "'unsafe-hashes'", "'report-sample'",
         "'unsafe-allow-redirects'", "'wasm-unsafe-eval'", "'none'", "'self", "self",
         "'nonce-abc'", "'NONCE-abc'", "'nonce-ab+/-_=='", "'nonce-ab==='", "'nonce-'", "'nonce-abc", "'nonce-a b'", "'nonce",
         "'sha256-abc='", "'SHA384-abc'", "'sha512-AbC_-+/=='", "'sha1-abc'", "'sha256-'", "'sha256abc'", "'sha256-abc", "'sha2560-abc'",
         "", " ", "https://example.com:443/", "http://[::1]:80/", "http://127.0.0.1/", "data:", "blob:"]
for e in exprs:
    for p in ["scheme", "host", "keyword", "nonce", "hash"]:
        case("expr", p, e)

# mark: URL matching
lines.append("# Does url match expression in origin with redirect count?")
urls = ["https://example.com/", "http://example.com/", "https://www.example.com/a/b.js", "https://example.com:8443/x", "http://example.com:80/x",
        "ws://example.com/", "wss://example.com/", "data:text/plain,hi", "blob:https://example.com/uuid", "https://127.0.0.1/", "https://[::1]/",
        "file:///tmp/a.html", "about:blank", "https://EXAMPLE.com/A%2Fb/c", "https://example.com/a/b/", "https://sub.self.test/", "http://self.test/",
        "https://self.test/", "https://self.test:444/", "custom:thing"]
origins = ["https://self.test/", "http://self.test/", "https://example.com/", "opaque", "http://example.com/", "custom:thing"]
match_exprs = ["*", "https:", "http:", "ws:", "wss:", "data:", "blob:", "custom:", "example.com", "*.example.com", "https://example.com",
               "http://example.com", "example.com:443", "example.com:*", "example.com:80", "example.com:8443", "example.com/a/", "example.com/a/b.js",
               "example.com/A%2fb/c", "example.com/", "'self'", "'SELF'", "'none'", "self.test", "*", "'nonce-x'", "127.0.0.1", "ws://example.com",
               "example.com:99999999"]
for u in urls:
    for e in match_exprs:
        case("url_expr", u, "https://self.test/", "0", e)
    for e in ["*", "https:", "'self'", "example.com", "custom:", "data:"]:
        case("url_expr", u, "opaque", "0", e)
for u in ["https://self.test/", "http://self.test/", "https://self.test:444/", "wss://self.test/", "ws://self.test/", "https://sub.self.test/", "http://self.test:80/"]:
    for o in origins:
        case("url_expr", u, o, "0", "'self'")
        case("url_expr", u, o, "0", "*")
        case("url_expr", u, o, "0", "self.test")
for rc in ["0", "1", "3"]:
    case("url_expr", "https://example.com/other/path", "https://self.test/", rc, "example.com/a/b.js")
    case("url_expr", "https://example.com/a/b.js", "https://self.test/", rc, "example.com/a/b.js")
lines.append("# Does url match source list in origin with redirect count?")
for lst in [[], ["'none'"], ["'NONE'"], ["'none'", "https://example.com"], ["a.test", "example.com"], ["https:"], ["'self'"], ["*.example.com", "'self'"], ["'none'", "'none'"]]:
    for u in ["https://example.com/", "https://www.example.com/", "https://self.test/x", "data:,x"]:
        case("url_list", u, "https://self.test/", "0", *lst)

# mark: effective directives
lines.append("# The effective directive of a request (destination, initiator), of inline checks, fallback lists.")
dests = ["-", "audio", "audioworklet", "document", "embed", "font", "frame", "iframe", "image", "json", "manifest", "object", "paintworklet",
         "report", "script", "serviceworker", "sharedworker", "style", "track", "video", "webidentity", "worker", "xslt"]
for d in dests:
    for i in ["-", "prefetch", "prerender", "download", "imageset", "manifest", "xslt"]:
        case("effective", d, i)
for t in ["navigation", "script", "script-attribute", "style", "style-attribute"]:
    case("inline_effective", t)
for n in ["-", "script-src-elem", "script-src-attr", "style-src-elem", "style-src-attr", "worker-src", "connect-src", "manifest-src", "object-src",
          "frame-src", "media-src", "font-src", "img-src", "default-src", "script-src", "child-src", "unknown"]:
    case("fallback", n)
lines.append("# Should fetch directive execute?")
for eff in ["-", "script-src-elem", "worker-src", "img-src", "frame-src", "connect-src"]:
    for dn in ["script-src-elem", "script-src", "child-src", "default-src", "img-src", "worker-src", "frame-src"]:
        for pol in ["", "default-src 'self'", "script-src a", "child-src c; default-src d", "script-src-elem e; script-src s", "worker-src w"]:
            case("execute", eff, dn, pol)

# mark: pre- and post-request checks
lines.append("# Pre-request checks of every directive of a policy, and does request violate policy?")
sha256 = "'sha256-" + base64.b64encode(hashlib.sha256(b"x").digest()).decode() + "'"
policies = [
    "default-src 'self'", "default-src 'none'", "default-src *", "default-src https://example.com", "img-src 'self'; default-src 'none'",
    "script-src 'self'", "script-src 'nonce-abc'", "script-src 'strict-dynamic' 'nonce-abc'", "script-src 'sha256-abc=' https://other.test",
    "script-src-elem https://example.com; script-src 'none'", "script-src-attr 'none'", "style-src 'nonce-n1' https://cdn.test", "style-src-elem 'none'",
    "child-src https://example.com", "child-src 'none'; frame-src https://example.com", "worker-src 'none'; child-src *", "connect-src 'self' wss://example.com",
    "font-src data: https://example.com", "media-src 'none'", "object-src 'none'", "manifest-src 'self'", "frame-src 'none'",
    "img-src https://example.com/a/; default-src 'none'", "base-uri 'none'; form-action 'none'; frame-ancestors 'none'; sandbox; report-uri /r; webrtc 'block'",
    "default-src 'self' https://example.com:8443", "script-src 'unsafe-inline' 'unsafe-eval'", "img-src *; script-src 'none'",
    "script-src " + sha256 + " 'sha384-xyz'", "script-src 'nonce-abc' 'sha512-AAAA'",
]
reqs = [
    # url, destination, initiator, nonce, integrity, parser, redirect count
    ["https://example.com/a/b.js", "script", "-", "", "", "-", "0"],
    ["https://self.test/a.js", "script", "-", "", "", "-", "0"],
    ["https://example.com/a/b.js", "script", "-", "abc", "", "parser-inserted", "0"],
    ["https://evil.test/x.js", "script", "-", "abc", "", "-", "0"],
    ["https://evil.test/x.js", "script", "-", "", "", "parser-inserted", "0"],
    ["https://evil.test/x.js", "script", "-", "", "", "not-parser-inserted", "0"],
    ["https://evil.test/x.js", "script", "-", "", "sha256-abc=", "-", "0"],
    ["https://evil.test/x.js", "script", "-", "", "sha256-abc= sha384-xyz", "-", "0"],
    ["https://evil.test/x.js", "script", "-", "", "sha384-xyz", "-", "0"],
    ["https://evil.test/x.js", "script", "-", "", sha256[1:-1], "-", "0"],
    ["https://evil.test/x.js", "worker", "-", "", "", "-", "0"],
    ["https://example.com/a/img.png", "image", "-", "", "", "-", "0"],
    ["https://example.com/b/img.png", "image", "-", "", "", "-", "0"],
    ["https://self.test/img.png", "image", "-", "", "", "-", "0"],
    ["https://example.com/a/img.png|https://example.com/b/img.png", "image", "-", "", "", "-", "1"],
    ["https://cdn.test/s.css", "style", "-", "n1", "", "-", "0"],
    ["https://evil.test/s.css", "style", "-", "n1", "", "-", "0"],
    ["https://evil.test/s.css", "style", "-", "", "", "-", "0"],
    ["https://example.com/frame.html", "iframe", "-", "", "", "-", "0"],
    ["https://evil.test/frame.html", "frame", "-", "", "", "-", "0"],
    ["https://example.com:8443/api", "-", "-", "", "", "-", "0"],
    ["wss://example.com/socket", "-", "-", "", "", "-", "0"],
    ["data:font/woff2,abc", "font", "-", "", "", "-", "0"],
    ["https://evil.test/v.mp4", "video", "-", "", "", "-", "0"],
    ["https://evil.test/o.swf", "embed", "-", "", "", "-", "0"],
    ["https://self.test/m.json", "manifest", "-", "", "", "-", "0"],
    ["https://evil.test/p", "-", "prefetch", "", "", "-", "0"],
    ["https://self.test/p", "-", "prefetch", "", "", "-", "0"],
    ["https://evil.test/doc", "document", "-", "", "", "-", "0"],
    ["https://evil.test/r", "report", "-", "", "", "-", "0"],
    ["https://evil.test/w.js", "audioworklet", "-", "", "", "-", "0"],
]
for p in policies:
    for r in reqs:
        case("request", p, "https://self.test/", *r)
lines.append("# Post-request checks (the response's URL).")
for p in policies:
    for r, resp in [(reqs[0], "https://example.com/a/b.js"), (reqs[0], "https://evil.test/b.js"), (reqs[2], "https://evil.test/b.js"),
                    (reqs[5], "https://evil.test/x.js"), (reqs[11], "https://example.com/a/img.png"), (reqs[11], "https://evil.test/img.png"),
                    (reqs[15], "https://evil.test/s.css"), (reqs[20], "https://example.com:8443/api"), (reqs[22], "data:font/woff2,abc")]:
        case("response", p, "https://self.test/", *r, resp)
lines.append("# Should request / response be blocked by Content Security Policy? (enforce and report)")
for p in ["default-src 'none'", "img-src https://example.com/a/", "script-src 'nonce-abc'", "connect-src 'self'"]:
    for disp in ["enforce", "report"]:
        for r in [reqs[0], reqs[3], reqs[11], reqs[12], reqs[20]]:
            case("should_request", p, disp, "https://self.test/", *r)
            case("should_response", p, disp, "https://self.test/", *r, "https://evil.test/x")

# mark: inline checks
lines.append("# Inline checks of every directive and should element's inline type behavior be blocked?")
src_x_hashes = ["'sha256-" + base64.b64encode(hashlib.sha256(b"alert(1)").digest()).decode() + "'",
                "'sha384-" + base64.b64encode(hashlib.sha384(b"alert(1)").digest()).decode() + "'",
                "'sha512-" + base64.urlsafe_b64encode(hashlib.sha512(b"alert(1)").digest()).decode() + "'",
                "'SHA256-" + base64.b64encode(hashlib.sha256("café".encode()).digest()).decode() + "'"]
inline_policies = [
    "script-src 'self'", "script-src 'unsafe-inline'", "script-src 'unsafe-inline' 'nonce-abc'", "script-src 'nonce-abc'",
    "script-src " + src_x_hashes[0], "script-src " + src_x_hashes[1], "script-src " + src_x_hashes[2], "script-src " + src_x_hashes[3],
    "script-src 'unsafe-hashes' " + src_x_hashes[0], "script-src 'strict-dynamic' 'unsafe-inline'", "script-src 'strict-dynamic'",
    "script-src 'report-sample'", "style-src 'unsafe-inline'", "style-src 'nonce-abc'", "style-src " + src_x_hashes[0],
    "default-src 'none'", "default-src 'unsafe-inline'", "script-src-attr 'unsafe-inline'; script-src 'none'", "script-src-elem 'none'; script-src 'unsafe-inline'",
    "style-src-attr 'none'; style-src 'unsafe-inline'", "style-src-elem 'unsafe-inline'", "img-src 'none'", "default-src 'self'; style-src 'unsafe-hashes' " + src_x_hashes[0],
]
elements = [["-"], ["html:script"], ["html:script", "nonce=abc"], ["html:script", "nonce=abd"], ["html:script", "nonce=abc", "flag:parser-inserted"],
            ["html:script", "nonce=abc", "data-x=<script>"], ["html:script", "nonce=abc", "flag:duplicate-attribute"], ["html:style", "nonce=abc"],
            ["svg:script", "nonce=abc"], ["svg:script"], ["svg:script", "flag:parser-inserted"], ["html:div", "nonce=abc"], ["html:div"]]
for p in inline_policies:
    for t, src in [("script", "alert(1)"), ("script-attribute", "alert(1)"), ("style", "alert(1)"), ("style-attribute", "alert(1)"), ("navigation", "javascript:alert(1)"), ("script", "café")]:
        for el in (elements if p in inline_policies[:4] or "nonce" in p or "strict-dynamic" in p else elements[:3] + [elements[7]]):
            if t == "navigation" and el != ["-"]:
                continue
            if t != "navigation" and el == ["-"]:
                continue
            case("element", p, "enforce", t, src, *el)
for p in ["script-src 'none'", "style-src 'none'"]:
    case("element", p, "report", "script", "x", "html:script")
    case("element", p, "report", "style", "x", "html:style")
lines.append("# Does element match source list for type and source?")
for lst in ["'unsafe-inline'", "'unsafe-inline' 'nonce-a'", "'unsafe-inline' 'sha256-x'", "'strict-dynamic' 'unsafe-inline'", "'nonce-abc'", "'NONCE-abc'",
            "'unsafe-hashes' " + src_x_hashes[0], src_x_hashes[0], src_x_hashes[0].upper(), "'strict-dynamic'", "'self'"]:
    for t, src in [("script", "alert(1)"), ("style", "alert(1)"), ("script-attribute", "alert(1)"), ("navigation", "alert(1)")]:
        for el in [["-"], ["html:script", "nonce=abc"], ["html:script"], ["svg:script", "nonce=abc"], ["html:script", "NONCE=abc"]]:
            case("matches_element", lst, t, src, *el)

# mark: navigations, base, integrity, webrtc, violations, SRI
lines.append("# Navigation requests (form-action, javascript: URLs).")
for p in ["form-action 'self'", "form-action 'none'", "form-action https://example.com", "script-src 'none'", "script-src 'unsafe-inline'", "default-src 'none'",
          "script-src " + "'sha256-" + base64.b64encode(hashlib.sha256(b"javascript:alert(1)").digest()).decode() + "' 'unsafe-hashes'", "img-src 'none'"]:
    for u in ["https://self.test/submit", "https://example.com/submit", "javascript:alert(1)"]:
        for t in ["form", "other"]:
            case("navigation", p, "https://self.test/", u, t)
lines.append("# Is base allowed for document?")
for p in ["base-uri 'self'", "base-uri 'none'", "base-uri https://example.com", "default-src 'none'", "base-uri *"]:
    for d in ["enforce", "report"]:
        for b in ["https://self.test/base/", "https://example.com/", "data:,x", "about:blank"]:
            case("base", p, d, "https://self.test/", b)
lines.append("# Should request be blocked by integrity policy?")
for srcs, dests, rsrcs, rdests in [("", "", "", ""), ("inline", "script", "", ""), ("inline", "style,script", "", ""), ("other", "script", "", ""), ("", "", "inline", "script")]:
    for u, d, mode, meta in [("https://evil.test/x.js", "script", "no-cors", ""), ("https://evil.test/x.js", "script", "cors", "sha256-abc"),
                             ("https://evil.test/x.js", "script", "cors", ""), ("https://evil.test/x.js", "script", "no-cors", "sha256-abc"),
                             ("about:blank", "script", "no-cors", ""), ("https://evil.test/x.css", "style", "same-origin", "sha512-x"),
                             ("https://evil.test/x.png", "image", "no-cors", ""), ("https://evil.test/x", "-", "no-cors", "")]:
        case("integrity", srcs, dests, rsrcs, rdests, u, d, mode, meta)
lines.append("# WebRTC pre-connect checks.")
for p in ["webrtc 'allow'", "webrtc 'ALLOW'", "webrtc 'block'", "webrtc", "webrtc 'allow' 'allow'", "default-src *"]:
    case("webrtc", p)
lines.append("# Violations: the blocked URI of a resource; a violation for a request.")
for b in ["inline", "eval", "wasm-eval", "https://user:pass@example.com:8080/a/b?q=1#frag", "http://example.com/", "data:text/plain,hi", "blob:https://e.test/x", "file:///tmp/x", "about:blank", "ws://example.com/"]:
    case("blocked_uri", b)
for r in [reqs[0], reqs[11], reqs[20], reqs[26], ["https://u:p@evil.test/a#f", "font", "-", "", "", "-", "0"]]:
    case("violation_request", "default-src 'none'", *r)
lines.append("# SRI: parse metadata, the strongest metadata, matching bytes, the hashes.")
for m in ["", "sha256-abc", "sha256-abc sha384-def", "sha512-x?opt sha256-y", "md5-x sha1-y sha256-z", "sha384-a sha384-b", "sha256", "sha256-a-b", "  sha256-a  ", "SHA256-x", "sha512-?a"]:
    case("sri_parse", m)
for data, meta in [("alert(1)", ""), ("alert(1)", "sha256-" + base64.b64encode(hashlib.sha256(b"alert(1)").digest()).decode()), ("alert(1)", "sha256-wrong"),
                   ("alert(1)", "sha256-wrong sha384-" + base64.b64encode(hashlib.sha384(b"alert(1)").digest()).decode()),
                   ("alert(1)", "sha512-wrong sha384-" + base64.b64encode(hashlib.sha384(b"alert(1)").digest()).decode()), ("alert(1)", "md5-xyz")]:
    case("sri_match", data, meta)
for alg in ["sha256", "sha384", "sha512"]:
    for data in ["", "abc", "café", "x" * 200]:
        case("sri_hash", alg, data)

print("\n".join(lines))
