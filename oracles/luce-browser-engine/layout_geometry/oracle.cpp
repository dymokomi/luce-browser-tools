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
//   selection S SO E EO (r50) selection states of a range over text nodes and the fragments' range rects
//   paint      (r50) the paint tree's queries and hit tests of a grid of points; paintables: the queries only
//   layout   the three dumps of a Layout test: dump_tree of the layout tree and of the paint tree, StackingContext::dump
//   table      (r48) TableGrid::calculate_row_column_grid of every table box: rows, cells and the occupied slots
//   svg        (r49) every SVG paintable of the paint tree (unconnected mask/clip/pattern subtrees included), in
//              the layout tree's pre-order: class, absolute rect, computed transforms, computed path, clip path flags
//   border-specificity (r48) TableFormattingContext::border_is_less_specific over pairs of border styles and widths
#define private public
#define protected public
#include <LibGfx/Font/FontDatabase.h>
#include <LibGfx/Font/PathFontProvider.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/CSSStyleSheet.h>
#include <LibWeb/CSS/ComputedProperties.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/StyleScope.h>
#include <LibWeb/CSS/StyleSheetList.h>
#include <LibWeb/CSS/StyleComputer.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/Range.h>
#include <LibWeb/DOM/Text.h>
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
#include <LibWeb/Layout/TableGrid.h>
#include <LibWeb/Layout/TableFormattingContext.h>
#include <LibWeb/Layout/TreeBuilder.h>
#include <LibWeb/Layout/BlockFormattingContext.h>
#include <LibWeb/Layout/LayoutState.h>
#include <LibWeb/Layout/Viewport.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Painting/PaintableWithLines.h>
#include <LibWeb/Painting/StackingContext.h>
#include <LibWeb/Painting/TextPaintable.h>
#include <LibWeb/Painting/ViewportPaintable.h>
#include <LibWeb/Painting/SVGPaintable.h>
#include <LibWeb/Painting/SVGGraphicsPaintable.h>
#include <LibWeb/Painting/SVGPathPaintable.h>
#include <LibWeb/Painting/SVGSVGPaintable.h>
#include <LibWeb/Layout/SVGSVGBox.h>
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

// The SystemFontProvider of the oracle (r45): the test fonts of the engine's tests/libweb/fonts (a PathFontProvider
// that is not one itself, so FontPlugin's constructor does not add the system's font directories), with the
// FontPlugin in layout test mode, as Ladybird's test-web runs and as the Luce tests (test_fonts_setup) do.
static std::string g_fonts_directory = "/Users/sedov/Dev/luce_dev/luce-browser-engine/tests/libweb/fonts";
class OracleFontProvider final : public Gfx::SystemFontProvider {
public:
    Gfx::PathFontProvider path;
    virtual StringView name() const override { return "FontConfig"sv; } // TypefaceSkia then uses FreeType, as test-web (--force-fontconfig)
    virtual RefPtr<Gfx::Font> get_font(FlyString const& family, float point_size, unsigned weight, unsigned width, unsigned slope, Optional<Gfx::FontVariationSettings> const& variations = {}, Optional<Gfx::ShapeFeatures> const& features = {}) override
    {
        return path.get_font(family, point_size, weight, width, slope, variations, features);
    }
    virtual void for_each_typeface_with_family_name(FlyString const& family_name, Function<void(Gfx::Typeface const&)> callback) override
    {
        path.for_each_typeface_with_family_name(family_name, move(callback));
    }
};

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
static void dump_line_boxes(Layout::LayoutState&, Layout::Node&);
static bool g_dump_line_boxes = false;

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
    // r45's "lines" mode: the line boxes of the layout state, before commit moves their fragments to paintables.
    if (g_dump_line_boxes)
        dump_line_boxes(layout_state, layout_root);
    layout_state.commit(layout_root);

    // r50: the rest of Document::update_layout after the commit (the viewport rect broadcast, the scroll frames and the
    // paint and hit testing properties), as test-web's Document::update_layout runs before its dumps.
    document->inform_all_viewport_clients_about_the_current_viewport_rect();
    if (auto* viewport_paintable = document->unsafe_paintable()) {
        viewport_paintable->assign_scroll_frames();
        document->set_needs_accumulated_visual_contexts_update(true);
        document->update_paint_and_hit_testing_properties_if_needed();
    }
    return document;
}

