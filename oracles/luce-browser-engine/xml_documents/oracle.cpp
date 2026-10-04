// Oracle for luce-browser-engine region p2x (XML documents: XMLDocumentBuilder over the XML parser,
// XMLFragmentParser, resolve_named_html_entity): runs the reference build's LibWeb (LibXML over
// libxml2) on the cases of cases.txt and prints what tests_xml_document_builder compares. Each case
// line is "<mode>\t<arg>..." with \n, \t and \\ escaped in the arguments.
//   load <xml>         load_xml_document's run_xml_parser: { preserve_cdata, preserve_comments,
//                      resolve_named_html_entity }, XMLScriptingSupport::Enabled; on a parse error
//                      the document is converted to the XML error document
//   build <xml>        build_xml_document (UTF-8 bytes, no Content-Type charset): its result and tree
//   svg <xml>          SVGDecodedImageData's parse: { resolve_named_html_entity }, scripting disabled
//   frag <xml> <markup>  XMLFragmentParser::parse_xml_fragment in the document element of <xml>
//                      (parsed as by load): the nodes, or the exception
//   entity <name>      resolve_named_html_entity: the code points in hex, or "none"
// The documents are DOM::Document::create'd in a window's realm (no browsing context) with type XML.
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/DOM/Attr.h>
#include <LibWeb/DOM/CDATASection.h>
#include <LibWeb/DOM/Comment.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/DocumentLoading.h>
#include <LibWeb/DOM/DocumentType.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/DOM/ProcessingInstruction.h>
#include <LibWeb/DOM/Text.h>
#include <LibWeb/HTML/HTMLTemplateElement.h>
#include <LibWeb/HTML/TagNames.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Namespace.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
#include <LibWeb/WebIDL/DOMException.h>
#include <LibWeb/XML/XMLDocumentBuilder.h>
#include <LibWeb/XML/XMLFragmentParser.h>
#include <LibXML/Parser/Parser.h>
#include <LibCore/EventLoop.h>
#undef private
#undef protected
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

static GC::Ref<DOM::Document> new_document()
{
    auto document = DOM::Document::create(window_document().realm());
    document->set_document_type(DOM::Document::Type::XML);
    return document;
}

// mark: The dump ================================================================================

static void indent(int level)
{
    g_out += "| ";
    for (int i = 0; i < level; i++)
        g_out += "  ";
}

