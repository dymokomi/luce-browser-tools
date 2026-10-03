// Oracle for luce-browser-engine region r36 (CSS values and units): runs the reference build's
// Length, Angle, Time, Frequency, Resolution, Flex, Percentage, Ratio, NumericRange, Display,
// SystemColor, Sizing, EdgeRect, Size, LengthPercentage(OrAuto), GridLineNames/GridTrackSizeList,
// CounterStyle, ColorInterpolation (its internal helpers: ColorInterpolation.cpp is included here
// and its object left out of the link), ColorFunctionDescriptor, ValueType, PropertyNameAndID,
// DescriptorNameAndID, URL and the embedded style sheets, and prints what the Luce test
// (tests_css_values_units, against tests_css_values_units_expected) prints. Doubles and floats
// are printed as their bits, CSSPixels as raw values, colors as their ARGB value.
#define private public
#define protected public
#include <LibGfx/Font/Font.h>
#include <LibWeb/CSS/Angle.h>
#include <LibWeb/CSS/ColorFunctionDescriptor.h>
#include <LibWeb/CSS/CounterStyle.h>
#include <LibWeb/CSS/CounterStyleDefinition.h>
#include <LibWeb/CSS/DescriptorNameAndID.h>
#include <LibWeb/CSS/Display.h>
#include <LibWeb/CSS/EdgeRect.h>
#include <LibWeb/CSS/Flex.h>
#include <LibWeb/CSS/Frequency.h>
#include <LibWeb/CSS/GridTrackSize.h>
#include <LibWeb/CSS/Length.h>
#include <LibWeb/CSS/LengthBox.h>
#include <LibWeb/CSS/NumericRange.h>
#include <LibWeb/CSS/Percentage.h>
#include <LibWeb/CSS/PercentageOr.h>
#include <LibWeb/CSS/PropertyName.h>
#include <LibWeb/CSS/PropertyNameAndID.h>
#include <LibWeb/CSS/Ratio.h>
#include <LibWeb/CSS/Resolution.h>
#include <LibWeb/CSS/Size.h>
#include <LibWeb/CSS/Sizing.h>
#include <LibWeb/CSS/SystemColor.h>
#include <LibWeb/CSS/Time.h>
#include <LibWeb/CSS/URL.h>
#include <LibWeb/CSS/ValueType.h>
#include <LibWeb/CSS/ColorInterpolation.cpp>
#undef private
#undef protected
#include <stdio.h>
#include <string.h>
#include <string>

using namespace Web::CSS;
using Web::CSSPixels;
using Web::CSSPixelRect;
using Web::CSSPixelSize;
using Web::CSSPixelFraction;

namespace Web::CSS {
extern String default_stylesheet_source;
extern String quirks_mode_stylesheet_source;
}

static std::string g_out;

static void put(std::string const& text) { g_out += text; g_out += "\n"; }
static std::string s(StringView view) { return std::string(view.characters_without_null_termination(), view.length()); }
static std::string s(String const& string) { return s(string.bytes_as_string_view()); }
static std::string s(FlyString const& string) { return s(string.bytes_as_string_view()); }
static std::string d(double value)
{
    u64 bits;
    memcpy(&bits, &value, 8);
    char buffer[32];
    snprintf(buffer, sizeof buffer, "%016llx", (unsigned long long)bits);
    return buffer;
}
static std::string f(float value)
{
    u32 bits;
    memcpy(&bits, &value, 4);
    char buffer[16];
    snprintf(buffer, sizeof buffer, "%08x", bits);
    return buffer;
}
static std::string n(long long value) { return std::to_string(value); }
static std::string px(CSSPixels value) { return n(value.raw_value()); }
static std::string b(bool value) { return value ? "1" : "0"; }
static std::string hex(u32 value)
{
    char buffer[16];
    snprintf(buffer, sizeof buffer, "%08x", value);
    return buffer;
}
static std::string comps(Gfx::ColorComponents const& c)
{
    return f(c[0]) + " " + f(c[1]) + " " + f(c[2]) + " " + f(c.alpha());
}

static double const values[] = { 0, 1.5, -2.25, 0.1, 100, 1e10, 1.0 / 3 };

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