static void put_lines(StringView text)
{
    for (auto line : text.split_view('\n', SplitBehavior::KeepEmpty))
        g_out += "  " + str(line) + "\n";
}

// The expectation of a Layout test (r50): test-web's LayoutTree | PaintTree | StackingContextTree, i.e. dump_tree of the
// layout tree, dump_tree of the paint tree and StackingContext::dump, separated by blank lines
// (ConnectionFromClient::request_internal_page_info).
static void run_layout(std::string const& html)
{
    auto document = build(html);
    StringBuilder builder;
    auto& layout_root = *document->unsafe_layout_node();
    dump_tree(builder, layout_root, false, false);
    builder.append("\n"sv);
    dump_tree(builder, *layout_root.first_paintable());
    builder.append("\n"sv);
    auto& viewport_paintable = static_cast<Painting::ViewportPaintable&>(*layout_root.first_paintable());
    viewport_paintable.build_stacking_context_tree_if_needed();
    if (auto* stacking_context = viewport_paintable.stacking_context())
        stacking_context->dump(builder);
    put_lines(builder.string_view().trim_whitespace(TrimMode::Right));
}

// r50's "paint" and "paintables" modes: the queries of the paint tree, in pre-order, that the dumps do not show
// (flags, containing blocks and stacking contexts, every absolute rect, radii, overflow and resize predicates, the
// fragments' rects, range rects and the code unit under points), and in "paint" mode hit tests of a grid of points.
static char const* yn(bool b) { return b ? "1" : "0"; }
static String desc(Painting::Paintable const* p) { return p ? p->debug_description() : "-"_string; }
static void put_radii(StringBuilder& b, char const* name, Painting::BorderRadiiData const& r)
{
    b.appendff("  {} {} {} {} {} {} {} {} {}\n", name, r.top_left.horizontal_radius, r.top_left.vertical_radius, r.top_right.horizontal_radius, r.top_right.vertical_radius,
        r.bottom_right.horizontal_radius, r.bottom_right.vertical_radius, r.bottom_left.horizontal_radius, r.bottom_left.vertical_radius);
}
static void put_hit(StringBuilder& b, Optional<Painting::HitTestResult> const& result)
{
    if (!result.has_value()) {
        b.append("-"sv);
        return;
    }
    b.appendff("{} {}", desc(result->paintable.ptr()), result->index_in_node);
    if (result->vertical_distance.has_value())
        b.appendff(" v {}", *result->vertical_distance);
    if (result->horizontal_distance.has_value())
        b.appendff(" h {}", *result->horizontal_distance);
}
static void run_paint(std::string const& html, bool hit_tests)
{
    auto document = build(html);
    auto& viewport = static_cast<Painting::ViewportPaintable&>(*document->unsafe_layout_node()->first_paintable());
    viewport.build_stacking_context_tree_if_needed();
    StringBuilder b;
    size_t index = 0;
    viewport.for_each_in_inclusive_subtree([&](Painting::Paintable& p) {
        b.appendff("#{} {} positioned {} fixed {} sticky {} abspos {} floating {} inline {} visible {} hittable {} sc {}\n", index++, desc(&p),
            yn(p.is_positioned()), yn(p.is_fixed_position()), yn(p.is_sticky_position()), yn(p.is_absolutely_positioned()), yn(p.is_floating()),
            yn(p.is_inline()), yn(p.is_visible()), yn(p.visible_for_hit_testing()), yn(p.has_stacking_context()));
        b.appendff("  containing_block {}\n", desc(p.containing_block()));
        if (p.is_paintable_box() || p.is_inline())
            b.appendff("  agnostic {}\n", p.box_type_agnostic_position());
        auto selection_style = p.selection_style();
        b.appendff("  selection {} {}\n", selection_style.background_color.value(), yn(selection_style.has_styling()));
        if (auto* box = as_if<Painting::PaintableBox>(p)) {
            if (!p.is_viewport_paintable())
                b.appendff("  enclosing_sc {}\n", desc(&p.enclosing_stacking_context()->paintable_box()));
            b.appendff("  rect {} padding {} border {}\n", box->absolute_rect(), box->absolute_padding_box_rect(), box->absolute_border_box_rect());
            b.appendff("  united border {} content {} padding {}\n", box->absolute_united_border_box_rect(), box->absolute_united_content_rect(), box->absolute_united_padding_box_rect());
            b.appendff("  clip_edge {} reference {}\n", box->overflow_clip_edge_rect(), box->transform_reference_box());
            put_radii(b, "radii", box->border_radii_data());
            put_radii(b, "shrunk", box->normalized_border_radii_data(Painting::PaintableBox::ShrinkRadiiForBorders::Yes));
            auto z = box->effective_z_index();
            if (z.has_value())
                b.appendff("  z {}", *z);
            else
                b.append("  z auto"sv);
            auto axes = box->physical_resize_axes();
            b.appendff(" scrollable {} wheel {} {} overflow_applies {} mirrored {} resizer {} axes {} {} transform {}\n", yn(box->has_scrollable_overflow()),
                yn(box->could_be_scrolled_by_wheel_event(Painting::PaintableBox::ScrollDirection::Horizontal)), yn(box->could_be_scrolled_by_wheel_event(Painting::PaintableBox::ScrollDirection::Vertical)),
                yn(box->overflow_property_applies()), yn(box->is_chrome_mirrored()), yn(box->has_resizer()), yn(axes.horizontal), yn(axes.vertical), yn(box->has_css_transform()));
            b.appendff("  outline_offset {} scrollable_ancestor {}", box->outline_offset(), desc(box->nearest_scrollable_ancestor()));
            if (auto clip = box->get_clip_rect(); clip.has_value())
                b.appendff(" clip {}", *clip);
            b.append("\n"sv);
        }
        if (auto* lines = as_if<Painting::PaintableWithLines>(p)) {
            for (size_t i = 0; i < lines->fragments().size(); ++i) {
                auto const& f = lines->fragments()[i];
                auto rect = f.absolute_rect();
                auto start = f.start_offset();
                auto length = f.length_in_code_units();
                b.appendff("  frag {} {} {} {} orientation {} trailing {}\n", i, desc(&f.paintable()), start, rect, to_underlying(f.orientation()), yn(f.has_trailing_whitespace()));
                b.appendff("    range {} full {} start {} end {} cursor {}\n",
                    f.range_rect(Painting::Paintable::SelectionState::StartAndEnd, start + 1, start + (length > 1 ? length - 1 : 0)),
                    f.range_rect(Painting::Paintable::SelectionState::Full, 0, 0),
                    f.range_rect(Painting::Paintable::SelectionState::Start, start + 1, 0),
                    f.range_rect(Painting::Paintable::SelectionState::End, 0, start + 2),
                    f.range_rect(Painting::Paintable::SelectionState::StartAndEnd, start + 1, start + 1));
                b.append("    index"sv);
                for (int k = -1; k <= 5; ++k) {
                    CSSPixelPoint point { rect.x() + rect.width() * k / 4, rect.y() + rect.height() / 2 };
                    b.appendff(" {}", f.index_in_node_for_point(point));
                }
                b.append("\n"sv);
            }
        }
        return TraversalDecision::Continue;
    });
    if (hit_tests) {
        for (int y = 2; y < 300; y += 14) {
            for (int x = 3; x < 420; x += 21) {
                b.appendff("hit {},{} exact ", x, y);
                put_hit(b, static_cast<Painting::PaintableBox&>(viewport).hit_test({ x, y }, Painting::HitTestType::Exact));
                b.append(" cursor "sv);
                put_hit(b, static_cast<Painting::PaintableBox&>(viewport).hit_test({ x, y }, Painting::HitTestType::TextCursor));
                b.append("\n"sv);
            }
        }
    }
    put_lines(b.string_view().trim_whitespace(TrimMode::Right));
}

