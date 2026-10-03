// Oracle for luce-browser-engine region r32 (CSS ValueParsing and GradientParsing): runs the reference
// build's Web::CSS::Parser value parsers on the cases in cases.txt and prints what the Luce test
// (tests_css_value_parsing, against tests_css_value_parsing_cases) prints. Each case line is
// "<mode>\t<input>" with \n, \t and \\ escaped in <input>. A mode is "[option+]...<parser>[:<argument>]";
// the options are "svg" (an SVG presentation attribute) and "prop=<property>" (a property value context).
// After each case: the tokens the parser left ("rest: N") and the ErrorReporter's errors.
#define private public
#define protected public
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/Parser/ErrorReporter.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/CSS/StyleValues/CustomIdentStyleValue.h>
#include <LibWeb/CSS/StyleValues/RadialSizeStyleValue.h>
#include <LibWeb/CSS/StyleValues/BorderRadiusRectStyleValue.h>
#include <LibWeb/CSS/StyleValues/ColorInterpolationMethodStyleValue.h>
#include <LibWeb/CSS/StyleValues/UnicodeRangeStyleValue.h>
#include <LibWeb/CSS/StyleValues/RandomValueSharingStyleValue.h>
#include <LibWeb/CSS/StyleValues/StyleValueList.h>
#include <LibWeb/CSS/PropertyID.h>
#include <LibWeb/CSS/ValueType.h>
#include <LibWeb/CSS/GridTrackSize.h>
#include <LibWeb/CSS/URL.h>
#include <LibWeb/CSS/Serialize.h>
#undef private
#undef protected
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace Web::CSS::Parser;
using namespace Web::CSS;

static std::string g_out;
static void line(String const& text)
{
    g_out += "  ";
    g_out += std::string(text.bytes_as_string_view().characters_without_null_termination(), text.bytes_as_string_view().length());
    g_out += "\n";
}

static void dump_errors()
{
    for (auto const& [error, metadata] : ErrorReporter::the().m_errors)
        line(MUST(String::formatted("error: {} ({})", serialize_parsing_error(error), metadata.occurrences)));
}

static void value(RefPtr<StyleValue const> const& v)
{
    if (!v)
        line("none"_string);
    else
        line(MUST(String::formatted("value: {}", v->to_string(SerializationMode::Normal))));
}

static void fly(Optional<FlyString> const& v)
{
    line(v.has_value() ? MUST(String::formatted("'{}'", *v)) : "none"_string);
}

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
        value(parser.parse_value(value_type_named(argument_view), tokens));
    } else if (mode == "keyword") {
        value(parser.parse_keyword_value(tokens));
    } else if (mode == "builtin") {
        value(parser.parse_builtin_value(tokens));
    } else if (mode == "family") {
        value(parser.parse_family_name_value(tokens));
    } else if (mode == "cim") {
        value(parser.parse_color_interpolation_method_value(tokens));
    } else if (mode == "radialsize") {
        value(parser.parse_radial_size(tokens));
    } else if (mode == "borderradius") {
        value(parser.parse_border_radius_rect_value(tokens));
    } else if (mode == "random") {
        value(parser.parse_random_value_sharing(tokens));
    } else if (mode == "numberlist") {
        value(parser.parse_comma_separated_value_list(tokens, [&](auto& it) { return parser.parse_number_value(it, infinite_range); }));
    } else if (mode == "urangevalue") {
        value(parser.parse_unicode_range_value(tokens));
    } else if (mode == "urange" || mode == "urangetext") {
        auto range = mode == "urange" ? parser.parse_unicode_range(tokens) : parser.parse_unicode_range(input);
        line(range.has_value() ? MUST(String::formatted("{}-{}", range->min_code_point(), range->max_code_point())) : "none"_string);
    } else if (mode == "uranges") {
        auto ranges = parser.parse_unicode_ranges(tokens);
        StringBuilder builder;
        for (auto& range : ranges)
            builder.appendff("{}-{} ", range.min_code_point(), range.max_code_point());
        line(MUST(String::formatted("{} ranges: {}", ranges.size(), builder.string_view())));
    } else if (mode == "declvalue") {
        auto result = Web::CSS::Parser::Parser::parse_declaration_value(tokens, argument.empty() ? Optional<Token::Type> {} : Token::Type::Comma);
        if (!result.has_value()) {
            line("none"_string);
        } else {
            StringBuilder builder;
            for (auto& component_value : *result)
                builder.appendff("[{}]", component_value.to_string());
            line(MUST(String::formatted("{}: {}", result->size(), builder.string_view())));
        }
    } else if (mode == "url") {
        auto url = parser.parse_url_function(tokens);
        line(url.has_value() ? MUST(String::formatted("url: {} / {}", url->to_string(), url->url())) : "none"_string);
    } else if (mode == "customident") {
        fly(parser.parse_custom_ident(tokens, { { "none"sv } }));
    } else if (mode == "dashedident") {
        fly(parser.parse_dashed_ident(tokens));
    } else if (mode == "counterstylename") {
        fly(parser.parse_counter_style_name(tokens));
    } else if (mode == "linenames") {
        auto names = parser.parse_grid_line_names(tokens);
        line(names.has_value() ? MUST(String::formatted("names: '{}'", names->to_string())) : "none"_string);
    } else if (mode == "tracklist" || mode == "autotracklist" || mode == "explicittracklist") {
        auto list = mode == "tracklist" ? parser.parse_grid_track_list(tokens) : mode == "autotracklist" ? parser.parse_grid_auto_track_list(tokens) : parser.parse_explicit_track_list(tokens);
        line(MUST(String::formatted("list: '{}'", list.to_string(SerializationMode::Normal))));
    } else {
        line("unknown mode"_string);
    }
    line(MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    dump_errors();
}

int main(int argc, char** argv)
{
    std::ifstream in(argc > 1 ? argv[1] : "cases.txt");
    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty() || raw[0] == '#')
            continue;
        // A leading "!" marks a case waiting for another region (r33, r35): it runs here, the Luce table skips it.
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