static void lengths()
{
    put("# lengths");
    auto context = Length::ResolutionContext { CSSPixelRect(0, 0, 800, 600), font_metrics(), root_font_metrics() };
    auto metrics = font_metrics();
    put("metrics " + px(metrics.font_size) + " " + px(metrics.x_height) + " " + px(metrics.cap_height) + " " + px(metrics.zero_advance) + " " + px(metrics.line_height));
    for (u8 unit = 0; unit <= to_underlying(LengthUnit::Vw); unit++) {
        for (auto value : values) {
            Length length(value, static_cast<LengthUnit>(unit));
            auto absolutized = length.absolutize(context);
            put(s(length.unit_name()) + " " + d(value) + " " + b(length.is_px()) + b(length.is_absolute()) + b(length.is_font_relative()) + b(length.is_viewport_relative()) + b(length.is_relative()) + b(length.is_computationally_independent())
                + " " + s(length.to_string()) + " " + s(length.to_string(SerializationMode::ResolvedValue))
                + " " + px(length.to_px(context)) + " " + d(length.to_px_without_rounding(context))
                + " " + (absolutized.has_value() ? s(absolutized->to_string()) : std::string("none"))
                + " " + b(length == Length(value, static_cast<LengthUnit>(unit))) + b(length == Length::make_px(value)));
        }
    }
    put("percentage_of " + s(Length(12, LengthUnit::Em).percentage_of(Percentage(25)).to_string()));
    put("make_px " + s(Length::make_px(CSSPixels::from_raw(97)).to_string()));
    auto auto_ = LengthOrAuto::make_auto();
    LengthOrAuto length_or_auto(Length(3, LengthUnit::Rem));
    put("length_or_auto " + s(auto_.to_string()) + " " + b(auto_.is_auto()) + b(auto_.is_length()) + b(auto_.is_font_relative()) + b(auto_.is_computationally_independent())
        + " " + s(length_or_auto.to_string()) + " " + b(length_or_auto.is_auto()) + b(length_or_auto.is_length()) + b(length_or_auto.is_font_relative()) + b(length_or_auto.is_computationally_independent())
        + " " + b(auto_ == LengthOrAuto::make_auto()) + b(auto_ == length_or_auto));
}

template<typename T, typename Unit>
static void dimension(char const* name, Unit last, T reference, double (T::*canonical)() const)
{
    put(std::string("# ") + name);
    for (u8 unit = 0; unit <= to_underlying(last); unit++) {
        for (auto value : values) {
            T dimension(value, static_cast<Unit>(unit));
            put(s(dimension.unit_name()) + " " + d(value) + " " + d((dimension.*canonical)()) + " " + s(dimension.to_string()) + " " + s(dimension.to_string(SerializationMode::ResolvedValue))
                + " " + n(dimension <=> reference) + " " + b(dimension == reference) + " " + s(dimension.percentage_of(Percentage(150)).to_string()));
        }
    }
}

static void units()
{
    dimension<Angle, AngleUnit>("angles", AngleUnit::Turn, Angle::make_degrees(90), &Angle::to_degrees);
    for (u8 unit = 0; unit <= to_underlying(AngleUnit::Turn); unit++)
        put("radians " + d(Angle(1.25, static_cast<AngleUnit>(unit)).to_radians()));
    dimension<Time, TimeUnit>("times", TimeUnit::S, Time::make_seconds(1.5), &Time::to_seconds);
    for (u8 unit = 0; unit <= to_underlying(TimeUnit::S); unit++)
        put("milliseconds " + d(Time(1.25, static_cast<TimeUnit>(unit)).to_milliseconds()));
    dimension<Frequency, FrequencyUnit>("frequencies", FrequencyUnit::KHz, Frequency::make_hertz(100), &Frequency::to_hertz);
    put("# resolutions");
    for (u8 unit = 0; unit <= to_underlying(ResolutionUnit::X); unit++) {
        for (auto value : values) {
            Resolution resolution(value, static_cast<ResolutionUnit>(unit));
            auto reference = Resolution::make_dots_per_pixel(2);
            put(s(resolution.unit_name()) + " " + d(value) + " " + d(resolution.to_dots_per_pixel()) + " " + s(resolution.to_string()) + " " + s(resolution.to_string(SerializationMode::ResolvedValue))
                + " " + n(resolution <=> reference) + " " + b(resolution == reference));
        }
    }
    put("# flexes");
    for (auto value : values) {
        Flex flex(value, FlexUnit::Fr);
        auto reference = Flex::make_fr(1);
        put(s(flex.unit_name()) + " " + d(value) + " " + d(flex.to_fr()) + " " + s(flex.to_string()) + " " + s(flex.to_string(SerializationMode::ResolvedValue))
            + " " + n(flex <=> reference) + " " + b(flex == reference) + " " + s(flex.percentage_of(Percentage(150)).to_string()));
    }
    put("# percentages");
    for (auto value : values) {
        Percentage percentage(value);
        put(d(value) + " " + d(percentage.as_fraction()) + " " + s(percentage.to_string()) + " " + n(percentage <=> Percentage(50)) + " " + b(percentage == Percentage(100)));
    }
    put("# ratios");
    double const ratio_values[][2] = { { 16, 9 }, { 1, 0 }, { 0, 1 }, { 4, 3 }, { 1.5, 2.5 }, { INFINITY, 1 }, { -2, 4 } };
    for (auto const& pair : ratio_values) {
        Ratio ratio(pair[0], pair[1]);
        put(s(ratio.to_string()) + " " + d(ratio.value()) + " " + d(ratio.numerator()) + " " + d(ratio.denominator()) + " " + b(ratio.is_degenerate()) + " " + n(ratio <=> Ratio(4, 3)) + " " + b(ratio == Ratio(8, 6)));
    }
    put("default_ratio " + s(Ratio(2).to_string()));
    put("# numeric ranges");
    for (auto range : { infinite_range, non_negative_range, infinite_integer_range, non_negative_integer_range })
        put(d(range.min) + " " + d(range.max) + " " + b(range.contains(-1)) + b(range.contains(0)) + b(range.contains(3e38)) + b(range.contains(4e9)));
    put(b(NumericRange { 1, 1000 }.contains(1)) + b(NumericRange { 1, 1000 }.contains(1000.5)));
}