static std::string namespace_prefix(Optional<FlyString> const& ns)
{
    if (!ns.has_value())
        return "";
    if (*ns == Namespace::HTML)
        return "html ";
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

static std::string qualified(Optional<FlyString> const& prefix, FlyString const& local_name)
{
    if (prefix.has_value())
        return str(*prefix) + ":" + str(local_name);
    return str(local_name);
}

static void dump_node(DOM::Node const& node, int level);
static void dump_children(DOM::Node const& node, int level)
{
    for (auto* child = node.first_child(); child; child = child->next_sibling())
        dump_node(*child, level);
}

// An element: its namespace, qualified name and class; its attributes in order (namespace,
// qualified name, value); a template's contents.
static void dump_node(DOM::Node const& node, int level)
{
    indent(level);
    if (is<DOM::Element>(node)) {
        auto& element = static_cast<DOM::Element const&>(node);
        g_out += "<" + namespace_prefix(element.namespace_uri()) + qualified(element.prefix(), element.local_name()) + "> " + str(element.class_name()) + "\n";
        element.for_each_attribute([&](DOM::Attr const& attr) {
            indent(level + 1);
            g_out += namespace_prefix(attr.namespace_uri()) + qualified(attr.prefix(), attr.local_name()) + "=\"" + str(attr.value()) + "\"\n";
        });
        if (is<HTML::HTMLTemplateElement>(element)) {
            indent(level + 1);
            g_out += "content\n";
            dump_children(*static_cast<HTML::HTMLTemplateElement const&>(element).content(), level + 2);
        }
        dump_children(node, level + 1);
        return;
    }
    if (is<DOM::CDATASection>(node)) {
        g_out += "<![CDATA[" + str(static_cast<DOM::CDATASection const&>(node).data()) + "]]>\n";
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
    if (is<DOM::ProcessingInstruction>(node)) {
        auto& pi = static_cast<DOM::ProcessingInstruction const&>(node);
        g_out += "<?" + str(pi.target()) + " " + str(pi.data()) + "?>\n";
        return;
    }
    if (is<DOM::DocumentType>(node)) {
        auto& doctype = static_cast<DOM::DocumentType const&>(node);
        g_out += "<!DOCTYPE " + str(doctype.name()) + " \"" + str(doctype.public_id()) + "\" \"" + str(doctype.system_id()) + "\">\n";
        return;
    }
    g_out += "?\n";
}

// mark: Modes ===================================================================================

static std::string escape_line(std::string const& text)
{
    std::string out;
    for (char c : text) {
        if (c == '\n')
            out += "\\n";
        else if (c == '\\')
            out += "\\\\";
        else
            out += c;
    }
    return out;
}

// load_xml_document's run_xml_parser (DocumentLoading.cpp), with its static
// convert_to_xml_error_document.
static void convert_to_xml_error_document(DOM::Document& document, Utf16String error_string)
{
    auto html_element = MUST(DOM::create_element(document, HTML::TagNames::html, Namespace::HTML));
    auto body_element = MUST(DOM::create_element(document, HTML::TagNames::body, Namespace::HTML));
    MUST(html_element->append_child(body_element));
    MUST(body_element->append_child(document.realm().create<DOM::Text>(document, move(error_string))));
    document.remove_all_children();
    MUST(document.append_child(html_element));
}

static GC::Ref<DOM::Document> load(std::string const& source, bool print)
{
    auto document = new_document();
    XML::Parser parser(StringView(source.data(), source.size()), { .preserve_cdata = true, .preserve_comments = true, .resolve_named_html_entity = resolve_named_html_entity });
    XMLDocumentBuilder builder { document };
    auto result = parser.parse_with_listener(builder);
    if (print)
        g_out += std::string("has_error ") + (builder.has_error() ? "true" : "false") + "\n";
    if (result.is_error()) {
        auto message = MUST(String::formatted("Failed to parse XML document: {}", result.error()));
        if (print)
            g_out += "error " + escape_line(str(message)) + "\n";
        convert_to_xml_error_document(document, Utf16String::from_utf8(message));
    }
    return document;
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    if (mode == "load") {
        auto document = load(args[1], true);
        dump_children(*document, 0);
    } else if (mode == "build") {
        auto document = new_document();
        auto bytes = MUST(ByteBuffer::copy(args[1].data(), args[1].size()));
        auto ok = build_xml_document(*document, bytes, {});
        g_out += std::string("result ") + (ok ? "true" : "false") + "\n";
        dump_children(*document, 0);
    } else if (mode == "svg") {
        auto document = new_document();
        XML::Parser parser(StringView(args[1].data(), args[1].size()), { .resolve_named_html_entity = resolve_named_html_entity });
        XMLDocumentBuilder builder { document, XMLScriptingSupport::Disabled };
        auto result = parser.parse_with_listener(builder);
        g_out += std::string("has_error ") + (builder.has_error() ? "true" : "false") + "\n";
        if (result.is_error())
            g_out += "error " + escape_line(str(MUST(String::formatted("{}", result.error())))) + "\n";
        dump_children(*document, 0);
    } else if (mode == "frag") {
        auto document = load(args[1], false);
        auto* context = document->document_element();
        VERIFY(context);
        auto result = XMLFragmentParser::parse_xml_fragment(*context, StringView(args[2].data(), args[2].size()));
        if (result.is_exception()) {
            auto exception = result.exception();
            if (auto* dom_exception = exception.get_pointer<GC::Ref<WebIDL::DOMException>>())
                g_out += "exception " + str((*dom_exception)->name()) + " " + escape_line(str((*dom_exception)->message().to_utf16_string())) + "\n";
            else
                g_out += "exception ?\n";
            return;
        }
        for (auto& node : result.value())
            dump_node(*node, 0);
    } else if (mode == "entity") {
        auto resolved = resolve_named_html_entity(StringView(args[1].data(), args[1].size()));
        if (!resolved.has_value()) {
            g_out += "none\n";
            return;
        }
        for (auto code_point : resolved->code_points()) {
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%x ", code_point);
            g_out += buffer;
        }
        g_out += "\n";
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
            out += c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c;
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
        bool deviates = raw[0] == '!';
        if (deviates)
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
        g_out += std::string(deviates ? "deviates " : "case ") + raw + "\n";
        run(args);
    }
    std::cout << g_out;
}