// r50's "selection S SO E EO" mode: a range from offset SO of the document's S-th text node (pre-order) to offset EO
// of its E-th, ViewportPaintable::recompute_selection_states over it, then each paintable's selection state and
// each fragment's range rect for its paintable's state and the range's offsets (what selection_rect computes).
static void run_selection(std::string const& html, std::string const& mode)
{
    unsigned start_index = 0, start_offset = 0, end_index = 0, end_offset = 0;
    sscanf(mode.c_str(), "selection %u %u %u %u", &start_index, &start_offset, &end_index, &end_offset);
    auto document = build(html);
    auto& viewport = static_cast<Painting::ViewportPaintable&>(*document->unsafe_layout_node()->first_paintable());
    Vector<DOM::Text*> texts;
    document->for_each_in_inclusive_subtree_of_type<DOM::Text>([&](DOM::Text& text) {
        texts.append(&text);
        return TraversalDecision::Continue;
    });
    auto range = DOM::Range::create(*document);
    MUST(range->set_start(*texts[start_index], start_offset));
    MUST(range->set_end(*texts[end_index], end_offset));
    viewport.recompute_selection_states(*range);
    StringBuilder b;
    size_t index = 0;
    viewport.for_each_in_inclusive_subtree([&](Painting::Paintable& p) {
        b.appendff("#{} {} state {}\n", index++, desc(&p), to_underlying(p.selection_state()));
        if (auto* lines = as_if<Painting::PaintableWithLines>(p)) {
            for (size_t i = 0; i < lines->fragments().size(); ++i) {
                auto const& f = lines->fragments()[i];
                b.appendff("  frag {} {}\n", i, f.range_rect(f.paintable().selection_state(), range->start_offset(), range->end_offset()));
            }
        }
        return TraversalDecision::Continue;
    });
    put_lines(b.string_view().trim_whitespace(TrimMode::Right));
}

