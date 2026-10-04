// Oracle for luce-browser-engine region p2b (fetch_http): runs the reference build's LibHTTP
// (Header, HeaderList, Method, Status) and Fetch/Infrastructure/HTTP (CORS, MIME, Statuses) on
// the cases in cases.txt and prints one result line per case, the text the Luce tests compare.
//
// A case line is "<op>\t<arg>\t<arg>..." with \n, \t, \\ and \xHH escaped in each argument, and "\-" for
// an empty argument. The
// output repeats the case line, then "= <result>" on the next line (escaped the same way).
#include <AK/ByteString.h>
#include <AK/GenericLexer.h>
#include <AK/StringBuilder.h>
#include <LibHTTP/HTTP.h>
#include <LibHTTP/Header.h>
#include <LibHTTP/HeaderList.h>
#include <LibHTTP/Method.h>
#include <LibHTTP/Status.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/CORS.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/MIME.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/Statuses.h>
#include <LibWeb/MimeSniff/MimeType.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/Requests.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/Responses.h>
#include <LibWeb/ReferrerPolicy/ReferrerPolicy.h>
#include <LibJS/Runtime/VM.h>
#include <LibURL/Parser.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web::Fetch::Infrastructure;

static std::string unescape(std::string const& s)
{
    // "\-" alone is the empty argument (a trailing empty field would not survive editors).
    if (s == "\\-")
        return {};
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char c = s[i + 1];
            if (c == 'n') { out += '\n'; i++; continue; }
            if (c == 't') { out += '\t'; i++; continue; }
            if (c == '\\') { out += '\\'; i++; continue; }
            if (c == 'x' && i + 3 < s.size()) {
                out += (char)std::stoi(s.substr(i + 2, 2), nullptr, 16);
                i += 3;
                continue;
            }
        }
        out += s[i];
    }
    return out;
}

static std::string escape(StringView v)
{
    std::string out;
    for (size_t i = 0; i < v.length(); i++) {
        unsigned char c = v[i];
        if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20 || c >= 0x7f) {
            char buf[8];
            snprintf(buf, sizeof buf, "\\x%02X", c);
            out += buf;
        } else out += (char)c;
    }
    return out;
}

static StringView sv(std::string const& s) { return StringView(s.data(), s.size()); }
static std::string str(StringView v) { return std::string(v.characters_without_null_termination(), v.length()); }
static std::string b(bool v) { return v ? "true" : "false"; }

template<typename T>
static std::string list(Vector<T> const& values)
{
    std::string out = "[";
    bool first = true;
    for (auto const& value : values) {
        if (!first) out += ", ";
        first = false;
        out += "\"" + escape(value.bytes_as_string_view()) + "\"";
    }
    return out + "]";
}

static std::string list_bs(Vector<ByteString> const& values)
{
    std::string out = "[";
    bool first = true;
    for (auto const& value : values) {
        if (!first) out += ", ";
        first = false;
        out += "\"" + escape(value.view()) + "\"";
    }
    return out + "]";
}

static std::string headers(Vector<HTTP::Header> const& hs)
{
    std::string out = "[";
    bool first = true;
    for (auto const& h : hs) {
        if (!first) out += ", ";
        first = false;
        out += "(" + escape(h.name.view()) + ": " + escape(h.value.view()) + ")";
    }
    return out + "]";
}

static std::string opt_u64(Optional<u64> v) { return v.has_value() ? std::to_string(*v) : "null"; }

static std::string mime(Optional<Web::MimeSniff::MimeType> const& m)
{
    if (!m.has_value()) return "failure";
    auto serialized = m->serialized();
    return escape(serialized.bytes_as_string_view());
}

