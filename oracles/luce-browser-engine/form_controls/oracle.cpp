// Oracle for luce-browser-engine regions r24/r25 (the form controls): runs the reference build's
// HTMLInputElement, HTMLSelectElement, HTMLOptionElement, HTMLTextAreaElement, HTMLButtonElement,
// HTMLMeterElement and HTMLProgressElement on the cases of cases.txt and prints what
// tests_form_controls compares. Each case line is "<mode>\t<arg>..." with \n, \t and \\ escaped in
// the arguments; values in the output escape \n, \r, \t and \\ the same way. An <html> argument
// may instead be a detached spec, "@<tag> <attributes>[|<children markup>]" (make_subject).
//   input <html>                       every input: type, value, checked, indeterminate, dirty value
//   set <html> <value>                 the first input's value setter, then its value (or the exception)
//   attr <html> <name> <value|!>       sets (or with "!" removes) an attribute of the first input, then `input`
//   num <html>                         the first input's numbers: valueAsNumber, min, max, step, step base,
//                                      reversed range and the suffering-from states
//   step <html> <up|down> <n>          stepUp(n)/stepDown(n) of the first input, then its value
//   setnum <html> <number>             valueAsNumber's setter, then the value
//   numstr <type> <number>             convert_number_to_string of an input of that type
//   strnum <type> <string>             convert_string_to_number of an input of that type
//   checked <html> <index> <0|1>       setChecked of input #index (the binding), then every input's checkedness
//   select <html>                      the first select: selectedIndex, value, type, length, size, the
//                                      selected options and each option of its list of options
//   selectset <html> <index|value> <arg> selectedIndex or value's setter, then `select`
//   optsel <html> <index> <0|1>        option #index's selected setter, then `select`
//   textarea <html>                    the first textarea: value, defaultValue, textLength, cols, rows, maxLength
//   taset <html> <value>               the first textarea's value setter, then `textarea`
//   selection <html> <start> <end> <direction|!>  setSelectionRange on the first input or textarea
//   rangetext <html> <replacement> <start> <end> <mode>  setRangeText on the first input or textarea
//   meter <html>                       the first meter's value, min, max, low, high, optimum and state
//   progress <html>                    the first progress' value, max and position
//   button <html>                      the first button's type, submit button, value and command
//   validity <html> <tag>              the first <tag>'s suffering-from-being-missing and mutability
//   role <html> <tag>                  the first <tag>'s default ARIA role (its enum value)
//   shadow <html> <tag>                the first <tag>'s user-agent shadow tree (html5lib dump)
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/ARIA/Roles.h>
#include <LibWeb/CSS/CSSStyleProperties.h>
#include <LibWeb/CSS/PseudoElement.h>
#include <LibWeb/DOM/Attr.h>
#include <LibWeb/DOM/Comment.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/DocumentType.h>
#include <LibWeb/DOM/HTMLCollection.h>
#include <LibWeb/DOM/ShadowRoot.h>
#include <LibWeb/DOM/Text.h>
#include <LibWeb/HTML/AttributeNames.h>
#include <LibWeb/HTML/HTMLButtonElement.h>
#include <LibWeb/HTML/HTMLDocument.h>
#include <LibWeb/HTML/HTMLInputElement.h>
#include <LibWeb/HTML/HTMLMeterElement.h>
#include <LibWeb/HTML/HTMLOptionElement.h>
#include <LibWeb/HTML/HTMLProgressElement.h>
#include <LibWeb/HTML/HTMLSelectElement.h>
#include <LibWeb/HTML/HTMLTextAreaElement.h>
#include <LibWeb/HTML/Parser/HTMLParser.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Namespace.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
#include <LibWeb/WebIDL/DOMException.h>
#include <LibCore/EventLoop.h>
#undef private
#undef protected
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web;

static std::string g_out;
static std::string str(StringView view) { return std::string(view.characters_without_null_termination(), view.length()); }
static std::string str(String const& s) { return str(s.bytes_as_string_view()); }
static std::string str(FlyString const& s) { return str(s.bytes_as_string_view()); }
static std::string str(Utf16String const& s) { return str(s.to_utf8()); }

// A value as one line: \n, \r, \t and \\ escaped.
static std::string esc(std::string const& s)
{
    std::string out;
    for (char c : s) {
        if (c == '\n')
            out += "\\n";
        else if (c == '\r')
            out += "\\r";
        else if (c == '\t')
            out += "\\t";
        else if (c == '\\')
            out += "\\\\";
        else
            out += c;
    }
    return out;
}

