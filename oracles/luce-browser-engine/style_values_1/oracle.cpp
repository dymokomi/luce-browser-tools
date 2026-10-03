// Oracle for luce-browser-engine region r34 (CSS style values I): builds style values directly with the reference
// build's constructors (the value parsers are regions r32/r33) and prints what the Luce test
// (tests_style_values_1_*, against tests_style_values_1_expected) prints: each value's serialization in both modes,
// computational independence, absolutization (and whether it answered the value itself), colors in a light and a
// dark context, the full equality matrix, tokenization, the free helpers, the static instances, and the geometry
// of radial sizes and gradient angles. Colors are printed as ARGB, CSSPixels as raw values, doubles as bits.
#define private public
#define protected public
#include <LibGfx/Font/Font.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/PropertyNameAndID.h>
#include <LibWeb/CSS/StyleValues/AnchorSizeStyleValue.h>
#include <LibWeb/CSS/StyleValues/AnchorStyleValue.h>
#include <LibWeb/CSS/StyleValues/AngleStyleValue.h>
#include <LibWeb/CSS/StyleValues/BackgroundSizeStyleValue.h>
#include <LibWeb/CSS/StyleValues/BorderImageSliceStyleValue.h>
#include <LibWeb/CSS/StyleValues/BorderRadiusRectStyleValue.h>
#include <LibWeb/CSS/StyleValues/BorderRadiusStyleValue.h>
#include <LibWeb/CSS/StyleValues/ColorFunctionStyleValue.h>
#include <LibWeb/CSS/StyleValues/ColorInterpolationMethodStyleValue.h>
#include <LibWeb/CSS/StyleValues/ColorMixStyleValue.h>
#include <LibWeb/CSS/StyleValues/ColorSchemeStyleValue.h>
#include <LibWeb/CSS/StyleValues/ColorStyleValue.h>
#include <LibWeb/CSS/StyleValues/ConicGradientStyleValue.h>
#include <LibWeb/CSS/StyleValues/ContentStyleValue.h>
#include <LibWeb/CSS/StyleValues/CursorStyleValue.h>
#include <LibWeb/CSS/StyleValues/CustomIdentStyleValue.h>
#include <LibWeb/CSS/StyleValues/DisplayStyleValue.h>
#include <LibWeb/CSS/StyleValues/EdgeStyleValue.h>
#include <LibWeb/CSS/StyleValues/FlexStyleValue.h>
#include <LibWeb/CSS/StyleValues/FrequencyStyleValue.h>
#include <LibWeb/CSS/StyleValues/FunctionStyleValue.h>
#include <LibWeb/CSS/StyleValues/GuaranteedInvalidStyleValue.h>
#include <LibWeb/CSS/StyleValues/ImageSetStyleValue.h>
#include <LibWeb/CSS/StyleValues/ImageStyleValue.h>
#include <LibWeb/CSS/StyleValues/IntegerStyleValue.h>
#include <LibWeb/CSS/StyleValues/KeywordStyleValue.h>
#include <LibWeb/CSS/StyleValues/LengthStyleValue.h>
#include <LibWeb/CSS/StyleValues/LightDarkStyleValue.h>
#include <LibWeb/CSS/StyleValues/LinearGradientStyleValue.h>
#include <LibWeb/CSS/StyleValues/NumberStyleValue.h>
#include <LibWeb/CSS/StyleValues/OpacityValueStyleValue.h>
#include <LibWeb/CSS/StyleValues/OpenTypeTaggedStyleValue.h>
#include <LibWeb/CSS/StyleValues/PendingSubstitutionStyleValue.h>
#include <LibWeb/CSS/StyleValues/PercentageStyleValue.h>
#include <LibWeb/CSS/StyleValues/PositionStyleValue.h>
#include <LibWeb/CSS/StyleValues/RadialGradientStyleValue.h>
#include <LibWeb/CSS/StyleValues/RadialSizeStyleValue.h>
#include <LibWeb/CSS/StyleValues/RandomValueSharingStyleValue.h>
#include <LibWeb/CSS/StyleValues/RatioStyleValue.h>
#include <LibWeb/CSS/StyleValues/RectStyleValue.h>
#include <LibWeb/CSS/StyleValues/RepeatStyleStyleValue.h>
#include <LibWeb/CSS/StyleValues/ResolutionStyleValue.h>
#include <LibWeb/CSS/StyleValues/ScrollbarColorStyleValue.h>
#include <LibWeb/CSS/StyleValues/ScrollbarGutterStyleValue.h>
#include <LibWeb/CSS/StyleValues/ShadowStyleValue.h>
#include <LibWeb/CSS/StyleValues/ShorthandStyleValue.h>
#include <LibWeb/CSS/StyleValues/StringStyleValue.h>
#include <LibWeb/CSS/StyleValues/StyleValueList.h>
#include <LibWeb/CSS/StyleValues/SuperellipseStyleValue.h>
#include <LibWeb/CSS/StyleValues/TextIndentStyleValue.h>
#include <LibWeb/CSS/StyleValues/TextUnderlinePositionStyleValue.h>
#include <LibWeb/CSS/StyleValues/TimeStyleValue.h>
#include <LibWeb/CSS/StyleValues/TreeCountingFunctionStyleValue.h>
#include <LibWeb/CSS/StyleValues/TupleStyleValue.h>
#include <LibWeb/CSS/StyleValues/URLStyleValue.h>
#include <LibWeb/CSS/StyleValues/UnicodeRangeStyleValue.h>
#include <LibWeb/CSS/StyleValues/UnresolvedStyleValue.h>
#include <LibWeb/HTML/SupportedImageTypes.h>
#undef private
#undef protected
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

