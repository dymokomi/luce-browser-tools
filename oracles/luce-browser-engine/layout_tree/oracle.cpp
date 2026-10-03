// Oracle for luce-browser-engine region r43 (the layout tree): runs the reference build's LibWeb on
// the documents of cases.txt and prints what the Luce tests (layout/tests_layout_tree*) compare.
// Each case line is "<mode>\t<html>[\t<css>]" with \n, \t and \\ escaped. A document is parsed with
// scripting disabled into a new HTML document, which then gets the top-level traversable as its
// navigable (800x600) and the traversable's browsing context, so Document::update_style computes
// its style; Layout::TreeBuilder then builds the layout tree, before any layout (no paintables).
//
// Modes:
//   tree     dump_tree of the layout tree (the first part of a Layout test's expectation, without
//            geometry: every box is "(not painted)")
//   nodes    per layout node: its debug description and the Node / NodeWithStyle predicates, after
//            recompute_containing_block, as Document::update_layout runs it
//   chunks   per text node: its text for rendering and the ChunkIterator chunks, wrapping and not
//   markers  per list item marker box: its marker string
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/CSSStyleSheet.h>
#include <LibWeb/CSS/ComputedProperties.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/StyleScope.h>
#include <LibWeb/CSS/StyleSheetList.h>
#include <LibWeb/CSS/StyleComputer.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/Dump.h>
#include <LibWeb/HTML/BrowsingContext.h>
#include <LibWeb/HTML/HTMLDocument.h>
#include <LibWeb/HTML/Parser/HTMLParser.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Layout/BlockContainer.h>
#include <LibWeb/Layout/Box.h>
#include <LibWeb/Layout/InlineNode.h>
#include <LibWeb/Layout/ListItemMarkerBox.h>
#include <LibWeb/Layout/Node.h>
#include <LibWeb/Layout/TextNode.h>
#include <LibWeb/Layout/TreeBuilder.h>
#include <LibWeb/Layout/Viewport.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
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

// A PageClient for a Page whose documents the oracle makes (no IPC, no input, no display). Its preferred color
// scheme is light: what `auto` resolves to with the light palette of Ladybird's test runner.
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
    virtual CSS::PreferredColorScheme preferred_color_scheme() const override { return CSS::PreferredColorScheme::Light; }
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
        page->top_level_traversable()->set_viewport_size({ 800, 600 });
        g_page = GC::make_root(page);
        g_window_document = GC::make_root(*page->top_level_traversable()->active_document());
    }
    return *g_window_document;
}

static std::string g_css;

// The document of `html` (with g_css as its author style sheet), its style computed and its layout tree built.
static GC::Ref<DOM::Document> build(std::string const& html)
{
    auto document = HTML::HTMLDocument::create(window_document().realm());
    document->set_document_type(DOM::Document::Type::HTML);
    auto parser = HTML::HTMLParser::create(*document, StringView(html.data(), html.size()), HTML::ParserScriptingMode::Disabled, "UTF-8"sv);
    parser->run(document->url());
    if (!g_css.empty()) {
        auto sheet = Web::parse_css_stylesheet(CSS::Parser::ParsingParams { *document }, StringView(g_css.data(), g_css.size()));
        document->style_sheets().m_sheets.append(sheet);
        sheet->m_owning_documents_or_shadow_roots.set(document);
        document->style_scope().invalidate_rule_cache();
    }
    document->set_navigable(g_page->top_level_traversable());
    document->m_browsing_context = window_document().browsing_context();
    document->update_style();
    Layout::TreeBuilder tree_builder;
    auto root = tree_builder.build(*document);
    document->m_layout_root = as<Layout::Viewport>(*root);
    for (auto* node = root.ptr(); node; node = node->next_in_pre_order())
        node->recompute_containing_block({});
    return document;
}

static void put_lines(StringView text)
{
    for (auto line : text.split_view('\n', SplitBehavior::KeepEmpty))
        g_out += "  " + str(line) + "\n";
}

static void run_tree(std::string const& html)
{
    auto document = build(html);
    StringBuilder builder;
    dump_tree(builder, *document->unsafe_layout_node(), false, false);
    put_lines(builder.string_view().trim_whitespace(TrimMode::Right));
}

static char const* flag(bool value) { return value ? "1" : "0"; }