static std::string display_flags(Display const& display)
{
    std::string flags;
    flags += b(display.is_none()) + b(display.is_contents()) + b(display.is_internal()) + b(display.is_outside_and_inside());
    flags += " " + b(display.is_table_column()) + b(display.is_table_row_group()) + b(display.is_table_header_group()) + b(display.is_table_footer_group()) + b(display.is_table_row()) + b(display.is_table_cell()) + b(display.is_table_column_group()) + b(display.is_table_caption()) + b(display.is_internal_table());
    flags += " " + b(display.is_block_outside()) + b(display.is_inline_outside()) + b(display.is_inline_block()) + b(display.is_list_item());
    flags += " " + b(display.is_flow_inside()) + b(display.is_flow_root_inside()) + b(display.is_table_inside()) + b(display.is_flex_inside()) + b(display.is_grid_inside()) + b(display.is_ruby_inside()) + b(display.is_math_inside());
    if (display.is_outside_and_inside())
        flags += " " + s(to_string(display.outside())) + "/" + s(to_string(display.inside()));
    if (display.is_internal())
        flags += " " + s(to_string(display.internal()));
    return flags;
}

static void displays()
{
    put("# displays");
    char const* keywords[] = { "none", "contents", "block", "inline", "flow", "flow-root", "inline-block", "run-in", "list-item", "flex", "inline-flex", "grid", "inline-grid", "ruby", "table", "inline-table", "math",
        "table-row-group", "table-header-group", "table-footer-group", "table-row", "table-cell", "table-column-group", "table-column", "table-caption", "ruby-base", "ruby-text", "ruby-base-container", "ruby-text-container" };
    for (auto const* keyword_name : keywords) {
        auto keyword = keyword_from_string(StringView { keyword_name, strlen(keyword_name) }).value();
        auto display = Display::from_keyword(keyword);
        auto to_keyword = display.to_keyword();
        put(std::string(keyword_name) + " " + s(display.to_string()) + " " + (to_keyword.has_value() ? s(string_from_keyword(*to_keyword)) : std::string("none")) + " " + display_flags(display));
    }
    for (int short_ = 0; short_ <= static_cast<int>(Display::Short::Math); short_++) {
        auto display = Display::from_short(static_cast<Display::Short>(short_));
        put("short " + n(short_) + " " + s(display.to_string()) + " " + display_flags(display));
    }
    Display custom[] = { Display(DisplayOutside::Inline, DisplayInside::Flow, Display::ListItem::Yes), Display(DisplayOutside::RunIn, DisplayInside::Table), Display(DisplayOutside::Block, DisplayInside::FlowRoot, Display::ListItem::Yes), Display(DisplayOutside::Inline, DisplayInside::Grid, Display::ListItem::Yes), Display() };
    for (auto const& display : custom) {
        auto to_keyword = display.to_keyword();
        put("custom " + s(display.to_string()) + " " + (to_keyword.has_value() ? s(string_from_keyword(*to_keyword)) : std::string("none")) + " " + display_flags(display) + " " + b(display == Display::from_short(Display::Short::InlineListItem)));
    }
}

static void system_colors()
{
    put("# system colors");
    using Function = Color (*)(PreferredColorScheme);
    Function functions[] = { SystemColor::accent_color, SystemColor::accent_color_text, SystemColor::active_text, SystemColor::button_border, SystemColor::button_face, SystemColor::button_text, SystemColor::canvas, SystemColor::canvas_text, SystemColor::field, SystemColor::field_text, SystemColor::gray_text, SystemColor::highlight, SystemColor::highlight_text, SystemColor::link_text, SystemColor::mark, SystemColor::mark_text, SystemColor::selected_item, SystemColor::selected_item_text, SystemColor::visited_text };
    for (auto function : functions)
        put(hex(function(PreferredColorScheme::Light).value()) + " " + hex(function(PreferredColorScheme::Dark).value()) + " " + hex(function(PreferredColorScheme::Auto).value()));
}

static std::string optional_px(Optional<CSSPixels> value) { return value.has_value() ? px(*value) : std::string("-"); }

