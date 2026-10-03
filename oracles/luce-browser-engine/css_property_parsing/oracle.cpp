// Oracle for luce-browser-engine region r33 (CSS property parsing, arbitrary substitution
// functions, and the generated value-dependent property data): runs the cases of cases.txt
// through the reference build's Web::CSS parser and prints what the Luce test
// (tests_css_property_parsing_cases) compares. Each case line is "<mode>\t<arg>\t<arg>..." with
// \n, \t and \\ escaped in the arguments; a line starting with "!" is a case whose path reaches
// other regions' stubs in the port (gen_luce_cases.py leaves it out).
#define private public
#define protected public
#include <LibWeb/CSS/DescriptorID.h>
#include <LibWeb/CSS/DescriptorNameAndID.h>
#include <LibWeb/CSS/EnvironmentVariable.h>
#include <LibWeb/CSS/Parser/ArbitrarySubstitutionFunctions.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/PropertyID.h>
#include <LibWeb/CSS/StyleValues/ShorthandStyleValue.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/CSS/StyleValues/StyleValueList.h>
#include <LibWeb/CSS/ValueType.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibCore/EventLoop.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#undef private
#undef protected
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web::CSS;
using namespace Web;

static std::string g_out;
static void line(int indent, StringView text)
{
    for (int i = 0; i < indent; i++)
        g_out += "  ";
    g_out += std::string(text.characters_without_null_termination(), text.length());
    g_out += "\n";
}
static void line(int indent, String const& text) { line(indent, text.bytes_as_string_view()); }
static void line(int indent, std::string const& text) { line(indent, StringView { text.data(), text.size() }); }
static std::string s(String const& string) { return std::string(string.bytes_as_string_view().characters_without_null_termination(), string.bytes().size()); }
static std::string s(StringView view) { return std::string(view.characters_without_null_termination(), view.length()); }
static std::string d(double value) { return s(MUST(String::formatted("{}", value))); }

