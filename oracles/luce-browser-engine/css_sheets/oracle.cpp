// Oracle for luce-browser-engine region r37 (CSS style sheets and rules): parses the style sheets
// of cases.txt with the reference build's Web::CSS parser into CSSStyleSheets (in the internal
// CSS realm), runs the CSSOM operation the case names, and prints what the Luce test
// (tests_css_sheets_cases) compares. Each case line is "<mode>\t<arg>\t<arg>..." with \n, \t and \\
// escaped in the arguments.
#define private public
#define protected public
#include <LibURL/Parser.h>
#include <LibWeb/CSS/CSS.h>
#include <LibWeb/CSS/CSSContainerRule.h>
#include <LibWeb/CSS/ContainerQuery.h>
#include <LibWeb/CSS/CSSCounterStyleRule.h>
#include <LibWeb/CSS/CSSFontFaceDescriptors.h>
#include <LibWeb/CSS/CSSFontFaceRule.h>
#include <LibWeb/CSS/CSSFontFeatureValuesRule.h>
#include <LibWeb/CSS/CSSFunctionDescriptors.h>
#include <LibWeb/CSS/CSSGroupingRule.h>
#include <LibWeb/CSS/CSSImportRule.h>
#include <LibWeb/CSS/CSSKeyframeRule.h>
#include <LibWeb/CSS/CSSMarginRule.h>
#include <LibWeb/CSS/CSSNestedDeclarations.h>
#include <LibWeb/CSS/CSSPageDescriptors.h>
#include <LibWeb/CSS/CSSPropertyRule.h>
#include <LibWeb/CSS/CSSStyleProperties.h>
#include <LibWeb/CSS/StyleSheetIdentifier.h>
#include <LibWeb/CSS/CSSKeyframesRule.h>
#include <LibWeb/CSS/CSSLayerBlockRule.h>
#include <LibWeb/CSS/CSSLayerStatementRule.h>
#include <LibWeb/CSS/CSSNamespaceRule.h>
#include <LibWeb/CSS/CSSPageRule.h>
#include <LibWeb/CSS/CSSRuleList.h>
#include <LibWeb/CSS/CSSStyleRule.h>
#include <LibWeb/CSS/CSSStyleSheet.h>
#include <LibWeb/CSS/MediaList.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/StyleScope.h>
#include <LibWeb/Dump.h>
#include <LibWeb/WebIDL/DOMException.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibCore/EventLoop.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#undef private
#undef protected
#include <fstream>
#include <iostream>
#include <stdio.h>
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

static Parser::ParsingParams params()
{
    Parser::ParsingParams p { Web::internal_css_realm() };
    return p;
}

static GC::Ref<CSSStyleSheet> sheet_of(StringView css)
{
    return Web::parse_css_stylesheet(params(), css);
}

// The rules of a list, recursively: class, type for bindings, cssText.
static void print_rules(CSSRuleList& rules, int indent)
{
    for (auto& rule : rules) {
        line(indent, MUST(String::formatted("{} type={} internal={} text={}", rule->class_name(), rule->type_for_bindings(), to_underlying(rule->type()), rule->css_text())));
        if (auto* grouping = as_if<CSSGroupingRule>(*rule))
            print_rules(grouping->css_rules(), indent + 1);
        if (auto* keyframes = as_if<CSSKeyframesRule>(*rule))
            print_rules(*keyframes->css_rules(), indent + 1);
    }
}

static void print_sheet(CSSStyleSheet& sheet)
{
    line(1, MUST(String::formatted("rules {}", sheet.rules().length())));
    print_rules(sheet.rules(), 2);
}

static void print_dump(CSSStyleSheet& sheet)
{
    StringBuilder builder;
    for (auto& rule : sheet.rules())
        Web::dump_rule(builder, rule, 1);
    for (auto l : builder.string_view().split_view('\n'))
        line(1, MUST(String::formatted("dump: {}", l)));
}