static void sizing()
{
    put("# sizing");
    Optional<CSSPixels> widths[] = { {}, CSSPixels(100), CSSPixels(37.5) };
    Optional<CSSPixels> heights[] = { {}, CSSPixels(50), CSSPixels(0) };
    SizeWithAspectRatio naturals[] = {
        { {}, {}, {} },
        { CSSPixels(300), CSSPixels(150), CSSPixelFraction(CSSPixels(300), CSSPixels(150)) },
        { CSSPixels(300), {}, {} },
        { {}, CSSPixels(80), {} },
        { {}, {}, CSSPixelFraction(CSSPixels(16), CSSPixels(9)) },
        { {}, {}, CSSPixelFraction(CSSPixels(1), CSSPixels(3)) },
        { {}, {}, CSSPixelFraction(CSSPixels::max(), CSSPixels(1)) },
    };
    CSSPixelSize defaults[] = { CSSPixelSize(300, 150), CSSPixelSize(0, 150) };
    for (auto const& width : widths) {
        for (auto const& height : heights) {
            for (auto const& natural : naturals) {
                for (auto const& default_size : defaults) {
                    auto size = run_default_sizing_algorithm(width, height, natural, default_size);
                    put(optional_px(width) + " " + optional_px(height) + " " + optional_px(natural.width) + " " + optional_px(natural.height) + " " + b(natural.has_aspect_ratio()) + " " + px(default_size.width()) + "x" + px(default_size.height()) + " -> " + px(size.width()) + "x" + px(size.height()));
                }
            }
        }
    }
}

static void boxes()
{
    put("# edge rects");
    EdgeRect rects[] = {
        { LengthOrAuto::make_auto(), LengthOrAuto::make_auto(), LengthOrAuto::make_auto(), LengthOrAuto::make_auto() },
        { Length(5, LengthUnit::Px), Length(1, LengthUnit::In), Length(2, LengthUnit::Cm), Length(3, LengthUnit::Pt) },
        { Length(-5.5, LengthUnit::Px), LengthOrAuto::make_auto(), Length(10, LengthUnit::Mm), LengthOrAuto::make_auto() },
    };
    for (auto const& rect : rects) {
        auto resolved = rect.resolved(CSSPixelRect(CSSPixels(10), CSSPixels(20.25), CSSPixels(300), CSSPixels(150)));
        put(px(resolved.x()) + " " + px(resolved.y()) + " " + px(resolved.width()) + " " + px(resolved.height()) + " " + b(rect == rects[0]));
    }
    put("# sizes");
    Size sizes[] = { Size::make_auto(), Size::make_px(CSSPixels(12.5)), Size::make_length(Length(2, LengthUnit::Em)), Size::make_percentage(Percentage(40)), Size::make_min_content(), Size::make_max_content(), Size::make_fit_content(), Size::make_fit_content(LengthPercentage(Percentage(30))), Size::make_fit_content(LengthPercentage(Length(4, LengthUnit::Px))), Size::make_none(), Size::make_length_percentage(LengthPercentage(Length(7, LengthUnit::Vh))), Size::make_length_percentage(LengthPercentage(Percentage(8))) };
    for (auto const& size : sizes) {
        put(s(size.to_string(SerializationMode::Normal)) + " " + n(to_underlying(size.type())) + " " + b(size.is_auto()) + b(size.is_calculated()) + b(size.is_length()) + b(size.is_percentage()) + b(size.is_min_content()) + b(size.is_max_content()) + b(size.is_fit_content()) + b(size.is_none())
            + " " + b(size.is_intrinsic_sizing_constraint()) + b(size.is_length_percentage()) + b(size.contains_percentage()) + " " + b(size == sizes[1]) + b(size == Size::make_fit_content(LengthPercentage(Percentage(30)))));
    }
    put("# length percentages");
    LengthPercentage length_percentages[] = { LengthPercentage(Length(3, LengthUnit::Px)), LengthPercentage(Percentage(12.5)), LengthPercentage(Length(0, LengthUnit::Em)) };
    for (auto const& length_percentage : length_percentages) {
        LengthPercentageOrAuto or_auto(length_percentage);
        put(s(length_percentage.to_string(SerializationMode::Normal)) + " " + b(length_percentage.is_length()) + b(length_percentage.is_percentage()) + b(length_percentage.is_calculated()) + b(length_percentage.contains_percentage())
            + " " + s(or_auto.to_string(SerializationMode::Normal)) + " " + b(or_auto.is_auto()) + b(or_auto.is_length()) + b(or_auto.is_percentage()) + b(or_auto.is_calculated()) + b(or_auto.contains_percentage())
            + " " + b(length_percentage == length_percentages[0]) + b(length_percentage == Length(3, LengthUnit::Px)) + b(length_percentage == Percentage(12.5)) + b(or_auto == LengthPercentageOrAuto::make_auto()));
    }
    auto auto_ = LengthPercentageOrAuto::make_auto();
    put("auto " + s(auto_.to_string(SerializationMode::Normal)) + " " + b(auto_.is_auto()) + b(auto_.is_length()) + b(auto_.is_percentage()) + b(auto_.contains_percentage()) + " " + b(auto_ == LengthPercentageOrAuto::make_auto()));
    LengthBox box;
    LengthBox other(Length(1, LengthUnit::Px), Percentage(2), LengthPercentageOrAuto::make_auto(), Length(4, LengthUnit::Em));
    put("box " + s(box.top().to_string(SerializationMode::Normal)) + " " + s(other.top().to_string(SerializationMode::Normal)) + " " + s(other.right().to_string(SerializationMode::Normal)) + " " + s(other.bottom().to_string(SerializationMode::Normal)) + " " + s(other.left().to_string(SerializationMode::Normal)) + " " + b(box == LengthBox()) + b(box == other));
}

