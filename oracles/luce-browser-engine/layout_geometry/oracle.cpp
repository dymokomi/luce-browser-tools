// Oracle for luce-browser-engine region r44 (block layout geometry): runs the reference build's LibWeb on the
// documents of cases.txt and prints the layout half of a Layout test's expectation (dump_tree of the laid-out
// layout tree), which the Luce tests (layout/tests_layout_geometry*) compare with the port's.
// Each case line is "<mode>\t<html>[\t<css>]" with \n, \t and \\ escaped. A document is parsed with scripting
// disabled into a new HTML document, gets its author style sheet, the top-level traversable as its navigable
// (800x600) and the traversable's browsing context, so Document::update_style computes its style;
// Layout::TreeBuilder builds the layout tree, and the layout steps of Document::update_layout run on it (layout
// indices, containing blocks, contained abspos children, a LayoutState, the root BlockFormattingContext, commit).
//
// Modes:
//   layout   dump_tree of the laid-out layout tree
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
#include <LibWeb/HTML/HTMLBodyElement.h>
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
#include <LibWeb/Layout/BlockFormattingContext.h>
#include <LibWeb/Layout/LayoutState.h>
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

// Document.cpp's propagate_scrollbar_width_to_viewport and propagate_overflow_to_viewport (file-static there).
static void propagate_scrollbar_width_to_viewport(DOM::Element& root_element, Layout::Viewport& viewport)
{
    // https://drafts.csswg.org/css-scrollbars/#scrollbar-width
    // UAs must apply the scrollbar-color value set on the root element to the viewport.
    auto& viewport_computed_values = viewport.mutable_computed_values();
    // NB: Called during layout tree construction.
    auto& root_element_computed_values = root_element.unsafe_layout_node()->computed_values();
    viewport_computed_values.set_scrollbar_width(root_element_computed_values.scrollbar_width());
}

// https://drafts.csswg.org/css-overflow-3/#overflow-propagation
static void propagate_overflow_to_viewport(DOM::Element& root_element, Layout::Viewport& viewport)
{
    // https://drafts.csswg.org/css-contain-2/#contain-property
    // Additionally, when any containments are active on either the HTML <html> or <body> elements, propagation of
    // properties from the <body> element to the initial containing block, the viewport, or the canvas background, is
    // disabled. Notably, this affects:
    // - 'overflow' and its longhands (see CSS Overflow 3 § 3.3 Overflow Viewport Propagation)
    if (root_element.is_html_html_element() && !root_element.computed_properties()->contain().is_empty())
        return;

    auto* body_element = root_element.first_child_of_type<HTML::HTMLBodyElement>();
    if (body_element && !body_element->computed_properties()->contain().is_empty())
        return;

    // UAs must apply the overflow-* values set on the root element to the viewport
    // when the root element’s display value is not none.
    // NB: Called during layout tree construction.
    auto overflow_origin_node = root_element.unsafe_layout_node();
    auto& viewport_computed_values = viewport.mutable_computed_values();

    // However, when the root element is an [HTML] html element (including XML syntax for HTML)
    // whose overflow value is visible (in both axes), and that element has as a child
    // a body element whose display value is also not none,
    // user agents must instead apply the overflow-* values of the first such child element to the viewport.
    if (root_element.is_html_html_element()) {
        auto root_element_layout_node = root_element.unsafe_layout_node();
        auto& root_element_computed_values = root_element_layout_node->mutable_computed_values();
        if (root_element_computed_values.overflow_x() == CSS::Overflow::Visible && root_element_computed_values.overflow_y() == CSS::Overflow::Visible) {
            auto* body_element = root_element.first_child_of_type<HTML::HTMLBodyElement>();
            if (body_element && body_element->unsafe_layout_node())
                overflow_origin_node = body_element->unsafe_layout_node();
        }
    }

    // If 'visible' is applied to the viewport, it must be interpreted as 'auto'. If 'clip' is applied to the viewport, it must be interpreted as 'hidden'.
    auto& overflow_origin_computed_values = overflow_origin_node->mutable_computed_values();
    auto overflow_x_to_apply = overflow_origin_computed_values.overflow_x();
    if (overflow_x_to_apply == CSS::Overflow::Visible) {
        overflow_x_to_apply = CSS::Overflow::Auto;
    } else if (overflow_x_to_apply == CSS::Overflow::Clip) {
        overflow_x_to_apply = CSS::Overflow::Hidden;
    }
    auto overflow_y_to_apply = overflow_origin_computed_values.overflow_y();
    if (overflow_y_to_apply == CSS::Overflow::Visible) {
        overflow_y_to_apply = CSS::Overflow::Auto;
    } else if (overflow_y_to_apply == CSS::Overflow::Clip) {
        overflow_y_to_apply = CSS::Overflow::Hidden;
    }
    viewport_computed_values.set_overflow_x(overflow_x_to_apply);
    viewport_computed_values.set_overflow_y(overflow_y_to_apply);

    // The element from which the value is propagated must then have a used overflow value of visible.
    // FIXME: Apply this to the used values, not the computed ones.
    overflow_origin_computed_values.set_overflow_x(CSS::Overflow::Visible);
    overflow_origin_computed_values.set_overflow_y(CSS::Overflow::Visible);
}