template<typename T>
static void print_exception(WebIDL::ExceptionOr<T> const& result)
{
    result.exception().visit(
        [&](GC::Ref<WebIDL::DOMException> const& exception) {
            line(1, MUST(String::formatted("exception {}: {}", exception->name(), exception->message())));
        },
        [&](WebIDL::SimpleException const& exception) {
            line(1, MUST(String::formatted("simple exception: {}", exception.message.visit([](auto const& m) { return MUST(String::formatted("{}", m)); }))));
        },
        [&](auto const&) {
            line(1, "completion"_string);
        });
}

static void print_layer_names(CSSRuleList& rules, int indent)
{
    rules.for_each_effective_rule(TraversalOrder::Preorder, [&](CSSRule const& rule) {
        if (auto* block = as_if<CSSLayerBlockRule>(rule)) {
            line(indent, MUST(String::formatted("block name='{}' qualified='{}'", block->name(), block->internal_qualified_name({}))));
        } else if (auto* statement = as_if<CSSLayerStatementRule>(rule)) {
            StringBuilder names;
            for (auto& name : statement->internal_qualified_name_list({})) {
                if (!names.is_empty())
                    names.append(","sv);
                names.append(name);
            }
            line(indent, MUST(String::formatted("statement '{}'", names.string_view())));
        } else if (auto* style = as_if<CSSStyleRule>(rule)) {
            line(indent, MUST(String::formatted("style '{}' layer='{}'", style->selector_text(), style->qualified_layer_name())));
        }
    });
}