// r45: the line boxes of every block container the layout state has used values for, in pre-order, with their
// fragments and glyph runs (the internals Dump's layout tree does not show).
static void dump_line_boxes(Layout::LayoutState& layout_state, Layout::Node& root)
{
    StringBuilder builder;
    size_t index = 0;
    root.for_each_in_inclusive_subtree([&](Layout::Node& node) {
        auto node_index = index++;
        auto* block = as_if<Layout::BlockContainer>(node);
        if (!block)
            return TraversalDecision::Continue;
        auto const* used_values = layout_state.try_get(*block);
        if (!used_values || used_values->line_boxes.is_empty())
            return TraversalDecision::Continue;
        builder.appendff("{} #{}: {} line boxes\n", block->class_name(), node_index, used_values->line_boxes.size());
        for (size_t line_index = 0; line_index < used_values->line_boxes.size(); ++line_index) {
            auto const& line_box = used_values->line_boxes[line_index];
            builder.appendff("  line {}: inline {} block {} bottom {} baseline {} width {} height {} available {} break {} forced {} trailing {}\n",
                line_index, line_box.inline_length(), line_box.block_length(), line_box.bottom(), line_box.baseline(),
                line_box.width(), line_box.height(), line_box.original_available_width().to_string(), line_box.m_has_break,
                line_box.m_has_forced_break, line_box.get_trailing_whitespace_width());
            for (size_t fragment_index = 0; fragment_index < line_box.fragments().size(); ++fragment_index) {
                auto const& fragment = line_box.fragments()[fragment_index];
                size_t fragment_node_index = 0;
                size_t i = 0;
                root.for_each_in_inclusive_subtree([&](Layout::Node& candidate) {
                    if (&candidate == &fragment.layout_node())
                        fragment_node_index = i;
                    ++i;
                    return TraversalDecision::Continue;
                });
                builder.appendff("    frag {}: {} #{} start {} length {} offset {} size {} border_box_top {} baseline {} writing_mode {} direction {} trailing_ws {} truncated {} ends_ws {} justifiable {} atomic {}\n",
                    fragment_index, fragment.layout_node().class_name(), fragment_node_index, fragment.start(), fragment.length_in_code_units(),
                    fragment.offset(), fragment.size(), fragment.border_box_top(), fragment.baseline(), to_underlying(fragment.writing_mode()),
                    to_underlying(fragment.m_direction), fragment.has_trailing_whitespace(), fragment.is_fully_truncated(), fragment.ends_in_whitespace(),
                    fragment.is_justifiable_whitespace(), fragment.is_atomic_inline());
                if (auto glyph_run = fragment.glyph_run()) {
                    // The floats are printed as doubles (exact; they are multiples of 1/64) except the glyphs' y, printed as
                    // its bits: luce-browser-foundation's float formatting breaks a tie in the last digit the other way
                    // (144.203125f prints 144.20313 there and 144.20312 here; -10.2350006103515625 as a double ...563/...562).
                    builder.appendff("      glyphs {} type {} size {} width {}:", glyph_run->glyphs().size(), to_underlying(glyph_run->text_type()), static_cast<double>(glyph_run->font().pixel_size()), static_cast<double>(glyph_run->width()));
                    for (auto const& glyph : glyph_run->glyphs())
                        builder.appendff(" {},{} {} {} {}", static_cast<double>(glyph.position.x()), bit_cast<u32>(glyph.position.y()), static_cast<double>(glyph.glyph_width), glyph.glyph_id, glyph.length_in_code_units);
                    builder.append("\n"sv);
                }
            }
        }
        return TraversalDecision::Continue;
    });
    put_lines(builder.string_view().trim_whitespace(TrimMode::Right));
}