using namespace Web::CSS;
using Web::CSSPixels;
using Web::CSSPixelPoint;
using Web::CSSPixelRect;
using Web::CSSPixelSize;
using V = ValueComparingNonnullRefPtr<StyleValue const>;

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
static std::string color(Optional<Color> c) { return c.has_value() ? hex(c->value()) : std::string("none"); }

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

static Length::ResolutionContext length_context() { return Length::ResolutionContext { CSSPixelRect(0, 0, 800, 600), font_metrics(), root_font_metrics() }; }

static V num(double value) { return NumberStyleValue::create(value); }
static V len(double value, LengthUnit unit) { return LengthStyleValue::create(Length(value, unit)); }
static V pct(double value) { return PercentageStyleValue::create(Percentage(value)); }
static V kw(Keyword keyword) { return KeywordStyleValue::create(keyword); }
static ValueComparingNonnullRefPtr<EdgeStyleValue const> edge(Optional<PositionEdge> e, RefPtr<StyleValue const> offset) { return EdgeStyleValue::create(e, offset); }
static V rgb(double r, double g, double b_, RefPtr<StyleValue const> alpha = {}, ColorSyntax syntax = ColorSyntax::Legacy, Optional<FlyString> name = {})
{
    return ColorFunctionStyleValue::create(ColorStyleValue::ColorType::RGB, num(r), num(g), num(b_), alpha, syntax, name);
}
static ColorStopListElement stop(V color, RefPtr<StyleValue const> position = {}, RefPtr<StyleValue const> hint = {})
{
    return ColorStopListElement { .transition_hint = hint, .color_stop = { .color = color, .position = position, .second_position = {} } };
}
static Vector<Parser::ComponentValue> component_values(StringView text)
{
    return Parser::Parser::create(Parser::ParsingParams {}, text).parse_as_list_of_component_values();
}

struct Entry {
    std::string label;
    V value;
};