// A header list built by the ops in args: "A" name value (append), "S" name value (set), "C"
// name value (combine), "D" name (delete); query ops append their result to the output.
static std::string header_list(std::vector<std::string> const& args)
{
    auto list_ = HTTP::HeaderList::create();
    std::vector<std::string> out;
    for (size_t i = 0; i < args.size();) {
        auto const& op = args[i++];
        if (op == "A") { list_->append(HTTP::Header { ByteString(sv(args[i])), ByteString(sv(args[i + 1])) }); i += 2; }
        else if (op == "S") { list_->set(HTTP::Header { ByteString(sv(args[i])), ByteString(sv(args[i + 1])) }); i += 2; }
        else if (op == "C") { list_->combine(HTTP::Header { ByteString(sv(args[i])), ByteString(sv(args[i + 1])) }); i += 2; }
        else if (op == "D") { list_->delete_(sv(args[i])); i += 1; }
        else if (op == "headers") out.push_back(headers(list_->headers()));
        else if (op == "contains") { out.push_back(b(list_->contains(sv(args[i])))); i += 1; }
        else if (op == "get") {
            auto v = list_->get(sv(args[i]));
            out.push_back(v.has_value() ? "\"" + escape(v->view()) + "\"" : "null");
            i += 1;
        } else if (op == "decode_split") {
            auto v = list_->get_decode_and_split(sv(args[i]));
            out.push_back(v.has_value() ? list(*v) : "null");
            i += 1;
        } else if (op == "extract_values") {
            auto v = list_->extract_header_list_values(sv(args[i]));
            if (v.has<Empty>()) out.push_back("null");
            else if (v.has<HTTP::HeaderList::ExtractHeaderParseFailure>()) out.push_back("failure");
            else out.push_back(list_bs(v.get<Vector<ByteString>>()));
            i += 1;
        } else if (op == "length") {
            auto v = list_->extract_length();
            if (v.has<Empty>()) out.push_back("null");
            else if (v.has<HTTP::HeaderList::ExtractLengthFailure>()) out.push_back("failure");
            else out.push_back(std::to_string(v.get<u64>()));
        } else if (op == "content_range") {
            auto v = list_->extract_content_range_values();
            if (v.has<HTTP::HeaderList::ExtractContentRangeFailure>()) out.push_back("failure");
            else {
                auto r = v.get<HTTP::HeaderList::ContentRangeValues>();
                out.push_back(std::to_string(r.first_byte_pos) + "-" + std::to_string(r.last_byte_pos) + "/" + opt_u64(r.complete_length));
            }
        } else if (op == "sort_combine") out.push_back(headers(list_->sort_and_combine()));
        else if (op == "unique") out.push_back(list_bs(list_->unique_names()));
        else if (op == "mime") out.push_back(mime(extract_mime_type(*list_)));
        else if (op == "legacy_encoding") {
            out.push_back(escape(legacy_extract_an_encoding(extract_mime_type(*list_), sv(args[i]))));
            i += 1;
        } else if (op == "cors_unsafe_names") out.push_back(list_bs(get_cors_unsafe_header_names(*list_)));
        else if (op == "vary") {
            std::string s = "[";
            bool first = true;
            list_->for_each_vary_header([&](StringView v) {
                if (!first) s += ", ";
                first = false;
                s += "\"" + escape(v) + "\"";
                return IterationDecision::Continue;
            });
            out.push_back(s + "]");
        } else {
            out.push_back("unknown op " + op);
        }
    }
    std::string result;
    for (size_t i = 0; i < out.size(); i++) {
        if (i) result += " ; ";
        result += out[i];
    }
    return result;
}


static JS::VM& the_vm()
{
    static auto vm = JS::VM::create();
    return *vm;
}

static URL::URL url(std::string const& s)
{
    return URL::Parser::basic_parse(sv(s)).release_value();
}

static Request::OriginType origin_of(std::string const& s)
{
    if (s == "client") return Request::Origin::Client;
    if (s == "opaque") return URL::Origin::create_opaque();
    return url(s).origin();
}

static StringView taint_name(RedirectTaint taint)
{
    switch (taint) {
    case RedirectTaint::SameOrigin: return "same-origin"sv;
    case RedirectTaint::SameSite: return "same-site"sv;
    case RedirectTaint::CrossSite: return "cross-site"sv;
    }
    return "?"sv;
}

