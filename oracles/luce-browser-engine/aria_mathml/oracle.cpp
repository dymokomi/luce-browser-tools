// Oracle for luce-browser-engine region r13b (ARIA, MathML): runs the reference build's ARIA and
// MathML code on the cases of cases.txt and prints what tests_aria_mathml_cases compares. Each case
// line is "<mode>\t<arg>..." with \n, \t and \\ escaped in the arguments.
//   role <tag> <html>        role_or_default of the first <tag> element: its role name, or none
//   defaultrole <tag> <html> default_role of the first <tag> element: its role name, or none
//   states <tag> <html>      AriaData::build_data of the first <tag> element: every state and
//                            property as its attribute name and string value
//   rolejson <tag> <html>    RoleType::build_role_object(role_or_default, is_focusable, data) and
//                            its serialize_as_json (or the error, for an abstract role)
//   rolename <string>        role_from_string: the role name, or none
//   roles                    every Role: its name and the role categories
//   classes                  every non-abstract Role's role object: accessible_name_required,
//                            children_are_presentational, name_from_source and its tables' sizes
//   global <tag> <html>      has_global_aria_attribute of the first <tag> element
//   idrefs <tag> <attr> <html> parse_id_reference_list of the attribute <attr> of the first <tag>
//   hints <tag> <html>       is_presentational_hint of each attribute of the first <tag> element,
//                            then apply_presentational_hints' properties
//   class <tag> <html>       the class name of the first <tag> element
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/ARIA/ARIAMixin.h>
#include <LibWeb/ARIA/AriaData.h>
#include <LibWeb/ARIA/RoleType.h>
#include <LibWeb/ARIA/Roles.h>
#include <LibWeb/ARIA/StateAndProperties.h>
#include <AK/JsonObjectSerializer.h>
#include <LibWeb/CSS/StyleProperty.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/DOM/Attr.h>
#include <LibWeb/DOM/Comment.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/CSS/PropertyID.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/DocumentFragment.h>
#include <LibWeb/DOM/DocumentType.h>
#include <LibWeb/DOM/Text.h>
#include <LibWeb/DOM/NodeList.h>
#include <LibWeb/HTML/AttributeNames.h>
#include <LibWeb/HTML/HTMLCanvasElement.h>
#include <LibWeb/HTML/HTMLDocument.h>
#include <LibWeb/HTML/HTMLFontElement.h>
#include <LibWeb/HTML/HTMLMarqueeElement.h>
#include <LibWeb/HTML/HTMLScriptElement.h>
#include <LibWeb/HTML/HTMLTemplateElement.h>
#include <LibWeb/HTML/Parser/HTMLParser.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Namespace.h>
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

using namespace Web;

static std::string g_out;
static std::string str(StringView view) { return std::string(view.characters_without_null_termination(), view.length()); }
static std::string str(String const& s) { return str(s.bytes_as_string_view()); }
static std::string str(FlyString const& s) { return str(s.bytes_as_string_view()); }
static std::string str(Utf16String const& s) { return str(s.to_utf8()); }

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

// A new HTML document without a browsing context (as the Luce tests' heap document), parsed from
// `html` with scripting disabled.
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