// r48's "table" mode: TableGrid::calculate_row_column_grid of every box with a table-inside display, in pre-order (nodes
// are numbered in the layout tree's inclusive pre-order): the column count, the rows with their collapsed flags, the
// cells with their slots and spans, and the occupied slots in the occupancy grid's (AK HashMap) iteration order.
static void run_table(std::string const& html)
{
    auto document = build(html);
    StringBuilder builder;
    auto& root = *document->unsafe_layout_node();
    Vector<Layout::Node const*> nodes;
    root.for_each_in_inclusive_subtree([&](Layout::Node& node) {
        nodes.append(&node);
        return TraversalDecision::Continue;
    });
    auto index_of = [&](Layout::Node const* node) {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i] == node)
                return i;
        }
        return nodes.size();
    };
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto const* box = as_if<Layout::Box>(*nodes[i]);
        if (!box || !box->display().is_table_inside())
            continue;
        Vector<Layout::TableGrid::Cell> cells;
        Vector<Layout::TableGrid::Row> rows;
        auto grid = Layout::TableGrid::calculate_row_column_grid(*box, cells, rows);
        builder.appendff("table #{}: {} columns, {} rows, {} cells\n", i, grid.column_count(), rows.size(), cells.size());
        for (auto const& row : rows)
            builder.appendff("  row #{} collapsed {}\n", index_of(row.box), row.is_collapsed);
        for (auto const& cell : cells)
            builder.appendff("  cell #{} at {},{} span {}x{}\n", index_of(cell.box), cell.column_index, cell.row_index, cell.column_span, cell.row_span);
        builder.append("  occupied:"sv);
        for (auto const& it : grid.occupancy_grid())
            builder.appendff(" {},{}", it.key.x, it.key.y);
        builder.append("\n"sv);
    }
    put_lines(builder.string_view().trim_whitespace(TrimMode::Right));
}

// r49's "svg" mode: for every layout node (inclusive pre-order, numbered) whose paintable is an SVG paintable or an
// SVGSVGPaintable: the paintable's class and absolute rect; for an SVGGraphicsPaintable its computed transforms
// (svg_to_viewbox_transform, svg_transform, svg_to_css_pixels_transform), the mask and clip areas; for an
// SVGPathPaintable the computed path (to_svg_string) and its bounding box; for an SVGPaintable whether it contributes to a
// clip path, its clip path bounds and whether it anti-aliases. Transforms print their six values with AK's formatter.
static void put_transform(StringBuilder& builder, StringView name, Gfx::AffineTransform const& t)
{
    builder.appendff(" {} [{} {} {} {} {} {}]", name, t.a(), t.b(), t.c(), t.d(), t.e(), t.f());
}

