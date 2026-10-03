// Oracle for luce-browser-engine region r42 (media queries, @supports, container queries and style
// invalidation): runs the reference build's LibWeb on the inputs of cases.txt and prints what the
// Luce tests (tests_media_queries, tests_invalidation_cases) compare. Each case line is
// "<mode>\t<arg>\t<arg>..." with \n, \t and \\ escaped in the arguments.
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/CSSContainerRule.h>
#include <LibWeb/CSS/CSSRuleList.h>
#include <LibWeb/CSS/CSSStyleRule.h>
#include <LibWeb/CSS/CSSStyleSheet.h>
#include <LibWeb/CSS/ContainerQuery.h>
#include <LibWeb/CSS/MediaQuery.h>
#include <LibWeb/CSS/MediaQueryList.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/StyleInvalidation.h>
#include <LibWeb/CSS/StyleInvalidationData.h>
#include <LibWeb/CSS/StyleSheetInvalidation.h>
#include <LibWeb/CSS/Supports.h>
#include <LibWeb/CSS/Invalidation/HasMutationInvalidator.h>
#include <LibWeb/CSS/Invalidation/StyleInvalidator.h>
#include <LibWeb/CSS/SelectorEngine.h>
#include <LibWeb/CSS/StyleComputer.h>
#include <LibWeb/CSS/StyleScope.h>
#include <LibWeb/CSS/StyleSheetList.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/DOM/Text.h>
#include <LibWeb/HTML/HTMLDocument.h>
#include <LibWeb/Namespace.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
#include <LibCore/EventLoop.h>
#undef private
#undef protected
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web::CSS;
using namespace Web;

static std::string g_out;
static std::string str(StringView view) { return std::string(view.characters_without_null_termination(), view.length()); }
static std::string str(String const& s) { return str(s.bytes_as_string_view()); }
static void line(int indent, std::string const& text)
{
    for (int i = 0; i < indent; i++)
        g_out += "  ";
    g_out += text + "\n";
}

static Parser::ParsingParams params() { return Parser::ParsingParams { Web::internal_css_realm() }; }

// A PageClient for a Page whose documents the oracle makes (no IPC, no input, no display).
class OraclePageClient final : public PageClient {
    GC_CELL(OraclePageClient, PageClient);
    GC_DECLARE_ALLOCATOR(OraclePageClient);

public:
    GC::Ptr<Page> m_page;
    virtual void visit_edges(Visitor& visitor) override
    {
        Base::visit_edges(visitor);
        visitor.visit(m_page);
    }
    virtual u64 id() const override { return 1; }
    virtual Page& page() override { return *m_page; }
    virtual Page const& page() const override { return *m_page; }
    virtual bool is_connection_open() const override { return false; }
    virtual Gfx::Palette palette() const override { VERIFY_NOT_REACHED(); }
    virtual DevicePixelRect screen_rect() const override { return { 0, 0, 800, 600 }; }
    virtual double zoom_level() const override { return 1; }
    virtual double device_pixel_ratio() const override { return 1; }
    virtual double device_pixels_per_css_pixel() const override { return 1; }
    virtual CSS::PreferredColorScheme preferred_color_scheme() const override { return CSS::PreferredColorScheme::Auto; }
    virtual CSS::PreferredContrast preferred_contrast() const override { return CSS::PreferredContrast::Auto; }
    virtual CSS::PreferredMotion preferred_motion() const override { return CSS::PreferredMotion::Auto; }
    virtual size_t screen_count() const override { return 1; }
    virtual Queue<QueuedInputEvent>& input_event_queue() override { VERIFY_NOT_REACHED(); }
    virtual void report_finished_handling_input_event(u64, EventResult) override { }
    virtual void request_frame() override { }
    virtual void request_file(FileRequest) override { }
    virtual DisplayListPlayerType display_list_player_type() const override { return DisplayListPlayerType::SkiaCPU; }
    virtual bool is_headless() const override { return true; }
};

GC_DEFINE_ALLOCATOR(OraclePageClient);

static GC::Root<Page> g_page;
static GC::Root<DOM::Document> g_window_document;

