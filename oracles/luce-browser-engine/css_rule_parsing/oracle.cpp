// Oracle for luce-browser-engine region r31 (CSS rule, selector, media, descriptor and syntax
// parsing): runs the reference build's Web::CSS::Parser on the cases in cases.txt and prints what
// the Luce tests (tests_css_rule_parsing_cases) compare. Each case line is "<mode>\t<input>" with
// \n, \t and \\ escaped in <input>. After each case the ErrorReporter's errors are listed.
#define private public
#define protected public
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/Parser/ErrorReporter.h>
#include <LibWeb/CSS/Parser/Syntax.h>
#include <LibWeb/CSS/Parser/SyntaxParsing.h>
#include <LibWeb/CSS/Selector.h>
#include <LibWeb/CSS/CSSFunctionRule.h>
#include <LibWeb/CSS/CSSNestedDeclarations.h>
#include <LibWeb/CSS/Serialize.h>
#include <LibWeb/CSS/Keyword.h>
#include <LibWeb/CSS/PseudoClass.h>
#undef private
#undef protected
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

static ParsingParams params()
{
    ParsingParams p;
    p.declared_namespaces.set("svg"_fly_string);
    return p;
}

static Vector<ComponentValue> values_of(StringView input)
{
    auto parser = Web::CSS::Parser::Parser::create(params(), input);
    return parser.parse_as_list_of_component_values();
}

static StringView combinator_name(Selector::Combinator c)
{
    switch (c) {
    case Selector::Combinator::None: return "None"sv;
    case Selector::Combinator::ImmediateChild: return "ImmediateChild"sv;
    case Selector::Combinator::Descendant: return "Descendant"sv;
    case Selector::Combinator::NextSibling: return "NextSibling"sv;
    case Selector::Combinator::SubsequentSibling: return "SubsequentSibling"sv;
    case Selector::Combinator::Column: return "Column"sv;
    }
    VERIFY_NOT_REACHED();
}

static String qualified_name(Selector::SimpleSelector::QualifiedName const& q)
{
    return MUST(String::formatted("ns={} ns_name='{}' name='{}' lower='{}'", to_underlying(q.namespace_type), q.namespace_, q.name.name, q.name.lowercase_name));
}

static String joined_fly(Vector<FlyString> const& v)
{
    StringBuilder b;
    for (auto& s : v) {
        if (!b.is_empty())
            b.append(","sv);
        b.append(s);
    }
    return MUST(b.to_string());
}

static void dump_simple(Selector::SimpleSelector const& s, int indent)
{
    switch (s.type) {
    case Selector::SimpleSelector::Type::Universal:
        line(indent, MUST(String::formatted("Universal {}", qualified_name(s.qualified_name()))));
        break;
    case Selector::SimpleSelector::Type::TagName:
        line(indent, MUST(String::formatted("TagName {}", qualified_name(s.qualified_name()))));
        break;
    case Selector::SimpleSelector::Type::Id:
        line(indent, MUST(String::formatted("Id name='{}' lower='{}'", s.name(), s.lowercase_name())));
        break;
    case Selector::SimpleSelector::Type::Class:
        line(indent, MUST(String::formatted("Class name='{}' lower='{}'", s.name(), s.lowercase_name())));
        break;
    case Selector::SimpleSelector::Type::Attribute: {
        auto& a = s.attribute();
        line(indent, MUST(String::formatted("Attribute match={} case={} {} value='{}'", to_underlying(a.match_type), to_underlying(a.case_type), qualified_name(a.qualified_name), a.value)));
        break;
    }
    case Selector::SimpleSelector::Type::PseudoClass: {
        auto& p = s.pseudo_class();
        StringBuilder levels;
        for (auto l : p.levels)
            levels.appendff("{},", l);
        line(indent, MUST(String::formatted("PseudoClass {} anb={},{} forgiving={} args={} langs=[{}] ident={} levels=[{}]", pseudo_class_name(p.type), p.an_plus_b_pattern.step_size, p.an_plus_b_pattern.offset,
                             p.is_forgiving ? 1 : 0, p.argument_selector_list.size(), joined_fly(p.languages),
                             p.ident.has_value() ? MUST(String::formatted("{}/{}", string_from_keyword(p.ident->keyword), p.ident->string_value)) : "-"_string, MUST(levels.to_string()))));
        break;
    }
    case Selector::SimpleSelector::Type::PseudoElement:
        line(indent, "PseudoElement"_string);
        break;
    case Selector::SimpleSelector::Type::Nesting:
        line(indent, "Nesting"_string);
        break;
    case Selector::SimpleSelector::Type::Invalid:
        line(indent, "Invalid"_string);
        break;
    }
}

static void dump_errors()
{
    for (auto const& [error, metadata] : ErrorReporter::the().m_errors)
        line(1, MUST(String::formatted("error: {} ({})", serialize_parsing_error(error), metadata.occurrences)));
}