static std::string num(double value) { return str(MUST(String::formatted("{}", value))); }
static std::string opt(Optional<double> value) { return value.has_value() ? num(*value) : "none"; }
static std::string b(bool value) { return value ? "1" : "0"; }
static Utf16String utf16(std::string const& s) { return Utf16String::from_utf8(StringView(s.data(), s.size())); }

// The exception of a failed ExceptionOr: a DOMException's name, or "simple".
static std::string exception_name(WebIDL::Exception const& exception)
{
    if (exception.has<GC::Ref<WebIDL::DOMException>>())
        return str(exception.get<GC::Ref<WebIDL::DOMException>>()->name());
    return "simple";
}

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

// A new HTML document without a browsing context (as the Luce tests' document), parsed from `html`
// with scripting disabled.
static GC::Ref<DOM::Document> parse(std::string const& html)
{
    auto document = HTML::HTMLDocument::create(window_document().realm());
    document->set_document_type(DOM::Document::Type::HTML);
    auto parser = HTML::HTMLParser::create(*document, StringView(html.data(), html.size()), HTML::ParserScriptingMode::Disabled, "UTF-8"sv);
    parser->run(document->url());
    return document;
}

static DOM::Element* first_element(DOM::Document& document, std::string const& tag)
{
    DOM::Element* found = nullptr;
    document.for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
        if (str(element.local_name()) == tag) {
            found = &element;
            return TraversalDecision::Break;
        }
        return TraversalDecision::Continue;
    });
    VERIFY(found);
    return found;
}

// The attributes of "name=value name=\"a value\" name" (in order).
static std::vector<std::pair<std::string, std::string>> parse_attributes(std::string const& text)
{
    std::vector<std::pair<std::string, std::string>> attributes;
    size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && text[i] == ' ')
            i++;
        if (i >= text.size())
            break;
        std::string name;
        while (i < text.size() && text[i] != ' ' && text[i] != '=')
            name += text[i++];
        std::string value;
        if (i < text.size() && text[i] == '=') {
            i++;
            if (i < text.size() && text[i] == '"') {
                i++;
                while (i < text.size() && text[i] != '"')
                    value += text[i++];
                i++;
            } else {
                while (i < text.size() && text[i] != ' ')
                    value += text[i++];
            }
        }
        attributes.push_back({ name, value });
    }
    return attributes;
}

// The document of a case and its detached subject element, if the case's markup is a detached
// spec: "@<tag> <attributes>[|<children markup>]" creates <tag> with DOM::create_element in a
// document parsed from "<div>children</div>", sets the attributes in order and moves the div's
// children into it. The element is never inserted, so it gets no user-agent shadow tree (the
// number input's steppers and the select's chevron are SVG, which the port does not have yet).
struct Subject {
    GC::Ref<DOM::Document> document;
    DOM::Element* detached { nullptr };
};

static Subject make_subject(std::string const& spec)
{
    if (spec.empty() || spec[0] != '@')
        return { parse(spec), nullptr };
    auto bar = spec.find('|');
    auto head = spec.substr(1, bar == std::string::npos ? std::string::npos : bar - 1);
    auto children = bar == std::string::npos ? std::string() : spec.substr(bar + 1);
    auto document = parse("<div>" + children + "</div>");
    auto space = head.find(' ');
    auto tag = head.substr(0, space);
    auto element = MUST(DOM::create_element(*document, FlyString::from_utf8_without_validation(ReadonlyBytes(tag.data(), tag.size())), Namespace::HTML));
    if (space != std::string::npos) {
        for (auto& [name, value] : parse_attributes(head.substr(space + 1)))
            element->set_attribute_value(FlyString::from_utf8_without_validation(ReadonlyBytes(name.data(), name.size())), MUST(String::from_utf8(StringView(value.data(), value.size()))));
    }
    auto* div = first_element(*document, "div");
    while (auto* child = div->first_child())
        MUST(element->append_child(*child));
    return { document, element.ptr() };
}

// The subject element: the detached one, or the document's first <tag>.
static DOM::Element* subject_element(Subject& subject, std::string const& tag)
{
    if (subject.detached) {
        if (str(subject.detached->local_name()) == tag)
            return subject.detached;
        DOM::Element* found = nullptr;
        subject.detached->for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
            if (str(element.local_name()) == tag) {
                found = &element;
                return TraversalDecision::Break;
            }
            return TraversalDecision::Continue;
        });
        VERIFY(found);
        return found;
    }
    return first_element(*subject.document, tag);
}