// The active document of a top-level traversable (it has a Window), made once.
static DOM::Document& window_document()
{
    if (!g_window_document) {
        auto& vm = Bindings::main_thread_vm();
        auto client = vm.heap().allocate<OraclePageClient>();
        auto page = Page::create(vm, *client);
        client->m_page = page;
        page->set_is_scripting_enabled(false);
        page->set_top_level_traversable(HTML::TraversableNavigable::create_a_new_top_level_traversable(*page, nullptr, {}));
        g_page = GC::make_root(page);
        g_window_document = GC::make_root(*page->top_level_traversable()->active_document());
    }
    return *g_window_document;
}

// A document without a Window, in the window document's realm (as the Luce tests' heap document).
static GC::Root<DOM::Document> g_document;
static DOM::Document& document()
{
    if (!g_document)
        g_document = GC::make_root(DOM::Document::create(window_document().realm()));
    return *g_document;
}

static void print_dump_lines(int indent, StringView dumped)
{
    for (auto l : dumped.split_view('\n'))
        line(indent, "dump: " + str(l));
}

// mark: Media queries, @supports, @container ====================================================

static void run_media(std::string const& text)
{
    auto list = parse_media_query_list(params(), StringView(text.data(), text.size()));
    line(1, "list: " + str(serialize_a_media_query_list(list)));
    for (auto& query : list) {
        line(1, "query: " + str(query->to_string()));
        bool matches = query->evaluate(document());
        line(2, std::string("matches: ") + (matches ? "true" : "false"));
        StringBuilder builder;
        query->dump(builder, 0);
        print_dump_lines(2, builder.string_view());
    }
}

static void run_supports(std::string const& text)
{
    auto supports = parse_css_supports(params(), StringView(text.data(), text.size()));
    if (!supports) {
        line(1, "none");
        return;
    }
    line(1, "text: " + str(supports->to_string()));
    line(1, std::string("matches: ") + (supports->matches() ? "true" : "false"));
    StringBuilder builder;
    supports->dump(builder, 0);
    print_dump_lines(1, builder.string_view());
}

static void run_container(std::string const& css)
{
    auto sheet = Web::parse_css_stylesheet(params(), StringView(css.data(), css.size()));
    for (auto& rule : sheet->rules()) {
        auto* container = as_if<CSSContainerRule>(*rule);
        if (!container)
            continue;
        for (auto& condition : container->m_conditions) {
            if (!condition.container_query) {
                line(1, "no query");
                continue;
            }
            line(1, "query: " + str(condition.container_query->to_string()));
            line(2, std::string("matches: ") + (condition.container_query->matches() ? "true" : "false"));
            StringBuilder builder;
            condition.container_query->dump(builder, 0);
            print_dump_lines(2, builder.string_view());
        }
    }
}

// mark: Invalidation data =======================================================================

static std::string property_text(InvalidationSet::Property const& property)
{
    return str(MUST(String::formatted("{}", property)));
}

// A set's properties, sorted, joined by ", " (AK's Formatter writes no separator: a donor quirk).
static std::string set_text(InvalidationSet const& set)
{
    std::vector<std::string> properties;
    set.for_each_property([&](auto const& property) {
        properties.push_back(property_text(property));
        return IterationDecision::Continue;
    });
    std::sort(properties.begin(), properties.end());
    std::string out = "[";
    for (size_t i = 0; i < properties.size(); i++)
        out += (i ? " " : "") + properties[i];
    return out + "]";
}

static std::string plan_text(InvalidationPlan const& plan)
{
    std::string out = "{";
    if (plan.invalidate_self)
        out += "self ";
    if (plan.invalidate_whole_subtree)
        out += "subtree ";
    for (auto& rule : plan.descendant_rules)
        out += "desc(" + (rule.match_any ? std::string("any") : set_text(rule.match_set)) + " => " + plan_text(*rule.payload) + ") ";
    for (auto& rule : plan.sibling_rules)
        out += std::string(rule.reach == SiblingInvalidationReach::Adjacent ? "adjacent(" : "subsequent(") + (rule.match_any ? std::string("any") : set_text(rule.match_set)) + " => " + plan_text(*rule.payload) + ") ";
    return out + "}";
}

template<typename Map, typename KeyText>
static void print_has_map(char const* name, Map const& map, KeyText key_text)
{
    std::vector<std::string> lines;
    for (auto& entry : map) {
        std::string text = key_text(entry.key) + ":";
        for (auto& metadata : entry.value)
            text += " " + (metadata.relative_selector ? str(metadata.relative_selector->serialize()) : std::string("-")) + "/" + std::to_string((int)metadata.scope);
        lines.push_back(text);
    }
    std::sort(lines.begin(), lines.end());
    for (auto& l : lines)
        line(1, std::string(name) + " " + l);
}