static void run_nodes(std::string const& html)
{
    auto document = build(html);
    for (Layout::Node* node = document->unsafe_layout_node(); node; node = node->next_in_pre_order()) {
        std::string line = str(node->debug_description());
        line += std::string(" display=") + str(node->display().to_string());
        line += std::string(" inline=") + flag(node->is_inline());
        line += std::string(" inline_block=") + flag(node->is_inline_block());
        line += std::string(" atomic=") + flag(node->is_atomic_inline());
        line += std::string(" floating=") + flag(node->is_floating());
        line += std::string(" positioned=") + flag(node->is_positioned());
        line += std::string(" abspos=") + flag(node->is_absolutely_positioned());
        line += std::string(" out_of_flow=") + flag(node->is_out_of_flow());
        line += std::string(" root=") + flag(node->is_root_element());
        line += std::string(" generated=") + flag(node->is_generated_for_pseudo_element());
        line += std::string(" anonymous=") + flag(node->is_anonymous());
        if (node->has_style_or_parent_with_style()) {
            line += std::string(" stacking=") + flag(node->establishes_stacking_context());
            line += std::string(" abs_cb=") + flag(node->establishes_an_absolute_positioning_containing_block());
            line += std::string(" fixed_cb=") + flag(node->establishes_a_fixed_positioning_containing_block());
            line += std::string(" size_contain=") + flag(node->has_size_containment());
            line += std::string(" layout_contain=") + flag(node->has_layout_containment());
            line += std::string(" style_contain=") + flag(node->has_style_containment());
            line += std::string(" paint_contain=") + flag(node->has_paint_containment());
            line += std::string(" user_select=") + std::to_string(to_underlying(node->user_select_used_value()));
            line += std::string(" transform=") + flag(node->has_css_transform());
        }
        if (auto* with_style = as_if<Layout::NodeWithStyle>(*node))
            line += std::string(" scroll=") + flag(with_style->is_scroll_container());
        if (auto* metrics = as_if<Layout::NodeWithStyleAndBoxModelMetrics>(*node)) {
            line += std::string(" continuation=") + (metrics->continuation_of_node() ? str(metrics->continuation_of_node()->debug_description()) : std::string("-"));
            line += std::string(" inline_continuation=") + flag(metrics->should_create_inline_continuation());
        }
        line += std::string(" cb=") + (node->containing_block() ? str(node->containing_block()->debug_description()) : std::string("-"));
        line += std::string(" inline_cb=") + (node->inline_containing_block_if_applicable() ? str(node->inline_containing_block_if_applicable()->debug_description()) : std::string("-"));
        if (auto* box = as_if<Layout::Box>(*node)) {
            auto size = box->auto_content_box_size();
            auto px = [](Optional<CSSPixels> value) { return value.has_value() ? std::to_string(value->raw_value()) : std::string("-"); };
            line += std::string(" auto_size=") + px(size.width) + "x" + px(size.height);
            line += std::string(" has_auto_size=") + flag(box->has_auto_content_box_size());
            auto ratio = box->preferred_aspect_ratio();
            line += std::string(" ratio=") + (ratio.has_value() ? std::to_string(ratio->numerator().raw_value()) + "/" + std::to_string(ratio->denominator().raw_value()) : std::string("-"));
        }
        g_out += "  " + line + "\n";
    }
}

static std::string escape(Utf16View const& view)
{
    std::string out;
    auto utf8 = MUST(view.to_utf8());
    for (auto c : str(utf8)) {
        if (c == '\n')
            out += "\\n";
        else if (c == '\t')
            out += "\\t";
        else if (c == '\\')
            out += "\\\\";
        else
            out += c;
    }
    return out;
}

static void run_chunks(std::string const& html)
{
    auto document = build(html);
    for (Layout::Node* node = document->unsafe_layout_node(); node; node = node->next_in_pre_order()) {
        auto* text_node = as_if<Layout::TextNode>(*node);
        if (!text_node)
            continue;
        g_out += "  text \"" + escape(text_node->text_for_rendering()) + "\"\n";
        for (int mode = 0; mode < 3; mode++) {
            bool wrap = mode != 0;
            bool respect = mode == 2;
            Layout::TextNode::ChunkIterator iterator { *text_node, wrap, respect };
            g_out += std::string("    wrap=") + flag(wrap) + " linebreaks=" + flag(respect) + " collapse=" + flag(iterator.should_collapse_whitespace()) + "\n";
            for (auto chunk = iterator.next(); chunk.has_value(); chunk = iterator.next()) {
                g_out += "      " + std::to_string(chunk->start) + "+" + std::to_string(chunk->length) + " \"" + escape(chunk->view) + "\" newline=" + flag(chunk->has_breaking_newline) + " tab=" + flag(chunk->has_breaking_tab) + " ws=" + flag(chunk->is_all_whitespace) + " break=" + flag(chunk->can_break_after) + " type=" + std::to_string(to_underlying(chunk->text_type)) + "\n";
            }
        }
    }
}

static void run_markers(std::string const& html)
{
    auto document = build(html);
    for (Layout::Node* node = document->unsafe_layout_node(); node; node = node->next_in_pre_order()) {
        auto* marker = as_if<Layout::ListItemMarkerBox>(*node);
        if (!marker)
            continue;
        auto text = marker->text();
        g_out += std::string("  marker ") + (text.has_value() ? "\"" + str(*text) + "\"" : std::string("-")) + " position=" + std::to_string(to_underlying(marker->list_style_position())) + "\n";
    }
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    g_css = args.size() > 2 ? args[2] : std::string();
    if (mode == "tree")
        run_tree(args[1]);
    else if (mode == "nodes")
        run_nodes(args[1]);
    else if (mode == "chunks")
        run_chunks(args[1]);
    else if (mode == "markers")
        run_markers(args[1]);
    else
        g_out += "  unknown mode\n";
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
        // A case marked "!" needs other regions' functions in the port (gen_luce_cases.py leaves it out).
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