static std::string role_text(Optional<ARIA::Role> role)
{
    return role.has_value() ? str(ARIA::role_name(*role)) : std::string("none");
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    if (mode == "role") {
        auto document = parse(args[2]);
        g_out += role_text(first_element(*document, args[1])->role_or_default()) + "\n";
    } else if (mode == "defaultrole") {
        auto document = parse(args[2]);
        g_out += role_text(first_element(*document, args[1])->default_role()) + "\n";
    } else if (mode == "states") {
        auto document = parse(args[2]);
        auto data = MUST(ARIA::AriaData::build_data(*first_element(*document, args[1])));
        for (int i = 0; i <= to_underlying(ARIA::StateAndProperties::AriaValueText); i++) {
            auto state = static_cast<ARIA::StateAndProperties>(i);
            g_out += str(ARIA::state_or_property_to_string(state)) + "=" + str(MUST(ARIA::state_or_property_to_string_value(state, *data))) + "\n";
        }
    } else if (mode == "rolejson") {
        auto document = parse(args[2]);
        auto* element = first_element(*document, args[1]);
        auto role = element->role_or_default();
        if (!role.has_value()) {
            g_out += "none\n";
            return;
        }
        auto data = MUST(ARIA::AriaData::build_data(*element));
        auto role_object = ARIA::RoleType::build_role_object(*role, element->is_focusable(), *data);
        if (role_object.is_error()) {
            g_out += "error " + str(role_object.error().string_literal()) + "\n";
            return;
        }
        StringBuilder builder;
        auto object = MUST(JsonObjectSerializer<>::try_create(builder));
        MUST(role_object.value()->serialize_as_json(object));
        MUST(object.finish());
        g_out += str(builder.string_view()) + "\n";
    } else if (mode == "rolename") {
        g_out += role_text(ARIA::role_from_string(StringView(args[1].data(), args[1].size()))) + "\n";
    } else if (mode == "roles") {
        for (int i = 0; i <= to_underlying(ARIA::Role::window); i++) {
            auto role = static_cast<ARIA::Role>(i);
            g_out += str(ARIA::role_name(role)) + " " + std::to_string(ARIA::is_abstract_role(role)) + std::to_string(ARIA::is_widget_role(role))
                + std::to_string(ARIA::is_document_structure_role(role)) + std::to_string(ARIA::is_landmark_role(role))
                + std::to_string(ARIA::is_live_region_role(role)) + std::to_string(ARIA::is_windows_role(role))
                + std::to_string(ARIA::allows_name_from_content(role)) + "\n";
        }
    } else if (mode == "classes") {
        ARIA::AriaData data;
        for (int i = 0; i <= to_underlying(ARIA::Role::window); i++) {
            auto role = static_cast<ARIA::Role>(i);
            if (ARIA::is_abstract_role(role))
                continue;
            for (int focusable = 0; focusable < (role == ARIA::Role::separator ? 2 : 1); focusable++) {
                auto object = MUST(ARIA::RoleType::build_role_object(role, focusable, data));
                g_out += str(ARIA::role_name(role)) + " " + std::to_string(object->accessible_name_required()) + std::to_string(object->children_are_presentational())
                    + " " + std::to_string(to_underlying(object->name_from_source()))
                    + " " + std::to_string(object->supported_states().size()) + "," + std::to_string(object->supported_properties().size())
                    + "," + std::to_string(object->required_states().size()) + "," + std::to_string(object->required_properties().size())
                    + "," + std::to_string(object->prohibited_states().size()) + "," + std::to_string(object->prohibited_properties().size())
                    + " context";
                for (auto context : object->required_context_roles())
                    g_out += " " + str(ARIA::role_name(context));
                g_out += " owned";
                for (auto owned : object->required_owned_elements())
                    g_out += " " + str(ARIA::role_name(owned));
                g_out += " defaults";
                for (int s = 0; s <= to_underlying(ARIA::StateAndProperties::AriaValueText); s++) {
                    auto value = object->default_value_for_property_or_state(static_cast<ARIA::StateAndProperties>(s));
                    if (value.has<Empty>())
                        continue;
                    g_out += " " + str(ARIA::state_or_property_to_string(static_cast<ARIA::StateAndProperties>(s))) + "=";
                    value.visit(
                        [&](Empty) {},
                        [&](f64 v) { g_out += str(String::number(v)); },
                        [&](ARIA::AriaOrientation v) { g_out += "orientation" + std::to_string(to_underlying(v)); },
                        [&](ARIA::AriaLive v) { g_out += "live" + std::to_string(to_underlying(v)); },
                        [&](bool v) { g_out += v ? "true" : "false"; },
                        [&](ARIA::AriaHasPopup v) { g_out += "haspopup" + std::to_string(to_underlying(v)); });
                }
                g_out += "\n";
            }
        }
    } else if (mode == "global") {
        auto document = parse(args[2]);
        g_out += std::to_string(first_element(*document, args[1])->has_global_aria_attribute()) + "\n";
    } else if (mode == "idrefs") {
        auto document = parse(args[3]);
        auto* element = first_element(*document, args[1]);
        auto list = element->parse_id_reference_list(element->get_attribute(FlyString::from_utf8_without_validation(ReadonlyBytes(args[2].data(), args[2].size()))));
        for (auto& id : list)
            g_out += str(id) + "\n";
        g_out += "end\n";
    } else if (mode == "hints") {
        auto document = parse(args[2]);
        auto* element = first_element(*document, args[1]);
        element->for_each_attribute([&](auto const& name, auto const&) {
            g_out += "hint " + str(name) + " " + (element->is_presentational_hint(name) ? "1" : "0") + "\n";
        });
        Vector<CSS::StyleProperty> properties;
        element->apply_presentational_hints(properties);
        for (auto& property : properties)
            g_out += str(CSS::string_from_property_id(property.property_id)) + ": " + str(property.value->to_string(CSS::SerializationMode::Normal)) + "\n";
    } else if (mode == "class") {
        auto document = parse(args[2]);
        g_out += str(first_element(*document, args[1])->class_name()) + "\n";
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
