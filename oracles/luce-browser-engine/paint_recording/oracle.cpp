// Oracle for luce-browser-engine regions r51b + r52 (painting: DisplayListRecordingContext, DevicePixelConverter,
// ChromeMetrics, BorderRadiusCornerClipper, the border/background/shadow/gradient painters): runs the reference build's
// LibWeb on the documents of cases.txt and prints, per case,
//   dl                 Document::dump_display_list() of the laid-out document (the recorded commands);
//   px X Y W H         the pixels DisplayListPlayerSkia renders for the display list a screenshot records
//                      (PaintConfig { paint_overlay, canvas_fill_rect = 800x600 }), as Ladybird's screenshots do:
//                      the crop X,Y WxH as run-length encoded 0xAARRGGBB words ("COUNTxWORD" tokens, rows top to
//                      bottom), then how many pixels outside the crop are not opaque white;
//   values             DevicePixelConverter, DisplayListRecordingContext and ChromeMetrics conversions.
// Each case line is "<mode>\t<html>[\t<css>]" with \n, \t and \\ escaped. The document is built as the layout_geometry
// oracle builds it (scripting disabled, the top-level traversable's 800x600 viewport, the test fonts in layout test mode).
#include "preamble.inc"


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
        auto available_space = Layout::AvailableSpace(Layout::AvailableSize::make_definite(viewport_rect.width()), Layout::AvailableSize::make_definite(viewport_rect.height()));
        Layout::BlockFormattingContext root_formatting_context(layout_state, Layout::LayoutMode::Normal, layout_root, nullptr);
        root_formatting_context.run(available_space);
    }
    layout_state.commit(layout_root);
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

static void run_display_list(std::string const& html)
{
    auto document = build(html);
    auto dump = document->dump_display_list();
    put_lines(dump.bytes_as_string_view().trim_whitespace(TrimMode::Right));
}

static void run_pixels(std::string const& html, std::string const& mode)
{
    int cx = 0, cy = 0, cw = 0, ch = 0;
    sscanf(mode.c_str(), "px %d %d %d %d", &cx, &cy, &cw, &ch);
    auto document = build(html);
    // TraversableNavigable::process_screenshot_requests -> take_screenshot -> Navigable::render_screenshot
    // (record_display_list_and_scroll_state) -> RenderingThread's screenshot command (the Skia player).
    HTML::PaintConfig config { .paint_overlay = true, .canvas_fill_rect = Gfx::IntRect { 0, 0, 800, 600 } };
    auto display_list = document->record_display_list(config);
    auto& viewport_paintable = *document->paintable();
    viewport_paintable.refresh_scroll_state();
    auto bitmap = MUST(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, { 800, 600 }));
    auto surface = Gfx::PaintingSurface::wrap_bitmap(*bitmap);
    Painting::DisplayListPlayerSkia player;
    player.execute(*display_list, viewport_paintable.scroll_state_snapshot(), surface);
    StringBuilder b;
    u32 run_word = 0;
    size_t run_count = 0;
    size_t tokens = 0;
    auto flush = [&] {
        if (run_count == 0)
            return;
        b.appendff("{}x{:08x}", run_count, run_word);
        b.append((++tokens % 12 == 0) ? "\n"sv : " "sv);
    };
    for (int y = cy; y < cy + ch; ++y) {
        for (int x = cx; x < cx + cw; ++x) {
            u32 word = bitmap->scanline(y)[x];
            if (run_count && word == run_word) {
                ++run_count;
                continue;
            }
            flush();
            run_word = word;
            run_count = 1;
        }
    }
    flush();
    size_t outside = 0;
    for (int y = 0; y < 600; ++y)
        for (int x = 0; x < 800; ++x)
            if ((x < cx || x >= cx + cw || y < cy || y >= cy + ch) && bitmap->scanline(y)[x] != 0xffffffff)
                ++outside;
    int min_x = 800, min_y = 600, max_x = -1, max_y = -1;
    for (int y = 0; y < 600; ++y)
        for (int x = 0; x < 800; ++x)
            if (bitmap->scanline(y)[x] != 0xffffffff) {
                min_x = min(min_x, x);
                min_y = min(min_y, y);
                max_x = max(max_x, x);
                max_y = max(max_y, y);
            }
    b.appendff("\noutside {} painted {},{} {}x{}", outside, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    put_lines(b.string_view().trim_whitespace(TrimMode::Right));
}

