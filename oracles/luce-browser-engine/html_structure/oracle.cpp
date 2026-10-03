// Oracle for luce-browser-engine region r26 (HTML structure: tables, lists, details, images and
// source sets, title/meta, CORS settings, the list of available images): runs the reference build's
// LibWeb on the cases of cases.txt and prints what tests_html_structure_cases compares. Each case line is "<mode>\t<arg>..." with \n, \t and \\ escaped in the arguments.
//   srcset <input>           parse_a_srcset_attribute: each source's URL and descriptor
//   select <html>            the first img's select_an_image_source (URL, density in millionths)
//   cors <keyword or ->       cors_setting_attribute_from_keyword and its credentials mode
//   keyhash <url> <mode> <origin URL or ->   ListOfAvailableImages::Key::hash
//   tablerows <html>         the first table's rows, sections, row/cell indices and spans
//   tableop <op> <html>      a table operation on the first table (or its first row), then its tree
//   ol <html>                the first ol's start, starting value and each li's value
//   details <html>           each details element's open attribute after parsing
//   title <html>             the document's title and the first title element's text
//   meta <html>              each meta's http-equiv state
//   referrer <html>          the document's referrer policy after its <meta name=referrer>
//   dialog <op> <html>       a dialog operation on the first dialog, then its state
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
#include <LibWeb/HTML/HTMLImageElement.h>
#include <LibWeb/HTML/HTMLTableElement.h>
#include <LibWeb/HTML/HTMLTableRowElement.h>
#include <LibWeb/HTML/HTMLTableSectionElement.h>
#include <LibWeb/HTML/HTMLTableCellElement.h>
#include <LibWeb/HTML/HTMLTableCaptionElement.h>
#include <LibWeb/HTML/HTMLTableColElement.h>
#include <LibWeb/HTML/HTMLOListElement.h>
#include <LibWeb/HTML/HTMLLIElement.h>
#include <LibWeb/HTML/HTMLDetailsElement.h>
#include <LibWeb/HTML/HTMLDialogElement.h>
#include <LibWeb/HTML/HTMLTitleElement.h>
#include <LibWeb/HTML/HTMLMetaElement.h>
#include <LibWeb/HTML/SourceSet.h>
#include <LibWeb/HTML/CORSSettingAttribute.h>
#include <LibWeb/HTML/ListOfAvailableImages.h>
#include <LibWeb/HTML/PolicyContainers.h>
#include <LibWeb/DOM/HTMLCollection.h>
#include <LibWeb/WebIDL/DOMException.h>
#include <LibURL/Parser.h>
#include <cmath>
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

static std::string num(i64 v) { return std::to_string(v); }
static std::string millionths(double d) { return num((i64)std::llround(d * 1000000.0)); }

static void put_source(HTML::ImageSource const& source)
{
    g_out += str(source.url) + "|";
    source.descriptor.visit(
        [&](Empty) { g_out += "-"; },
        [&](HTML::ImageSource::PixelDensityDescriptorValue const& v) { g_out += "x " + millionths(v.value); },
        [&](HTML::ImageSource::WidthDescriptorValue const& v) { g_out += "w " + num(v.value.raw_value()); });
    g_out += "\n";
}

static std::string id_of(DOM::Element const& element)
{
    return str(element.get_attribute_value(HTML::AttributeNames::id));
}

