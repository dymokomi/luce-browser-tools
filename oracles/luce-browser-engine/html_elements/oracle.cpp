// Oracle for luce-browser-engine region r27 (the remaining HTML elements): runs the reference build's
// HTML parser and element classes on the cases of cases.txt and prints what tests_html_elements_cases
// compares. Each case line is "<mode>\t<arg>..." with \n, \t and \\ escaped in the arguments.
//   doc <html>              the html5lib dump of a document parsed with scripting disabled
//   frag <context> <html>   the html5lib dump of a fragment parsed in a <context> element
//   fontsize <string>       HTMLFontElement::parse_legacy_font_size
//   hints <tag> <html>      the first <tag> element of the parsed document: is_presentational_hint of
//                           each attribute, then apply_presentational_hints' properties
//   role <tag> <html>       the first <tag> element's default ARIA role (its enum value)
//   canvas <html>           the first canvas' width, height and bitmap size
//   script <html>           each script element's preparation state after parsing
//   marquee <html>          the first marquee's scrollAmount and scrollDelay
//   query <selector> <html> the ids of querySelectorAll's elements (:heading(n) and the heading levels)
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/ARIA/Roles.h>
#include <LibWeb/CSS/StyleProperty.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/DOM/Attr.h>
#include <LibWeb/DOM/Comment.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/CSS/PropertyID.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/DocumentFragment.h>
#include <LibWeb/DOM/DocumentType.h>
#include <LibWeb/DOM/Text.h>
#include <LibWeb/DOM/NodeList.h>
#include <LibWeb/HTML/AttributeNames.h>
#include <LibWeb/HTML/HTMLCanvasElement.h>
#include <LibWeb/HTML/HTMLDocument.h>
#include <LibWeb/HTML/HTMLFontElement.h>
#include <LibWeb/HTML/HTMLMarqueeElement.h>
#include <LibWeb/HTML/HTMLScriptElement.h>
#include <LibWeb/HTML/HTMLTemplateElement.h>
#include <LibWeb/HTML/Parser/HTMLParser.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Namespace.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
#include <LibCore/EventLoop.h>
#undef private
#undef protected
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web;

static std::string g_out;
static std::string str(StringView view) { return std::string(view.characters_without_null_termination(), view.length()); }
static std::string str(String const& s) { return str(s.bytes_as_string_view()); }
static std::string str(FlyString const& s) { return str(s.bytes_as_string_view()); }
static std::string str(Utf16String const& s) { return str(s.to_utf8()); }

// A PageClient for a Page whose documents the oracle makes (no IPC, no input, no display).
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

// A new HTML document without a browsing context (as the Luce tests' heap document), parsed from
// `html` with scripting disabled.
static GC::Ref<DOM::Document> parse(std::string const& html)
{
    auto document = HTML::HTMLDocument::create(window_document().realm());
    document->set_document_type(DOM::Document::Type::HTML);
    auto parser = HTML::HTMLParser::create(*document, StringView(html.data(), html.size()), HTML::ParserScriptingMode::Disabled, "UTF-8"sv);
    parser->run(document->url());
    return document;
}

// mark: The html5lib dump (as tests_html_parser_support's parser_test_dump) ======================

static void indent(int level)
{
    g_out += "| ";
    for (int i = 0; i < level; i++)
        g_out += "  ";
}

static std::string namespace_prefix(Optional<FlyString> const& ns)
{
    if (!ns.has_value() || *ns == Namespace::HTML)
        return "";
    if (*ns == Namespace::SVG)
        return "svg ";
    if (*ns == Namespace::MathML)
        return "math ";
    if (*ns == Namespace::XLink)
        return "xlink ";
    if (*ns == Namespace::XML)
        return "xml ";
    if (*ns == Namespace::XMLNS)
        return "xmlns ";
    return "{" + str(*ns) + "} ";
}

static void dump_node(DOM::Node const& node, int level);
static void dump_children(DOM::Node const& node, int level)
{
    for (auto* child = node.first_child(); child; child = child->next_sibling())
        dump_node(*child, level);
}

static void dump_node(DOM::Node const& node, int level)
{
    indent(level);
    if (is<DOM::Element>(node)) {
        auto& element = static_cast<DOM::Element const&>(node);
        g_out += "<" + namespace_prefix(element.namespace_uri()) + str(element.local_name()) + ">\n";
        std::vector<std::string> lines;
        element.for_each_attribute([&](DOM::Attr const& attr) {
            lines.push_back(namespace_prefix(attr.namespace_uri()) + str(attr.local_name()) + "=\"" + str(attr.value()) + "\"");
        });
        std::sort(lines.begin(), lines.end());
        for (auto& l : lines) {
            indent(level + 1);
            g_out += l + "\n";
        }
        if (is<HTML::HTMLTemplateElement>(element)) {
            indent(level + 1);
            g_out += "content\n";
            dump_children(*static_cast<HTML::HTMLTemplateElement const&>(element).content(), level + 2);
        }
        dump_children(node, level + 1);
        return;
    }
    if (is<DOM::Text>(node)) {
        g_out += "\"" + str(static_cast<DOM::Text const&>(node).data()) + "\"\n";
        return;
    }
    if (is<DOM::Comment>(node)) {
        g_out += "<!-- " + str(static_cast<DOM::Comment const&>(node).data()) + " -->\n";
        return;
    }
    if (is<DOM::DocumentType>(node)) {
        auto& doctype = static_cast<DOM::DocumentType const&>(node);
        g_out += "<!DOCTYPE " + str(doctype.name());
        if (!doctype.public_id().is_empty() || !doctype.system_id().is_empty())
            g_out += " \"" + str(doctype.public_id()) + "\" \"" + str(doctype.system_id()) + "\"";
        g_out += ">\n";
        return;
    }
    g_out += "?\n";
}

