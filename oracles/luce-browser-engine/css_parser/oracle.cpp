// Oracle for luce-browser-engine region r30b (CSS Parser core): runs the reference build's
// Web::CSS::Parser::Parser on the cases in cases.txt and prints what the Luce tests compare.
// Each case line is "<mode>\t<input>" with \n, \t and \\ escaped in <input>.
#define private public
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/Parser/ErrorReporter.h>
#include <LibWeb/CSS/Serialize.h>
#undef private
#include <AK/ByteBuffer.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace Web::CSS::Parser;
using namespace Web::CSS;

static std::string g_out;
static void line(int indent, String const& text)
{
    for (int i = 0; i < indent; i++)
        g_out += "  ";
    g_out += std::string(text.bytes_as_string_view().characters_without_null_termination(), text.bytes_as_string_view().length());
    g_out += "\n";
}

static String ser(Vector<ComponentValue> const& values) { return serialize_a_series_of_component_values(values); }

static void dump_declaration(Declaration const& d, int indent)
{
    line(indent, MUST(String::formatted("{}: [{}] important:{} original:{}", d.name, ser(d.value), d.important == Important::Yes ? 1 : 0,
                                        d.original_value_text.has_value() ? d.original_value_text.value() : "-"_string)));
}
static void dump_rule(Rule const& rule, int indent);
static void dump_rule_or_list(RuleOrListOfDeclarations const& item, int indent)
{
    item.visit([&](Rule const& r) { dump_rule(r, indent); },
        [&](Vector<Declaration> const& list) {
            line(indent, "declarations:"_string);
            for (auto& d : list)
                dump_declaration(d, indent + 1);
        });
}
static void dump_rule(Rule const& rule, int indent)
{
    rule.visit([&](AtRule const& a) {
            line(indent, MUST(String::formatted("@{} prelude:[{}] block:{}", a.name, ser(a.prelude), a.is_block_rule ? 1 : 0)));
            for (auto& c : a.child_rules_and_lists_of_declarations)
                dump_rule_or_list(c, indent + 1); },
        [&](QualifiedRule const& q) {
            line(indent, MUST(String::formatted("qualified prelude:[{}]", ser(q.prelude))));
            for (auto& d : q.declarations)
                dump_declaration(d, indent + 1);
            for (auto& c : q.child_rules)
                dump_rule_or_list(c, indent + 1); });
}
static void dump_values(Vector<ComponentValue> const& values, int indent)
{
    for (auto& v : values)
        line(indent, v.to_debug_string());
    line(indent, MUST(String::formatted("= {}", ser(values))));
}

static ParsingParams params(bool style)
{
    ParsingParams p;
    if (style)
        p.rule_context.append(RuleContext::Style);
    return p;
}

