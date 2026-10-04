// Oracle for luce-browser-engine regions p2e + p2f (Content Security Policy): runs the reference
// build's LibWeb ContentSecurityPolicy (policies, directives, source expressions, the blocking
// algorithms) and SRI on the cases of cases.txt and prints one result line per case, the text
// tests_csp_1 compares.
//
// A case line is "<op>\t<arg>\t<arg>..." with \n, \t, \\ and \xHH escaped in each argument and
// "\-" for an empty argument. The output repeats the case line, then "= <result>".
//
// Requests and elements live in the active document of a test page's top-level traversable (an
// about:blank document with a Window), as xml_documents' oracle makes it; policies that the
// element, <base> and navigation checks consult are enforced in that window's CSP list for the
// case and removed after. Violations are reported (their tasks queued, never run).
#define private public
#define protected public
#include <AK/Base64.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/ContentSecurityPolicy/BlockingAlgorithms.h>
#include <LibWeb/ContentSecurityPolicy/Directives/Directive.h>
#include <LibWeb/ContentSecurityPolicy/Directives/DirectiveFactory.h>
#include <LibWeb/ContentSecurityPolicy/Directives/DirectiveOperations.h>
#include <LibWeb/ContentSecurityPolicy/Directives/Names.h>
#include <LibWeb/ContentSecurityPolicy/Directives/SourceExpression.h>
#include <LibWeb/ContentSecurityPolicy/Policy.h>
#include <LibWeb/ContentSecurityPolicy/PolicyList.h>
#include <LibWeb/ContentSecurityPolicy/Violation.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/Element.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/Requests.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/Responses.h>
#include <LibWeb/HTML/HTMLScriptElement.h>
#include <LibWeb/HTML/PolicyContainers.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/HTML/Window.h>
#include <LibWeb/Namespace.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
#include <LibWeb/SRI/SRI.h>
#include <LibWeb/SVG/SVGScriptElement.h>
#include <LibURL/Parser.h>
#include <LibCore/EventLoop.h>
#undef private
#undef protected
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web;
using namespace Web::ContentSecurityPolicy;
using Web::Fetch::Infrastructure::Request;
using Web::Fetch::Infrastructure::Response;