static u32 to_u32(std::string const& s) { return static_cast<u32>(std::stoul(s)); }

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    auto arg = [&](size_t i) { return StringView(args[i].data(), args[i].size()); };
    auto sheet = sheet_of(arg(1));
    if (mode == "sheet") {
        print_sheet(sheet);
    } else if (mode == "dump") {
        print_dump(sheet);
    } else if (mode == "insert") {
        auto result = sheet->insert_rule(arg(2), to_u32(args[3]));
        if (result.is_exception())
            print_exception(result);
        else
            line(1, MUST(String::formatted("inserted at {}", result.value())));
        print_sheet(sheet);
    } else if (mode == "delete") {
        auto result = sheet->delete_rule(to_u32(args[2]));
        if (result.is_exception())
            print_exception(result);
        else
            line(1, "deleted"_string);
        print_sheet(sheet);
    } else if (mode == "addrule") {
        Optional<String> selector = args[2] == "-" ? Optional<String> {} : MUST(String::from_utf8(arg(2)));
        Optional<String> style = args[3] == "-" ? Optional<String> {} : MUST(String::from_utf8(arg(3)));
        Optional<u32> index = args[4] == "-" ? Optional<u32> {} : Optional<u32> { to_u32(args[4]) };
        auto result = sheet->add_rule(selector, style, index);
        if (result.is_exception())
            print_exception(result);
        else
            line(1, MUST(String::formatted("returned {}", result.value())));
        print_sheet(sheet);
    } else if (mode == "ginsert") {
        auto& grouping = as<CSSGroupingRule>(*sheet->rules().item(0));
        auto result = grouping.insert_rule(arg(2), to_u32(args[3]));
        if (result.is_exception())
            print_exception(result);
        else
            line(1, MUST(String::formatted("inserted at {}", result.value())));
        print_sheet(sheet);
    } else if (mode == "gdelete") {
        auto& grouping = as<CSSGroupingRule>(*sheet->rules().item(0));
        auto result = grouping.delete_rule(to_u32(args[2]));
        if (result.is_exception())
            print_exception(result);
        else
            line(1, "deleted"_string);
        print_sheet(sheet);
    } else if (mode == "selector") {
        auto& rule = as<CSSStyleRule>(*sheet->rules().item(0));
        rule.set_selector_text(arg(2));
        line(1, MUST(String::formatted("selectorText {}", rule.selector_text())));
        print_sheet(sheet);
    } else if (mode == "listinsert") {
        auto result = sheet->rules().insert_a_css_rule(arg(2), to_u32(args[3]), CSSRuleList::Nested::No, sheet->declared_namespaces());
        if (result.is_exception())
            print_exception(result);
        else
            line(1, MUST(String::formatted("inserted at {}", result.value())));
        print_sheet(sheet);
    } else if (mode == "listremove") {
        auto result = sheet->rules().remove_a_css_rule(to_u32(args[2]));
        if (result.is_exception())
            print_exception(result);
        else
            line(1, "deleted"_string);
        print_sheet(sheet);
    } else if (mode == "selectorfree") {
        auto& rule = as<CSSStyleRule>(*sheet->rules().item(0));
        rule.set_parent_style_sheet(nullptr);
        rule.set_selector_text(arg(2));
        line(1, MUST(String::formatted("selectorText {}", rule.selector_text())));
        print_sheet(sheet);
    } else if (mode == "pageselector") {
        auto& rule = as<CSSPageRule>(*sheet->rules().item(0));
        rule.set_selector_text(arg(2));
        line(1, MUST(String::formatted("selectorText {}", rule.selector_text())));
        print_sheet(sheet);
    } else if (mode == "namespace") {
        auto uri = sheet->namespace_uri(arg(2));
        line(1, MUST(String::formatted("namespace_uri {}", uri.has_value() ? MUST(String::formatted("'{}'", *uri)) : "none"_string)));
        auto def = sheet->default_namespace();
        line(1, MUST(String::formatted("default {}", def.has_value() ? MUST(String::formatted("'{}'", *def)) : "none"_string)));
        Vector<String> declared;
        for (auto& n : sheet->declared_namespaces())
            declared.append(n.to_string());
        quick_sort(declared, [](auto& a, auto& b) { return a < b; });
        StringBuilder b;
        for (auto& d : declared) {
            b.append("'"sv);
            b.append(d);
            b.append("' "sv);
        }
        line(1, MUST(String::formatted("declared {}", b.string_view())));
        line(1, MUST(String::formatted("imports {}", sheet->import_rules().size())));
    } else if (mode == "properties") {
        // An empty declaration block: CSSStyleProperties::create with no declarations.
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        auto props = CSSStyleProperties::create(realm, {}, {});
        line(1, MUST(String::formatted("length {} computed {} readonly {}", props->length(), props->is_computed(), props->is_readonly())));
        line(1, MUST(String::formatted("item0 '{}' cssText '{}'", props->item(0), props->css_text())));
        line(1, MUST(String::formatted("color '{}' priority '{}' --x '{}' bogus '{}'", props->get_property_value("color"_fly_string), props->get_property_priority("color"_fly_string), props->get_property_value("--x"_fly_string), props->get_property_value("bogus"_fly_string))));
        line(1, MUST(String::formatted("margin '{}' cssFloat '{}' generated color '{}'", props->get_property_value("margin"_fly_string), props->css_float(), props->color())));
        line(1, MUST(String::formatted("remove color '{}'", MUST(props->remove_property("color"_fly_string)))));
        line(1, MUST(String::formatted("set color '' ok {}", !props->set_property("color"_fly_string, ""sv, ""sv).is_exception())));
        line(1, MUST(String::formatted("set color priority bogus ok {}", !props->set_property("color"_fly_string, "red"sv, "bogus"sv).is_exception())));
        line(1, MUST(String::formatted("set bogus ok {}", !props->set_property("bogus"_fly_string, "red"sv, ""sv).is_exception())));
        line(1, MUST(String::formatted("generated set_color '' ok {}", !props->set_color(""sv).is_exception())));
        line(1, MUST(String::formatted("cssText '' ok {} length {}", !props->set_css_text(""sv).is_exception(), props->length())));
        line(1, MUST(String::formatted("has color {} custom count {}", props->has_property(PropertyID::Color), props->custom_property_count())));
        line(1, MUST(String::formatted("serialize list '{}'", props->serialize_a_css_value(Vector<StyleProperty> {}))));
        auto resolved = CSSStyleProperties::create_resolved_style(realm, {});
        line(1, MUST(String::formatted("resolved length {} computed {} readonly {} item0 '{}' cssText '{}' color '{}'", resolved->length(), resolved->is_computed(), resolved->is_readonly(), resolved->item(0), resolved->css_text(), resolved->get_property_value("color"_fly_string))));
        auto set_result = resolved->set_property("color"_fly_string, "red"sv, ""sv);
        if (set_result.is_exception())
            print_exception(set_result);
        auto remove_result = resolved->remove_property("color"_fly_string);
        if (remove_result.is_exception())
            print_exception(remove_result);
        auto text_result = resolved->set_css_text("color: red"sv);
        if (text_result.is_exception())
            print_exception(text_result);
    } else if (mode == "descriptors") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        auto font_face = CSSFontFaceDescriptors::create(realm, {});
        line(1, MUST(String::formatted("length {} item0 '{}' cssText '{}' src '{}' family '{}' priority '{}'", font_face->length(), font_face->item(0), font_face->css_text(), font_face->get_property_value("src"_fly_string), font_face->font_family(), font_face->get_property_priority("src"_fly_string))));
        line(1, MUST(String::formatted("set bogus ok {} set src '' ok {} remove src '{}' remove bogus '{}'", !font_face->set_property("bogus"_fly_string, "x"sv, ""sv).is_exception(), !font_face->set_property("src"_fly_string, ""sv, ""sv).is_exception(), MUST(font_face->remove_property("src"_fly_string)), MUST(font_face->remove_property("bogus"_fly_string)))));
        line(1, MUST(String::formatted("cssText '' ok {}", !font_face->set_css_text(""sv).is_exception())));
        auto rule = CSSFontFaceRule::create(realm, font_face);
        line(1, MUST(String::formatted("rule valid {} text '{}' parent ok {}", rule->is_valid(), rule->css_text(), font_face->parent_rule() == rule.ptr())));
        StringBuilder dumped;
        Web::dump_rule(dumped, rule, 1);
        for (auto l : dumped.string_view().split_view('\n'))
            line(1, MUST(String::formatted("dump: {}", l)));
        auto page = CSSPageDescriptors::create(realm, {});
        line(1, MUST(String::formatted("page margin '{}' margin-top '{}' size '{}' shorthand margin {} margin-top {} font-face src {}", page->margin(), page->margin_top(), page->size(),
            is_shorthand(AtRuleID::Page, DescriptorNameAndID::from_id(DescriptorID::Margin)), is_shorthand(AtRuleID::Page, DescriptorNameAndID::from_id(DescriptorID::MarginTop)), is_shorthand(AtRuleID::FontFace, DescriptorNameAndID::from_id(DescriptorID::Src)))));
        StringBuilder longhands;
        for_each_expanded_longhand(AtRuleID::Page, DescriptorNameAndID::from_id(DescriptorID::Margin), nullptr, [&](DescriptorNameAndID const& name, auto value) {
            longhands.appendff("{}:{} ", name.name(), value ? "value"sv : "null"sv);
        });
        line(1, MUST(String::formatted("page margin longhands {}", longhands.string_view())));
        line(1, MUST(String::formatted("page remove margin '{}'", MUST(page->remove_property("margin"_fly_string)))));
        auto function = CSSFunctionDescriptors::create(realm, {});
        line(1, MUST(String::formatted("function result '{}'", function->result())));
    } else if (mode == "counterstyle") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        auto rule = CSSCounterStyleRule::create(realm, "foo"_fly_string, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
        line(1, MUST(String::formatted("text '{}' name '{}' system '{}' symbols '{}' speak-as '{}'", rule->css_text(), rule->name(), rule->system(), rule->symbols(), rule->speak_as())));
        for (auto name : { "none"sv, "DISC"sv, "Decimal"sv, "lower-ROMAN"sv, "Upper-Alpha"sv, "bar"sv, "Baz"sv }) {
            rule->set_name(MUST(FlyString::from_utf8(name)));
            line(1, MUST(String::formatted("set_name {} -> {}", name, rule->name())));
        }
        for (auto name : { "decimal"sv, "DISC"sv, "square"sv, "circle"sv, "disclosure-open"sv, "disclosure-closed"sv, "foo"sv, "none"sv })
            line(1, MUST(String::formatted("non-overridable {} {}", name, CSSCounterStyleRule::matches_non_overridable_counter_style_name(MUST(FlyString::from_utf8(name))))));
    } else if (mode == "property") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        auto rule = CSSPropertyRule::create(realm, "--my-prop"_fly_string, "*"_fly_string, false, {});
        line(1, MUST(String::formatted("text '{}' initial '{}'", rule->css_text(), rule->initial_value().has_value() ? "value"sv : "none"sv)));
        auto rule2 = CSSPropertyRule::create(realm, "--a b"_fly_string, "<length> | auto"_fly_string, true, {});
        line(1, MUST(String::formatted("text '{}' inherits {}", rule2->css_text(), rule2->inherits())));
        StringBuilder dumped;
        Web::dump_rule(dumped, rule2, 1);
        for (auto l : dumped.string_view().split_view('\n'))
            line(1, MUST(String::formatted("dump: {}", l)));
    } else if (mode == "container") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        Vector<CSSContainerRule::Condition> conditions;
        conditions.append({ "sidebar"_fly_string, {} });
        auto rules = CSSRuleList::create(realm);
        auto rule = CSSContainerRule::create(realm, move(conditions), rules);
        line(1, MUST(String::formatted("text '{}' condition '{}' name '{}' query '{}' conditions {}", rule->css_text(), rule->condition_text(), rule->container_name(), rule->container_query(), rule->conditions().size())));
        Vector<CSSContainerRule::Condition> two;
        two.append({ "a b"_fly_string, {} });
        two.append({ {}, {} });
        auto rule2 = CSSContainerRule::create(realm, move(two), CSSRuleList::create(realm));
        line(1, MUST(String::formatted("text '{}' name '{}' conditions {}", rule2->css_text(), rule2->container_name(), rule2->conditions().size())));
        auto inner = Web::parse_css_stylesheet(params(), "@layer x; a {} @layer y { b {} } @page {}"sv);
        Vector<GC::Ref<CSSRule>> children;
        for (auto& r : inner->rules())
            children.append(r);
        auto rule3 = CSSContainerRule::create(realm, {}, CSSRuleList::create(realm, children));
        StringBuilder visited;
        rule3->for_each_effective_rule(TraversalOrder::Preorder, [&](CSSRule const& r) { visited.appendff("{} ", r.class_name()); });
        line(1, MUST(String::formatted("effective preorder {}", visited.string_view())));
        StringBuilder post;
        rule3->for_each_effective_rule(TraversalOrder::Postorder, [&](CSSRule const& r) { post.appendff("{} ", r.class_name()); });
        line(1, MUST(String::formatted("effective postorder {}", post.string_view())));
    } else if (mode == "import") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        for (auto layer : { Optional<FlyString> {}, Optional<FlyString> { ""_fly_string }, Optional<FlyString> { "x.y"_fly_string } }) {
            auto rule = CSSImportRule::create(realm, Web::CSS::URL { "foo.css"_string }, nullptr, layer, {}, MediaList::create(realm, {}));
            line(1, MUST(String::formatted("text '{}' href '{}' layerName '{}' supportsText '{}' matches {} state {}", rule->css_text(), rule->href(), rule->layer_name().has_value() ? *rule->layer_name() : "null"_fly_string, rule->supports_text().has_value() ? *rule->supports_text() : "null"_string, rule->matches(), CSSStyleSheet::loading_state_name(rule->loading_state()))));
            StringBuilder dumped;
            Web::dump_rule(dumped, rule, 1);
            for (auto l : dumped.string_view().split_view('\n'))
                line(1, MUST(String::formatted("dump: {}", l.starts_with("    Layer: `"sv) ? "    Layer: <anonymous>"sv : l)));
        }
    } else if (mode == "keyframe") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        auto rule = CSSKeyframeRule::create(realm, Percentage(50), CSSStyleProperties::create(realm, {}, {}));
        line(1, MUST(String::formatted("text '{}' key '{}'", rule->css_text(), rule->key_text())));
        auto rule2 = CSSKeyframeRule::create(realm, Percentage(12.5), CSSStyleProperties::create(realm, {}, {}));
        line(1, MUST(String::formatted("text '{}' key '{}'", rule2->css_text(), rule2->key_text())));
        auto keyframes = CSSKeyframesRule::create(realm, "spin"_fly_string, CSSRuleList::create(realm, { { rule, rule2 } }));
        line(1, MUST(String::formatted("text '{}' length {} parent ok {}", keyframes->css_text(), keyframes->length(), rule->parent_rule() == keyframes.ptr())));
        StringBuilder dumped;
        Web::dump_rule(dumped, keyframes, 1);
        for (auto l : dumped.string_view().split_view('\n'))
            line(1, MUST(String::formatted("dump: {}", l)));
    } else if (mode == "names") {
        for (auto name : { "top-left-corner"sv, "TOP-LEFT"sv, "top-center"sv, "right-bottom"sv, "left-middle"sv, "top"sv, "bottom-middle"sv, ""sv })
            line(1, MUST(String::formatted("margin {} {}", name, is_margin_rule_name(name))));
        for (auto name : { "stylistic"sv, "historical-forms"sv, "styleset"sv, "character-variant"sv, "swash"sv, "ornaments"sv, "annotation"sv, "Swash"sv, "font"sv })
            line(1, MUST(String::formatted("feature {} {}", name, CSSFontFeatureValuesRule::is_font_feature_value_type_at_keyword(MUST(FlyString::from_utf8(name))))));
        for (auto type : { StyleSheetIdentifier::Type::StyleElement, StyleSheetIdentifier::Type::LinkElement, StyleSheetIdentifier::Type::ImportRule, StyleSheetIdentifier::Type::UserAgent, StyleSheetIdentifier::Type::UserStyle })
            line(1, MUST(String::formatted("identifier {} {}", style_sheet_identifier_type_to_string(type), to_underlying(*style_sheet_identifier_type_from_string(style_sheet_identifier_type_to_string(type))))));
        line(1, MUST(String::formatted("identifier bogus {}", style_sheet_identifier_type_from_string("bogus"sv).has_value())));
        for (auto state : { CSSStyleSheet::LoadingState::Unloaded, CSSStyleSheet::LoadingState::Loading, CSSStyleSheet::LoadingState::Loaded, CSSStyleSheet::LoadingState::Error })
            line(1, MUST(String::formatted("loading state {}", CSSStyleSheet::loading_state_name(state))));
        for (auto ident : { "a"sv, "a b"sv, "1x"sv, "-"sv, "--x"sv, "\xc3\xa9"sv })
            line(1, MUST(String::formatted("escape {}", MUST(escape(Web::Bindings::main_thread_vm(), ident)))));
    } else if (mode == "nested") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        auto declarations = CSSNestedDeclarations::create(realm, CSSStyleProperties::create(realm, {}, {}));
        line(1, MUST(String::formatted("text '{}' parent ok {}", declarations->css_text(), declarations->declaration().parent_rule() == declarations.ptr())));
        StringBuilder dumped;
        Web::dump_rule(dumped, declarations, 1);
        for (auto l : dumped.string_view().split_view('\n'))
            line(1, MUST(String::formatted("dump: {}", l)));
    } else if (mode == "media") {
        auto realm_ref = Web::internal_css_realm(); auto& realm = *realm_ref;
        auto media = MediaList::create(realm, {});
        line(1, MUST(String::formatted("length {} matches {} item0 {}", media->length(), media->matches(), media->item(0).has_value() ? "value"sv : "none"sv)));
        media->set_media_text(""sv);
        line(1, MUST(String::formatted("after '' length {}", media->length())));
        StringBuilder dumped;
        media->dump(dumped, 1);
        for (auto l : dumped.string_view().split_view('\n'))
            line(1, MUST(String::formatted("dump: {}", l)));
        line(1, MUST(String::formatted("sheet type {} disabled {} alternate {} origin-clean {} href {} title {}", sheet->type(), sheet->disabled(), sheet->is_alternate(), sheet->is_origin_clean(), sheet->href().has_value() ? "value"sv : "none"sv, sheet->title_for_bindings().has_value() ? "value"sv : "none"sv)));
        sheet->set_title("t"_string);
        sheet->set_location(::URL::Parser::basic_parse("https://example.com/a/b.css"sv));
        line(1, MUST(String::formatted("title {} href {} loading {}", *sheet->title_for_bindings(), *sheet->href(), CSSStyleSheet::loading_state_name(sheet->loading_state()))));
    } else if (mode == "layers") {
        print_layer_names(sheet->rules(), 1);
    } else {
        fprintf(stderr, "unknown mode %s\n", mode.c_str());
        exit(1);
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
