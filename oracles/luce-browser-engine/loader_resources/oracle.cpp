// Oracle for luce-browser-engine region p2d (loader_resources): runs the reference build's
// Loader (ContentFilter's AsciiStringMatcher, ProxyMappings, the user agent, the generated error
// page), PotentialCORSRequest, PreloadEntry's preload keys, LibCore's MIME type guessing and
// LibRequests' network errors on fixed cases and prints one line per case:
//     <op>\t<a>\t<b>\t<c>\t<expected>
// with \n, \t, \\ and \xHH escaped and "\-" for an empty field; gen_luce_cases.py turns the
// lines into the Luce case table (src/web/loader/tests_loader_cases.lucb).
// Usage: oracle <repository data/res directory>
#include <cstdio>
#include <string>
#include <vector>
#define private public
#include <AK/StringBuilder.h>
#include <AK/Time.h>
#include <LibCore/MimeData.h>
#include <LibCore/ResourceImplementationFile.h>
#include <LibJS/Runtime/VM.h>
#include <LibRequests/NetworkError.h>
#include <LibURL/Parser.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/Requests.h>
#include <LibWeb/HTML/CORSSettingAttribute.h>
#include <LibWeb/HTML/PotentialCORSRequest.h>
#include <LibWeb/HTML/PreloadEntry.h>
#include <LibWeb/Loader/ContentFilter.h>
#include <LibWeb/Loader/GeneratedPagesLoader.h>
#include <LibWeb/Loader/ProxyMappings.h>
#include <LibWeb/Loader/UserAgent.h>
#undef private

using namespace Web;
using Fetch::Infrastructure::Request;

static std::string escape(std::string const& s)
{
    if (s.empty())
        return "\\-";
    std::string out;
    for (unsigned char c : s) {
        if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c == '\\') out += "\\\\";
        else if (c < 0x20 || c >= 0x7f) { char b[8]; snprintf(b, sizeof b, "\\x%02X", c); out += b; }
        else out += (char)c;
    }
    return out;
}

static void emit(std::string const& op, std::string const& a, std::string const& b, std::string const& c, std::string const& expected)
{
    printf("%s\t%s\t%s\t%s\t%s\n", op.c_str(), escape(a).c_str(), escape(b).c_str(), escape(c).c_str(), escape(expected).c_str());
}

static std::string sv(StringView v) { return std::string(v.characters_without_null_termination(), v.length()); }
static std::string bs(ByteString const& s) { return sv(s.view()); }

static JS::VM& the_vm()
{
    static auto vm = JS::VM::create();
    return *vm;
}

static std::vector<std::string> split(std::string const& s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); } else cur += c;
    }
    out.push_back(cur);
    return out;
}