static void grids()
{
    put("# grids");
    GridLineNames names;
    names.append("a"_fly_string);
    names.append("b-c"_fly_string);
    GridLineNames more;
    more.append("d"_fly_string);
    auto list = GridTrackSizeList::make_none();
    put("none " + s(list.to_string(SerializationMode::Normal)) + " " + b(list.is_empty()) + " " + n(list.track_list().size()));
    list.append(GridLineNames { names });
    put("one " + s(list.to_string(SerializationMode::Normal)) + " " + n(list.list().size()));
    list.append(GridLineNames { more });
    put("merged " + s(list.to_string(SerializationMode::Normal)) + " " + n(list.list().size()) + " " + n(list.track_list().size()) + " " + s(names.to_string()) + " " + b(names.is_empty()) + " " + b(GridLineNames {}.is_empty()));
    auto other = GridTrackSizeList::make_none();
    other.append(GridLineNames { names });
    put("equal " + b(list == other) + b(GridTrackSizeList::make_none() == GridTrackSizeList::make_none()) + b(names == names) + b(names == more));
    GridTemplateAreas areas;
    put("areas " + b(areas.is_empty()));
}

static std::string optional_string(Optional<String> const& value) { return value.has_value() ? s(*value) : std::string("(fallback)"); }

static Vector<FlyString> symbols(std::initializer_list<char const*> texts)
{
    Vector<FlyString> result;
    for (auto const* text : texts)
        result.append(MUST(FlyString::from_utf8(StringView { text, strlen(text) })));
    return result;
}

static i64 const counter_values[] = { 0, 1, 2, 3, 9, 10, 11, 15, 19, 20, 21, 99, 100, 101, 110, 999, 1000, 1001, 1010, 1100, 9999, 10000, 10001, 11111, 12345, 20000, 100000, 1000000, 10000000, 99999999, 100000000, 100010001, 123456789, 1234567890123 };

static void counter_styles()
{
    put("# counter styles");
    Vector<CounterStyleRangeEntry> infinite { { NumericLimits<i32>::min(), NumericLimits<i32>::max() } };
    CounterStyleNegativeSign negative { "-"_fly_string, ""_fly_string };
    CounterStylePad no_pad { 0, ""_fly_string };
    Vector<CounterStyleAlgorithm> algorithms;
    algorithms.append(GenericCounterStyleAlgorithm { CounterStyleSystem::Cyclic, symbols({ "a", "b", "c" }) });
    algorithms.append(GenericCounterStyleAlgorithm { CounterStyleSystem::Numeric, symbols({ "0", "1", "2" }) });
    algorithms.append(GenericCounterStyleAlgorithm { CounterStyleSystem::Alphabetic, symbols({ "a", "b", "c" }) });
    algorithms.append(GenericCounterStyleAlgorithm { CounterStyleSystem::Symbolic, symbols({ "*", "\xe2\x80\xa0" }) });
    algorithms.append(FixedCounterStyleAlgorithm { 3, symbols({ "x", "y", "z" }) });
    algorithms.append(AdditiveCounterStyleAlgorithm { { { 1000, "M"_fly_string }, { 900, "CM"_fly_string }, { 500, "D"_fly_string }, { 400, "CD"_fly_string }, { 100, "C"_fly_string }, { 90, "XC"_fly_string }, { 50, "L"_fly_string }, { 40, "XL"_fly_string }, { 10, "X"_fly_string }, { 9, "IX"_fly_string }, { 5, "V"_fly_string }, { 4, "IV"_fly_string }, { 1, "I"_fly_string } } });
    algorithms.append(AdditiveCounterStyleAlgorithm { { { 5, "five"_fly_string }, { 0, "zero"_fly_string } } });
    algorithms.append(EthiopicNumericCounterStyleAlgorithm {});
    for (u8 type = 0; type <= to_underlying(ExtendedCJKCounterStyleAlgorithm::Type::KoreanHanjaFormal); type++)
        algorithms.append(ExtendedCJKCounterStyleAlgorithm { static_cast<ExtendedCJKCounterStyleAlgorithm::Type>(type) });
    for (size_t index = 0; index < algorithms.size(); index++) {
        auto style = CounterStyle::create("test"_fly_string, algorithms[index], negative, ""_fly_string, ". "_fly_string, infinite, "decimal"_fly_string, no_pad);
        std::string line = "algorithm " + n(index) + " " + b(style->uses_a_negative_sign()) + ":";
        // Symbolic and additive representations grow with the value: those algorithms stop early.
        i64 maximum = index == 3 ? 110 : (index == 5 || index == 6) ? 2000 : NumericLimits<i64>::max();
        for (auto value : counter_values) {
            if (value <= maximum)
                line += " " + optional_string(style->generate_an_initial_representation_for_the_counter_value(value));
        }
        put(line);
    }
    put("auto ranges");
    for (size_t index = 0; index < 7; index++) {
        auto range = AutoRange::resolve(algorithms[index]);
        put(n(index) + " " + n(range.size()) + " " + n(range[0].start) + " " + n(range[0].end));
    }
    auto decimal = CounterStyle::decimal();
    auto disc = CounterStyle::disc();
    auto* scope = reinterpret_cast<StyleScope const*>(&decimal);
    std::string line = "decimal " + s(decimal->name()) + " " + s(decimal->prefix()) + "|" + s(decimal->suffix()) + " " + b(decimal->fallback().has_value()) + ":";
    for (i32 value = -12; value <= 12; value++)
        line += " " + s(generate_a_counter_representation(decimal, *scope, value));
    put(line);
    line = "disc " + s(disc->name()) + " " + s(*disc->fallback()) + ":";
    for (i32 value = -3; value <= 3; value++)
        line += " " + s(generate_a_counter_representation(disc, *scope, value));
    put(line);
    put("equals " + b(decimal->equals(*decimal)) + b(decimal->equals(*disc)) + b(CounterStyle::decimal() == decimal));
    auto padded = CounterStyle::create("padded"_fly_string, GenericCounterStyleAlgorithm { CounterStyleSystem::Numeric, symbols({ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }) }, CounterStyleNegativeSign { "(-"_fly_string, ")"_fly_string }, ""_fly_string, ""_fly_string, infinite, "decimal"_fly_string, CounterStylePad { 5, "0"_fly_string });
    line = "padded:";
    for (i32 value : { -12345, -123, -1, 0, 7, 42, 123456, NumericLimits<i32>::min(), NumericLimits<i32>::max() })
        line += " " + s(generate_a_counter_representation(padded, *scope, value));
    put(line);
    auto additive = CounterStyle::create("roman"_fly_string, algorithms[5], CounterStyleNegativeSign { "-"_fly_string, ""_fly_string }, ""_fly_string, ""_fly_string, infinite, "decimal"_fly_string, CounterStylePad { 3, "_"_fly_string });
    line = "roman:";
    for (i32 value : { -14, 1, 4, 9, 14, 40, 90, 400, 1994, 3999 })
        line += " " + s(generate_a_counter_representation(additive, *scope, value));
    put(line);
}