static std::vector<Entry> build_values()
{
    std::vector<Entry> values;
    auto add = [&](char const* label, V value) { values.push_back({ label, value }); };

    // Keywords
    add("kw-inherit", kw(Keyword::Inherit));
    add("kw-initial", kw(Keyword::Initial));
    add("kw-unset", kw(Keyword::Unset));
    add("kw-revert", kw(Keyword::Revert));
    add("kw-revert-layer", kw(Keyword::RevertLayer));
    add("kw-auto", kw(Keyword::Auto));
    add("kw-none", kw(Keyword::None));
    add("kw-currentcolor", kw(Keyword::Currentcolor));
    add("kw-canvas", kw(Keyword::Canvas));
    add("kw-canvastext", kw(Keyword::Canvastext));
    add("kw-buttonface", kw(Keyword::Buttonface));
    add("kw-accentcolor", kw(Keyword::Accentcolor));
    add("kw-accentcolortext", kw(Keyword::Accentcolortext));
    add("kw-highlight", kw(Keyword::Highlight));
    add("kw-mark", kw(Keyword::Mark));
    add("kw-linktext", kw(Keyword::Linktext));
    add("kw-libweb-buttonfacedisabled", kw(Keyword::LibwebButtonfacedisabled));
    add("kw-libweb-buttonfacehover", kw(Keyword::LibwebButtonfacehover));
    add("kw-libweb-palette-base", kw(Keyword::LibwebPaletteBase));
    add("kw-block", kw(Keyword::Block));
    // Numbers, integers and dimensions
    add("number-0", num(0));
    add("number-1.5", num(1.5));
    add("number--2.25", num(-2.25));
    add("number-third", num(1.0 / 3));
    add("number-1e10", num(1e10));
    add("integer-0", IntegerStyleValue::create(0));
    add("integer-42", IntegerStyleValue::create(42));
    add("integer--7", IntegerStyleValue::create(-7));
    add("length-0px", len(0, LengthUnit::Px));
    add("length-1px", len(1, LengthUnit::Px));
    add("length-12px", len(12, LengthUnit::Px));
    add("length-1.5em", len(1.5, LengthUnit::Em));
    add("length-2rem", len(2, LengthUnit::Rem));
    add("length-50vw", len(50, LengthUnit::Vw));
    add("length-3in", len(3, LengthUnit::In));
    add("percentage-50", pct(50));
    add("percentage-12.5", pct(12.5));
    add("angle-90deg", AngleStyleValue::create(Angle(90, AngleUnit::Deg)));
    add("angle-0.25turn", AngleStyleValue::create(Angle(0.25, AngleUnit::Turn)));
    add("angle-1rad", AngleStyleValue::create(Angle(1, AngleUnit::Rad)));
    add("time-1.5s", TimeStyleValue::create(Time(1.5, TimeUnit::S)));
    add("time-250ms", TimeStyleValue::create(Time(250, TimeUnit::Ms)));
    add("frequency-2khz", FrequencyStyleValue::create(Frequency(2, FrequencyUnit::KHz)));
    add("resolution-2x", ResolutionStyleValue::create(Resolution(2, ResolutionUnit::X)));
    add("resolution-96dpi", ResolutionStyleValue::create(Resolution(96, ResolutionUnit::Dpi)));
    add("flex-1fr", FlexStyleValue::create(Flex(1, FlexUnit::Fr)));
    // Strings, identifiers, URLs, displays
    add("string-plain", StringStyleValue::create("hello"_fly_string));
    add("string-quotes", StringStyleValue::create(FlyString::from_utf8_without_validation("a\"b\\c\nd"sv.bytes())));
    add("custom-ident-foo", CustomIdentStyleValue::create("foo"_fly_string));
    add("custom-ident-escaped", CustomIdentStyleValue::create("1st item"_fly_string));
    add("url-plain", URLStyleValue::create(Web::CSS::URL("image.png"_string)));
    add("url-fallback", URLStyleValue::create(Web::CSS::URL("paint.svg#p"_string), kw(Keyword::None)));
    add("display-block", DisplayStyleValue::create(Display(DisplayOutside::Block, DisplayInside::Flow)));
    add("display-inline-flex", DisplayStyleValue::create(Display(DisplayOutside::Inline, DisplayInside::Flex)));
    add("unicode-range", UnicodeRangeStyleValue::create(Gfx::UnicodeRange(0x41, 0x5a)));
    // Colors
    add("rgb-red", rgb(255, 0, 0));
    add("rgb-named-Red", rgb(255, 0, 0, {}, ColorSyntax::Legacy, "Red"_fly_string));
    add("rgb-half-alpha", rgb(10, 20, 30, num(0.5)));
    add("rgb-modern-none", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::RGB, kw(Keyword::None), num(128), pct(50), pct(25), ColorSyntax::Modern));
    add("rgb-out-of-range", rgb(300, -5, 127.5, num(2)));
    add("hsl", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::HSL, num(120), pct(50), pct(50), {}, ColorSyntax::Legacy));
    add("hsl-angle", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::HSL, AngleStyleValue::create(Angle(0.5, AngleUnit::Turn)), num(100), num(25), num(0.25), ColorSyntax::Modern));
    add("hwb", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::HWB, num(200), pct(10), pct(20)));
    add("hwb-gray", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::HWB, num(0), pct(70), pct(60), pct(80)));
    add("lab", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::Lab, num(50), num(40), num(-30)));
    add("lch", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::LCH, pct(60), num(50), num(270)));
    add("oklab", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::OKLab, num(0.7), num(-0.1), num(0.1), pct(90)));
    add("oklch", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::OKLCH, num(0.5), num(0.2), num(30)));
    add("color-srgb", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::sRGB, num(0.25), pct(50), num(1)));
    add("color-display-p3", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::DisplayP3, num(1), num(0), num(0), num(0.75)));
    add("color-xyz-d65", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::XYZD65, num(0.2), num(0.3), num(0.4)));
    add("color-rec2020-negative-alpha", ColorFunctionStyleValue::create(ColorStyleValue::ColorType::Rec2020, num(0.5), num(0.5), num(0.5), num(-1)));
    add("from-color", ColorStyleValue::create_from_color(Color(1, 2, 3, 128), ColorSyntax::Modern));
    add("color-mix-default", ColorMixStyleValue::create({}, { rgb(255, 0, 0), {} }, { rgb(0, 0, 255), {} }));
    add("color-mix-srgb-40", ColorMixStyleValue::create(ColorInterpolationMethodStyleValue::create(RectangularColorSpace::Srgb), { rgb(255, 0, 0), pct(40) }, { rgb(0, 0, 255), {} }));
    add("color-mix-p2-30", ColorMixStyleValue::create({}, { rgb(255, 255, 0), {} }, { rgb(0, 128, 0), pct(30) }));
    add("color-mix-both-sum-50", ColorMixStyleValue::create(ColorInterpolationMethodStyleValue::create(ColorInterpolationMethodStyleValue::PolarColorInterpolationMethod { PolarColorSpace::Hsl, HueInterpolationMethod::Longer }), { rgb(255, 0, 0), pct(20) }, { rgb(0, 0, 255), pct(30) }));
    add("color-mix-currentcolor", ColorMixStyleValue::create({}, { kw(Keyword::Currentcolor), pct(50) }, { rgb(0, 0, 255), pct(50) }));
    add("light-dark", LightDarkStyleValue::create(rgb(255, 255, 255), rgb(0, 0, 0)));
    add("color-scheme-normal", ColorSchemeStyleValue::normal());
    add("color-scheme-light-dark-only", ColorSchemeStyleValue::create({ "light"_string, "dark"_string }, true));
    add("cim-srgb", ColorInterpolationMethodStyleValue::create(RectangularColorSpace::Srgb));
    add("cim-hsl-longer", ColorInterpolationMethodStyleValue::create(ColorInterpolationMethodStyleValue::PolarColorInterpolationMethod { PolarColorSpace::Hsl, HueInterpolationMethod::Longer }));
    add("cim-lch-shorter", ColorInterpolationMethodStyleValue::create(ColorInterpolationMethodStyleValue::PolarColorInterpolationMethod { PolarColorSpace::Lch, HueInterpolationMethod::Shorter }));
    // Positions and edges
    add("edge-center", edge(PositionEdge::Center, {}));
    add("edge-left-10px", edge(PositionEdge::Left, len(10, LengthUnit::Px)));
    add("edge-right", edge(PositionEdge::Right, {}));
    add("edge-25%", edge({}, pct(25)));
    add("position-center", PositionStyleValue::create_center());
    add("position-computed-center", PositionStyleValue::create_computed_center());
    add("position-left-top", PositionStyleValue::create(edge(PositionEdge::Left, {}), edge(PositionEdge::Top, len(1, LengthUnit::Em))));
    // Gradients
    Vector<ColorStopListElement> two_stops { stop(rgb(255, 0, 0)), stop(rgb(0, 0, 255), pct(50)) };
    Vector<ColorStopListElement> hinted_stops { stop(kw(Keyword::Currentcolor), len(1, LengthUnit::Em)), stop(rgb(0, 255, 0), pct(30), pct(20)), stop(ColorFunctionStyleValue::create(ColorStyleValue::ColorType::OKLab, num(0.5), num(0), num(0)), pct(60)) };
    add("linear-to-bottom", LinearGradientStyleValue::create(SideOrCorner::Bottom, two_stops, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::No, {}));
    add("linear-to-top-left", LinearGradientStyleValue::create(SideOrCorner::TopLeft, two_stops, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::Yes, ColorInterpolationMethodStyleValue::create(RectangularColorSpace::Oklab)));
    add("linear-angle-modern", LinearGradientStyleValue::create(NonnullRefPtr<StyleValue const>(AngleStyleValue::create(Angle(45, AngleUnit::Deg))), hinted_stops, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::No, ColorInterpolationMethodStyleValue::create(RectangularColorSpace::Srgb)));
    add("linear-webkit", LinearGradientStyleValue::create(SideOrCorner::Left, two_stops, LinearGradientStyleValue::GradientType::WebKit, GradientRepeating::No, {}));
    add("conic-default", ConicGradientStyleValue::create({}, PositionStyleValue::create_center(), two_stops, GradientRepeating::No, {}));
    add("conic-from-at", ConicGradientStyleValue::create(AngleStyleValue::create(Angle(90, AngleUnit::Deg)), PositionStyleValue::create(edge(PositionEdge::Left, {}), edge({}, pct(30))), hinted_stops, GradientRepeating::Yes, ColorInterpolationMethodStyleValue::create(ColorInterpolationMethodStyleValue::PolarColorInterpolationMethod { PolarColorSpace::Oklch, HueInterpolationMethod::Increasing })));
    add("radial-size-farthest-corner", RadialSizeStyleValue::create({ RadialExtent::FarthestCorner }));
    add("radial-size-closest-side", RadialSizeStyleValue::create({ RadialExtent::ClosestSide }));
    add("radial-size-lengths", RadialSizeStyleValue::create({ NonnullRefPtr<StyleValue const>(len(10, LengthUnit::Px)), NonnullRefPtr<StyleValue const>(pct(20)) }));
    add("radial-size-em", RadialSizeStyleValue::create({ NonnullRefPtr<StyleValue const>(len(2, LengthUnit::Em)) }));
    add("radial-default", RadialGradientStyleValue::create(RadialGradientStyleValue::EndingShape::Ellipse, RadialSizeStyleValue::create({ RadialExtent::FarthestCorner }), PositionStyleValue::create_center(), two_stops, GradientRepeating::No, {}));
    add("radial-circle-at", RadialGradientStyleValue::create(RadialGradientStyleValue::EndingShape::Circle, RadialSizeStyleValue::create({ RadialExtent::ClosestSide }), PositionStyleValue::create(edge(PositionEdge::Right, {}), edge(PositionEdge::Bottom, {})), hinted_stops, GradientRepeating::Yes, ColorInterpolationMethodStyleValue::create(RectangularColorSpace::DisplayP3)));
    // Boxes, borders and backgrounds
    add("background-size-auto-auto", BackgroundSizeStyleValue::create(kw(Keyword::Auto), kw(Keyword::Auto)));
    add("background-size-em-auto", BackgroundSizeStyleValue::create(len(2, LengthUnit::Em), kw(Keyword::Auto)));
    add("border-image-slice-1", BorderImageSliceStyleValue::create(num(1), num(1), num(1), num(1), false));
    add("border-image-slice-2", BorderImageSliceStyleValue::create(num(1), num(2), num(1), num(2), true));
    add("border-image-slice-3", BorderImageSliceStyleValue::create(num(1), num(2), num(3), num(2), false));
    add("border-image-slice-4", BorderImageSliceStyleValue::create(num(1), pct(2), num(3), num(4), false));
    add("border-radius-circle", BorderRadiusStyleValue::create(len(5, LengthUnit::Px), len(5, LengthUnit::Px)));
    add("border-radius-ellipse", BorderRadiusStyleValue::create(len(1, LengthUnit::Em), pct(10)));
    add("border-radius-zero", BorderRadiusStyleValue::create_zero());
    add("border-radius-rect-zero", BorderRadiusRectStyleValue::create_zero());
    add("border-radius-rect-mixed", BorderRadiusRectStyleValue::create(BorderRadiusStyleValue::create(len(1, LengthUnit::Px), len(2, LengthUnit::Px)), BorderRadiusStyleValue::create(len(3, LengthUnit::Px), len(3, LengthUnit::Px)), BorderRadiusStyleValue::create(len(1, LengthUnit::Px), len(2, LengthUnit::Px)), BorderRadiusStyleValue::create(len(3, LengthUnit::Px), len(3, LengthUnit::Px))));
    add("rect", RectStyleValue::create(len(1, LengthUnit::Px), kw(Keyword::Auto), len(2, LengthUnit::Em), len(3, LengthUnit::Px)));
    add("repeat-repeat-repeat", RepeatStyleStyleValue::create(Repetition::Repeat, Repetition::Repeat));
    add("repeat-x", RepeatStyleStyleValue::create(Repetition::Repeat, Repetition::NoRepeat));
    add("repeat-y", RepeatStyleStyleValue::create(Repetition::NoRepeat, Repetition::Repeat));
    add("repeat-space-round", RepeatStyleStyleValue::create(Repetition::Space, Repetition::Round));
    add("scrollbar-gutter-auto", ScrollbarGutterStyleValue::create(ScrollbarGutter::Auto));
    add("scrollbar-gutter-both", ScrollbarGutterStyleValue::create(ScrollbarGutter::BothEdges));
    add("scrollbar-color", ScrollbarColorStyleValue::create(rgb(1, 2, 3), kw(Keyword::Canvas)));
    add("shadow-box", ShadowStyleValue::create(ShadowStyleValue::ShadowType::Normal, rgb(0, 0, 0, num(0.5)), len(1, LengthUnit::Px), len(2, LengthUnit::Px), len(3, LengthUnit::Px), len(4, LengthUnit::Em), ShadowPlacement::Inner));
    add("shadow-text-minimal", ShadowStyleValue::create(ShadowStyleValue::ShadowType::Text, {}, len(1, LengthUnit::Px), len(1, LengthUnit::Px), {}, len(9, LengthUnit::Px), ShadowPlacement::Outer));
    add("superellipse-2", SuperellipseStyleValue::create(num(2)));
    add("superellipse-inf", SuperellipseStyleValue::create(num(INFINITY)));
    add("superellipse--inf", SuperellipseStyleValue::create(num(-INFINITY)));
    add("superellipse-0.5", SuperellipseStyleValue::create(num(0.5)));
    add("text-indent", TextIndentStyleValue::create(len(2, LengthUnit::Em), TextIndentStyleValue::Hanging::Yes, TextIndentStyleValue::EachLine::Yes));
    add("text-indent-plain", TextIndentStyleValue::create(pct(5), TextIndentStyleValue::Hanging::No, TextIndentStyleValue::EachLine::No));
    add("tup-auto-auto", TextUnderlinePositionStyleValue::create(TextUnderlinePositionHorizontal::Auto, TextUnderlinePositionVertical::Auto));
    add("tup-under", TextUnderlinePositionStyleValue::create(TextUnderlinePositionHorizontal::Under, TextUnderlinePositionVertical::Auto));
    add("tup-left", TextUnderlinePositionStyleValue::create(TextUnderlinePositionHorizontal::Auto, TextUnderlinePositionVertical::Left));
    add("tup-from-font-right", TextUnderlinePositionStyleValue::create(TextUnderlinePositionHorizontal::FromFont, TextUnderlinePositionVertical::Right));
    // Lists, tuples, functions and the rest
    add("list-space", StyleValueList::create({ len(1, LengthUnit::Px), len(2, LengthUnit::Em) }, StyleValueList::Separator::Space));
    add("list-space-same", StyleValueList::create({ len(3, LengthUnit::Px), len(3, LengthUnit::Px) }, StyleValueList::Separator::Space));
    add("list-space-same-not-collapsible", StyleValueList::create({ len(3, LengthUnit::Px), len(3, LengthUnit::Px) }, StyleValueList::Separator::Space, StyleValueList::Collapsible::No));
    add("list-comma", StyleValueList::create({ kw(Keyword::Auto), num(2), StringStyleValue::create("x"_fly_string) }, StyleValueList::Separator::Comma));
    add("list-empty", StyleValueList::create({}, StyleValueList::Separator::Comma));
    add("tuple", TupleStyleValue::create({ kw(Keyword::Normal), nullptr, len(1, LengthUnit::Em) }));
    add("function", FunctionStyleValue::create("scroll"_fly_string, len(2, LengthUnit::Em)));
    add("opacity-percentage", OpacityValueStyleValue::create(pct(40)));
    add("opacity-number", OpacityValueStyleValue::create(num(1.5)));
    add("opacity-half", OpacityValueStyleValue::create(num(0.5)));
    add("ott-feature-1", OpenTypeTaggedStyleValue::create(OpenTypeTaggedStyleValue::Mode::FontFeatureSettings, "liga"_fly_string, IntegerStyleValue::create(1)));
    add("ott-feature-0", OpenTypeTaggedStyleValue::create(OpenTypeTaggedStyleValue::Mode::FontFeatureSettings, "smcp"_fly_string, IntegerStyleValue::create(0)));
    add("ott-variation", OpenTypeTaggedStyleValue::create(OpenTypeTaggedStyleValue::Mode::FontVariationSettings, "wght"_fly_string, num(1)));
    add("ratio", RatioStyleValue::create(num(16), num(9)));
    add("anchor", AnchorStyleValue::create("--a"_fly_string, kw(Keyword::Top), len(5, LengthUnit::Px)));
    add("anchor-no-name", AnchorStyleValue::create({}, pct(50), {}));
    add("anchor-size", AnchorSizeStyleValue::create("--b"_fly_string, AnchorSize::Width, len(1, LengthUnit::Em)));
    add("anchor-size-empty", AnchorSizeStyleValue::create({}, {}, {}));
    add("anchor-size-only-size", AnchorSizeStyleValue::create({}, AnchorSize::SelfBlock, {}));
    add("content", ContentStyleValue::create(StyleValueList::create({ StringStyleValue::create("a"_fly_string), kw(Keyword::OpenQuote) }, StyleValueList::Separator::Space), StyleValueList::create({ StringStyleValue::create("alt"_fly_string) }, StyleValueList::Separator::Space)));
    add("content-no-alt", ContentStyleValue::create(StyleValueList::create({ StringStyleValue::create("b"_fly_string) }, StyleValueList::Separator::Space), {}));
    add("random-fixed", RandomValueSharingStyleValue::create_fixed(num(0.25)));
    add("random-auto-shared", RandomValueSharingStyleValue::create_auto("width 0"_fly_string, true));
    add("random-dashed", RandomValueSharingStyleValue::create_dashed_ident("--r"_fly_string, false));
    add("random-dashed-shared", RandomValueSharingStyleValue::create_dashed_ident("--r"_fly_string, true));
    add("sibling-count", TreeCountingFunctionStyleValue::create(TreeCountingFunctionStyleValue::TreeCountingFunction::SiblingCount, TreeCountingFunctionStyleValue::ComputedType::Integer));
    add("sibling-index", TreeCountingFunctionStyleValue::create(TreeCountingFunctionStyleValue::TreeCountingFunction::SiblingIndex, TreeCountingFunctionStyleValue::ComputedType::Number));
    add("image", ImageStyleValue::create(Web::CSS::URL("a.png"_string)));
    add("image-set", ImageSetStyleValue::create({ { ImageStyleValue::create(Web::CSS::URL("a.png"_string)), ResolutionStyleValue::create(Resolution(1, ResolutionUnit::X)), {} }, { ImageStyleValue::create(Web::CSS::URL("b.webp"_string)), ResolutionStyleValue::create(Resolution(2, ResolutionUnit::X)), "image/webp"_string }, { LinearGradientStyleValue::create(SideOrCorner::Bottom, two_stops, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::No, {}), ResolutionStyleValue::create(Resolution(3, ResolutionUnit::X)), "image/\"x\""_string } }));
    add("cursor", CursorStyleValue::create(ImageStyleValue::create(Web::CSS::URL("c.cur"_string)), num(2), num(3)));
    add("cursor-no-hotspot", CursorStyleValue::create(ImageStyleValue::create(Web::CSS::URL("d.cur"_string)), {}, {}));
    add("guaranteed-invalid", GuaranteedInvalidStyleValue::create());
    add("pending-substitution", PendingSubstitutionStyleValue::create(*num(1)));
    add("unresolved", UnresolvedStyleValue::create(component_values("  var(--x, 3px)  calc(1px + 2px) "sv), Parser::SubstitutionFunctionsPresence { .var = true }));
    add("unresolved-original-text", UnresolvedStyleValue::create(component_values("attr(x)"sv), Parser::SubstitutionFunctionsPresence { .attr = true }, "attr( x )"_string));
    return values;
}

