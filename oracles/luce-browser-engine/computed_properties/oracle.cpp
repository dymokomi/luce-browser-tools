// Oracle for luce-browser-engine region r40 (ComputedProperties' typed readers). Each case line is
// "<reader>\t<declarations>" with declarations "property: value; property: value". The oracle builds a
// ComputedProperties the way StyleComputer::compute_property_values does without an element: every longhand's
// specified value is its initial value, or the declared value (parse_css_value in the internal CSS realm), and
// its computed value is StyleComputer::compute_value_of_property in an 800x600 context with a 16px font
// (r35's length context, ComputationContext without an element, 1 device pixel per CSS pixel). Then it calls
// the reader the case names and prints the result in the format the Luce test (css/tests_computed_properties)
// prints: enums as their underlying integers, CSSPixels and floats with AK's `{}`, colors as their u32 value,
// lengths, sizes and style values serialized, optionals as "-" when empty.
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/ComputedProperties.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/StyleComputer.h>
#include <LibWeb/CSS/StyleValues/ComputationContext.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/CSS/StyleValues/TransformationStyleValue.h>
#include <LibWeb/CSS/StyleValues/AbstractImageStyleValue.h>
#include <LibWeb/CSS/StyleValues/CursorStyleValue.h>
#include <LibCore/EventLoop.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#undef private
#undef protected
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace Web::CSS;
using namespace Web;

static std::string to_std(StringView s) { return std::string(s.characters_without_null_termination(), s.length()); }
static std::string to_std(String const& s) { return to_std(s.bytes_as_string_view()); }
static std::string to_std(FlyString const& s) { return to_std(s.bytes_as_string_view()); }
static std::string num(double value) { return to_std(MUST(String::formatted("{}", value))); }
static std::string px(CSSPixels value) { return num(value.to_double()); }
static std::string flag(bool value) { return value ? "1" : "0"; }
template<typename E>
static std::string en(E value) { return std::to_string(static_cast<long long>(to_underlying(value))); }
static std::string color(Color value) { return std::to_string(value.value()); }
static std::string sv(StyleValue const& value) { return to_std(value.to_string(SerializationMode::Normal)); }
static std::string lp(LengthPercentage const& value) { return to_std(value.to_string(SerializationMode::Normal)); }
static std::string lpa(LengthPercentageOrAuto const& value) { return to_std(value.to_string(SerializationMode::Normal)); }
static std::string len(Length const& value) { return to_std(value.to_string(SerializationMode::Normal)); }
static std::string size(Size const& value) { return to_std(value.to_string(SerializationMode::Normal)); }
static std::string loa(LengthOrAuto const& value) { return to_std(value.to_string(SerializationMode::Normal)); }
static std::string position(Position const& value) { return en(value.edge_x) + " " + lp(value.offset_x) + " " + en(value.edge_y) + " " + lp(value.offset_y); }
static std::string box(LengthBox const& value) { return lpa(value.top()) + " " + lpa(value.right()) + " " + lpa(value.bottom()) + " " + lpa(value.left()); }
static std::string filter(Filter const& value) { return value.has_filters() ? sv(*value.m_filter_value_list) : "none"; }
static std::string gap(Variant<LengthPercentage, NormalGap> const& value)
{
    return value.visit([](LengthPercentage const& v) { return lp(v); }, [](NormalGap const&) { return std::string("normal-gap"); });
}
static std::string shadow_free_join(std::vector<std::string> const& parts)
{
    std::string out;
    for (size_t i = 0; i < parts.size(); i++)
        out += (i ? " | " : "") + parts[i];
    return "[" + out + "]";
}

static Length::FontMetrics font_metrics()
{
    Gfx::FontPixelMetrics metrics;
    metrics.size = 16;
    metrics.x_height = 7.5f;
    metrics.advance_of_ascii_zero = 9.25f;
    metrics.ascent = 12.796875f;
    return Length::FontMetrics(CSSPixels(16), metrics, CSSPixels(18.5));
}