static std::string missing(MissingComponents const& m) { return b(m.components[0]) + b(m.components[1]) + b(m.components[2]) + b(m.alpha); }
static std::string categories(ComponentCategories const& c) { return n(to_underlying(c.components[0])) + n(to_underlying(c.components[1])) + n(to_underlying(c.components[2])); }

static void color_interpolation()
{
    put("# color interpolation");
    float const hue_pairs[][2] = { { 10, 350 }, { 350, 10 }, { 0, 180 }, { 180, 0 }, { 90, 90 }, { 30, 200 }, { 200, 30 }, { 0, 0 } };
    for (u8 method = 0; method <= to_underlying(HueInterpolationMethod::Decreasing); method++) {
        std::string line = "fixup_hue " + n(method) + ":";
        for (auto const& pair : hue_pairs) {
            float hue1 = pair[0], hue2 = pair[1];
            fixup_hue(hue1, hue2, static_cast<HueInterpolationMethod>(method));
            line += " " + f(hue1) + "," + f(hue2);
        }
        put(line);
    }
    put("interpolate " + f(interpolate_color_component(0.25f, 0.75f, 0.3f)) + " " + f(interpolate_color_component(1, -1, 1.5f)));
    Gfx::ColorComponents const colors[] = { { 1, 0.5f, 0.25f, 1 }, { 0, 0, 0, 0.5f }, { 0.2f, 0.4f, 0.6f, 0.8f }, { 1, 1, 1, 1 } };
    for (auto const& color : colors) {
        for (u8 space = 0; space <= to_underlying(RectangularColorSpace::XyzD65); space++)
            put("rectangular " + n(space) + " " + comps(srgb_to_rectangular_color_space(color, static_cast<RectangularColorSpace>(space))));
        for (u8 space = 0; space <= to_underlying(PolarColorSpace::Oklch); space++)
            put("polar " + n(space) + " " + comps(srgb_to_polar_color_space(color, static_cast<PolarColorSpace>(space))));
        for (u8 type = 0; type <= to_underlying(ColorStyleValue::ColorType::XYZD65); type++)
            put("native " + n(type) + " " + comps(native_components_to_srgb(color, static_cast<ColorStyleValue::ColorType>(type))));
        for (int polar = 0; polar <= 1; polar++) {
            auto premultiplied = premultiply_color_components(color, polar, 2);
            auto interpolated = interpolate_premultiplied_components(premultiplied, premultiply_color_components(colors[2], polar, 2), 0.25f);
            put("premultiply " + n(polar) + " " + comps(premultiplied) + " " + comps(interpolated) + " " + comps(unpremultiply_color_components(interpolated, 0.4f, polar, 2)));
        }
    }
    for (u8 space = 0; space <= to_underlying(RectangularColorSpace::XyzD65); space++)
        put("categories rectangular " + n(space) + " " + categories(categories_for_rectangular_space(static_cast<RectangularColorSpace>(space))));
    for (u8 space = 0; space <= to_underlying(PolarColorSpace::Oklch); space++)
        put("categories polar " + n(space) + " " + categories(categories_for_polar_space(static_cast<PolarColorSpace>(space))) + " " + n(hue_index_for_color_space(static_cast<PolarColorSpace>(space))));
    for (u8 type = 0; type <= to_underlying(ColorStyleValue::ColorType::XYZD65); type++) {
        std::string line = "color type " + n(type) + " " + categories(categories_for_color_type(static_cast<ColorStyleValue::ColorType>(type))) + " ";
        for (u8 space = 0; space <= to_underlying(RectangularColorSpace::XyzD65); space++)
            line += b(color_type_matches_rectangular_space(static_cast<ColorStyleValue::ColorType>(type), static_cast<RectangularColorSpace>(space)));
        line += " ";
        for (u8 space = 0; space <= to_underlying(PolarColorSpace::Oklch); space++)
            line += b(color_type_matches_polar_space(static_cast<ColorStyleValue::ColorType>(type), static_cast<PolarColorSpace>(space)));
        put(line);
    }
    ComponentCategories const category_sets[] = {
        { ComponentCategory::Red, ComponentCategory::Green, ComponentCategory::Blue },
        { ComponentCategory::Lightness, ComponentCategory::OpponentA, ComponentCategory::OpponentB },
        { ComponentCategory::Hue, ComponentCategory::Colorfulness, ComponentCategory::Lightness },
        { ComponentCategory::Hue, ComponentCategory::NotAnalogous, ComponentCategory::NotAnalogous },
        { ComponentCategory::Lightness, ComponentCategory::Colorfulness, ComponentCategory::Hue },
        {},
    };
    for (u8 bits = 0; bits < 16; bits++) {
        MissingComponents source { (bits & 1) != 0, (bits & 2) != 0, (bits & 4) != 0, (bits & 8) != 0 };
        std::string line = "carry " + missing(source) + ":";
        for (auto const& from : category_sets) {
            for (auto const& to : category_sets)
                line += " " + missing(carry_forward_missing_components(source, from, to));
        }
        put(line);
        MissingComponents marked = source;
        mark_powerless_for_zero_alpha(false, 0.0f, marked);
        MissingComponents unmarked = source;
        mark_powerless_for_zero_alpha(true, 0.0f, unmarked);
        MissingComponents opaque = source;
        mark_powerless_for_zero_alpha(false, 0.5f, opaque);
        InterpolationSpaceState state;
        state.from_missing = source;
        state.to_missing = MissingComponents { (bits & 8) != 0, (bits & 1) != 0, true, (bits & 2) != 0 };
        state.from_components = colors[2];
        state.to_components = colors[0];
        auto both_alpha_missing = reinsert_carried_forward_values(state);
        put("powerless " + missing(marked) + " " + missing(unmarked) + " " + missing(opaque) + " " + missing(result_missing_components(state)) + " " + b(both_alpha_missing) + " " + comps(state.from_components) + " " + comps(state.to_components));
    }
    InterpolationSpaceState polar_state;
    polar_state.is_polar = true;
    polar_state.hue_index = 2;
    polar_state.hue_interpolation_method = HueInterpolationMethod::Longer;
    polar_state.from_components = { 50, 20, 30, 1 };
    polar_state.to_components = { 60, 25, 100, 1 };
    fixup_hues_if_required(polar_state);
    put("fixup_hues " + comps(polar_state.from_components) + " " + comps(polar_state.to_components) + " " + categories(polar_state.polar_target_categories));
}