static std::vector<HTML::HTMLInputElement*> inputs(Subject& subject)
{
    std::vector<HTML::HTMLInputElement*> list;
    if (subject.detached) {
        list.push_back(&as<HTML::HTMLInputElement>(*subject.detached));
        return list;
    }
    subject.document->for_each_in_subtree_of_type<HTML::HTMLInputElement>([&](HTML::HTMLInputElement& input) {
        list.push_back(&input);
        return TraversalDecision::Continue;
    });
    return list;
}

// mark: The html5lib dump of a shadow tree ======================================================

static void indent(int level)
{
    g_out += "| ";
    for (int i = 0; i < level; i++)
        g_out += "  ";
}

static void dump_node(DOM::Node const& node, int level);
static void dump_children(DOM::Node const& node, int level)
{
    for (auto* child = node.first_child(); child; child = child->next_sibling())
        dump_node(*child, level);
}

static void dump_node(DOM::Node const& node, int level)
{
    indent(level);
    if (is<DOM::Element>(node)) {
        auto& element = static_cast<DOM::Element const&>(node);
        std::string prefix = element.namespace_uri() == Namespace::SVG ? "svg " : "";
        g_out += "<" + prefix + str(element.local_name()) + ">";
        if (auto pseudo = element.use_pseudo_element(); pseudo.has_value())
            g_out += " ::" + str(CSS::pseudo_element_name(*pseudo));
        g_out += "\n";
        std::vector<std::string> lines;
        element.for_each_attribute([&](DOM::Attr const& attr) {
            lines.push_back(str(attr.local_name()) + "=\"" + esc(str(attr.value())) + "\"");
        });
        std::sort(lines.begin(), lines.end());
        for (auto& l : lines) {
            indent(level + 1);
            g_out += l + "\n";
        }
        if (auto inline_style = element.inline_style()) {
            indent(level + 1);
            g_out += "style: " + esc(str(inline_style->serialized())) + "\n";
        }
        dump_children(node, level + 1);
        return;
    }
    if (is<DOM::Text>(node)) {
        g_out += "\"" + esc(str(static_cast<DOM::Text const&>(node).data())) + "\"\n";
        return;
    }
    g_out += "?\n";
}

// mark: Dumps ===================================================================================

static void dump_inputs(Subject& subject)
{
    for (auto* input : inputs(subject)) {
        g_out += "type=" + str(input->type()) + " value=" + esc(str(input->value())) + " checked=" + b(input->checked())
            + " indeterminate=" + b(input->indeterminate()) + " dirty=" + b(input->m_dirty_value) + "\n";
    }
}

static void dump_checked(Subject& subject)
{
    for (auto* input : inputs(subject))
        g_out += b(input->checked());
    g_out += "\n";
}

static void dump_select(HTML::HTMLSelectElement& select)
{
    g_out += "selectedIndex=" + std::to_string(select.selected_index()) + " value=" + esc(str(select.value())) + " type=" + str(select.type())
        + " length=" + std::to_string(select.length()) + " size=" + std::to_string(select.size()) + " selectedOptions="
        + std::to_string(select.selected_options()->length()) + "\n";
    for (auto& option : select.list_of_options()) {
        g_out += "option index=" + std::to_string(option->index()) + " selected=" + b(option->selected()) + " disabled=" + b(option->disabled())
            + " label=" + esc(str(option->label())) + " text=" + esc(str(option->text())) + " value=" + esc(str(option->value())) + "\n";
    }
}

static void dump_textarea(HTML::HTMLTextAreaElement& textarea)
{
    g_out += "value=" + esc(str(textarea.value())) + " defaultValue=" + esc(str(textarea.default_value())) + " textLength="
        + std::to_string(textarea.text_length()) + " cols=" + std::to_string(textarea.cols()) + " rows=" + std::to_string(textarea.rows())
        + " maxLength=" + std::to_string(textarea.max_length()) + " dirty=" + b(textarea.m_dirty_value) + "\n";
}

static HTML::FormAssociatedTextControlElement* first_text_control(Subject& subject)
{
    if (subject.detached) {
        if (auto* input = as_if<HTML::HTMLInputElement>(*subject.detached))
            return input;
        return &as<HTML::HTMLTextAreaElement>(*subject.detached);
    }
    HTML::FormAssociatedTextControlElement* found = nullptr;
    subject.document->for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
        if (auto* input = as_if<HTML::HTMLInputElement>(element)) {
            found = input;
            return TraversalDecision::Break;
        }
        if (auto* textarea = as_if<HTML::HTMLTextAreaElement>(element)) {
            found = textarea;
            return TraversalDecision::Break;
        }
        return TraversalDecision::Continue;
    });
    VERIFY(found);
    return found;
}