// The document of `html` (with g_css as its author style sheet), its style computed, its layout tree built and
// laid out by the layout steps of Document::update_layout.
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
    auto& layout_root = *document->m_layout_root;
    if (auto* root_element = document->document_element(); root_element && root_element->unsafe_layout_node()) {
        propagate_overflow_to_viewport(*root_element, layout_root);
        propagate_scrollbar_width_to_viewport(*root_element, layout_root);
    }

    // Document::update_layout, from the layout indices to the commit.
    u32 layout_index_counter = 0;
    layout_root.for_each_in_inclusive_subtree([&](auto& layout_node) {
        if (auto* node_with_style = as_if<Layout::NodeWithStyle>(layout_node))
            node_with_style->set_layout_index(layout_index_counter++);
        layout_node.recompute_containing_block({});
        auto* box = as_if<Layout::Box>(layout_node);
        if (!box)
            return TraversalDecision::Continue;
        box->clear_contained_abspos_children();
        if (!box->is_absolutely_positioned())
            return TraversalDecision::Continue;
        if (auto containing_block = box->containing_block()) {
            auto closest = containing_block;
            while (closest) {
                if (closest == &layout_root)
                    break;
                if (Layout::FormattingContext::formatting_context_type_created_by_box(*closest).has_value())
                    break;
                closest = closest->containing_block();
            }
            VERIFY(closest);
            closest->add_contained_abspos_child(*box);
        }
        return TraversalDecision::Continue;
    });

    CSSPixelRect viewport_rect { 0, 0, 800, 600 };
    auto document_element = document->document_element();
    Layout::LayoutState layout_state;
    layout_state.ensure_capacity(layout_index_counter);
    {
        auto& viewport_state = layout_state.get_mutable(layout_root);
        viewport_state.set_content_width(viewport_rect.width());
        viewport_state.set_content_height(viewport_rect.height());
        if (document_element && document_element->unsafe_layout_node()) {
            auto& icb_state = layout_state.get_mutable(as<Layout::NodeWithStyleAndBoxModelMetrics>(*document_element->unsafe_layout_node()));
            icb_state.set_content_width(viewport_rect.width());
        }
        auto available_space = Layout::AvailableSpace(
            Layout::AvailableSize::make_definite(viewport_rect.width()),
            Layout::AvailableSize::make_definite(viewport_rect.height()));
        Layout::BlockFormattingContext root_formatting_context(layout_state, Layout::LayoutMode::Normal, layout_root, nullptr);
        root_formatting_context.run(available_space);
    }
    layout_state.commit(layout_root);
    return document;
}

static void put_lines(StringView text)
{
    for (auto line : text.split_view('\n', SplitBehavior::KeepEmpty))
        g_out += "  " + str(line) + "\n";
}

static void run_layout(std::string const& html)
{
    auto document = build(html);
    StringBuilder builder;
    dump_tree(builder, *document->unsafe_layout_node(), false, false);
    put_lines(builder.string_view().trim_whitespace(TrimMode::Right));
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    g_css = args.size() > 2 ? args[2] : std::string();
    if (mode == "layout")
        run_layout(args[1]);
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
