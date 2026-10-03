// Oracle for luce-browser-engine region r35 (CSS style values II: CalculatedStyleValue, NumericType and the
// basic-shape, counter, easing, filter, font, grid and transformation style values). For each case of cases.txt
// ("<mode>\t<input>", \n \t \\ escaped) it parses the input with the reference build's value parser and prints
// what the Luce test (css/style_values/tests_style_values_2) prints: the serializations in both modes,
// computational independence, the absolutized value, and for a CalculatedStyleValue its calculation dump, its
// resolved type, its resolves_to_* flags and the resolve_* results in three resolution contexts.
// Modes are r32's "[option+]type:<value-type>" (options "svg" and "prop=<property>").
#define private public
#define protected public
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/Parser/ErrorReporter.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/CSS/StyleValues/CalculatedStyleValue.h>
#include <LibWeb/CSS/StyleValues/StyleValueList.h>
#include <LibWeb/CSS/StyleValues/TransformationStyleValue.h>
#include <LibWeb/CSS/StyleValues/EasingStyleValue.h>
#include <LibWeb/CSS/StyleValues/BasicShapeStyleValue.h>
#include <LibWeb/CSS/StyleValues/ComputationContext.h>
#include <LibWeb/CSS/CalculationResolutionContext.h>
#include <LibWeb/CSS/PropertyID.h>
#include <LibWeb/CSS/ValueType.h>
#include <LibWeb/CSS/Serialize.h>
#include <LibWeb/CSS/NumericType.h>
#include <LibWeb/CSS/DescriptorID.h>
#include <LibWeb/CSS/DescriptorNameAndID.h>
#include <LibWeb/CSS/StyleValues/CounterStyleSystemStyleValue.h>
#undef private
#undef protected
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace Web::CSS::Parser;
using namespace Web::CSS;
using Web::CSSPixels;
using Web::CSSPixelRect;