static void print_invalidation_data(StyleInvalidationData const& data)
{
    std::vector<std::string> plans;
    for (auto& entry : data.invalidation_plans)
        plans.push_back(property_text(entry.key) + " " + plan_text(*entry.value));
    std::sort(plans.begin(), plans.end());
    for (auto& plan : plans)
        line(1, "plan " + plan);
    auto fly = [](FlyString const& name) { return str(name.bytes_as_string_view()); };
    print_has_map("has-id", data.ids_used_in_has_selectors, fly);
    print_has_map("has-class", data.class_names_used_in_has_selectors, fly);
    print_has_map("has-attribute", data.attribute_names_used_in_has_selectors, fly);
    print_has_map("has-tag", data.tag_names_used_in_has_selectors, fly);
    print_has_map("has-pseudo", data.pseudo_classes_used_in_has_selectors, [](PseudoClass pseudo_class) { return str(pseudo_class_name(pseudo_class)); });
    if (data.has_selectors_sensitive_to_featureless_subtree_changes)
        line(1, "featureless-sensitive");
}

static void run_invalidation(std::string const& text)
{
    auto selectors = parse_selector(params(), StringView(text.data(), text.size()));
    if (!selectors.has_value()) {
        line(1, "invalid");
        return;
    }
    StyleInvalidationData data;
    for (auto& selector : *selectors)
        data.build_invalidation_sets_for_selector(*selector);
    print_invalidation_data(data);
}

static void run_sheet_set(std::string const& css)
{
    auto sheet = Web::parse_css_stylesheet(params(), StringView(css.data(), css.size()));
    StyleSheetInvalidationSet result;
    for (auto& rule : sheet->rules()) {
        if (auto* style_rule = as_if<CSSStyleRule>(*rule))
            extend_style_sheet_invalidation_set_with_style_rule(result, *style_rule);
    }
    line(1, "set " + set_text(result.invalidation_set));
    line(1, std::string("may-match-shadow-host ") + (result.may_match_shadow_host ? "true" : "false"));
    line(1, std::string("may-match-light-dom ") + (result.may_match_light_dom_under_shadow_host ? "true" : "false"));
    for (auto& rule : result.pseudo_element_rules)
        line(1, "pseudo-element-rule " + set_text(rule.anchor_set) + " " + (rule.anchor_selector ? str(rule.anchor_selector->serialize()) : std::string("-")));
    for (auto& rule : result.trailing_universal_rules)
        line(1, "trailing-universal-rule " + set_text(rule.anchor_set) + " " + (rule.anchor_selector ? str(rule.anchor_selector->serialize()) : std::string("-")) + " " + std::to_string((int)rule.combinator));
}

static void run_light_dom(std::string const& text)
{
    line(1, std::string("may-match-light-dom ") + (selector_may_match_light_dom_under_shadow_host(StringView(text.data(), text.size())) ? "true" : "false"));
}


// mark: Invalidation outcomes ===================================================================

// A tree spec: `name#id.class[attr=value](children) 'text' ...`.
struct TreeParser {
    DOM::Document& document;
    std::string const& text;
    size_t pos { 0 };

    bool at_end() const { return pos >= text.size(); }
    void skip_spaces()
    {
        while (!at_end() && text[pos] == ' ')
            pos++;
    }
    std::string word()
    {
        size_t start = pos;
        while (!at_end() && (isalnum((unsigned char)text[pos]) || text[pos] == '-' || text[pos] == '_'))
            pos++;
        return text.substr(start, pos - start);
    }
    GC::Ref<DOM::Node> node()
    {
        if (text[pos] == '\'') {
            size_t end = text.find('\'', pos + 1);
            auto data = text.substr(pos + 1, end - pos - 1);
            pos = end + 1;
            return document.create_text_node(Utf16String::from_utf8(StringView(data.data(), data.size())));
        }
        auto name = word();
        auto element = MUST(DOM::create_element(document, FlyString::from_utf8_without_validation(StringView(name.data(), name.size()).bytes()), Namespace::HTML));
        while (!at_end() && (text[pos] == '#' || text[pos] == '.' || text[pos] == '[')) {
            char kind = text[pos++];
            if (kind == '[') {
                size_t eq = text.find_first_of("=]", pos);
                auto attr = text.substr(pos, eq - pos);
                std::string value;
                if (text[eq] == '=') {
                    size_t close = text.find(']', eq);
                    value = text.substr(eq + 1, close - eq - 1);
                    pos = close + 1;
                } else {
                    pos = eq + 1;
                }
                element->set_attribute_value(FlyString::from_utf8_without_validation(StringView(attr.data(), attr.size()).bytes()), String::from_utf8_without_validation(StringView(value.data(), value.size()).bytes()));
                continue;
            }
            auto value = word();
            auto attr_name = kind == '#' ? "id"_fly_string : "class"_fly_string;
            auto existing = element->get_attribute(attr_name);
            std::string combined = value;
            if (kind == '.' && existing.has_value())
                combined = str(*existing) + " " + value;
            element->set_attribute_value(attr_name, String::from_utf8_without_validation(StringView(combined.data(), combined.size()).bytes()));
        }
        if (!at_end() && text[pos] == '(') {
            pos++;
            while (true) {
                skip_spaces();
                if (text[pos] == ')') {
                    pos++;
                    break;
                }
                MUST(element->append_child(node()));
            }
        }
        return element;
    }
};