static void dump_selection(HTML::FormAssociatedTextControlElement& control)
{
    auto direction = control.selection_direction();
    auto selected = control.selected_text_for_stringifier();
    g_out += "value=" + esc(str(control.relevant_value())) + " start=" + std::to_string(control.selection_start()) + " end="
        + std::to_string(control.selection_end()) + " direction=" + (direction.has_value() ? str(*direction) : "null") + " selected="
        + (selected.has_value() ? esc(str(*selected)) : "none") + "\n";
}

// mark: Modes ===================================================================================

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    if (mode == "input") {
        auto subject = make_subject(args[1]);
        dump_inputs(subject);
    } else if (mode == "set") {
        auto subject = make_subject(args[1]);
        auto& input = as<HTML::HTMLInputElement>(*subject_element(subject, "input"));
        auto result = input.set_value(utf16(args[2]));
        if (result.is_exception())
            g_out += "exception " + exception_name(result.exception()) + "\n";
        dump_inputs(subject);
    } else if (mode == "attr") {
        auto subject = make_subject(args[1]);
        auto& input = as<HTML::HTMLInputElement>(*subject_element(subject, "input"));
        auto name = FlyString::from_utf8_without_validation(ReadonlyBytes(args[2].data(), args[2].size()));
        if (args[3] == "!")
            input.remove_attribute(name);
        else
            input.set_attribute_value(name, MUST(String::from_utf8(StringView(args[3].data(), args[3].size()))));
        dump_inputs(subject);
    } else if (mode == "num") {
        auto subject = make_subject(args[1]);
        auto& input = as<HTML::HTMLInputElement>(*subject_element(subject, "input"));
        g_out += "valueAsNumber=" + num(input.value_as_number()) + " min=" + opt(input.min()) + " max=" + opt(input.max()) + " step="
            + opt(input.allowed_value_step()) + " stepBase=" + num(input.step_base()) + " reversed=" + b(input.has_reversed_range())
            + " underflow=" + b(input.suffering_from_an_underflow()) + " overflow=" + b(input.suffering_from_an_overflow())
            + " stepMismatch=" + b(input.suffering_from_a_step_mismatch()) + " typeMismatch=" + b(input.suffering_from_a_type_mismatch())
            + " patternMismatch=" + b(input.suffering_from_a_pattern_mismatch()) + " missing=" + b(input.suffering_from_being_missing()) + "\n";
    } else if (mode == "step") {
        auto subject = make_subject(args[1]);
        auto& input = as<HTML::HTMLInputElement>(*subject_element(subject, "input"));
        auto n = std::stoi(args[3]);
        auto result = args[2] == "up" ? input.step_up(n) : input.step_down(n);
        if (result.is_exception())
            g_out += "exception " + exception_name(result.exception()) + "\n";
        dump_inputs(subject);
    } else if (mode == "setnum") {
        auto subject = make_subject(args[1]);
        auto& input = as<HTML::HTMLInputElement>(*subject_element(subject, "input"));
        auto result = input.set_value_as_number(std::stod(args[2]));
        if (result.is_exception())
            g_out += "exception " + exception_name(result.exception()) + "\n";
        dump_inputs(subject);
    } else if (mode == "numstr") {
        auto subject = make_subject("@input type=" + args[1]);
        auto& input = as<HTML::HTMLInputElement>(*subject_element(subject, "input"));
        g_out += esc(str(input.convert_number_to_string(std::stod(args[2])))) + "\n";
    } else if (mode == "strnum") {
        auto subject = make_subject("@input type=" + args[1]);
        auto& input = as<HTML::HTMLInputElement>(*subject_element(subject, "input"));
        g_out += opt(input.convert_string_to_number(StringView(args[2].data(), args[2].size()))) + "\n";
    } else if (mode == "checked") {
        auto subject = make_subject(args[1]);
        auto list = inputs(subject);
        list[std::stoi(args[2])]->set_checked_binding(args[3] == "1");
        dump_checked(subject);
    } else if (mode == "select") {
        auto subject = make_subject(args[1]);
        dump_select(as<HTML::HTMLSelectElement>(*subject_element(subject, "select")));
    } else if (mode == "selectset") {
        auto subject = make_subject(args[1]);
        auto& select = as<HTML::HTMLSelectElement>(*subject_element(subject, "select"));
        if (args[2] == "index")
            MUST(select.set_selected_index(std::stoi(args[3])));
        else
            MUST(select.set_value(utf16(args[3])));
        dump_select(select);
    } else if (mode == "optsel") {
        auto subject = make_subject(args[1]);
        auto& select = as<HTML::HTMLSelectElement>(*subject_element(subject, "select"));
        auto options = select.list_of_options();
        options[std::stoi(args[2])]->set_selected(args[3] == "1");
        dump_select(select);
    } else if (mode == "textarea") {
        auto subject = make_subject(args[1]);
        dump_textarea(as<HTML::HTMLTextAreaElement>(*subject_element(subject, "textarea")));
    } else if (mode == "taset") {
        auto subject = make_subject(args[1]);
        auto& textarea = as<HTML::HTMLTextAreaElement>(*subject_element(subject, "textarea"));
        textarea.set_value(utf16(args[2]));
        dump_textarea(textarea);
    } else if (mode == "selection") {
        auto subject = make_subject(args[1]);
        auto& control = *first_text_control(subject);
        Optional<String> direction;
        if (args[4] != "!")
            direction = MUST(String::from_utf8(StringView(args[4].data(), args[4].size())));
        auto result = control.set_selection_range(std::stoul(args[2]), std::stoul(args[3]), direction);
        if (result.is_exception())
            g_out += "exception " + exception_name(result.exception()) + "\n";
        dump_selection(control);
    } else if (mode == "rangetext") {
        auto subject = make_subject(args[1]);
        auto& control = *first_text_control(subject);
        Bindings::SelectionMode selection_mode = Bindings::SelectionMode::Preserve;
        if (args[5] == "select")
            selection_mode = Bindings::SelectionMode::Select;
        else if (args[5] == "start")
            selection_mode = Bindings::SelectionMode::Start;
        else if (args[5] == "end")
            selection_mode = Bindings::SelectionMode::End;
        auto result = control.set_range_text_binding(utf16(args[2]), std::stoul(args[3]), std::stoul(args[4]), selection_mode);
        if (result.is_exception())
            g_out += "exception " + exception_name(result.exception()) + "\n";
        dump_selection(control);
    } else if (mode == "meter") {
        auto subject = make_subject(args[1]);
        auto& meter = as<HTML::HTMLMeterElement>(*subject_element(subject, "meter"));
        g_out += "value=" + num(meter.value()) + " min=" + num(meter.min()) + " max=" + num(meter.max()) + " low=" + num(meter.low())
            + " high=" + num(meter.high()) + " optimum=" + num(meter.optimum()) + " state=" + std::to_string(to_underlying(meter.value_state())) + "\n";
    } else if (mode == "progress") {
        auto subject = make_subject(args[1]);
        auto& progress = as<HTML::HTMLProgressElement>(*subject_element(subject, "progress"));
        g_out += "value=" + num(progress.value()) + " max=" + num(progress.max()) + " position=" + num(progress.position()) + "\n";
    } else if (mode == "button") {
        auto subject = make_subject(args[1]);
        auto& button = as<HTML::HTMLButtonElement>(*subject_element(subject, "button"));
        g_out += "type=" + str(button.type_for_bindings()) + " submit=" + b(button.is_submit_button()) + " value=" + esc(str(button.value()))
            + " command=" + esc(str(button.command())) + "\n";
    } else if (mode == "validity") {
        auto subject = make_subject(args[1]);
        auto* element = subject_element(subject, args[2]);
        auto* form_associated = as_if<HTML::FormAssociatedElement>(*element);
        VERIFY(form_associated);
        g_out += "missing=" + b(form_associated->suffering_from_being_missing()) + " mutable=" + b(form_associated->is_mutable()) + "\n";
    } else if (mode == "role") {
        auto subject = make_subject(args[1]);
        auto* element = subject_element(subject, args[2]);
        auto role = element->default_role();
        g_out += role.has_value() ? std::to_string(to_underlying(*role)) + "\n" : "none\n";
    } else if (mode == "shadow") {
        auto subject = make_subject(args[1]);
        auto* element = subject_element(subject, args[2]);
        if (auto shadow_root = element->shadow_root())
            dump_children(*shadow_root, 0);
        else
            g_out += "no shadow root\n";
    } else {
        g_out += "unknown mode\n";
    }
}

static std::string unescape(std::string const& escaped)
{
    std::string out;
    for (size_t i = 0; i < escaped.size(); i++) {
        if (escaped[i] == '\\' && i + 1 < escaped.size()) {
            char c = escaped[++i];
            out += c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c;
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