template<typename T>
static void put_exception(WebIDL::ExceptionOr<T> result)
{
    if (result.is_exception()) {
        auto exception = result.exception();
        if (auto* dom_exception = exception.template get_pointer<GC::Ref<WebIDL::DOMException>>())
            g_out += "exception " + str((*dom_exception)->name()) + "\n";
        else
            g_out += "exception\n";
    }
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    if (mode == "srcset") {
        auto set = HTML::parse_a_srcset_attribute(StringView(args[1].data(), args[1].size()));
        for (auto& source : set.m_sources)
            put_source(source);
    } else if (mode == "select") {
        auto document = parse(args[1]);
        auto& img = as<HTML::HTMLImageElement>(*first_element(*document, "img"));
        auto result = img.select_an_image_source();
        if (!result.has_value())
            g_out += "none\n";
        else
            g_out += str(result->source.url) + " " + millionths(result->pixel_density) + "\n";
        g_out += "dimension source " + str(img.dimension_attribute_source().local_name()) + "\n";
    } else if (mode == "cors") {
        Optional<String> keyword;
        if (args[1] != "-")
            keyword = MUST(String::from_utf8(StringView(args[1].data(), args[1].size())));
        auto state = HTML::cors_setting_attribute_from_keyword(keyword);
        g_out += num(to_underlying(state)) + " " + num(to_underlying(HTML::cors_settings_attribute_credentials_mode(state))) + "\n";
    } else if (mode == "keyhash") {
        HTML::ListOfAvailableImages::Key key;
        key.url = MUST(String::from_utf8(StringView(args[1].data(), args[1].size())));
        key.mode = static_cast<HTML::CORSSettingAttribute>(std::stoi(args[2]));
        if (args[3] != "-")
            key.origin = URL::Parser::basic_parse(StringView(args[3].data(), args[3].size()))->origin();
        g_out += num(key.hash()) + "\n";
    } else if (mode == "tablerows") {
        auto document = parse(args[1]);
        auto& table = as<HTML::HTMLTableElement>(*first_element(*document, "table"));
        g_out += "caption " + std::string(table.caption() ? "1" : "0") + " thead " + (table.t_head() ? "1" : "0") + " tfoot " + (table.t_foot() ? "1" : "0") + " tbodies " + num(table.t_bodies()->length()) + "\n";
        auto rows = table.rows();
        g_out += "rows";
        for (size_t i = 0; i < rows->length(); i++)
            g_out += " " + id_of(*rows->item(i));
        g_out += "\n";
        document->for_each_in_subtree_of_type<HTML::HTMLTableRowElement>([&](HTML::HTMLTableRowElement& row) {
            g_out += "tr " + id_of(row) + " " + num(row.row_index()) + " " + num(row.section_row_index()) + " cells " + num(row.cells()->length()) + "\n";
            return TraversalDecision::Continue;
        });
        document->for_each_in_subtree_of_type<HTML::HTMLTableCellElement>([&](HTML::HTMLTableCellElement& cell) {
            g_out += "cell " + id_of(cell) + " " + num(cell.cell_index()) + " " + num(cell.col_span()) + " " + num(cell.row_span()) + "\n";
            return TraversalDecision::Continue;
        });
        document->for_each_in_subtree_of_type<HTML::HTMLTableColElement>([&](HTML::HTMLTableColElement& col) {
            g_out += "col " + num(col.span()) + "\n";
            return TraversalDecision::Continue;
        });
        document->for_each_in_subtree_of_type<HTML::HTMLTableSectionElement>([&](HTML::HTMLTableSectionElement& section) {
            g_out += "section " + str(section.local_name()) + " rows " + num(section.rows()->length()) + "\n";
            return TraversalDecision::Continue;
        });
    } else if (mode == "tableop") {
        auto document = parse(args[2]);
        auto& table = as<HTML::HTMLTableElement>(*first_element(*document, "table"));
        auto const& op = args[1];
        auto colon = op.find(':');
        auto name = op.substr(0, colon);
        int index = colon == std::string::npos ? 0 : std::stoi(op.substr(colon + 1));
        if (name == "insertRow")
            put_exception(table.insert_row(index));
        else if (name == "deleteRow")
            put_exception(table.delete_row(index));
        else if (name == "createCaption")
            (void)table.create_caption();
        else if (name == "deleteCaption")
            table.delete_caption();
        else if (name == "createTHead")
            (void)table.create_t_head();
        else if (name == "deleteTHead")
            table.delete_t_head();
        else if (name == "createTFoot")
            (void)table.create_t_foot();
        else if (name == "deleteTFoot")
            table.delete_t_foot();
        else if (name == "createTBody")
            (void)table.create_t_body();
        else if (name == "insertCell")
            put_exception(as<HTML::HTMLTableRowElement>(*first_element(*document, "tr")).insert_cell(index));
        else if (name == "deleteCell")
            put_exception(as<HTML::HTMLTableRowElement>(*first_element(*document, "tr")).delete_cell(index));
        else if (name == "sectionInsertRow")
            put_exception(as<HTML::HTMLTableSectionElement>(*first_element(*document, "tbody")).insert_row(index));
        else if (name == "sectionDeleteRow")
            put_exception(as<HTML::HTMLTableSectionElement>(*first_element(*document, "tbody")).delete_row(index));
        else if (name == "setTHeadTFoot")
            put_exception(table.set_t_head(as<HTML::HTMLTableSectionElement>(first_element(*document, "tfoot"))));
        else if (name == "setCaptionNull")
            put_exception(table.set_caption(nullptr));
        else if (name == "setColSpan") {
            auto& cell = as<HTML::HTMLTableCellElement>(*first_element(*document, "td"));
            cell.set_col_span(index);
            g_out += "colspan " + num(cell.col_span()) + "\n";
        } else
            g_out += "unknown op\n";
        dump_node(table, 0);
    } else if (mode == "ol") {
        auto document = parse(args[1]);
        auto& ol = as<HTML::HTMLOListElement>(*first_element(*document, "ol"));
        g_out += "start " + num(ol.start()) + " starting " + num(ol.starting_value().value()) + "\n";
        document->for_each_in_subtree_of_type<HTML::HTMLLIElement>([&](HTML::HTMLLIElement& li) {
            g_out += "li " + num(li.value()) + "\n";
            return TraversalDecision::Continue;
        });
    } else if (mode == "details") {
        auto document = parse(args[1]);
        document->for_each_in_subtree_of_type<HTML::HTMLDetailsElement>([&](HTML::HTMLDetailsElement& details) {
            g_out += id_of(details) + " " + (details.has_attribute(HTML::AttributeNames::open) ? "open" : "closed") + "\n";
            return TraversalDecision::Continue;
        });
    } else if (mode == "title") {
        auto document = parse(args[1]);
        g_out += "title [" + str(document->title()) + "]\n";
        g_out += "text [" + str(as<HTML::HTMLTitleElement>(*first_element(*document, "title")).text()) + "]\n";
    } else if (mode == "meta") {
        auto document = parse(args[1]);
        document->for_each_in_subtree_of_type<HTML::HTMLMetaElement>([&](HTML::HTMLMetaElement& meta) {
            auto state = meta.http_equiv_state();
            g_out += "http-equiv " + (state.has_value() ? num(to_underlying(*state)) : std::string("none")) + "\n";
            return TraversalDecision::Continue;
        });
    } else if (mode == "referrer") {
        auto document = parse(args[1]);
        g_out += "referrer policy " + num(to_underlying(document->policy_container()->referrer_policy)) + "\n";
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