static Length::FontMetrics root_font_metrics()
{
    Gfx::FontPixelMetrics metrics;
    metrics.size = 20;
    metrics.x_height = 8.0f;
    metrics.advance_of_ascii_zero = 10.0f;
    metrics.ascent = 15.0f;
    return Length::FontMetrics(CSSPixels(20), metrics, CSSPixels(22));
}

static GC::Ref<ComputedProperties> build(std::string const& declarations)
{
    std::map<PropertyID, NonnullRefPtr<StyleValue const>> specified;
    for (auto i = to_underlying(first_longhand_property_id); i <= to_underlying(last_longhand_property_id); ++i)
        specified.emplace(static_cast<PropertyID>(i), property_initial_value(static_cast<PropertyID>(i)));
    Parser::ParsingParams params { Web::internal_css_realm() };
    size_t start = 0;
    while (start < declarations.size()) {
        auto end = declarations.find(';', start);
        if (end == std::string::npos)
            end = declarations.size();
        auto declaration = declarations.substr(start, end - start);
        start = end + 1;
        auto colon = declaration.find(':');
        if (colon == std::string::npos)
            continue;
        auto name = declaration.substr(0, colon);
        auto value = declaration.substr(colon + 1);
        while (!name.empty() && name.front() == ' ')
            name.erase(0, 1);
        while (!value.empty() && value.front() == ' ')
            value.erase(0, 1);
        auto id = property_id_from_string(StringView(name.c_str(), name.size())).release_value();
        auto parsed = parse_css_value(params, StringView(value.c_str(), value.size()), id);
        if (!parsed) {
            std::cerr << "PARSE " << name << ": " << value << std::endl;
            VERIFY_NOT_REACHED();
        }
        specified.insert_or_assign(id, *parsed);
    }
    ComputationContext context { .length_resolution_context = Length::ResolutionContext { CSSPixelRect(0, 0, 800, 600), font_metrics(), root_font_metrics() } };
    auto style = Web::Bindings::main_thread_vm().heap().allocate<ComputedProperties>();
    for (auto& [id, value] : specified) {
        auto computed = StyleComputer::compute_value_of_property(id, value, [&](PropertyID other) { return specified.at(other); }, context, 1.0);
        style->set_property(id, computed);
    }
    return style;
}

// InitialValues and a default-constructed ComputedValues (the readers ComputedValues.h defines).
static std::string initial_values()
{
    std::string out;
    out += px(InitialValues::font_size()) + " " + num(InitialValues::font_weight()) + " " + px(InitialValues::line_height()) + " " + len(InitialValues::border_spacing());
    out += " " + color(InitialValues::caret_color()) + " " + color(InitialValues::background_color()) + " " + to_std(InitialValues::display().to_string());
    out += " " + px(InitialValues::text_underline_offset()) + " " + px(InitialValues::outline_width()) + " " + num(InitialValues::stroke_miterlimit());
    auto scrollbar = InitialValues::scrollbar_color();
    out += " " + color(scrollbar.thumb_color) + " " + color(scrollbar.track_color);
    out += " " + box(InitialValues::margin()) + " " + box(InitialValues::inset()) + " " + size(InitialValues::max_width()) + " " + lp(InitialValues::stroke_width()) + " " + lpa(InitialValues::rx());
    out += " " + to_std(InitialValues::transition_delay().to_string()) + " " + to_std(InitialValues::grid_template_columns().to_string(SerializationMode::Normal));
    auto order = InitialValues::paint_order();
    out += " " + en(order[0]) + en(order[1]) + en(order[2]) + " " + en(InitialValues::quotes().type);
    ComputedValues values;
    out += " | " + px(values.font_size()) + " " + color(values.color()) + " " + std::to_string(values.cursor().size()) + " " + std::to_string(values.text_decoration_line().size());
    out += " " + size(values.width()) + " " + box(values.padding()) + " " + position(values.object_position()) + " " + position(values.perspective_origin());
    auto origin = values.transform_origin();
    out += " " + lp(origin.x) + " " + lp(origin.y) + " " + lp(origin.z) + " " + lp(values.border_top_left_radius().horizontal_radius) + " " + flag(values.border_top_left_radius().is_initial());
    auto t = values.touch_action();
    out += " " + flag(t.allow_left) + flag(t.allow_other) + " " + flag(values.has_noninitial_border_radii()) + " " + en(values.border_top().line_style) + " " + px(values.border_top().width);
    out += " " + flag(values.inline_axis_is_reverse()) + flag(values.block_axis_is_reverse()) + " " + flag(values.will_change().is_auto()) + " " + flag(values.contain().is_empty());
    return out;
}

