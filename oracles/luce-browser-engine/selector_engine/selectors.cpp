// Oracle for luce-browser-engine region r38 (CSS::Selector, PageSelector): runs the reference build's
// selector parser and Selector members on the cases in selector_cases.txt and prints what the Luce
// test (tests_selector_cases) compares. Build with ./build.sh (-> ./oracle), run ./oracle >
// selector_expected.txt, then python3 gen_selector_cases.py > .../web/css/tests_selector_cases.lucb. Each case line is "<mode>\t<input>":
//   selector  parse_selector(input): per selector, serialize, specificity, the fast-match and
//             ancestor-filter flags, the ancestor hashes, the sibling invalidation distance, the
//             nesting / webkit / slotted / part flags, the contained pseudo-classes, dump_selector
//   nested    parse_selector_for_nested_style_rule(input) (adapt_nested_relative_selector_list):
//             serialize, then absolutized against `:is(.p, #q)` and `:has(.y)` (serialize or "null")
//   anb       "A B": ANPlusBPattern::serialize and matches(1..12)
//   page      parse_page_selector_list(input): each PageSelector::serialize
#define private public
#define protected public
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/Parser/ErrorReporter.h>
#include <LibWeb/CSS/Selector.h>
#include <LibWeb/CSS/PageSelector.h>
#include <LibWeb/CSS/PseudoClass.h>
#include <LibWeb/Dump.h>
#undef private
#undef protected
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace Web::CSS;

static std::string g_out;
static void line(String const& text)
{
    g_out += std::string(text.bytes_as_string_view().characters_without_null_termination(), text.bytes_as_string_view().length());
    g_out += "\n";
}

static Parser::ParsingParams params()
{
    Parser::ParsingParams p;
    p.declared_namespaces.set("svg"_fly_string);
    return p;
}

static void describe(Selector const& selector)
{
    line(MUST(String::formatted("serialize: {}", selector.serialize())));
    line(MUST(String::formatted("specificity: {}", selector.specificity())));
    line(MUST(String::formatted("fast: {} ancestor_filter: {}", selector.can_use_fast_matches(), selector.can_use_ancestor_filter())));
    StringBuilder hashes;
    for (auto h : selector.ancestor_hashes())
        hashes.appendff(" {}", h);
    line(MUST(String::formatted("hashes:{}", hashes.string_view())));
    line(MUST(String::formatted("sibling_distance: {}", selector.sibling_invalidation_distance())));
    line(MUST(String::formatted("nesting: {} webkit: {} slotted: {} part: {}", selector.contains_the_nesting_selector(), selector.contains_unknown_webkit_pseudo_element(), selector.is_slotted(), selector.has_part_pseudo_element())));
    if (selector.target_pseudo_element().has_value())
        line(MUST(String::formatted("target: {}", selector.target_pseudo_element()->serialize())));
    StringBuilder pcs;
    for (size_t i = 0; i < to_underlying(PseudoClass::__Count); ++i) {
        if (selector.contains_pseudo_class(static_cast<PseudoClass>(i)))
            pcs.appendff(" {}", pseudo_class_name(static_cast<PseudoClass>(i)));
    }
    line(MUST(String::formatted("pseudo_classes:{}", pcs.string_view())));
    StringBuilder dump;
    Web::dump_selector(dump, selector);
    g_out += std::string(dump.string_view().characters_without_null_termination(), dump.string_view().length());
}

static void run(std::string const& mode, StringView input)
{
    if (mode == "selector") {
        auto list = Web::parse_selector(params(), input);
        if (!list.has_value()) {
            line("failure"_string);
            return;
        }
        for (auto const& selector : *list) {
            line("--"_string);
            describe(*selector);
        }
    } else if (mode == "nested") {
        auto list = Web::parse_selector_for_nested_style_rule(params(), input);
        if (!list.has_value()) {
            line("failure"_string);
            return;
        }
        auto parent_list = Web::parse_selector(params(), ":is(.p, #q)"sv);
        auto const& parent = parent_list->first()->compound_selectors().first().simple_selectors.first();
        auto has_parent_list = Web::parse_selector(params(), ":has(.y)"sv);
        auto const& has_parent = has_parent_list->first()->compound_selectors().first().simple_selectors.first();
        for (auto const& selector : *list) {
            line(MUST(String::formatted("nested: {}", selector->serialize())));
            auto absolutized = selector->absolutized(parent);
            if (absolutized)
                line(MUST(String::formatted("absolutized: {} specificity: {}", absolutized->serialize(), absolutized->specificity())));
            else
                line("absolutized: null"_string);
            auto absolutized_has = selector->absolutized(has_parent);
            if (absolutized_has)
                line(MUST(String::formatted("absolutized_has: {}", absolutized_has->serialize())));
            else
                line("absolutized_has: null"_string);
        }
    } else if (mode == "anb") {
        int a = 0, b = 0;
        sscanf(std::string(input.characters_without_null_termination(), input.length()).c_str(), "%d %d", &a, &b);
        Selector::SimpleSelector::ANPlusBPattern pattern { a, b };
        StringBuilder matches;
        for (int i = 1; i <= 12; ++i)
            matches.append(pattern.matches(i) ? '1' : '0');
        line(MUST(String::formatted("serialize: {} matches: {}", pattern.serialize(), matches.string_view())));
    } else if (mode == "page") {
        auto list = Web::parse_page_selector_list(params(), input);
        if (!list.has_value()) {
            line("failure"_string);
            return;
        }
        for (auto const& selector : *list)
            line(MUST(String::formatted("page: '{}'", selector.serialize())));
    }
    for (auto const& [error, metadata] : Parser::ErrorReporter::the().m_errors)
        line(MUST(String::formatted("error: {} ({})", Parser::serialize_parsing_error(error), metadata.occurrences)));
    Parser::ErrorReporter::the().m_errors.clear();
}

int main(int argc, char** argv)
{
    std::ifstream in(argc > 1 ? argv[1] : "selector_cases.txt");
    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty() || raw[0] == '#')
            continue;
        auto tab = raw.find('\t');
        std::string mode = raw.substr(0, tab);
        std::string input = raw.substr(tab + 1);
        g_out += "case " + mode + "\t" + input + "\n";
        run(mode, StringView(input.data(), input.size()));
    }
    std::cout << g_out;
}