static void run_values()
{
    StringBuilder b;
    double const scales[] = { 1, 1.5, 2, 0.75, 1.25 };
    CSSPixels const values[] = { CSSPixels(0), CSSPixels(1), CSSPixels::from_raw(33), CSSPixels::from_raw(-33), CSSPixels::from_raw(96), CSSPixels::from_raw(-96), CSSPixels::from_raw(1023), CSSPixels(-7), CSSPixels::nearest_value_for(10.3) };
    for (auto scale : scales) {
        Painting::DevicePixelConverter converter(scale);
        b.appendff("scale {}\n", scale);
        for (auto v : values)
            b.appendff("  px {} round {} enclose {} floor {}\n", v.raw_value(), converter.rounded_device_pixels(v).value(), converter.enclosing_device_pixels(v).value(), converter.floored_device_pixels(v).value());
        CSSPixelRect const rects[] = { { CSSPixels::from_raw(33), CSSPixels::from_raw(65), CSSPixels::from_raw(640), CSSPixels::from_raw(97) }, { CSSPixels(-3), CSSPixels::from_raw(-161), CSSPixels::from_raw(1000), CSSPixels(12) }, { 8, 8, 784, 18 } };
        for (auto r : rects) {
            b.appendff("  rect {} rounded {} enclosing {} point {} floored_point {} size {} enclosing_size {}\n", r, converter.rounded_device_rect(r), converter.enclosing_device_rect(r),
                converter.rounded_device_point(r.location()), converter.floored_device_point(r.location()), converter.rounded_device_size(r.size()), converter.enclosing_device_size(r.size()));
        }
        Painting::DisplayListRecorder* no_recorder = nullptr;
        auto display_list = Painting::DisplayList::create(Painting::AccumulatedVisualContextTree::create());
        Painting::DisplayListRecorder recorder(display_list);
        (void)no_recorder;
        DisplayListRecordingContext context(recorder, window_document().page().palette(), scale, ChromeMetrics(scale));
        context.set_device_viewport_rect({ 3, 5, 801, 599 });
        b.appendff("  css_viewport_rect {}\n", context.css_viewport_rect());
        int const devices[] = { 0, 1, 3, -5, 17, 799 };
        for (auto d : devices)
            b.appendff("  device {} css {}\n", d, context.scale_to_css_pixels(d).raw_value());
        b.appendff("  css_rect {}\n", context.scale_to_css_rect({ 3, 7, 11, 13 }));
        auto metrics = ChromeMetrics(scale);
        b.appendff("  metrics {} {} {} {} {} {} {}\n", metrics.scroll_thumb_min_length.raw_value(), metrics.scroll_thumb_padding_thin.raw_value(), metrics.scroll_thumb_thickness_thin.raw_value(),
            metrics.scroll_thumb_thickness.raw_value(), metrics.scroll_gutter_thickness.raw_value(), metrics.resize_gripper_size.raw_value(), metrics.resize_gripper_padding.raw_value());
    }
    put_lines(b.string_view().trim_whitespace(TrimMode::Right));
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    g_css = args.size() > 2 ? args[2] : std::string();
    if (mode == "dl")
        run_display_list(args[1]);
    else if (mode.starts_with("px "))
        run_pixels(args[1], mode);
    else if (mode == "values")
        run_values();
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
        std::vector<std::string> args;
        size_t start = 0;
        while (true) {
            auto tab = raw.find('\t', start);
            args.push_back(unescape(raw.substr(start, tab == std::string::npos ? std::string::npos : tab - start)));
            if (tab == std::string::npos)
                break;
            start = tab + 1;
        }
        g_out += "case " + raw + "\n";
        run(args);
    }
    std::cout << g_out;
}