static Parser::ParsingParams params()
{
    Parser::ParsingParams p { Web::internal_css_realm() };
    return p;
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

// A value: its type and serialization; a shorthand's longhands and a list's items, recursively
// (a shorthand's own serialization is not printed: it needs the cascade's helpers).
static void print_value(int indent, StyleValue const& value)
{
    if (value.is_shorthand()) {
        auto const& shorthand = value.as_shorthand();
        line(indent, MUST(String::formatted("shorthand {}", string_from_property_id(shorthand.m_properties.shorthand_property))));
        for (size_t i = 0; i < shorthand.sub_properties().size(); i++) {
            line(indent + 1, string_from_property_id(shorthand.sub_properties()[i]).to_string());
            print_value(indent + 2, shorthand.values()[i]);
        }
        return;
    }
    if (value.is_value_list()) {
        auto const& list = value.as_value_list();
        bool has_shorthand = false;
        for (auto const& item : list.values())
            has_shorthand |= item->is_shorthand();
        std::string header = std::string("value_list ") + (list.separator() == StyleValueList::Separator::Comma ? "comma " : "space ") + std::to_string(list.size());
        if (!has_shorthand)
            header += " " + s(value.to_string(SerializationMode::Normal));
        line(indent, header);
        for (auto const& item : list.values())
            print_value(indent + 1, item);
        return;
    }
    line(indent, MUST(String::formatted("{} {}", type_name(value.type()), value.to_string(SerializationMode::Normal))));
}

static Optional<PropertyID> property_named(StringView name)
{
    if (name == "custom"sv)
        return PropertyID::Custom;
    return property_id_from_string(name);
}

static constexpr double samples[] = { -1, 0, 0.5, 1, 100, 1000 };

// The generated data of one property: accepted types, ranges, the percentage basis and the
// range checks at the sample values.
static void print_property(PropertyID id)
{
    std::string accepts = "accepts";
    std::string ranges = "ranges";
    auto accepted_ranges = property_accepted_ranges_by_value_type(id);
    for (int type = 0; type <= to_underlying(ValueType::ViewTimelineInset); type++) {
        auto value_type = static_cast<ValueType>(type);
        if (property_accepts_type(id, value_type))
            accepts += " " + s(value_type_to_string(value_type));
        if (auto range = accepted_ranges.get(value_type); range.has_value())
            ranges += " " + s(value_type_to_string(value_type)) + "=" + d(range->min) + "," + d(range->max);
    }
    line(1, accepts);
    line(1, ranges);
    auto percentages = property_resolves_percentages_relative_to(id);
    line(1, "percentages " + (percentages.has_value() ? s(value_type_to_string(*percentages)) : std::string("none")));
    std::string bounds = "bounds";
    auto put = [&](char const* name, auto check) {
        bounds += std::string(" ") + name + "=";
        for (auto sample : samples)
            bounds += check(sample) ? "1" : "0";
    };
    put("angle", [&](double v) { return property_accepts_angle(id, Angle::make_degrees(v)); });
    put("flex", [&](double v) { return property_accepts_flex(id, Flex::make_fr(v)); });
    put("frequency", [&](double v) { return property_accepts_frequency(id, Frequency::make_hertz(v)); });
    put("length", [&](double v) { return property_accepts_length(id, Length::make_px(v)); });
    put("percentage", [&](double v) { return property_accepts_percentage(id, Percentage(v)); });
    put("resolution", [&](double v) { return property_accepts_resolution(id, Resolution::make_dots_per_pixel(v)); });
    put("time", [&](double v) { return property_accepts_time(id, Time::make_seconds(v)); });
    line(1, bounds);
}

// The component values of `text`, as the parser makes them.
static Vector<Parser::ComponentValue> component_values(Parser::Parser& parser)
{
    return parser.parse_a_list_of_component_values(parser.m_token_stream);
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    auto arg = [&](size_t i) { return i < args.size() ? StringView { args[i].data(), args[i].size() } : StringView {}; };
    if (mode == "property") {
        auto id = property_named(arg(1));
        if (!id.has_value()) {
            line(1, "unknown property");
            return;
        }
        print_property(*id);
    } else if (mode == "envvar") {
        auto variable = environment_variable_from_string(arg(1));
        line(1, "type " + s(value_type_to_string(environment_variable_type(*variable))));
    } else if (mode == "asf") {
        auto function = Parser::to_arbitrary_substitution_function(MUST(FlyString::from_utf8(arg(1))));
        line(1, function.has_value() ? "function " + std::to_string(to_underlying(*function)) : std::string("none"));
    } else if (mode == "value") {
        auto id = property_named(arg(1));
        auto parser = Parser::Parser::create(params(), arg(2));
        auto values = component_values(parser);
        Parser::TokenStream tokens { values };
        auto result = parser.parse_css_value(*id, tokens);
        if (result.is_error()) {
            line(1, result.error() == Parser::Parser::ParseError::SyntaxError ? "error SyntaxError" : "error IncludesIgnoredVendorPrefix");
            return;
        }
        print_value(1, result.value());
        line(1, "rest " + std::to_string(tokens.remaining_token_count()));
    } else if (mode == "for") {
        Vector<PropertyID> ids;
        for (auto name : arg(1).split_view(' '))
            ids.append(*property_named(name));
        auto parser = Parser::Parser::create(params(), arg(2));
        auto values = component_values(parser);
        Parser::TokenStream tokens { values };
        auto result = parser.parse_css_value_for_properties(ids, tokens);
        if (!result.has_value()) {
            line(1, "none");
        } else {
            line(1, "property " + s(string_from_property_id(result->property).to_string()));
            print_value(2, *result->style_value);
        }
        line(1, "rest " + std::to_string(tokens.remaining_token_count()));
    } else if (mode == "initial") {
        print_value(1, property_initial_value(*property_named(arg(1))));
    } else if (mode == "descriptor_initial") {
        Optional<AtRuleID> at_rule;
        for (int i = 0; i < 5; i++) {
            if (to_string(static_cast<AtRuleID>(i)) == arg(1))
                at_rule = static_cast<AtRuleID>(i);
        }
        auto descriptor = descriptor_id_from_string(*at_rule, arg(2));
        auto value = descriptor_initial_value(*at_rule, *descriptor);
        if (value)
            print_value(1, *value);
        else
            line(1, "null");
    } else if (mode == "grammar") {
        auto function = Parser::to_arbitrary_substitution_function(MUST(FlyString::from_utf8(arg(1))));
        auto parser = Parser::Parser::create(params(), arg(2));
        auto values = component_values(parser);
        auto arguments = Parser::parse_according_to_argument_grammar(*function, values);
        if (!arguments.has_value()) {
            line(1, "failure");
            return;
        }
        arguments->visit(
            [&](Parser::DeclarationValueList const& list) {
                line(1, "declaration values " + std::to_string(list.size()));
                for (auto const& value : list)
                    line(2, "[" + s(MUST(String::join(""sv, value))) + "]");
            },
            [&](Parser::IfArgs const& branches) {
                line(1, "if branches " + std::to_string(branches.size()));
                for (auto const& branch : branches)
                    line(2, "[" + s(MUST(String::join(""sv, branch.condition))) + "] : " + (branch.value.has_value() ? "[" + s(MUST(String::join(""sv, *branch.value))) + "]" : std::string("none")));
            });
    } else {
        line(1, "unknown mode");
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