static void run_svg(std::string const& html)
{
    auto document = build(html);
    StringBuilder builder;
    auto& root = *document->unsafe_layout_node();
    size_t index = 0;
    root.for_each_in_inclusive_subtree([&](Layout::Node& node) {
        auto this_index = index++;
        auto* paintable = node.first_paintable();
        if (!paintable)
            return TraversalDecision::Continue;
        if (!is<Painting::SVGPaintable>(*paintable) && !is<Painting::SVGSVGPaintable>(*paintable))
            return TraversalDecision::Continue;
        auto& box = static_cast<Painting::PaintableBox&>(*paintable);
        builder.appendff("#{} {} rect {}\n", this_index, paintable->class_name(), box.absolute_rect());
        if (auto* graphics = as_if<Painting::SVGGraphicsPaintable>(*paintable)) {
            auto const& transforms = graphics->computed_transforms();
            builder.append("  transforms"sv);
            put_transform(builder, "viewbox"sv, transforms.svg_to_viewbox_transform());
            put_transform(builder, "svg"sv, transforms.svg_transform());
            put_transform(builder, "css"sv, transforms.svg_to_css_pixels_transform());
            builder.append("\n"sv);
            auto mask_area = graphics->get_mask_area();
            auto clip_area = graphics->get_clip_area();
            builder.appendff("  mask {} clip {}\n", mask_area.has_value() ? MUST(String::formatted("{}", *mask_area)) : "none"_string, clip_area.has_value() ? MUST(String::formatted("{}", *clip_area)) : "none"_string);
        }
        if (auto* path_paintable = as_if<Painting::SVGPathPaintable>(*paintable)) {
            if (path_paintable->computed_path().has_value()) {
                builder.appendff("  path {}\n", path_paintable->computed_path()->to_svg_string());
                builder.appendff("  bbox {}\n", path_paintable->computed_path()->bounding_box());
            } else {
                builder.append("  path none\n"sv);
            }
        }
        if (auto* svg_paintable = as_if<Painting::SVGPaintable>(*paintable)) {
            auto bounds = svg_paintable->clip_path_geometry_bounds({});
            builder.appendff("  contributes {} bounds {} anti-alias {}\n", svg_paintable->contributes_to_clip_path(), bounds.has_value() ? MUST(String::formatted("{}", *bounds)) : "none"_string, svg_paintable->should_anti_alias() == Painting::ShouldAntiAlias::Yes);
        }
        return TraversalDecision::Continue;
    });
    put_lines(builder.string_view().trim_whitespace(TrimMode::Right));
}

// r48's "border-specificity" mode (no document): TableFormattingContext::border_is_less_specific(a, b) for every pair of
// borders of the ten line styles and the widths 0, 1 and 3px (style-major), one line per a with a 0 or 1 per b.
static void run_border_specificity()
{
    Vector<CSS::BorderData> borders;
    for (auto style : { CSS::LineStyle::None, CSS::LineStyle::Hidden, CSS::LineStyle::Dotted, CSS::LineStyle::Dashed, CSS::LineStyle::Solid,
             CSS::LineStyle::Double, CSS::LineStyle::Groove, CSS::LineStyle::Ridge, CSS::LineStyle::Inset, CSS::LineStyle::Outset }) {
        for (int width : { 0, 1, 3 }) {
            CSS::BorderData border;
            border.line_style = style;
            border.width = width;
            borders.append(border);
        }
    }
    StringBuilder builder;
    for (auto const& a : borders) {
        for (auto const& b : borders)
            builder.append(Layout::TableFormattingContext::border_is_less_specific(a, b) ? '1' : '0');
        builder.append('\n');
    }
    put_lines(builder.string_view().trim_whitespace(TrimMode::Right));
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    g_css = args.size() > 2 ? args[2] : std::string();
    if (mode == "layout")
        run_layout(args[1]);
    else if (mode.starts_with("selection "))
        run_selection(args[1], mode);
    else if (mode == "paint")
        run_paint(args[1], true);
    else if (mode == "paintables")
        run_paint(args[1], false);
    else if (mode == "table")
        run_table(args[1]);
    else if (mode == "svg")
        run_svg(args[1]);
    else if (mode == "border-specificity")
        run_border_specificity();
    else if (mode == "lines") {
        g_dump_line_boxes = true;
        build(args[1]);
        g_dump_line_boxes = false;
    }
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
    auto& provider = static_cast<OracleFontProvider&>(Gfx::FontDatabase::the().install_system_font_provider(make<OracleFontProvider>()));
    provider.path.load_all_fonts_from_uri(MUST(String::formatted("file://{}", StringView(g_fonts_directory.c_str(), g_fonts_directory.size()))));
    Web::Platform::FontPlugin::install(*new Web::Platform::FontPlugin(true, &provider));
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