static void run(std::string const& mode, StringView input)
{
    if (mode == "cv") {
        auto parser = Web::CSS::Parser::Parser::create(params(false), input);
        dump_values(parser.parse_as_list_of_component_values(), 1);
    } else if (mode == "one") {
        auto parser = Web::CSS::Parser::Parser::create(params(false), input);
        auto v = parser.parse_as_component_value();
        if (v.has_value())
            line(1, v->to_debug_string());
        else
            line(1, "none"_string);
    } else if (mode == "csv") {
        auto parser = Web::CSS::Parser::Parser::create(params(false), input);
        auto groups = parser.parse_a_comma_separated_list_of_component_values(parser.m_token_stream);
        for (auto& g : groups) {
            line(1, "group:"_string);
            dump_values(g, 2);
        }
    } else if (mode == "sheet") {
        auto parser = Web::CSS::Parser::Parser::create(params(false), input);
        auto rules = parser.parse_a_stylesheets_contents(parser.m_token_stream);
        for (auto& r : rules)
            dump_rule(r, 1);
    } else if (mode == "block" || mode == "style") {
        auto parser = Web::CSS::Parser::Parser::create(params(mode == "style"), input);
        auto items = parser.parse_a_blocks_contents(parser.m_token_stream);
        for (auto& i : items)
            dump_rule_or_list(i, 1);
    } else if (mode == "rule") {
        auto parser = Web::CSS::Parser::Parser::create(params(false), input);
        auto r = parser.parse_a_rule(parser.m_token_stream);
        if (r.has_value())
            dump_rule(*r, 1);
        else
            line(1, "none"_string);
    } else if (mode == "decl") {
        auto parser = Web::CSS::Parser::Parser::create(params(true), input);
        auto d = parser.parse_a_declaration(parser.m_token_stream);
        if (d.has_value())
            dump_declaration(*d, 1);
        else
            line(1, "none"_string);
    } else if (mode == "fulltext") {
        auto parser = Web::CSS::Parser::Parser::create(params(true), input);
        auto d = parser.consume_a_declaration(parser.m_token_stream, Web::CSS::Parser::Parser::Nested::No, Web::CSS::Parser::Parser::SaveOriginalText::Yes);
        if (d.has_value())
            line(1, MUST(String::formatted("{}", d->original_full_text.value_or("-"_string))));
        else
            line(1, "none"_string);
    } else if (mode == "vendor") {
        line(1, MUST(String::formatted("{}", Web::CSS::Parser::Parser::has_ignored_vendor_prefix(input))));
    } else if (mode == "ident") {
        line(1, MUST(String::formatted("{}", Web::is_valid_custom_ident(MUST(FlyString::from_utf8(input)), { { "auto"sv, "none"sv } }))));
    } else if (mode == "decldecl") {
        line(1, serialize_a_css_declaration(input, " red "sv, Important::No));
        line(1, serialize_a_css_declaration(input, "  "sv, Important::Yes));
    } else if (mode == "decode") {
        auto bytes = MUST(ByteBuffer::copy(input.bytes()));
        auto r = Web::css_decode_bytes({}, {}, bytes);
        if (r.is_error())
            line(1, "error"_string);
        else
            line(1, r.release_value());
    } else if (mode == "math") {
        ErrorReporter::the().m_errors.clear();
        auto parser = Web::CSS::Parser::Parser::create(params(false), input);
        auto value = parser.parse_as_component_value();
        auto node = parser.parse_math_function(value->function(), CalculationContext {});
        line(1, node ? "node"_string : "none"_string);
        for (auto const& [error, metadata] : ErrorReporter::the().m_errors)
            line(1, MUST(String::formatted("{} ({})", serialize_parsing_error(error), metadata.occurrences)));
    } else if (mode == "atctx") {
        line(1, MUST(String::formatted("{}", to_underlying(rule_context_type_for_at_rule(MUST(FlyString::from_utf8(input)))))));
    } else if (mode == "errors") {
        auto name = MUST(FlyString::from_utf8(input));
        auto text = MUST(String::from_utf8(input));
        Vector<ParsingError> errors {
            UnknownPropertyError { .property_name = name },
            UnknownRuleError { .rule_name = name },
            UnknownMediaFeatureError { .media_feature_name = name },
            UnknownPseudoClassOrElementError { .name = name },
            InvalidPropertyError { .property_name = name, .value_string = text, .description = "desc"_string },
            InvalidValueError { .value_type = name, .value_string = text, .description = "desc"_string },
            InvalidRuleError { .rule_name = name, .prelude = text, .description = "desc"_string },
            InvalidQueryError { .value_string = text, .description = "desc"_string },
            InvalidSelectorError { .value_string = text, .description = "desc"_string },
            InvalidPseudoClassOrElementError { .name = name, .value_string = text, .description = "desc"_string },
            InvalidRuleLocationError { .outer_rule_name = name, .inner_rule_name = "@inner"_fly_string },
        };
        for (auto& e : errors)
            line(1, MUST(String::formatted("{} {}", serialize_parsing_error(e), AK::Traits<ParsingError>::hash(e))));
    } else {
        fprintf(stderr, "unknown mode %s\n", mode.c_str());
        exit(1);
    }
}

int main(int argc, char** argv)
{
    std::ifstream in(argc > 1 ? argv[1] : "cases.txt");
    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty() || raw[0] == '#')
            continue;
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
        run(mode, StringView(input.data(), input.size()));
    }
    std::cout << g_out;
}