static std::string unescape(std::string const& s)
{
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
static String string(std::string const& s) { return MUST(String::from_utf8(sv(s))); }
static std::string str(StringView v) { return std::string(v.characters_without_null_termination(), v.length()); }
static std::string esc(String const& v) { return escape(v.bytes_as_string_view()); }

// mark: The test page ===========================================================================

class OraclePageClient final : public PageClient {
    GC_CELL(OraclePageClient, PageClient);
    GC_DECLARE_ALLOCATOR(OraclePageClient);

public:
    GC::Ptr<Page> m_page;
    virtual void visit_edges(Visitor& visitor) override
    {
        Base::visit_edges(visitor);
        visitor.visit(m_page);
    }
    virtual u64 id() const override { return 1; }
    virtual Page& page() override { return *m_page; }
    virtual Page const& page() const override { return *m_page; }
    virtual bool is_connection_open() const override { return false; }
    virtual Gfx::Palette palette() const override { VERIFY_NOT_REACHED(); }
    virtual DevicePixelRect screen_rect() const override { return { 0, 0, 800, 600 }; }
    virtual double zoom_level() const override { return 1; }
    virtual double device_pixel_ratio() const override { return 1; }
    virtual double device_pixels_per_css_pixel() const override { return 1; }
    virtual CSS::PreferredColorScheme preferred_color_scheme() const override { return CSS::PreferredColorScheme::Auto; }
    virtual CSS::PreferredContrast preferred_contrast() const override { return CSS::PreferredContrast::Auto; }
    virtual CSS::PreferredMotion preferred_motion() const override { return CSS::PreferredMotion::Auto; }
    virtual size_t screen_count() const override { return 1; }
    virtual Queue<QueuedInputEvent>& input_event_queue() override { VERIFY_NOT_REACHED(); }
    virtual void report_finished_handling_input_event(u64, EventResult) override { }
    virtual void request_frame() override { }
    virtual void request_file(FileRequest) override { }
    virtual DisplayListPlayerType display_list_player_type() const override { return DisplayListPlayerType::SkiaCPU; }
    virtual bool is_headless() const override { return true; }
};

GC_DEFINE_ALLOCATOR(OraclePageClient);

static GC::Root<Page> g_page;
static GC::Root<DOM::Document> g_window_document;

static DOM::Document& window_document()
{
    if (!g_window_document) {
        auto& vm = Bindings::main_thread_vm();
        auto client = vm.heap().allocate<OraclePageClient>();
        auto page = Page::create(vm, *client);
        client->m_page = page;
        page->set_is_scripting_enabled(false);
        page->set_top_level_traversable(HTML::TraversableNavigable::create_a_new_top_level_traversable(*page, nullptr, {}));
        g_page = GC::make_root(page);
        g_window_document = GC::make_root(*page->top_level_traversable()->active_document());
    }
    return *g_window_document;
}

static JS::VM& the_vm() { return Bindings::main_thread_vm(); }
static GC::Heap& the_heap() { return the_vm().heap(); }

// mark: Arguments ===============================================================================

static URL::URL url(std::string const& s)
{
    auto parsed = URL::Parser::basic_parse(sv(s));
    VERIFY(parsed.has_value());
    return parsed.release_value();
}

static URL::Origin origin_of(std::string const& s)
{
    if (s == "opaque")
        return URL::Origin::create_opaque();
    return url(s).origin();
}

static Policy::Disposition disposition_of(std::string const& s) { return s == "report" ? Policy::Disposition::Report : Policy::Disposition::Enforce; }

static GC::Ref<Policy> policy_of(std::string const& serialized, std::string const& self_origin = "https://self.test/", std::string const& disposition = "enforce")
{
    auto policy = Policy::parse_a_serialized_csp(the_heap(), string(serialized), Policy::Source::Header, disposition_of(disposition));
    if (!self_origin.empty())
        policy->m_self_origin = origin_of(self_origin);
    return policy;
}

static Optional<Request::Destination> destination_of(std::string const& s)
{
    if (s == "-")
        return {};
    for (int d = 0; d <= (int)Request::Destination::XSLT; d++) {
        if (str(Fetch::Infrastructure::request_destination_to_string((Request::Destination)d)) == s)
            return (Request::Destination)d;
    }
    VERIFY_NOT_REACHED();
}

static Optional<Request::Initiator> initiator_of(std::string const& s)
{
    if (s == "-") return {};
    if (s == "download") return Request::Initiator::Download;
    if (s == "imageset") return Request::Initiator::ImageSet;
    if (s == "manifest") return Request::Initiator::Manifest;
    if (s == "prefetch") return Request::Initiator::Prefetch;
    if (s == "prerender") return Request::Initiator::Prerender;
    if (s == "xslt") return Request::Initiator::XSLT;
    VERIFY_NOT_REACHED();
}

static Directives::Directive::InlineType inline_type_of(std::string const& s)
{
    if (s == "navigation") return Directives::Directive::InlineType::Navigation;
    if (s == "script") return Directives::Directive::InlineType::Script;
    if (s == "script-attribute") return Directives::Directive::InlineType::ScriptAttribute;
    if (s == "style") return Directives::Directive::InlineType::Style;
    if (s == "style-attribute") return Directives::Directive::InlineType::StyleAttribute;
    VERIFY_NOT_REACHED();
}

static Directives::Production production_of(std::string const& s)
{
    if (s == "scheme") return Directives::Production::SchemeSource;
    if (s == "host") return Directives::Production::HostSource;
    if (s == "keyword") return Directives::Production::KeywordSource;
    if (s == "nonce") return Directives::Production::NonceSource;
    if (s == "hash") return Directives::Production::HashSource;
    VERIFY_NOT_REACHED();
}

// A request: f[i] url, f[i+1] destination, f[i+2] initiator, f[i+3] nonce, f[i+4] integrity metadata,
// f[i+5] parser metadata, f[i+6] redirect count, then more URLs of its URL list ("|"-separated in f[i]).
static GC::Ref<Request> request_of(std::vector<std::string> const& f, size_t i)
{
    auto request = Request::create(the_vm());
    Vector<URL::URL> urls;
    for (auto part : sv(f[i]).split_view('|'))
        urls.append(url(str(part)));
    request->set_url_list(urls);
    request->set_destination(destination_of(f[i + 1]));
    request->set_initiator(initiator_of(f[i + 2]));
    request->set_cryptographic_nonce_metadata(string(f[i + 3]));
    request->set_integrity_metadata(string(f[i + 4]));
    if (f[i + 5] == "parser-inserted")
        request->set_parser_metadata(Request::ParserMetadata::ParserInserted);
    else if (f[i + 5] == "not-parser-inserted")
        request->set_parser_metadata(Request::ParserMetadata::NotParserInserted);
    request->set_redirect_count(std::stoi(f[i + 6]));
    request->set_client(&window_document().relevant_settings_object());
    return request;
}

// An element of the window's document: f[i] "-" (null) or "html:tag" / "svg:tag", then "name=value"
// attributes, "flag:parser-inserted" and "flag:duplicate-attribute".
static GC::Ptr<DOM::Element> element_of(std::vector<std::string> const& f, size_t i)
{
    if (f[i] == "-")
        return nullptr;
    auto colon = f[i].find(':');
    auto ns = f[i].substr(0, colon) == "svg" ? Namespace::SVG : Namespace::HTML;
    auto element = MUST(DOM::create_element(window_document(), MUST(FlyString::from_utf8(sv(f[i].substr(colon + 1)))), ns));
    for (size_t j = i + 1; j < f.size(); j++) {
        if (f[j] == "flag:parser-inserted") {
            if (auto* script = as_if<HTML::HTMLScriptElement>(*element))
                script->m_parser_document = &window_document();
            if (auto* script = as_if<SVG::SVGScriptElement>(*element))
                script->m_parser_inserted = true;
            continue;
        }
        if (f[j] == "flag:duplicate-attribute") {
            element->m_had_duplicate_attribute_during_tokenization = true;
            continue;
        }
        auto equals = f[j].find('=');
        element->set_attribute_value(MUST(FlyString::from_utf8(sv(f[j].substr(0, equals)))), string(f[j].substr(equals + 1)));
    }
    return element;
}

// The window's CSP list, enforcing `policy` for the case (removed by `restore`).
static GC::Ref<PolicyList> window_csp_list()
{
    return *PolicyList::from_object(window_document().realm().global_object());
}

// mark: Results =================================================================================

static std::string result(Directives::Directive::Result r) { return r == Directives::Directive::Result::Allowed ? "Allowed" : "Blocked"; }
static std::string match(Directives::MatchResult r) { return r == Directives::MatchResult::Matches ? "Matches" : "DoesNotMatch"; }

static std::string describe_policy(Policy const& policy)
{
    std::string out = policy.disposition() == Policy::Disposition::Enforce ? "enforce" : "report";
    out += policy.source() == Policy::Source::Header ? "/header" : "/meta";
    out += " self=";
    out += policy.m_self_origin.has_value() ? esc(policy.m_self_origin->serialize()) : "-";
    out += " pre=" + escape(policy.m_pre_parsed_policy_string.bytes_as_string_view()) + " [";
    bool first = true;
    for (auto const& directive : policy.directives()) {
        if (!first) out += " ; ";
        first = false;
        out += escape(directive->name().bytes_as_string_view()) + "(" + str(directive->class_name()) + ")";
        for (auto const& value : directive->value())
            out += " " + escape(value.bytes_as_string_view());
    }
    return out + "]";
}

static std::string opt(Optional<StringView> const& v) { return v.has_value() ? escape(*v) : "-"; }

// does_request_violate_policy (BlockingAlgorithms.cpp, static): the directive whose pre-request check blocks.
static std::string violates(GC::Ref<Request> request, GC::Ref<Policy> policy)
{
    if (request->initiator() == Request::Initiator::Prefetch) {
        auto has_default = false;
        for (auto directive : policy->directives())
            has_default |= directive->name() == Directives::Names::DefaultSrc;
        if (!has_default)
            return "none";
        for (auto directive : policy->directives()) {
            if (directive->pre_request_check(the_heap(), request, policy) == Directives::Directive::Result::Allowed)
                return "none";
        }
        return "default-src";
    }
    std::string violating = "none";
    for (auto directive : policy->directives()) {
        if (directive->pre_request_check(the_heap(), request, policy) == Directives::Directive::Result::Blocked)
            violating = str(directive->name().bytes_as_string_view());
    }
    return violating;
}

static std::string run(std::vector<std::string> const& f)
{
    auto const& op = f[0];
    if (op == "parse") {
        auto policy = Policy::parse_a_serialized_csp(the_heap(), string(f[3]), f[1] == "meta" ? Policy::Source::Meta : Policy::Source::Header, disposition_of(f[2]));
        return describe_policy(*policy);
    }
    if (op == "headers") {
        auto response = Response::create(the_vm());
        response->set_url_list({ url(f[1]) });
        for (size_t i = 2; i + 1 < f.size(); i += 2)
            response->header_list()->append(HTTP::Header { ByteString(sv(f[i])), ByteString(sv(f[i + 1])) });
        auto list = Policy::parse_a_responses_content_security_policies(the_heap(), response);
        std::string out = std::to_string(list->policies().size());
        for (auto policy : list->policies())
            out += " || " + describe_policy(*policy);
        return out;
    }
    if (op == "expr") {
        auto parsed = Directives::parse_source_expression(production_of(f[1]), sv(f[2]));
        if (!parsed.has_value())
            return "none";
        return "scheme=" + opt(parsed->scheme_part) + " host=" + opt(parsed->host_part) + " port=" + opt(parsed->port_part) + " path=" + opt(parsed->path_part)
            + " keyword=" + opt(parsed->keyword_source) + " base64=" + opt(parsed->base64_value) + " hash=" + opt(parsed->hash_algorithm);
    }
    if (op == "url_expr")
        return match(Directives::does_url_match_expression_in_origin_with_redirect_count(url(f[1]), string(f[4]), origin_of(f[2]), std::stoi(f[3])));
    if (op == "url_list") {
        Vector<String> list;
        for (size_t i = 4; i < f.size(); i++)
            list.append(string(f[i]));
        return match(Directives::does_url_match_source_list_in_origin_with_redirect_count(url(f[1]), list, origin_of(f[2]), std::stoi(f[3])));
    }
    if (op == "effective") {
        auto request = Request::create(the_vm());
        request->set_destination(destination_of(f[1]));
        request->set_initiator(initiator_of(f[2]));
        auto name = Directives::get_the_effective_directive_for_request(request);
        return name.has_value() ? str(name->bytes_as_string_view()) : "null";
    }
    if (op == "inline_effective")
        return str(Directives::get_the_effective_directive_for_inline_checks(inline_type_of(f[1])).bytes_as_string_view());
    if (op == "fallback") {
        Optional<FlyString> name;
        if (f[1] != "-")
            name = MUST(FlyString::from_utf8(sv(f[1])));
        std::string out = "[";
        for (auto item : Directives::get_fetch_directive_fallback_list(name))
            out += " " + str(item);
        return out + " ]";
    }
    if (op == "execute") {
        Optional<FlyString> effective;
        if (f[1] != "-")
            effective = MUST(FlyString::from_utf8(sv(f[1])));
        auto policy = policy_of(f[3]);
        auto r = Directives::should_fetch_directive_execute(effective, MUST(FlyString::from_utf8(sv(f[2]))), policy);
        return r == Directives::ShouldExecute::Yes ? "Yes" : "No";
    }
    if (op == "request") {
        // policy, self origin, then the request
        auto policy = policy_of(f[1], f[2]);
        auto request = request_of(f, 3);
        std::string out;
        for (auto directive : policy->directives())
            out += str(directive->name().bytes_as_string_view()) + ":" + result(directive->pre_request_check(the_heap(), request, policy)) + " ";
        return out + "; violates=" + violates(request, policy);
    }
    if (op == "response") {
        // policy, self origin, the request, then the response URL
        auto policy = policy_of(f[1], f[2]);
        auto request = request_of(f, 3);
        auto response = Response::create(the_vm());
        if (f[10] != "-")
            response->set_url_list({ url(f[10]) });
        std::string out;
        for (auto directive : policy->directives())
            out += str(directive->name().bytes_as_string_view()) + ":" + result(directive->post_request_check(the_heap(), request, response, policy)) + " ";
        return out;
    }
    if (op == "element") {
        // policy, disposition, type, source, then the element
        auto policy = policy_of(f[1], "https://self.test/", f[2]);
        auto type = inline_type_of(f[3]);
        auto source = string(f[4]);
        auto element = element_of(f, 5);
        std::string out;
        for (auto directive : policy->directives())
            out += str(directive->name().bytes_as_string_view()) + ":" + result(directive->inline_check(the_heap(), element, type, policy, source)) + " ";
        if (element && type != Directives::Directive::InlineType::Navigation) {
            auto list = window_csp_list();
            list->m_policies.append(policy);
            auto blocked = should_elements_inline_type_behavior_be_blocked_by_content_security_policy(window_document().realm(), *element, type, source);
            list->m_policies.clear();
            out += "; " + result(blocked);
        }
        return out;
    }
    if (op == "matches_element") {
        // source list (space separated), type, source, then the element
        Vector<String> list;
        for (auto part : sv(f[1]).split_view(' '))
            list.append(string(str(part)));
        auto element = element_of(f, 4);
        return match(Directives::does_element_match_source_list_for_type_and_source(element, list, inline_type_of(f[2]), string(f[3])));
    }
    if (op == "navigation") {
        // policy, self origin, URL, navigation type
        auto policy = policy_of(f[1], f[2]);
        auto request = Request::create(the_vm());
        request->set_url(url(f[3]));
        auto container = the_heap().allocate<HTML::PolicyContainer>(the_heap());
        container->csp_list->m_policies.append(policy);
        request->set_policy_container(container);
        request->set_client(&window_document().relevant_settings_object());
        auto type = f[4] == "form" ? Directives::Directive::NavigationType::FormSubmission : Directives::Directive::NavigationType::Other;
        std::string out;
        for (auto directive : policy->directives())
            out += str(directive->name().bytes_as_string_view()) + ":" + result(directive->pre_navigation_check(request, type, policy)) + " ";
        return out + "; " + result(should_navigation_request_of_type_be_blocked_by_content_security_policy(request, type));
    }
    if (op == "should_request") {
        // policy, disposition, self origin, then the request
        auto policy = policy_of(f[1], f[3], f[2]);
        auto request = request_of(f, 4);
        auto container = the_heap().allocate<HTML::PolicyContainer>(the_heap());
        container->csp_list->m_policies.append(policy);
        request->set_policy_container(container);
        return result(should_request_be_blocked_by_content_security_policy(window_document().realm(), request));
    }
    if (op == "should_response") {
        // policy, disposition, self origin, the request, then the response URL
        auto policy = policy_of(f[1], f[3], f[2]);
        auto request = request_of(f, 4);
        auto container = the_heap().allocate<HTML::PolicyContainer>(the_heap());
        container->csp_list->m_policies.append(policy);
        request->set_policy_container(container);
        auto response = Response::create(the_vm());
        response->set_url_list({ url(f[11]) });
        return result(should_response_to_request_be_blocked_by_content_security_policy(window_document().realm(), response, request));
    }
    if (op == "base") {
        // policy, disposition, self origin, base URL
        auto policy = policy_of(f[1], f[3], f[2]);
        auto list = window_csp_list();
        list->m_policies.append(policy);
        auto r = is_base_allowed_for_document(window_document().realm(), url(f[4]), window_document());
        list->m_policies.clear();
        return result(r);
    }
    if (op == "integrity") {
        // integrity sources, blocked destinations (comma separated), report-only sources, blocked
        // destinations, then the request (URL, destination, mode, integrity metadata)
        auto container = the_heap().allocate<HTML::PolicyContainer>(the_heap());
        auto fill = [](HTML::IntegrityPolicy& policy, std::string const& sources, std::string const& destinations) {
            for (auto part : sv(sources).split_view(','))
                policy.sources.append(string(str(part)));
            for (auto part : sv(destinations).split_view(','))
                policy.blocked_destinations.append(*destination_of(str(part)));
        };
        fill(container->integrity_policy, f[1], f[2]);
        fill(container->report_only_integrity_policy, f[3], f[4]);
        auto request = Request::create(the_vm());
        request->set_url(url(f[5]));
        request->set_destination(destination_of(f[6]));
        request->set_mode(f[7] == "cors" ? Request::Mode::CORS : f[7] == "same-origin" ? Request::Mode::SameOrigin : Request::Mode::NoCORS);
        request->set_integrity_metadata(string(f[8]));
        request->set_policy_container(container);
        request->set_client(&window_document().relevant_settings_object());
        return result(should_request_be_blocked_by_integrity_policy(request));
    }
    if (op == "webrtc") {
        auto policy = policy_of(f[1]);
        std::string out;
        for (auto directive : policy->directives())
            out += str(directive->name().bytes_as_string_view()) + ":" + result(directive->webrtc_pre_connect_check(policy)) + " ";
        return out;
    }
    if (op == "blocked_uri") {
        // a URL or a resource keyword
        auto policy = policy_of("default-src 'none'");
        auto violation = Violation::create_a_violation_object_for_global_policy_and_directive(window_document().realm(), window_document().window(), policy, "default-src"_string);
        if (f[1] == "inline") violation->set_resource(Violation::Resource::Inline);
        else if (f[1] == "eval") violation->set_resource(Violation::Resource::Eval);
        else if (f[1] == "wasm-eval") violation->set_resource(Violation::Resource::WasmEval);
        else violation->set_resource(url(f[1]));
        return esc(violation->obtain_the_blocked_uri_of_resource());
    }
    if (op == "violation_request") {
        // policy, then the request
        auto policy = policy_of(f[1]);
        auto request = request_of(f, 2);
        auto violation = Violation::create_a_violation_object_for_request_and_policy(window_document().realm(), request, policy);
        return escape(violation->effective_directive().bytes_as_string_view()) + " ; " + esc(violation->obtain_the_blocked_uri_of_resource())
            + " ; " + (violation->referrer().has_value() ? esc(violation->referrer()->serialize()) : "-");
    }
    if (op == "sri_parse") {
        auto metadata = MUST(SRI::parse_metadata(sv(f[1])));
        std::string out = "[";
        for (auto const& item : metadata)
            out += " " + escape(item.algorithm.bytes_as_string_view()) + ":" + escape(item.base64_value.bytes_as_string_view());
        out += " ] strongest [";
        for (auto const& item : MUST(SRI::get_strongest_metadata_from_set(metadata)))
            out += " " + escape(item.algorithm.bytes_as_string_view()) + ":" + escape(item.base64_value.bytes_as_string_view());
        return out + " ]";
    }
    if (op == "sri_match")
        return MUST(SRI::do_bytes_match_metadata_list(MUST(ByteBuffer::copy(sv(f[1]).bytes())), sv(f[2]))) ? "true" : "false";
    if (op == "sri_hash")
        return esc(MUST(SRI::apply_algorithm_to_bytes(sv(f[1]), MUST(ByteBuffer::copy(sv(f[2]).bytes())))));
    VERIFY_NOT_REACHED();
}

int main(int argc, char** argv)
{
    Core::EventLoop event_loop;
    Web::Platform::EventLoopPlugin::install(*new Web::Platform::EventLoopPlugin);
    Web::Platform::FontPlugin::install(*new Web::Platform::FontPlugin(false));
    Web::Bindings::initialize_main_thread_vm(Web::Bindings::AgentType::SimilarOriginWindow);
    (void)window_document();
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