static Request::Mode mode_of(std::string const& s)
{
    if (s == "same-origin") return Request::Mode::SameOrigin;
    if (s == "cors") return Request::Mode::CORS;
    if (s == "no-cors") return Request::Mode::NoCORS;
    if (s == "navigate") return Request::Mode::Navigate;
    return Request::Mode::WebSocket;
}

static Request::ResponseTainting tainting_of(std::string const& s)
{
    if (s == "basic") return Request::ResponseTainting::Basic;
    if (s == "cors") return Request::ResponseTainting::CORS;
    return Request::ResponseTainting::Opaque;
}

static std::string type_name(Response::Type type)
{
    switch (type) {
    case Response::Type::Basic: return "basic";
    case Response::Type::CORS: return "cors";
    case Response::Type::Default: return "default";
    case Response::Type::Error: return "error";
    case Response::Type::Opaque: return "opaque";
    case Response::Type::OpaqueRedirect: return "opaqueredirect";
    }
    return "?";
}

static std::string join(std::vector<std::string> const& parts)
{
    std::string result;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i) result += " ; ";
        result += parts[i];
    }
    return result;
}

static std::string describe_response(Response& response)
{
    std::vector<std::string> out;
    out.push_back(type_name(response.type()));
    out.push_back(std::to_string(response.status()));
    out.push_back("\"" + escape(response.status_message().view()) + "\"");
    out.push_back(headers(response.header_list()->headers()));
    out.push_back(std::to_string(response.url_list().size()));
    if (response.url().has_value()) {
        auto serialized = response.url()->serialize();
        out.push_back(escape(serialized.bytes_as_string_view()));
    } else {
        out.push_back("null");
    }
    out.push_back(b(response.body() == nullptr));
    out.push_back(b(response.is_network_error()));
    out.push_back(b(response.is_aborted_network_error()));
    out.push_back(b(response.is_cors_same_origin()));
    out.push_back(b(response.is_cors_cross_origin()));
    out.push_back(b(response.aborted()));
    out.push_back(response.network_error_message().has_value() ? escape(response.network_error_message()->bytes_as_string_view()) : "null");
    return join(out);
}