int main(int argc, char** argv)
{
    // ContentFilter and AsciiStringMatcher.
    std::vector<std::string> pattern_sets = {
        "ads.|tracker",
        "he|she|his|hers",
        "a|ab|bc|bca|c|caa",
        "doubleclick.net/|/banner/|googlesyndication",
        "xyz",
        "",
    };
    std::vector<std::string> texts = {
        "ushers", "https://ads.example.com/x", "https://example.com/tracker.js", "abccab", "https://doubleclick.net/ad",
        "https://example.com/img/banner/1.png", "https://example.com/", "xy", "xyzz", "", "hishe", "aaaa", "bcbcb", "cbca",
        "https://pagead2.googlesyndication.com/", "HTTPS://ADS.EXAMPLE.COM/",
    };
    for (auto const& set : pattern_sets) {
        Vector<String> patterns;
        if (!set.empty())
            for (auto const& p : split(set, '|'))
                patterns.append(MUST(String::from_utf8(StringView { p.data(), p.size() })));
        AsciiStringMatcher matcher { patterns.span() };
        for (auto const& text : texts)
            emit("contains", set, text, "", matcher.contains(StringView { text.data(), text.size() }) ? "true" : "false");
    }
    std::vector<std::string> urls = {
        "https://ads.example.com/x", "https://example.com/tracker.js", "data:text/html,ads.tracker", "https://example.com/",
        "file:///tmp/tracker.html", "https://user:pass@ads.example.com:8080/p?q#f",
    };
    for (bool enabled : { true, false }) {
        MUST(ContentFilter::the().set_patterns(Vector<String> { "ads."_string, "tracker"_string }.span()));
        ContentFilter::the().set_filtering_enabled(enabled);
        for (auto const& url : urls) {
            auto parsed = URL::Parser::basic_parse(StringView { url.data(), url.size() });
            emit("filtered", enabled ? "enabled" : "disabled", url, "", ContentFilter::the().is_filtered(*parsed) ? "true" : "false");
        }
    }

    // ProxyMappings.
    {
        Vector<ByteString> proxies { "socks5://127.0.0.1:1080", "http://10.0.0.1:3128", "socks5://localhost:9050", "socks5://10.1.2.3" };
        OrderedHashMap<ByteString, size_t> mappings;
        mappings.set("*://*.onion/*", 2);
        mappings.set("*example.com*", 0);
        mappings.set("*://insecure.test/*", 1);
        mappings.set("*://noport.test/*", 3);
        mappings.set("*", 0);
        ProxyMappings::the().set_mappings(proxies, mappings);
        for (auto url : { "https://www.example.com/a", "http://abc.onion/x", "http://insecure.test/", "http://noport.test/", "https://other.org/" }) {
            auto parsed = URL::Parser::basic_parse(StringView { url, strlen(url) });
            auto proxy = ProxyMappings::the().proxy_for_url(*parsed);
            emit("proxy", url, "", "", bs(ByteString::formatted("{} {} {}", (int)proxy.type, proxy.host_ipv4.to_string().value_or("?"_string), proxy.port)));
        }
    }

    // create_potential_CORS_request.
    std::vector<Optional<Request::Destination>> destinations { {}, Request::Destination::Image, Request::Destination::Script, Request::Destination::Font, Request::Destination::Audio };
    for (auto destination : destinations) {
        for (int cors = 0; cors < 3; cors++) {
            for (int flag = 0; flag < 2; flag++) {
                auto url = URL::Parser::basic_parse("https://example.com/a.png"sv);
                auto request = HTML::create_potential_CORS_request(the_vm(), *url, destination, (HTML::CORSSettingAttribute)cors, (HTML::SameOriginFallbackFlag)flag);
                emit("cors", destination.has_value() ? std::to_string((int)*destination) : "none", std::to_string(cors), std::to_string(flag),
                    bs(ByteString::formatted("{} {} {} {} {}", (int)request->mode(), (int)request->credentials_mode(), request->use_url_credentials(),
                        request->destination().has_value() ? (int)*request->destination() : -1, request->url())));
            }
        }
    }

    // translate_a_preload_destination.
    for (auto d : { "fetch", "font", "image", "script", "style", "track", "audio", "document", "", "Image", "worker" }) {
        auto result = HTML::translate_a_preload_destination(MUST(String::from_utf8(StringView { d, strlen(d) })));
        std::string text = result.visit(
            [](Empty) -> std::string { return "empty"; },
            [](Optional<Request::Destination> const& o) -> std::string { return o.has_value() ? std::to_string((int)*o) : "none"; });
        emit("translate", d, "", "", text);
    }
    {
        auto result = HTML::translate_a_preload_destination({});
        emit("translate", "(null)", "", "", result.has<Empty>() ? "empty" : "?");
    }

    // Traits<PreloadKey>::hash.
    for (auto url : { "https://example.com/a.png", "https://example.com/", "file:///tmp/x.css", "https://example.com/a.png#frag" }) {
        for (int d = -1; d < 3; d++) {
            for (int mode = 0; mode < 3; mode += 2) {
                for (int cred = 0; cred < 3; cred += 2) {
                    HTML::PreloadKey key {
                        .url = *URL::Parser::basic_parse(StringView { url, strlen(url) }),
                        .destination = d < 0 ? Optional<Request::Destination> {} : Optional<Request::Destination> { (Request::Destination)d },
                        .mode = (Request::Mode)mode,
                        .credentials_mode = (Request::CredentialsMode)cred,
                    };
                    emit("preload_hash", url, std::to_string(d), std::to_string(mode) + " " + std::to_string(cred), std::to_string(Traits<HTML::PreloadKey>::hash(key)));
                }
            }
        }
    }

    // Core::guess_mime_type_based_on_filename.
    for (auto path : { "/a/b.html", "/a/b.htm", "x.png", "x.PNG", "photo.jpeg", "a.jpg", "style.css", "s.js", "m.mjs", "d.json", "f.woff", "f.woff2",
             "f.ttf", "f.otf", "notes.txt", "a.c", "a.cpp", "a.h", ".history", "x.svg", "x.xhtml", "x.xht", "x.xml", "/dir/", "noext", "a.tar.gz",
             "x.gif", "x.webp", "x.ico", "x.bmp", "x.mp3", "x.wav", "x.webm", "x.pdf", "x.zip", "archive.7z", "x.md", "x.sh", "x.icc", "x.tvg" })
        emit("mime", path, "", "", sv(Core::guess_mime_type_based_on_filename(StringView { path, strlen(path) })));

    // Requests::network_error_to_string.
    for (int i = 0; i <= (int)Requests::NetworkError::Unknown; i++)
        emit("network_error", std::to_string(i), "", "", sv(Requests::network_error_to_string((Requests::NetworkError)i)));

    // The user agent and the platform (of the oracle's target).
    emit("user_agent", "", "", "", sv(default_user_agent));
    emit("platform", "", "", "", sv(default_platform));

    // response_headers_for_file's Last-Modified value.
    for (long long seconds : { 0LL, 86399LL, 951782400LL, 1700000000LL, 1791040470LL, 2147483647LL })
        emit("last_modified", std::to_string(seconds), "", "", bs(AK::UnixDateTime::from_seconds_since_epoch(seconds).to_byte_string("%a, %d %b %Y %H:%M:%S GMT"sv, AK::UnixDateTime::LocalTime::No)));

    // load_error_page over the repository's templates.
    if (argc > 1) {
        Core::ResourceImplementation::install(make<Core::ResourceImplementationFile>(MUST(String::from_utf8(StringView { argv[1], strlen(argv[1]) }))));
        struct { char const* url; char const* message; } pages[] = {
            { "https://example.com/missing", "Request finished with error: Unable to connect" },
            { "file:///tmp/<a>&b.html", "No such file or directory (errno=2)" },
            { "http://x.test/\"quoted\"", "<script>alert('x')</script>" },
        };
        for (auto const& page : pages) {
            auto url = URL::Parser::basic_parse(StringView { page.url, strlen(page.url) });
            emit("error_page", page.url, page.message, "", bs(MUST(load_error_page(*url, StringView { page.message, strlen(page.message) })).to_byte_string()));
        }
    }
    return 0;
}