static void descriptors()
{
    put("# color function descriptors");
    for (u8 type = 0; type <= to_underlying(ColorStyleValue::ColorType::XYZD65); type++) {
        auto const& descriptor = color_function_descriptor_for(static_cast<ColorStyleValue::ColorType>(type));
        std::string line = s(descriptor.function_name) + " " + n(to_underlying(descriptor.serialization_behavior)) + " " + n(to_underlying(descriptor.absolutizes_to_rgb));
        for (auto const& channel : descriptor.channels)
            line += " " + n(to_underlying(channel.kind)) + "," + f(channel.percent_reference) + "," + (channel.serialize_clamp_min.has_value() ? d(*channel.serialize_clamp_min) : std::string("-")) + "," + (channel.serialize_clamp_max.has_value() ? d(*channel.serialize_clamp_max) : std::string("-"));
        put(line);
    }
    for (auto const* name : { "srgb", "xyz", "xyz-d50", "xyz-d65", "rgb", "hsl", "lab", "display-p3", "a98-rgb", "rec2020", "prophoto-rgb", "srgb-linear", "display-p3-linear", "SRGB", "oklch", "" }) {
        auto type = color_type_from_color_function_name(StringView { name, strlen(name) });
        put(std::string("color function ") + name + " " + (type.has_value() ? n(to_underlying(*type)) : std::string("none")));
    }
    put("# value types");
    for (auto const* name : { "anchor", "anchor-size", "angle", "ANGLE", "angle-percentage", "background-position", "basic-shape", "color", "corner-shape", "counter", "counter-style", "custom-ident", "dashed-ident", "easing-function", "filter-value-list", "fit-content", "flex", "font-style", "font-variant-alternates", "font-variant-east-asian", "font-variant-ligatures", "font-variant-numeric", "frequency", "frequency-percentage", "image", "integer", "length", "Length-Percentage", "number", "opacity-value", "opentype-tag", "paint", "percentage", "position", "ratio", "rect", "resolution", "scroll-function", "string", "time", "time-percentage", "transform-function", "transform-list", "url", "view-function", "view-timeline-inset", "bogus", "" }) {
        auto type = value_type_from_string(StringView { name, strlen(name) });
        put(std::string("value type ") + name + " " + (type.has_value() ? n(to_underlying(*type)) + " " + s(value_type_to_string(*type)) : std::string("none")));
    }
    for (u8 type = 0; type <= to_underlying(ValueType::ViewTimelineInset); type++)
        put("value type name " + n(type) + " " + s(value_type_to_string(static_cast<ValueType>(type))));
    put("# property names");
    for (auto const* name : { "color", "COLOR", "background-color", "--foo", "--", "-webkit-appearance", "word-wrap", "float", "bogus", "" }) {
        auto property = PropertyNameAndID::from_name(FlyString::from_utf8_without_validation(StringView { name, strlen(name) }.bytes()));
        put(std::string("property ") + name + " " + b(is_a_valid_css_property(StringView { name, strlen(name) })) + " " + (property.has_value() ? n(to_underlying(property->id())) + " " + s(property->name()) + " " + s(property->to_string()) + " " + b(property->is_custom_property()) : std::string("none")));
    }
    auto from_id = PropertyNameAndID::from_id(PropertyID::MarginTop);
    put("property from_id " + s(from_id.name()) + " " + s(from_id.to_string()) + " " + b(from_id.is_custom_property()));
    for (auto at_rule : { AtRuleID::FontFace, AtRuleID::CounterStyle, AtRuleID::Property, AtRuleID::Page }) {
        for (auto const* name : { "font-family", "src", "system", "symbols", "syntax", "inherits", "initial-value", "size", "margin", "--x", "font-display", "bogus" }) {
            auto descriptor = DescriptorNameAndID::from_name(at_rule, FlyString::from_utf8_without_validation(StringView { name, strlen(name) }.bytes()));
            put("descriptor " + n(to_underlying(at_rule)) + " " + name + " " + (descriptor.has_value() ? n(to_underlying(descriptor->id())) + " " + s(descriptor->name()) + " " + n(Traits<DescriptorNameAndID>::hash(*descriptor)) + " " + b(*descriptor == DescriptorNameAndID::from_name(at_rule, "src"_fly_string)) : std::string("none")));
        }
    }
    auto descriptor_from_id = DescriptorNameAndID::from_id(DescriptorID::FontFamily);
    put("descriptor from_id " + s(descriptor_from_id.name()) + " " + n(Traits<DescriptorNameAndID>::hash(descriptor_from_id)));
}