static std::string run(std::vector<std::string> const& f)
{
    auto const& op = f[0];
    auto a = [&](size_t i) { return sv(f[i]); };
    if (op == "is_header_name") return b(HTTP::is_header_name(a(1)));
    if (op == "is_header_value") return b(HTTP::is_header_value(a(1)));
    if (op == "normalize_header_value") return "\"" + escape(HTTP::normalize_header_value(a(1))) + "\"";
    if (op == "is_forbidden_request_header") return b(HTTP::is_forbidden_request_header(HTTP::Header { ByteString(a(1)), ByteString(a(2)) }));
    if (op == "is_forbidden_response_header_name") return b(HTTP::is_forbidden_response_header_name(a(1)));
    if (op == "decode_split") return list(HTTP::get_decode_and_split_header_value(a(1)));
    if (op == "extract_header_values") {
        auto v = HTTP::Header { ByteString(a(1)), ByteString(a(2)) }.extract_header_values();
        return v.has_value() ? list_bs(*v) : "failure";
    }
    if (op == "sorted_lowercase_set") {
        Vector<ByteString> names;
        for (size_t i = 1; i < f.size(); i++)
            names.append(ByteString(a(i)));
        return list_bs(HTTP::convert_header_names_to_a_sorted_lowercase_set(names));
    }
    if (op == "build_content_range")
    {
        auto range = HTTP::build_content_range(std::stoull(f[1]), std::stoull(f[2]), std::stoull(f[3]));
        return escape(range.view());
    }
    if (op == "parse_single_range") {
        auto v = HTTP::parse_single_range_header_value(a(1), f[2] == "1");
        return v.has_value() ? opt_u64(v->start) + "-" + opt_u64(v->end) : "failure";
    }
    if (op == "isomorphic_encode") {
        auto h = HTTP::Header::isomorphic_encode(a(1), a(2));
        return "(" + escape(h.name.view()) + ": " + escape(h.value.view()) + ")";
    }
    if (op == "is_method") return b(HTTP::is_method(a(1)));
    if (op == "is_cors_safelisted_method") return b(HTTP::is_cors_safelisted_method(a(1)));
    if (op == "is_forbidden_method") return b(HTTP::is_forbidden_method(a(1)));
    if (op == "normalize_method") {
        auto method = HTTP::normalize_method(a(1));
        return escape(method.view());
    }
    if (op == "reason_phrase") return escape(HTTP::reason_phrase_for_code(std::stoul(f[1])));
    if (op == "token_code_points") {
        std::string s;
        for (u32 c = 0; c < 256; c++)
            s += HTTP::is_http_token_code_point(c) ? '1' : '0';
        return s;
    }
    if (op == "header_list") return header_list(std::vector<std::string>(f.begin() + 1, f.end()));
    if (op == "cors_safelisted_request_header") return b(is_cors_safelisted_request_header(HTTP::Header { ByteString(a(1)), ByteString(a(2)) }));
    if (op == "cors_unsafe_bytes") {
        std::string s;
        for (int c = 0; c < 256; c++)
            s += is_cors_unsafe_request_header_byte((u8)c) ? '1' : '0';
        return s;
    }
    if (op == "cors_non_wildcard") return b(is_cors_non_wildcard_request_header_name(a(1)));
    if (op == "privileged_no_cors") return b(is_privileged_no_cors_request_header_name(a(1)));
    if (op == "cors_safelisted_response_header_name") {
        Vector<StringView> exposed;
        for (size_t i = 2; i < f.size(); i++)
            exposed.append(a(i));
        return b(is_cors_safelisted_response_header_name(a(1), exposed));
    }
    if (op == "no_cors_safelisted_name") return b(is_no_cors_safelisted_request_header_name(a(1)));
    if (op == "no_cors_safelisted_header") return b(is_no_cors_safelisted_request_header(HTTP::Header { ByteString(a(1)), ByteString(a(2)) }));
    if (op == "statuses") {
        std::string null_body, ok, redirect;
        for (u16 s = 0; s < 1000; s++) {
            if (is_null_body_status(s)) null_body += " " + std::to_string(s);
            if (is_ok_status(s)) ok += " " + std::to_string(s);
            if (is_redirect_status(s)) redirect += " " + std::to_string(s);
        }
        return "null:" + null_body + " ; ok:" + ok + " ; redirect:" + redirect;
    }
    if (op == "request_taint") {
        auto request = Request::create(the_vm());
        request->set_origin(origin_of(f[1]));
        Vector<URL::URL> urls;
        for (size_t i = 2; i < f.size(); i++)
            urls.append(url(f[i]));
        request->set_url_list(urls);
        auto serialized = request->serialize_origin();
        auto byte_serialized = request->byte_serialize_origin();
        return str(taint_name(request->redirect_taint())) + " ; " + escape(serialized.bytes_as_string_view()) + " ; " + escape(byte_serialized.view());
    }
    if (op == "request_origin_header") {
        auto request = Request::create(the_vm());
        request->set_method(ByteString(a(1)));
        request->set_mode(mode_of(f[2]));
        request->set_response_tainting(tainting_of(f[3]));
        request->set_referrer_policy(Web::ReferrerPolicy::from_string(a(4)).value());
        request->set_origin(origin_of(f[5]));
        Vector<URL::URL> urls;
        for (size_t i = 6; i < f.size(); i++)
            urls.append(url(f[i]));
        request->set_url_list(urls);
        request->add_origin_header();
        return headers(request->header_list()->headers());
    }
    if (op == "request_range") {
        auto request = Request::create(the_vm());
        Optional<u64> last;
        if (f[2] != "null")
            last = std::stoull(f[2]);
        request->add_range_header(std::stoull(f[1]), last);
        return headers(request->header_list()->headers());
    }
    if (op == "request_destinations") {
        std::vector<std::string> out;
        for (int d = -1; d <= (int)Request::Destination::XSLT; d++) {
            auto request = Request::create(the_vm());
            std::string name = "(empty)";
            if (d >= 0) {
                request->set_destination((Request::Destination)d);
                name = str(request_destination_to_string((Request::Destination)d));
            }
            out.push_back(name + ":" + (request->is_subresource_request() ? "s" : "-") + (request->is_non_subresource_request() ? "n" : "-") + (request->is_navigation_request() ? "v" : "-") + (request->destination_is_script_like() ? "j" : "-"));
        }
        return join(out);
    }
    if (op == "translate_potential_destination") {
        auto d = translate_potential_destination(a(1));
        return d.has_value() ? str(request_destination_to_string(*d)) : "null";
    }
    if (op == "request_modes") {
        std::vector<std::string> out;
        for (int m = 0; m <= (int)Request::Mode::WebSocket; m++)
            out.push_back(str(request_mode_to_string((Request::Mode)m)));
        return join(out);
    }
    if (op == "initiator_types") {
        std::vector<std::string> out;
        for (int t = 0; t <= (int)Request::InitiatorType::Other; t++) {
            auto type = initiator_type_to_string((Request::InitiatorType)t);
            out.push_back(str(type.bytes_as_string_view()));
        }
        return join(out);
    }
    if (op == "priority") {
        auto p = request_priority_from_string(a(1));
        if (!p.has_value()) return "null";
        return *p == Request::Priority::High ? "high" : *p == Request::Priority::Low ? "low" : "auto";
    }
    if (op == "response_filter") {
        auto internal = Response::create(the_vm());
        internal->set_url_list({ url("https://a.test/x"), url("https://b.test/y") });
        internal->set_status_message("OK"sv);
        Vector<ByteString> exposed;
        if (f[2] != "-") {
            for (auto name : a(2).split_view(','))
                exposed.append(ByteString(name));
        }
        internal->set_cors_exposed_header_name_list(exposed);
        for (size_t i = 3; i + 1 < f.size(); i += 2)
            internal->header_list()->append(HTTP::Header { ByteString(a(i)), ByteString(a(i + 1)) });
        GC::Ptr<Response> response;
        if (f[1] == "basic") response = BasicFilteredResponse::create(the_vm(), internal);
        else if (f[1] == "cors") response = CORSFilteredResponse::create(the_vm(), internal);
        else if (f[1] == "opaque") response = OpaqueFilteredResponse::create(the_vm(), internal);
        else if (f[1] == "opaqueredirect") response = OpaqueRedirectFilteredResponse::create(the_vm(), internal);
        else response = internal;
        return describe_response(*response) + " ; " + b(response->unsafe_response().ptr() == internal.ptr());
    }
    if (op == "network_error") {
        auto response = f[1] == "aborted" ? Response::aborted_network_error(the_vm()) : Response::network_error(the_vm(), "Something failed"_string);
        return describe_response(*response);
    }
    if (op == "location") {
        auto response = Response::create(the_vm());
        response->set_status(std::stoul(f[1]));
        if (f[2] != "-")
            response->set_url_list({ url(f[2]) });
        Optional<String> fragment;
        if (f[3] != "-")
            fragment = MUST(String::from_utf8(a(3)));
        for (size_t i = 4; i < f.size(); i++)
            response->header_list()->append(HTTP::Header { "Location"sv, ByteString(a(i)) });
        auto location = response->location_url(fragment);
        if (location.is_error()) return "failure";
        if (!location.value().has_value()) return "null";
        auto serialized = location.value()->serialize();
        return escape(serialized.bytes_as_string_view());
    }
    return "unknown op";
}

int main(int argc, char** argv)
{
    std::ifstream in(argc > 1 ? argv[1] : "cases.txt");
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::vector<std::string> fields;
        size_t start = 0;
        while (true) {
            auto tab = line.find('\t', start);
            fields.push_back(unescape(line.substr(start, tab == std::string::npos ? std::string::npos : tab - start)));
            if (tab == std::string::npos)
                break;
            start = tab + 1;
        }
        std::cout << line << "\n= " << run(fields) << "\n";
    }
    return 0;
}