static std::string run(std::string const& reader, std::string const& declarations)
{
    if (reader == "initial_values")
        return initial_values();
    auto style = build(declarations);
    auto& s = *style;
    ColorResolutionContext no_context {};
    if (reader == "size_value") return size(s.size_value(PropertyID::Width)) + " " + size(s.size_value(PropertyID::MinHeight)) + " " + size(s.size_value(PropertyID::MaxWidth));
    if (reader == "gap_value") return gap(s.gap_value(PropertyID::ColumnGap)) + " " + gap(s.gap_value(PropertyID::RowGap));
    if (reader == "length_box") return box(s.length_box(PropertyID::MarginLeft, PropertyID::MarginTop, PropertyID::MarginRight, PropertyID::MarginBottom, Length::make_px(0))) + " / " + box(s.length_box(PropertyID::Left, PropertyID::Top, PropertyID::Right, PropertyID::Bottom, LengthPercentageOrAuto::make_auto()));
    if (reader == "color") return color(s.color(PropertyID::Color, no_context)) + " " + color(s.color(PropertyID::BackgroundColor, no_context)) + " " + color(s.color(PropertyID::BorderTopColor, no_context));
    if (reader == "anchor_names") {
        std::vector<std::string> names;
        s.for_each_anchor_name([&](FlyString const& name) { names.push_back(to_std(name)); });
        return shadow_free_join(names);
    }
    if (reader == "color_interpolation") return en(s.color_interpolation());
    if (reader == "color_scheme") return en(s.color_scheme(PreferredColorScheme::Auto, {})) + " " + en(s.color_scheme(PreferredColorScheme::Dark, {})) + " " + en(s.color_scheme(PreferredColorScheme::Light, {}));
    if (reader == "text_anchor") return en(s.text_anchor());
    if (reader == "dominant_baseline") { auto v = s.dominant_baseline(); return v.has_value() ? en(*v) : "-"; }
    if (reader == "text_align") return en(s.text_align());
    if (reader == "text_justify") return en(s.text_justify());
    if (reader == "text_overflow") return en(s.text_overflow());
    if (reader == "text_rendering") return en(s.text_rendering());
    if (reader == "text_underline_offset") return px(s.text_underline_offset());
    if (reader == "text_underline_position") { auto v = s.text_underline_position(); return en(v.horizontal) + " " + en(v.vertical); }
    if (reader == "background_layers") {
        std::vector<std::string> layers;
        for (auto const& layer : s.background_layers())
            layers.push_back(sv(*layer.background_image) + " " + en(layer.attachment) + " " + en(layer.origin) + " " + en(layer.clip) + " " + lp(layer.position_x) + " " + lp(layer.position_y) + " " + en(layer.size_type) + " " + lpa(layer.size_x) + " " + lpa(layer.size_y) + " " + en(layer.repeat_x) + " " + en(layer.repeat_y) + " " + en(layer.blend_mode));
        return shadow_free_join(layers) + " clip=" + en(s.background_color_clip());
    }
    if (reader == "border_spacing") return len(s.border_spacing_horizontal()) + " " + len(s.border_spacing_vertical());
    if (reader == "caption_side") return en(s.caption_side());
    if (reader == "clip") { auto c = s.clip(); auto r = c.to_rect(); return flag(c.is_auto()) + " " + loa(r.top_edge) + " " + loa(r.right_edge) + " " + loa(r.bottom_edge) + " " + loa(r.left_edge); }
    if (reader == "display") return to_std(s.display().to_string());
    if (reader == "float") return en(s.float_());
    if (reader == "clear") return en(s.clear());
    if (reader == "column_span") return en(s.column_span());
    if (reader == "content_visibility") return en(s.content_visibility());
    if (reader == "cursor") {
        std::vector<std::string> cursors;
        for (auto const& cursor : s.cursor())
            cursors.push_back(cursor.visit([](NonnullRefPtr<CursorStyleValue const> const& v) { return sv(*v); }, [](CursorPredefined p) { return en(p); }));
        return shadow_free_join(cursors);
    }
    if (reader == "tab_size") return s.tab_size().visit([](Length const& l) { return "length " + len(l); }, [](double d) { return "number " + num(d); });
    if (reader == "white_space") { auto t = s.white_space_trim(); return en(s.white_space_collapse()) + " " + flag(t.discard_before) + flag(t.discard_after) + flag(t.discard_inner); }
    if (reader == "word_break") return en(s.word_break());
    if (reader == "spacing") return px(s.word_spacing()) + " " + px(s.letter_spacing());
    if (reader == "line_style") return en(s.line_style(PropertyID::BorderTopStyle)) + " " + en(s.outline_style());
    if (reader == "text_decoration") {
        std::vector<std::string> lines;
        for (auto line : s.text_decoration_line())
            lines.push_back(en(line));
        auto thickness = s.text_decoration_thickness().value.visit([](TextDecorationThickness::Auto) { return std::string("auto"); }, [](TextDecorationThickness::FromFont) { return std::string("from-font"); }, [](LengthPercentage const& v) { return lp(v); });
        return shadow_free_join(lines) + " " + en(s.text_decoration_skip_ink()) + " " + en(s.text_decoration_style()) + " " + thickness;
    }
    if (reader == "text_transform") return en(s.text_transform());
    if (reader == "text_indent") { auto t = s.text_indent(); return lp(t.length_percentage) + " " + flag(t.each_line) + flag(t.hanging); }
    if (reader == "text_wrap_mode") return en(s.text_wrap_mode());
    if (reader == "list_style_position") return en(s.list_style_position());
    if (reader == "flex") {
        auto basis = s.flex_basis().visit([](FlexBasisContent) { return std::string("content"); }, [](Size const& v) { return size(v); });
        return en(s.flex_direction()) + " " + en(s.flex_wrap()) + " " + basis + " " + num(s.flex_grow()) + " " + num(s.flex_shrink()) + " " + std::to_string(s.order());
    }
    if (reader == "accent_color") return color(s.accent_color(ColorResolutionContext { .color_scheme = PreferredColorScheme::Light })) + " " + color(s.accent_color(ColorResolutionContext { .color_scheme = PreferredColorScheme::Dark }));
    if (reader == "alignment") return en(s.align_content()) + " " + en(s.align_items()) + " " + en(s.align_self()) + " " + en(s.justify_content()) + " " + en(s.justify_items()) + " " + en(s.justify_self());
    if (reader == "appearance") return en(s.appearance());
    if (reader == "filters") return filter(s.filter()) + " / " + filter(s.backdrop_filter());
    if (reader == "opacities") return num(s.opacity()) + " " + num(s.fill_opacity()) + " " + num(s.stroke_opacity()) + " " + num(s.stop_opacity()) + " " + num(s.flood_opacity());
    if (reader == "visibility") return en(s.visibility());
    if (reader == "image_rendering") return en(s.image_rendering());
    if (reader == "overflow") return en(s.overflow_x()) + " " + en(s.overflow_y());
    if (reader == "box_sizing") return en(s.box_sizing());
    if (reader == "pointer_events") return en(s.pointer_events());
    if (reader == "vertical_align") return s.vertical_align().visit([](VerticalAlign v) { return "keyword " + en(v); }, [](LengthPercentage const& v) { return lp(v); });
    if (reader == "font_variants") {
        std::string out;
        if (auto a = s.font_variant_alternates(); a.has_value()) {
            out += "alternates " + flag(a->historical_forms);
            for (auto const& entry : a->font_feature_value_entries)
                out += " " + en(entry.type) + ":" + to_std(entry.name);
        } else {
            out += "alternates -";
        }
        out += " caps " + en(s.font_variant_caps());
        if (auto e = s.font_variant_east_asian(); e.has_value())
            out += " east-asian " + flag(e->ruby) + " " + (e->variant.has_value() ? en(*e->variant) : "-") + " " + (e->width.has_value() ? en(*e->width) : "-");
        else
            out += " east-asian -";
        out += " emoji " + en(s.font_variant_emoji());
        if (auto l = s.font_variant_ligatures(); l.has_value())
            out += " ligatures " + flag(l->none) + " " + (l->common.has_value() ? en(*l->common) : "-") + " " + (l->discretionary.has_value() ? en(*l->discretionary) : "-") + " " + (l->historical.has_value() ? en(*l->historical) : "-") + " " + (l->contextual.has_value() ? en(*l->contextual) : "-");
        else
            out += " ligatures -";
        if (auto n = s.font_variant_numeric(); n.has_value())
            out += " numeric " + flag(n->ordinal) + flag(n->slashed_zero) + " " + (n->figure.has_value() ? en(*n->figure) : "-") + " " + (n->spacing.has_value() ? en(*n->spacing) : "-") + " " + (n->fraction.has_value() ? en(*n->fraction) : "-");
        else
            out += " numeric -";
        out += " position " + en(s.font_variant_position()) + " kerning " + en(s.font_kerning());
        auto language = s.font_language_override();
        out += " language " + (language.has_value() ? to_std(*language) : std::string("-"));
        return out;
    }
    if (reader == "font_settings") {
        auto features = s.font_feature_settings();
        auto variations = s.font_variation_settings();
        std::string out = "features " + std::to_string(features.size());
        for (auto key : { "liga"sv, "smcp"sv, "kern"sv, "ss01"sv })
            out += " " + to_std(key) + "=" + (features.get(MUST(FlyString::from_utf8(key))).has_value() ? std::to_string(*features.get(MUST(FlyString::from_utf8(key)))) : std::string("-"));
        out += " variations " + std::to_string(variations.size());
        for (auto key : { "wght"sv, "wdth"sv, "slnt"sv })
            out += " " + to_std(key) + "=" + (variations.get(MUST(FlyString::from_utf8(key))).has_value() ? num(*variations.get(MUST(FlyString::from_utf8(key)))) : std::string("-"));
        return out;
    }
    if (reader == "grid") {
        auto flow = s.grid_auto_flow();
        auto areas = s.grid_template_areas();
        return to_std(s.grid_template_columns().to_string(SerializationMode::Normal)) + " / " + to_std(s.grid_template_rows().to_string(SerializationMode::Normal)) + " / " + to_std(s.grid_auto_columns().to_string(SerializationMode::Normal)) + " / " + to_std(s.grid_auto_rows().to_string(SerializationMode::Normal)) + " / " + flag(flow.row) + flag(flow.dense) + " / " + to_std(s.grid_column_start().to_string(SerializationMode::Normal)) + " " + to_std(s.grid_column_end().to_string(SerializationMode::Normal)) + " " + to_std(s.grid_row_start().to_string(SerializationMode::Normal)) + " " + to_std(s.grid_row_end().to_string(SerializationMode::Normal)) + " / areas " + std::to_string(areas.areas.size()) + " " + std::to_string(areas.row_count) + "x" + std::to_string(areas.column_count);
    }
    if (reader == "table") return en(s.border_collapse()) + " " + en(s.empty_cells()) + " " + en(s.table_layout());
    if (reader == "object") return en(s.object_fit()) + " " + position(s.object_position());
    if (reader == "writing") return en(s.direction()) + " " + en(s.unicode_bidi()) + " " + en(s.writing_mode());
    if (reader == "user_select") return en(s.user_select()) + " " + en(s.isolation()) + " " + en(s.mix_blend_mode());
    if (reader == "touch_action") { auto t = s.touch_action(); return flag(t.allow_left) + flag(t.allow_right) + flag(t.allow_up) + flag(t.allow_down) + flag(t.allow_pinch_zoom) + flag(t.allow_other); }
    if (reader == "contain") { auto c = s.contain(); auto t = s.container_type(); return flag(c.size_containment) + flag(c.inline_size_containment) + flag(c.layout_containment) + flag(c.style_containment) + flag(c.paint_containment) + " " + flag(t.is_size_container) + flag(t.is_inline_size_container) + flag(t.is_scroll_state_container); }
    if (reader == "view_transition_name") { auto v = s.view_transition_name(); return v.has_value() ? to_std(*v) : "-"; }
    if (reader == "transforms") {
        std::vector<std::string> transforms;
        for (auto const& t : s.transformations())
            transforms.push_back(sv(*t));
        auto origin = s.transform_origin();
        auto optional = [](RefPtr<TransformationStyleValue const> const& v) { return v ? sv(*v) : std::string("-"); };
        auto perspective = s.perspective();
        return shadow_free_join(transforms) + " " + en(s.transform_box()) + " " + en(s.transform_style()) + " " + lp(origin.x) + " " + lp(origin.y) + " " + lp(origin.z) + " " + optional(s.rotate()) + " " + optional(s.translate()) + " " + optional(s.scale()) + " " + (perspective.has_value() ? px(*perspective) : "-") + " " + position(s.perspective_origin());
    }
    if (reader == "svg") {
        std::vector<std::string> dashes;
        for (auto const& dash : s.stroke_dasharray())
            dashes.push_back(dash.visit([](LengthPercentage const& v) { return lp(v); }, [](float f) { return num(f); }));
        return shadow_free_join(dashes) + " " + en(s.stroke_linecap()) + " " + en(s.stroke_linejoin()) + " " + num(s.stroke_miterlimit()) + " " + en(s.fill_rule()) + " " + en(s.clip_rule()) + " " + en(s.mask_type()) + " " + en(s.shape_rendering());
    }
    if (reader == "paint_order") { auto p = s.paint_order(); return en(p[0]) + en(p[1]) + en(p[2]); }
    if (reader == "will_change") { auto w = s.will_change(); return flag(w.is_auto()) + flag(w.has_contents()) + flag(w.has_scroll_position()) + flag(w.has_property(PropertyID::Opacity)) + flag(w.has_property(PropertyID::Transform)); }
    if (reader == "quotes") {
        auto q = s.quotes();
        std::string out = en(q.type);
        for (auto const& pair : q.strings)
            out += " " + to_std(pair[0]) + to_std(pair[1]);
        return out;
    }
    if (reader == "counters") {
        std::string out;
        for (auto id : { PropertyID::CounterReset, PropertyID::CounterIncrement, PropertyID::CounterSet }) {
            std::vector<std::string> items;
            for (auto const& c : s.counter_data(id))
                items.push_back(to_std(c.name) + " " + flag(c.is_reversed) + " " + (c.value.has_value() ? std::to_string(*c.value) : std::string("-")));
            out += shadow_free_join(items);
        }
        return out;
    }
    if (reader == "scrollbar") return en(s.scrollbar_width()) + " " + en(s.resize());
    if (reader == "math") return en(s.math_style()) + " " + std::to_string(s.math_depth());
    if (reader == "font") return px(s.font_size()) + " " + num(s.font_weight()) + " " + to_std(s.font_width().to_string()) + " " + std::to_string(s.font_slope()) + " " + en(s.font_optical_sizing()) + " " + px(s.line_height());
    if (reader == "position") { auto z = s.z_index(); return en(s.position()) + " " + (z.has_value() ? std::to_string(*z) : "-"); }
    return "unknown reader";
}

int main(int argc, char** argv)
{
    Core::EventLoop event_loop;
    Web::Platform::EventLoopPlugin::install(*new Web::Platform::EventLoopPlugin);
    Web::Bindings::initialize_main_thread_vm(Web::Bindings::AgentType::SimilarOriginWindow);
    std::ifstream in(argc > 1 ? argv[1] : "cases.txt");
    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty() || raw[0] == '#')
            continue;
        if (raw[0] == '!')
            raw = raw.substr(1);
        auto tab = raw.find('\t');
        std::cerr << raw << std::endl;
        std::cout << run(raw.substr(0, tab), tab == std::string::npos ? std::string() : raw.substr(tab + 1)) << std::endl;
    }
}