static char const* type_name(StyleValue::Type type)
{
    switch (type) {
#define __ENUMERATE_CSS_STYLE_VALUE_TYPE(title_case, snake_case, style_value_class_name) \
    case StyleValue::Type::title_case:                                                   \
        return #snake_case;
        ENUMERATE_CSS_STYLE_VALUE_TYPES
#undef __ENUMERATE_CSS_STYLE_VALUE_TYPE
    }
    return "?";
}

// Whether a position has an edge without an offset (EdgeStyleValue::is_computationally_independent dereferences it).
static bool position_lacks_offset(PositionStyleValue const& position)
{
    return !position.edge_x()->m_properties.offset || !position.edge_y()->m_properties.offset;
}

// The values whose is_computationally_independent() VERIFY_NOT_REACHED()s or dereferences a missing edge offset.
static bool independence_verifies(StyleValue const& value)
{
    if (value.is_edge())
        return !value.as_edge().m_properties.offset;
    if (value.is_position())
        return position_lacks_offset(value.as_position());
    if (value.is_conic_gradient())
        return position_lacks_offset(*value.as_conic_gradient().m_properties.position);
    if (value.is_radial_gradient())
        return position_lacks_offset(*value.as_radial_gradient().m_properties.position);
    if (value.is_tuple()) {
        for (auto const& member : value.as_tuple().tuple()) {
            if (!member)
                return true;
        }
        return false;
    }
    return value.is_guaranteed_invalid() || value.is_pending_substitution() || value.is_unicode_range() || value.is_unresolved();
}