static GC::Ref<DOM::Node> parse_tree(DOM::Document& document, std::string const& spec)
{
    TreeParser parser { document, spec };
    parser.skip_spaces();
    return parser.node();
}

static DOM::Element& element_by_id(DOM::Document& document, std::string const& id)
{
    GC::Ptr<DOM::Element> found;
    document.for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
        auto element_id = element.id();
        if (element_id.has_value() && str(element_id->bytes_as_string_view()) == id) {
            found = element;
            return TraversalDecision::Break;
        }
        return TraversalDecision::Continue;
    });
    VERIFY(found);
    return *found;
}

static void clear_style_flags(DOM::Document& document)
{
    document.for_each_in_inclusive_subtree([&](DOM::Node& node) {
        node.m_needs_style_update = false;
        node.m_child_needs_style_update = false;
        node.m_entire_subtree_needs_style_update = false;
        return TraversalDecision::Continue;
    });
    document.m_needs_full_style_update = false;
    document.m_needs_invalidation_of_elements_affected_by_has = false;
    document.style_scope().m_pending_has_invalidations.clear();
    document.style_invalidator().m_pending_invalidations.clear();
}

static std::string node_label(DOM::Node& node)
{
    if (auto* element = as_if<DOM::Element>(node)) {
        std::string label = str(element->local_name().bytes_as_string_view());
        if (auto id = element->id(); id.has_value())
            label += "#" + str(id->bytes_as_string_view());
        return label;
    }
    if (node.is_text())
        return "text";
    return "document";
}

static void print_style_flags(DOM::Document& document)
{
    line(1, std::string("full: ") + (document.needs_full_style_update() ? "true" : "false"));
    document.for_each_in_inclusive_subtree([&](DOM::Node& node) {
        std::string flags;
        flags += node.needs_style_update() ? "N" : "-";
        flags += node.child_needs_style_update() ? "C" : "-";
        flags += node.entire_subtree_needs_style_update() ? "E" : "-";
        line(1, node_label(node) + " " + flags);
        return TraversalDecision::Continue;
    });
}

// The author sheet's style rules matched against every element with the selector involvement metadata collected, as
// StyleComputer::collect_matching_rules does.
static void collect_involvement_metadata(DOM::Document& document, CSSStyleSheet& sheet)
{
    document.for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
        sheet.for_each_effective_style_producing_rule([&](CSSRule const& rule) {
            auto* style_rule = as_if<CSSStyleRule>(rule);
            if (!style_rule)
                return;
            for (auto& selector : style_rule->absolutized_selectors()) {
                SelectorEngine::MatchContext context {
                    .style_sheet_for_rule = sheet,
                    .subject = element,
                    .collect_per_element_selector_involvement_metadata = true,
                };
                (void)SelectorEngine::matches(*selector, element, {}, context);
            }
        });
        return TraversalDecision::Continue;
    });
}

static std::vector<std::string> split(std::string const& text, char separator)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        auto at = text.find(separator, start);
        parts.push_back(text.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos)
            break;
        start = at + 1;
    }
    return parts;
}