static std::string g_out;
static void line(String const& text)
{
    g_out += "  ";
    g_out += std::string(text.bytes_as_string_view().characters_without_null_termination(), text.bytes_as_string_view().length());
    g_out += "\n";
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

static Length::ResolutionContext length_context() { return Length::ResolutionContext { CSSPixelRect(0, 0, 800, 600), font_metrics(), root_font_metrics() }; }

// value_type_from_string() knows neither `anchor`, `corner-shape` nor `font-style`.
static ValueType value_type_named(StringView name)
{
    if (name == "anchor"sv)
        return ValueType::Anchor;
    if (name == "corner-shape"sv)
        return ValueType::CornerShape;
    if (name == "font-style"sv)
        return ValueType::FontStyle;
    return value_type_from_string(name).value();
}

// The percentage basis for a calculation's context: 200px, 90deg, 2s or 100hz.
static CalculationResolutionContext::PercentageBasis basis_for(Optional<ValueType> resolve_as)
{
    if (resolve_as == ValueType::Length)
        return Length::make_px(200);
    if (resolve_as == ValueType::Angle)
        return Angle::make_degrees(90);
    if (resolve_as == ValueType::Time)
        return Time::make_seconds(2);
    if (resolve_as == ValueType::Frequency)
        return Frequency::make_hertz(100);
    return {};
}

static String opt_double(Optional<double> value)
{
    return value.has_value() ? MUST(String::formatted("{}", *value)) : "-"_string;
}

// Whether resolve_value() VERIFYs in this context: the simplified tree is a numeric value that
// try_get_value_with_canonical_unit() cannot read (a non-canonical unit or an unresolved percentage).
static bool resolve_would_verify(CalculatedStyleValue const& calc, CalculationResolutionContext const& context)
{
    auto simplified = simplify_a_calculation_tree(calc.m_calculation, calc.m_context, context);
    if (!is<NumericCalculationNode>(*simplified))
        return false;
    auto const& numeric = as<NumericCalculationNode>(*simplified);
    return !numeric.is_in_canonical_unit() || (numeric.value().has<Percentage>() && calc.m_context.percentages_resolve_as.has_value()) || !numeric.numeric_type().has_value();
}

static void resolves(CalculatedStyleValue const& calc, char const* name, CalculationResolutionContext const& context)
{
    if (resolve_would_verify(calc, context)) {
        line(MUST(String::formatted("{}: verifies", name)));
        return;
    }
    StringBuilder b;
    b.appendff("{}: number={} integer={} length={} raw_length={} percentage={} angle={} time={} frequency={} resolution={} flex={}", name,
        opt_double(calc.resolve_number(context)),
        calc.resolve_integer(context).has_value() ? MUST(String::formatted("{}", *calc.resolve_integer(context))) : "-"_string,
        calc.resolve_length(context).has_value() ? calc.resolve_length(context)->to_string() : "-"_string,
        opt_double(calc.resolve_raw_length(context)),
        calc.resolve_percentage(context).has_value() ? calc.resolve_percentage(context)->to_string() : "-"_string,
        calc.resolve_angle(context).has_value() ? calc.resolve_angle(context)->to_string() : "-"_string,
        calc.resolve_time(context).has_value() ? calc.resolve_time(context)->to_string() : "-"_string,
        calc.resolve_frequency(context).has_value() ? calc.resolve_frequency(context)->to_string() : "-"_string,
        calc.resolve_resolution(context).has_value() ? calc.resolve_resolution(context)->to_string() : "-"_string,
        calc.resolve_flex(context).has_value() ? calc.resolve_flex(context)->to_string() : "-"_string);
    line(b.to_string_without_validation());
}

static void calculated(CalculatedStyleValue const& calc)
{
    auto dumped = calc.dump();
    for (auto dump_line : dumped.bytes_as_string_view().split_view('\n'))
        line(MUST(String::formatted("| {}", dump_line)));
    line(MUST(String::formatted("type: {}", calc.m_resolved_type.dump())));
    line(MUST(String::formatted("flags: angle={:d}{:d} flex={:d} frequency={:d}{:d} length={:d}{:d} percentage={:d} resolution={:d} time={:d}{:d} number={:d} dimension={:d} contains_percentage={:d}",
        calc.resolves_to_angle(), calc.resolves_to_angle_percentage(), calc.resolves_to_flex(), calc.resolves_to_frequency(), calc.resolves_to_frequency_percentage(),
        calc.resolves_to_length(), calc.resolves_to_length_percentage(), calc.resolves_to_percentage(), calc.resolves_to_resolution(),
        calc.resolves_to_time(), calc.resolves_to_time_percentage(), calc.resolves_to_number(), calc.resolves_to_dimension(), calc.contains_percentage())));
    resolves(calc, "empty", {});
    resolves(calc, "length", CalculationResolutionContext { .length_resolution_context = length_context() });
    resolves(calc, "basis", CalculationResolutionContext { .percentage_basis = basis_for(calc.m_context.percentages_resolve_as), .length_resolution_context = length_context() });
}

static void matrix(TransformationStyleValue const& transformation)
{
    auto result = transformation.to_matrix({});
    if (result.is_error()) {
        line(MUST(String::formatted("matrix: error {}", result.error().string_literal())));
        return;
    }
    StringBuilder b;
    b.append("matrix:"sv);
    for (size_t row = 0; row < 4; ++row)
        for (size_t column = 0; column < 4; ++column)
            b.appendff(" {}", result.value()[row, column]);
    line(b.to_string_without_validation());
}

// Whether a position has an edge without an offset (EdgeStyleValue::is_computationally_independent VERIFYs then).
static bool position_lacks_offset(RefPtr<StyleValue const> const& position)
{
    if (!position)
        return false;
    auto const& p = position->as_position();
    return !p.edge_x()->m_properties.offset || !p.edge_y()->m_properties.offset;
}

// Whether is_computationally_independent() VERIFYs: a circle() or ellipse() whose position lacks an edge offset.
static bool independence_verifies(StyleValue const& v)
{
    if (!v.is_basic_shape())
        return false;
    return v.as_basic_shape().basic_shape().visit(
        [](Circle const& circle) { return position_lacks_offset(circle.position); },
        [](Ellipse const& ellipse) { return position_lacks_offset(ellipse.position); },
        [](auto const&) { return false; });
}

static void value(RefPtr<StyleValue const> const& v, RefPtr<StyleValue const> const& reparsed = {})
{
    if (!v) {
        line("none"_string);
        return;
    }
    line(MUST(String::formatted("value: {}", v->to_string(SerializationMode::Normal))));
    line(MUST(String::formatted("resolved: {}", v->to_string(SerializationMode::ResolvedValue))));
    if (reparsed)
        line(MUST(String::formatted("reparsed equal: {:d}", v->equals(*reparsed))));
    if (v->is_transformation())
        matrix(v->as_transformation());
    if (v->is_value_list()) {
        for (auto const& item : v->as_value_list().values()) {
            if (item->is_transformation())
                matrix(item->as_transformation());
        }
    }
    if (v->is_counter_style_system()) {
        // is_computationally_independent() VERIFYs for this class.
        auto const& system = v->as_counter_style_system();
        line(MUST(String::formatted("symbol counts: {:d}{:d}{:d} additive: {:d}{:d}", system.is_valid_symbol_count(0), system.is_valid_symbol_count(1), system.is_valid_symbol_count(2), system.is_valid_additive_symbol_count(0), system.is_valid_additive_symbol_count(1))));
    } else if (independence_verifies(*v)) {
        line("independent: verifies"_string);
    } else {
        line(MUST(String::formatted("independent: {:d}", v->is_computationally_independent())));
    }
    ComputationContext context { .length_resolution_context = length_context() };
    auto absolutized = v->absolutized(context);
    line(MUST(String::formatted("absolutized: {} (same: {:d}, equal: {:d})", absolutized->to_string(SerializationMode::Normal), absolutized.ptr() == v.ptr(), absolutized->equals(*v))));
    if (v->is_calculated()) {
        calculated(v->as_calculated());
        if (absolutized->is_calculated())
            line(MUST(String::formatted("absolutized dump: {}", MUST(absolutized->as_calculated().dump().replace("\n"sv, "/"sv, ReplaceMode::All)))));
    }
}

// "length=1 percent=-1 hint=length" (empty: the number type).
static NumericType numeric_type_from(StringView text)
{
    NumericType type;
    for (auto part : text.split_view(' ')) {
        auto eq = part.find('=').value();
        auto key = part.substring_view(0, eq);
        auto value = part.substring_view(eq + 1);
        auto base_type_named = [](StringView name) -> NumericType::BaseType {
            for (auto i = 0; i < to_underlying(NumericType::BaseType::__Count); ++i) {
                if (NumericType::base_type_name(static_cast<NumericType::BaseType>(i)) == name)
                    return static_cast<NumericType::BaseType>(i);
            }
            VERIFY_NOT_REACHED();
        };
        if (key == "hint"sv)
            type.set_percent_hint(base_type_named(value));
        else
            type.set_exponent(base_type_named(key), value.to_number<int>().value());
    }
    return type;
}

static void optional_type(char const* label, Optional<NumericType> const& type)
{
    line(MUST(String::formatted("{}: {}", label, type.has_value() ? type->dump() : "failure"_string)));
}

// NumericType operations: argument is the operation, input "a|b".
static void numeric_type_case(StringView operation, StringView input)
{
    auto bar = input.find('|');
    bool typed = operation != "unit"sv && operation != "unitmap"sv;
    auto a = typed ? numeric_type_from(input.substring_view(0, bar.value_or(input.length())).trim_whitespace()) : NumericType {};
    auto b = typed && bar.has_value() ? numeric_type_from(input.substring_view(*bar + 1).trim_whitespace()) : NumericType {};
    if (operation == "add"sv) {
        optional_type("added", a.added_to(b));
        line(MUST(String::formatted("consistent: {:d}", a.has_consistent_type_with(b))));
    } else if (operation == "multiply"sv) {
        optional_type("multiplied", a.multiplied_by(b));
    } else if (operation == "invert"sv) {
        optional_type("inverted", a.inverted());
    } else if (operation == "consistent"sv) {
        optional_type("made consistent", a.made_consistent_with(b));
    } else if (operation == "matches"sv) {
        Optional<ValueType> resolve_as_values[] = { {}, ValueType::Length, ValueType::Number, ValueType::Angle, ValueType::Percentage };
        for (auto resolve_as : resolve_as_values) {
            line(MUST(String::formatted("{}: angle={:d}{:d} flex={:d} frequency={:d}{:d} length={:d}{:d} number={:d} percentage={:d} resolution={:d} time={:d}{:d} dimension={:d} entry={}",
                resolve_as.has_value() ? value_type_to_string(*resolve_as) : "none"sv,
                a.matches_angle(resolve_as), a.matches_angle_percentage(resolve_as), a.matches_flex(resolve_as), a.matches_frequency(resolve_as), a.matches_frequency_percentage(resolve_as),
                a.matches_length(resolve_as), a.matches_length_percentage(resolve_as), a.matches_number(resolve_as), a.matches_percentage(), a.matches_resolution(resolve_as),
                a.matches_time(resolve_as), a.matches_time_percentage(resolve_as), a.matches_dimension(),
                a.entry_with_value_1_while_all_others_are_0().map([](auto t) { return NumericType::base_type_name(t); }))));
        }
    } else if (operation == "unit"sv) {
        optional_type("from unit", NumericType::create_from_unit(MUST(FlyString::from_utf8(input))));
    } else if (operation == "unitmap"sv) {
        UnitMap map;
        for (auto part : input.split_view(' ')) {
            auto eq = part.find('=').value();
            map.set(MUST(FlyString::from_utf8(part.substring_view(0, eq))), part.substring_view(eq + 1).to_number<int>().value());
        }
        optional_type("from unit map", NumericType::create_from_unit_map(map));
    } else {
        line("unknown operation"_string);
    }
}

static void run(std::string const& full_mode, StringView input)
{
    ErrorReporter::the().m_errors.clear();

    ParsingParams params;
    std::string mode = full_mode;
    Optional<PropertyID> property;
    while (true) {
        auto plus = mode.find('+');
        if (plus == std::string::npos)
            break;
        auto option = mode.substr(0, plus);
        mode = mode.substr(plus + 1);
        if (option == "svg")
            params = ParsingParams { ParsingMode::SVGPresentationAttribute };
        else if (option.rfind("prop=", 0) == 0)
            property = property_id_from_string(StringView(option.data() + 5, option.size() - 5));
    }
    std::string argument;
    if (auto colon = mode.find(':'); colon != std::string::npos) {
        argument = mode.substr(colon + 1);
        mode = mode.substr(0, colon);
    }
    auto argument_view = StringView(argument.data(), argument.size());

    auto parser = Web::CSS::Parser::Parser::create(params, input);
    auto values = parser.parse_as_list_of_component_values();
    Web::CSS::Parser::TokenStream tokens { values };
    if (property.has_value())
        parser.m_value_context.append(*property);

    if (mode == "type") {
        auto reparser = Web::CSS::Parser::Parser::create(params, input);
        auto reparsed_values = reparser.parse_as_list_of_component_values();
        Web::CSS::Parser::TokenStream reparsed_tokens { reparsed_values };
        if (property.has_value())
            reparser.m_value_context.append(*property);
        auto reparsed = reparser.parse_value(value_type_named(argument_view), reparsed_tokens);
        value(parser.parse_value(value_type_named(argument_view), tokens), reparsed);
    } else if (mode == "prop") {
        auto id = property_id_from_string(argument_view).value();
        auto reparser = Web::CSS::Parser::Parser::create(params, input);
        auto reparsed_values = reparser.parse_as_list_of_component_values();
        Web::CSS::Parser::TokenStream reparsed_tokens { reparsed_values };
        auto reparsed = reparser.parse_css_value(id, reparsed_tokens);
        auto result = parser.parse_css_value(id, tokens);
        if (result.is_error())
            line("error"_string);
        else
            value(result.value(), reparsed.is_error() ? nullptr : RefPtr<StyleValue const> { reparsed.value() });
    } else if (mode == "desc") {
        auto slash = argument_view.find('/').value();
        auto at_rule_name = argument_view.substring_view(0, slash);
        auto at_rule = at_rule_name == "counter-style"sv ? AtRuleID::CounterStyle : AtRuleID::FontFace;
        auto descriptor = DescriptorNameAndID::from_name(at_rule, MUST(FlyString::from_utf8(argument_view.substring_view(slash + 1)))).value();
        auto result = parser.parse_descriptor_value(at_rule, descriptor, tokens);
        if (result.is_error())
            line("error"_string);
        else
            value(result.value());
    } else if (mode == "ntype") {
        numeric_type_case(argument_view, input);
    } else {
        line("unknown mode"_string);
    }
    line(MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    for (auto const& [error, metadata] : ErrorReporter::the().m_errors)
        line(MUST(String::formatted("error: {} ({})", serialize_parsing_error(error), metadata.occurrences)));
}

int main(int argc, char** argv)
{
    std::ifstream in(argc > 1 ? argv[1] : "cases.txt");
    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty() || raw[0] == '#')
            continue;
        // A leading "!" marks a case waiting for another region: it runs here, the Luce table skips it.
        auto tab = raw.find('\t');
        std::string mode = raw.substr(0, tab);
        std::string escaped = raw.substr(tab + 1), input;
        for (size_t i = 0; i < escaped.size(); i++) {
            if (escaped[i] == '\\' && i + 1 < escaped.size()) {
                char c = escaped[++i];
                input += c == 'n' ? '\n' : c == 't' ? '\t' : c;
            } else {
                input += escaped[i];
            }
        }
        g_out += "case " + mode + "\t" + escaped + "\n";
        run(mode[0] == '!' ? mode.substr(1) : mode, StringView(input.data(), input.size()));
    }
    std::cout << g_out;
}