static String parse_error_name(Web::CSS::Parser::Parser::ParseError e)
{
    return e == Web::CSS::Parser::Parser::ParseError::SyntaxError ? "SyntaxError"_string : "IncludesIgnoredVendorPrefix"_string;
}

static void run(std::string const& mode, StringView input)
{
    ErrorReporter::the().m_errors.clear();
    auto parser = Web::CSS::Parser::Parser::create(params(), input);
    auto values = values_of(input);
    Web::CSS::Parser::TokenStream tokens { values };
    if (mode == "syntax" || mode == "syntaxcustom") {
        auto node = parse_as_syntax(values, mode == "syntax" ? LimitSingleComponentIdentToCustomIdent::No : LimitSingleComponentIdentToCustomIdent::Yes);
        if (!node) {
            line(1, "none"_string);
        } else {
            auto dumped = node->dump();
            for (auto l : dumped.bytes_as_string_view().split_view('\n'))
                line(1, MUST(String::from_utf8(l)));
            line(1, MUST(String::formatted("to_string: {}", node->to_string())));
        }
    } else if (mode == "compound") {
        auto result = parser.parse_compound_selector(tokens);
        if (result.is_error()) {
            line(1, MUST(String::formatted("error {}", parse_error_name(result.error()))));
        } else {
            auto c = result.release_value();
            line(1, MUST(String::formatted("combinator {}", combinator_name(c.combinator))));
            for (auto& s : c.simple_selectors)
                dump_simple(s, 2);
        }
        line(1, MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    } else if (mode == "anb") {
        auto p = parser.parse_a_n_plus_b_pattern(tokens);
        if (p.has_value())
            line(1, MUST(String::formatted("{} {}", p->step_size, p->offset)));
        else
            line(1, "none"_string);
        line(1, MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    } else if (mode == "combinator") {
        auto c = parser.parse_selector_combinator(tokens);
        line(1, c.has_value() ? MUST(String::from_utf8(combinator_name(*c))) : "none"_string);
        line(1, MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    } else if (mode == "qname" || mode == "qnamenowild") {
        auto q = parser.parse_selector_qualified_name(tokens, mode == "qname" ? Web::CSS::Parser::Parser::AllowWildcardName::Yes : Web::CSS::Parser::Parser::AllowWildcardName::No);
        line(1, q.has_value() ? qualified_name(*q) : "none"_string);
        line(1, MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    } else if (mode == "complexerr" || mode == "relativeerr") {
        auto result = parser.parse_complex_selector(tokens, mode == "complexerr" ? Web::CSS::Parser::Parser::SelectorType::Standalone : Web::CSS::Parser::Parser::SelectorType::Relative);
        line(1, result.is_error() ? MUST(String::formatted("error {}", parse_error_name(result.error()))) : "selector"_string);
    } else if (mode == "pageerr") {
        auto result = parser.parse_a_page_selector_list(tokens);
        line(1, result.is_error() ? MUST(String::formatted("error {}", parse_error_name(result.error()))) : "list"_string);
    } else if (mode == "mediatype") {
        auto t = parser.parse_media_type(tokens);
        line(1, t.has_value() ? MUST(String::formatted("{}", t->name)) : "none"_string);
        line(1, MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    } else if (mode == "layername" || mode == "layernameblank") {
        auto n = parser.parse_layer_name(tokens, mode == "layername" ? Web::CSS::Parser::Parser::AllowBlankLayerName::No : Web::CSS::Parser::Parser::AllowBlankLayerName::Yes);
        line(1, n.has_value() ? MUST(String::formatted("'{}'", *n)) : "none"_string);
        line(1, MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    } else if (mode == "atrule" || mode == "atblock" || mode == "style") {
        Rule rule = QualifiedRule { .prelude = values, .declarations = {}, .child_rules = {} };
        if (mode != "style") {
            auto space = input.find(' ').value();
            auto prelude_parser = Web::CSS::Parser::Parser::create(params(), input.substring_view(space + 1));
            rule = AtRule { .name = MUST(FlyString::from_utf8(input.substring_view(0, space))), .prelude = prelude_parser.parse_as_list_of_component_values(), .child_rules_and_lists_of_declarations = {}, .is_block_rule = mode == "atblock" };
        }
        auto converted = parser.convert_to_rule<CSSNestedDeclarations>(rule, Web::CSS::Parser::Parser::Nested::No);
        line(1, converted ? "rule"_string : "none"_string);
    } else if (mode == "prelude") {
        auto p = parser.parse_function_prelude(tokens);
        if (!p.has_value()) {
            line(1, "none"_string);
        } else {
            line(1, MUST(String::formatted("name {} params {} returns {}", p->name, p->parameters.size(), p->return_type->to_string())));
        }
        line(1, MUST(String::formatted("rest: {}", tokens.remaining_token_count())));
    } else {
        fprintf(stderr, "unknown mode %s\n", mode.c_str());
        exit(1);
    }
    dump_errors();
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