// mark: Modes ===================================================================================

static DOM::Element* first_element(DOM::Document& document, std::string const& tag)
{
    DOM::Element* found = nullptr;
    document.for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
        if (str(element.local_name()) == tag) {
            found = &element;
            return TraversalDecision::Break;
        }
        return TraversalDecision::Continue;
    });
    VERIFY(found);
    return found;
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    if (mode == "doc") {
        auto document = parse(args[1]);
        dump_children(*document, 0);
    } else if (mode == "frag") {
        auto document = parse("<!DOCTYPE html><body>");
        auto context = MUST(DOM::create_element(*document, FlyString::from_utf8_without_validation(ReadonlyBytes(args[1].data(), args[1].size())), Namespace::HTML));
        MUST(document->body()->append_child(context));
        auto nodes = MUST(HTML::HTMLParser::parse_html_fragment(*context, StringView(args[2].data(), args[2].size())));
        for (auto& node : nodes)
            dump_node(*node, 0);
    } else if (mode == "fontsize") {
        auto keyword = HTML::HTMLFontElement::parse_legacy_font_size(StringView(args[1].data(), args[1].size()));
        g_out += keyword.has_value() ? str(CSS::string_from_keyword(*keyword)) + "\n" : "none\n";
    } else if (mode == "hints") {
        auto document = parse(args[2]);
        auto* element = first_element(*document, args[1]);
        element->for_each_attribute([&](auto const& name, auto const&) {
            g_out += "hint " + str(name) + " " + (element->is_presentational_hint(name) ? "1" : "0") + "\n";
        });
        Vector<CSS::StyleProperty> properties;
        element->apply_presentational_hints(properties);
        for (auto& property : properties)
            g_out += str(CSS::string_from_property_id(property.property_id)) + ": " + str(property.value->to_string(CSS::SerializationMode::Normal)) + "\n";
    } else if (mode == "role") {
        auto document = parse(args[2]);
        auto* element = first_element(*document, args[1]);
        auto role = element->default_role();
        g_out += role.has_value() ? std::to_string(to_underlying(*role)) + "\n" : "none\n";
    } else if (mode == "canvas") {
        auto document = parse(args[1]);
        auto& canvas = as<HTML::HTMLCanvasElement>(*first_element(*document, "canvas"));
        auto size = canvas.bitmap_size_for_canvas();
        g_out += std::to_string(canvas.width()) + " " + std::to_string(canvas.height()) + " " + std::to_string(size.width()) + "x" + std::to_string(size.height()) + "\n";
    } else if (mode == "marquee") {
        auto document = parse(args[1]);
        auto& marquee = as<HTML::HTMLMarqueeElement>(*first_element(*document, "marquee"));
        g_out += std::to_string(marquee.scroll_amount()) + " " + std::to_string(marquee.scroll_delay()) + "\n";
    } else if (mode == "script") {
        auto document = parse(args[1]);
        document->for_each_in_subtree_of_type<HTML::HTMLScriptElement>([&](HTML::HTMLScriptElement& script) {
            g_out += "already_started=" + std::to_string(script.m_already_started)
                + " type=" + std::to_string(to_underlying(script.m_script_type))
                + " parser_inserted=" + std::to_string(script.is_parser_inserted())
                + " force_async=" + std::to_string(script.m_force_async)
                + " async=" + std::to_string(script.async()) + "\n";
            return TraversalDecision::Continue;
        });
    } else if (mode == "query") {
        auto document = parse(args[2]);
        auto list = MUST(document->query_selector_all(StringView(args[1].data(), args[1].size())));
        for (size_t i = 0; i < list->length(); i++) {
            auto& element = as<DOM::Element>(*list->item(i));
            g_out += str(element.get_attribute_value(HTML::AttributeNames::id)) + "\n";
        }
    } else {
        g_out += "unknown mode\n";
    }
}

static std::string unescape(std::string const& escaped)
{
    std::string out;
    for (size_t i = 0; i < escaped.size(); i++) {
        if (escaped[i] == '\\' && i + 1 < escaped.size()) {
            char c = escaped[++i];
            out += c == 'n' ? '\n' : c == 't' ? '\t' : c;
        } else {
            out += escaped[i];
        }
    }
    return out;
}

int main(int argc, char** argv)
{
    Core::EventLoop event_loop;
    Web::Platform::EventLoopPlugin::install(*new Web::Platform::EventLoopPlugin);
    Web::Platform::FontPlugin::install(*new Web::Platform::FontPlugin(false));
    Web::Bindings::initialize_main_thread_vm(Web::Bindings::AgentType::SimilarOriginWindow);
    std::ifstream in(argc > 1 ? argv[1] : "cases.txt");
    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty() || raw[0] == '#')
            continue;
        bool later = raw[0] == '!';
        if (later)
            raw = raw.substr(1);
        std::vector<std::string> args;
        size_t start = 0;
        while (true) {
            auto tab = raw.find('\t', start);
            args.push_back(unescape(raw.substr(start, tab == std::string::npos ? std::string::npos : tab - start)));
            if (tab == std::string::npos)
                break;
            start = tab + 1;
        }
        g_out += std::string(later ? "later " : "case ") + raw + "\n";
        run(args);
    }
    std::cout << g_out;
}