// The values whose absolutized() needs an element.
static bool absolutize_needs_element(StyleValue const& value)
{
    if (value.is_tree_counting_function())
        return true;
    if (value.is_random_value_sharing())
        return !value.as_random_value_sharing().m_fixed_value;
    return false;
}

static std::string tokens(StyleValue const& value)
{
    std::string text;
    bool first = true;
    for (auto const& token : value.tokenize()) {
        if (!first)
            text += " | ";
        first = false;
        text += s(token.to_debug_string());
    }
    return text;
}

int main()
{
    auto values = build_values();
    ComputationContext context { .length_resolution_context = length_context() };
    ComputationContext dark_context { .length_resolution_context = length_context(), .color_scheme = PreferredColorScheme::Dark };
    ColorResolutionContext light { .color_scheme = PreferredColorScheme::Light, .current_color = Color(10, 20, 30), .accent_color = {}, .document = nullptr, .calculation_resolution_context = { .length_resolution_context = length_context() } };
    ColorResolutionContext dark { .color_scheme = PreferredColorScheme::Dark, .current_color = Color(40, 50, 60, 70), .accent_color = Color(200, 100, 50), .document = nullptr, .calculation_resolution_context = { .length_resolution_context = length_context() } };

    put("# values");
    for (size_t i = 0; i < values.size(); ++i) {
        auto const& value = *values[i].value;
        put(n(i) + " " + values[i].label + " " + type_name(value.type()));
        put("  N " + s(value.to_string(SerializationMode::Normal)));
        put("  R " + s(value.to_string(SerializationMode::ResolvedValue)));
        put("  flags " + b(value.is_abstract_image()) + b(value.is_dimension()) + b(value.is_color_function()) + b(value.has_color()) + b(value.has_auto()) + b(value.is_css_wide_keyword()) + b(value.is_inherit()) + b(value.is_initial()) + b(value.is_unset()) + b(value.is_revert()) + b(value.is_revert_layer()) + " " + s(string_from_keyword(value.to_keyword())) + " " + (independence_verifies(value) ? std::string("-") : b(value.is_computationally_independent())));
        put("  colors " + color(value.to_color(light)) + " " + color(value.to_color(dark)));
        if (absolutize_needs_element(value)) {
            put("  abs -");
        } else {
            auto absolutized = value.absolutized(context);
            auto dark_absolutized = value.absolutized(dark_context);
            put("  abs " + b(absolutized.ptr() == &value) + " " + s(absolutized->to_string(SerializationMode::Normal)) + " ; " + s(absolutized->to_string(SerializationMode::ResolvedValue)) + " ; " + b(dark_absolutized.ptr() == &value) + " " + s(dark_absolutized->to_string(SerializationMode::Normal)));
        }
    }

    put("# equals");
    for (size_t i = 0; i < values.size(); ++i) {
        std::string row;
        for (size_t j = 0; j < values.size(); ++j)
            row += b(values[i].value->equals(*values[j].value));
        put(n(i) + " " + row);
    }
    // Equal values built separately.
    put("separately " + b(rgb(1, 2, 3)->equals(*rgb(1, 2, 3))) + b(len(2, LengthUnit::Em)->equals(*len(2, LengthUnit::Em))) + b(StyleValueList::create({ num(1), num(2) }, StyleValueList::Separator::Space)->equals(*StyleValueList::create({ num(1), num(2) }, StyleValueList::Separator::Space))) + b(StyleValueList::create({ num(1), num(2) }, StyleValueList::Separator::Space)->equals(*StyleValueList::create({ num(1), num(2) }, StyleValueList::Separator::Comma)))
        + b(RadialSizeStyleValue::create({ NonnullRefPtr<StyleValue const>(pct(1)) })->equals(*RadialSizeStyleValue::create({ NonnullRefPtr<StyleValue const>(pct(1)) }))) + b(RadialSizeStyleValue::create({ RadialExtent::ClosestCorner })->equals(*RadialSizeStyleValue::create({ RadialExtent::ClosestCorner })))
        + b(LinearGradientStyleValue::create(SideOrCorner::Right, { stop(rgb(1, 1, 1)) }, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::No, {})->equals(*LinearGradientStyleValue::create(SideOrCorner::Right, { stop(rgb(1, 1, 1)) }, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::No, {})))
        + b(ShorthandStyleValue::create(PropertyID::Margin, { PropertyID::MarginTop, PropertyID::MarginRight }, { len(1, LengthUnit::Px), len(2, LengthUnit::Px) })->equals(*ShorthandStyleValue::create(PropertyID::Margin, { PropertyID::MarginTop, PropertyID::MarginRight }, { len(1, LengthUnit::Px), len(2, LengthUnit::Px) })))
        + b(ShorthandStyleValue::create(PropertyID::Margin, { PropertyID::MarginTop }, { len(1, LengthUnit::Px) })->equals(*ShorthandStyleValue::create(PropertyID::Padding, { PropertyID::MarginTop }, { len(1, LengthUnit::Px) })))
        + b(ImageSetStyleValue::create({ { ImageStyleValue::create(Web::CSS::URL("a.png"_string)), ResolutionStyleValue::create(Resolution(1, ResolutionUnit::X)), "image/png"_string } })->equals(*ImageSetStyleValue::create({ { ImageStyleValue::create(Web::CSS::URL("a.png"_string)), ResolutionStyleValue::create(Resolution(1, ResolutionUnit::X)), "image/png"_string } }))));

    put("# tokenize");
    for (size_t i = 0; i < values.size(); ++i) {
        auto const& value = *values[i].value;
        if (value.is_unresolved() || value.is_guaranteed_invalid() || value.is_pending_substitution() || value.is_keyword() || value.is_number() || value.is_integer() || value.is_dimension() || value.is_string() || value.is_custom_ident() || value.is_ratio() || value.is_value_list() || value.is_color() || value.is_edge())
            put(n(i) + " " + tokens(value));
    }

    put("# helpers");
    put("int " + n(int_from_style_value(IntegerStyleValue::create(-12))));
    put("number " + d(number_from_style_value(num(2.5), {})) + " " + d(number_from_style_value(pct(25), 200)));
    put("string " + s(string_from_style_value(StringStyleValue::create("s"_fly_string))) + " " + s(string_from_style_value(CustomIdentStyleValue::create("c"_fly_string))));
    put("statics " + b(len(0, LengthUnit::Px).ptr() == len(0, LengthUnit::Px).ptr()) + b(len(1, LengthUnit::Px).ptr() == len(1, LengthUnit::Px).ptr()) + b(len(2, LengthUnit::Px).ptr() == len(2, LengthUnit::Px).ptr()) + b(len(0, LengthUnit::Em).ptr() == len(0, LengthUnit::Em).ptr())
        + b(kw(Keyword::Inherit).ptr() == kw(Keyword::Inherit).ptr()) + b(kw(Keyword::RevertLayer).ptr() == kw(Keyword::RevertLayer).ptr()) + b(kw(Keyword::Auto).ptr() == kw(Keyword::Auto).ptr()) + b(GuaranteedInvalidStyleValue::create().ptr() == GuaranteedInvalidStyleValue::create().ptr()));
    std::string is_color;
    for (u16 keyword = 0; keyword <= to_underlying(Keyword::ZoomOut); ++keyword)
        is_color += b(KeywordStyleValue::is_color(static_cast<Keyword>(keyword)));
    put("is_color " + is_color);
    put("supported image types " + b(Web::HTML::is_supported_image_type(""sv)) + b(Web::HTML::is_supported_image_type("image/PNG"sv)) + b(Web::HTML::is_supported_image_type("image/svg+xml"sv)) + b(Web::HTML::is_supported_image_type("IMAGE/webp"sv)) + b(Web::HTML::is_supported_image_type("text/html"sv)) + b(Web::HTML::is_supported_image_type("image/x"sv)));
    auto list = StyleValueList::create({ num(1), num(2), num(3) }, StyleValueList::Separator::Comma);
    put("value_at " + s(list->value_at(4, true)->to_string(SerializationMode::Normal)) + " " + s(list->value_at(2, false)->to_string(SerializationMode::Normal)) + " " + n(list->size()));
    put("subdivide " + n(list->subdivide_into_iterations(PropertyNameAndID::from_id(PropertyID::BackgroundImage)).size()) + " " + n(list->subdivide_into_iterations(PropertyNameAndID::from_id(PropertyID::Color)).size()) + " " + n(num(1)->subdivide_into_iterations(PropertyNameAndID::from_id(PropertyID::BackgroundImage)).size()));
    auto shorthand = ShorthandStyleValue::create(PropertyID::Margin, { PropertyID::MarginTop, PropertyID::MarginRight }, { len(1, LengthUnit::Px), len(2, LengthUnit::Px) });
    put("longhand " + s(shorthand->longhand(PropertyID::MarginRight)->to_string(SerializationMode::Normal)) + " " + b(shorthand->longhand(PropertyID::MarginLeft) == nullptr));
    put("radius " + b(BorderRadiusStyleValue::create(len(1, LengthUnit::Px), len(1, LengthUnit::Px))->is_elliptical()) + b(BorderRadiusStyleValue::create(len(1, LengthUnit::Px), len(2, LengthUnit::Px))->is_elliptical()));
    put("edges " + b(edge(PositionEdge::Center, {})->is_center(SerializationMode::Normal)) + b(edge({}, pct(50))->is_center(SerializationMode::Normal)) + b(edge(PositionEdge::Left, pct(50))->is_center(SerializationMode::Normal)) + b(edge(PositionEdge::Left, {})->is_center(SerializationMode::Normal)));
    for (auto e : { edge(PositionEdge::Center, {}), edge(PositionEdge::Left, {}), edge(PositionEdge::Right, {}), edge(PositionEdge::Top, len(1, LengthUnit::Px)), edge(PositionEdge::Bottom, {}), edge({}, pct(7)) })
        put("resolved edge " + s(e->with_resolved_keywords()->to_string(SerializationMode::Normal)));

    put("# geometry");
    double const sides[][2] = { { 100, 50 }, { 50, 100 }, { 0, 40 }, { 33.5, 77.25 } };
    for (auto const& size : sides) {
        CSSPixelSize gradient_size { CSSPixels(size[0]), CSSPixels(size[1]) };
        std::string line = "angles " + px(gradient_size.width()) + "x" + px(gradient_size.height());
        for (auto side : { SideOrCorner::Top, SideOrCorner::Bottom, SideOrCorner::Left, SideOrCorner::Right, SideOrCorner::TopLeft, SideOrCorner::TopRight, SideOrCorner::BottomLeft, SideOrCorner::BottomRight }) {
            line += " " + f(LinearGradientStyleValue::create(side, { stop(rgb(1, 1, 1)) }, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::No, {})->angle_degrees(gradient_size));
            line += "/" + f(LinearGradientStyleValue::create(side, { stop(rgb(1, 1, 1)) }, LinearGradientStyleValue::GradientType::WebKit, GradientRepeating::No, {})->angle_degrees(gradient_size));
        }
        line += " " + f(LinearGradientStyleValue::create(NonnullRefPtr<StyleValue const>(AngleStyleValue::create(Angle(1, AngleUnit::Rad))), { stop(rgb(1, 1, 1)) }, LinearGradientStyleValue::GradientType::Standard, GradientRepeating::No, {})->angle_degrees(gradient_size));
        put(line);
    }
    put("conic " + f(ConicGradientStyleValue::create({}, PositionStyleValue::create_center(), { stop(rgb(1, 1, 1)) }, GradientRepeating::No, {})->angle_degrees()) + " " + f(ConicGradientStyleValue::create(AngleStyleValue::create(Angle(0.5, AngleUnit::Turn)), PositionStyleValue::create_center(), { stop(rgb(1, 1, 1)) }, GradientRepeating::No, {})->angle_degrees()));
    // Radial sizes for extents only (the length-percentage branch needs a layout node).
    CSSPixelRect boxes[] = { CSSPixelRect(0, 0, 200, 100), CSSPixelRect(10, 20, 30, 300), CSSPixelRect(0, 0, 1, 50), CSSPixelRect(5, 5, 64, 0) };
    CSSPixelPoint centers[] = { CSSPixelPoint(100, 50), CSSPixelPoint(0, 0), CSSPixelPoint(15, 200), CSSPixelPoint(CSSPixels(33.25), CSSPixels(-7.5)) };
    auto& node = *reinterpret_cast<Web::Layout::Node const*>(&boxes[0]);
    for (auto extent : { RadialExtent::ClosestSide, RadialExtent::FarthestSide, RadialExtent::ClosestCorner, RadialExtent::FarthestCorner }) {
        auto radial_size = RadialSizeStyleValue::create({ extent });
        std::string line = "radial " + s(Web::CSS::to_string(extent));
        for (auto const& box : boxes) {
            for (auto const& center : centers) {
                auto ellipse = radial_size->resolve_ellipse_size(center, box, node);
                line += " " + px(radial_size->resolve_circle_size(center, box, node)) + ":" + px(ellipse.width()) + "," + px(ellipse.height());
            }
        }
        put(line);
    }

    fwrite(g_out.data(), 1, g_out.size(), stdout);
    return 0;
}