static void run_mutation(DOM::Document& document, std::string const& mutation)
{
    auto space = mutation.find(' ');
    auto verb = mutation.substr(0, space);
    auto rest = mutation.substr(space + 1);
    auto second = rest.find(' ');
    auto id = rest.substr(1, second == std::string::npos ? std::string::npos : second - 1);
    auto argument = second == std::string::npos ? std::string() : rest.substr(second + 1);
    auto& element = element_by_id(document, id);
    auto fly = [](std::string const& s) { return FlyString::from_utf8_without_validation(StringView(s.data(), s.size()).bytes()); };
    if (verb == "attr") {
        auto name_end = argument.find(' ');
        auto name = argument.substr(0, name_end);
        auto value = name_end == std::string::npos ? std::string() : argument.substr(name_end + 1);
        element.set_attribute_value(fly(name), String::from_utf8_without_validation(StringView(value.data(), value.size()).bytes()));
    } else if (verb == "rmattr") {
        element.remove_attribute(fly(argument));
    } else if (verb == "append") {
        MUST(element.append_child(parse_tree(document, argument)));
    } else if (verb == "before") {
        element.parent()->insert_before(parse_tree(document, argument), element);
    } else if (verb == "remove") {
        element.remove();
    } else {
        line(1, "unknown mutation " + verb);
    }
}

// outcome <tree> <css> <mutations>: the tree is built in a new HTML document (its root is the document element), the
// sheet becomes its author sheet, the rule cache is built, every element is matched against the sheet's selectors
// (involvement metadata), the style flags are cleared, the mutations run (";"-separated), and the pending :has() and
// descendant invalidations are performed as Document::update_style does before it computes styles.
static void run_outcome(std::string const& tree, std::string const& css, std::string const& mutations)
{
    auto document = HTML::HTMLDocument::create(window_document().realm());
    MUST(document->append_child(parse_tree(*document, tree)));
    auto sheet = Web::parse_css_stylesheet(params(), StringView(css.data(), css.size()));
    document->style_sheets().m_sheets.append(sheet);
    sheet->m_owning_documents_or_shadow_roots.set(document);
    document->style_scope().invalidate_rule_cache();
    document->style_scope().build_rule_cache_if_needed();
    collect_involvement_metadata(*document, *sheet);
    clear_style_flags(*document);

    for (auto& mutation : split(mutations, ';'))
        run_mutation(*document, mutation);

    if (document->m_needs_invalidation_of_elements_affected_by_has) {
        document->m_needs_invalidation_of_elements_affected_by_has = false;
        CSS::Invalidation::invalidate_style_for_pending_has_mutations(*document);
    }
    document->style_invalidator().invalidate(*document);
    print_style_flags(*document);
}

// propinv <property> <old value or -> <new value or ->: compute_property_invalidation of two parsed values.
static void run_property_invalidation(std::string const& property, std::string const& old_text, std::string const& new_text)
{
    auto property_id = property_id_from_string(StringView(property.data(), property.size())).value();
    auto parse = [&](std::string const& text) -> RefPtr<StyleValue const> {
        if (text == "-")
            return nullptr;
        return parse_css_value(params(), StringView(text.data(), text.size()), property_id);
    };
    auto old_value = parse(old_text);
    auto new_value = parse(new_text);
    auto invalidation = compute_property_invalidation(property_id, old_value.ptr(), new_value.ptr());
    auto flag = [](bool value) { return std::string(value ? "1" : "0"); };
    line(1, "repaint " + flag(invalidation.repaint) + " stacking " + flag(invalidation.rebuild_stacking_context_tree) + " relayout " + flag(invalidation.relayout) + " layout-tree " + flag(invalidation.rebuild_layout_tree) + " visual-contexts " + flag(invalidation.rebuild_accumulated_visual_contexts) + " none " + flag(invalidation.is_none()) + " full " + flag(invalidation.is_full()));
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    if (mode == "media")
        run_media(args[1]);
    else if (mode == "supports")
        run_supports(args[1]);
    else if (mode == "container")
        run_container(args[1]);
    else if (mode == "invalidation")
        run_invalidation(args[1]);
    else if (mode == "sheetset")
        run_sheet_set(args[1]);
    else if (mode == "outcome")
        run_outcome(args[1], args[2], args[3]);
    else if (mode == "propinv")
        run_property_invalidation(args[1], args[2], args[3]);
    else if (mode == "lightdom")
        run_light_dom(args[1]);
    else
        line(1, "unknown mode");
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
    Web::Platform::FontPlugin::install(*new Web::Platform::FontPlugin(false));
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