static void urls()
{
    put("# urls");
    Vector<RequestURLModifier> modifiers;
    modifiers.append(RequestURLModifier::create_cross_origin(CrossOriginModifierValue::UseCredentials));
    modifiers.append(RequestURLModifier::create_integrity("sha384-\"x\""_fly_string));
    modifiers.append(RequestURLModifier::create_referrer_policy(ReferrerPolicyModifierValue::StrictOriginWhenCrossOrigin));
    modifiers.append(RequestURLModifier::create_cross_origin(CrossOriginModifierValue::Anonymous));
    for (auto const& modifier : modifiers)
        put(s(modifier.to_string()) + " " + n(to_underlying(modifier.type())) + " " + b(modifier == modifiers[0]) + b(modifier == RequestURLModifier::create_integrity("sha384-\"x\""_fly_string)));
    Web::CSS::URL plain("image.png"_string);
    Web::CSS::URL src("a \"b\"\\c.png"_string, Web::CSS::URL::Type::Src, modifiers);
    put(s(plain.to_string()) + " " + s(src.to_string()) + " " + b(plain == Web::CSS::URL("image.png"_string)) + b(plain == src) + b(src == Web::CSS::URL("a \"b\"\\c.png"_string, Web::CSS::URL::Type::Src, modifiers)) + b(src == Web::CSS::URL("a \"b\"\\c.png"_string, Web::CSS::URL::Type::Url, modifiers)));
}

static void style_sheets()
{
    put("# style sheets");
    for (auto const* sheet : { &default_stylesheet_source, &quirks_mode_stylesheet_source }) {
        auto bytes = sheet->bytes();
        u32 hash = 2166136261u;
        for (auto byte : bytes)
            hash = (hash ^ byte) * 16777619u;
        put(n(bytes.size()) + " " + hex(hash));
    }
}

int main()
{
    lengths();
    units();
    displays();
    system_colors();
    sizing();
    boxes();
    grids();
    counter_styles();
    color_interpolation();
    descriptors();
    urls();
    style_sheets();
    fwrite(g_out.data(), 1, g_out.size(), stdout);
    return 0;
}
