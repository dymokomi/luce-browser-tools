// Oracle for luce-browser-engine region r55 (SVG I): runs the reference build's SVG attribute parser,
// path conversion and SVG element classes on the cases of cases.txt and prints what
// svg/tests_svg_cases compares. Each case line is "<mode>\t<arg>..." with \n, \t and \\ escaped.
// Floats print as their bits (f32: 8 hex digits, f64: 16), so the port can compare them exactly on
// macOS and within a few ulps where libm differs.
//   coord|length|integer|numpct|poslen|viewbox|par|units|spread|table|points <s>
//                         the AttributeParser entry point of that name on <s>
//   transform <s>         parse_transform: each transform, then transform_from_transform_list's matrix
//   path <d>              parse_path_data: the instructions, serialize() and to_gfx_path()'s SkPath
//   shape <tag> <w> <h> <html>  get_path(<w>x<h>) of the first <tag> element of the parsed document
//   svgroot <html>        the first svg: viewBox, preserveAspectRatio, active viewBox, width/height
//                         style values from the attributes, the natural metrics
//   xform <tag> <html>    element_transform() of the first <tag> element
//   hints <tag> <html>    is_presentational_hint of each attribute, then apply_presentational_hints
//   role <tag> <html>     the default ARIA role of the first <tag> element
//   lengths <tag> <html>  the SVGAnimatedLength accessors of a rect, ellipse or line: base value and unit
//   href <html>           the first SVG script's href baseVal and the first svg's className baseVal
//   ctf <attr=value>...   an SVGComponentTransferFunctionElement (feFuncR) with those attributes: type,
//                         table values, slope/intercept/amplitude/exponent/offset and the color table
//   animnum <value> <second> <first|second>  an SVGAnimatedNumber of "stdDeviation" on a <g>:
//                         baseVal, then the attribute after setting baseVal to 7.5
//   animint <value> <second> <first|second>  the same with an SVGAnimatedInteger and 7
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/ARIA/Roles.h>
#include <LibWeb/CSS/StyleProperty.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/CSS/PropertyID.h>
#include <LibWeb/DOM/Attr.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/HTML/HTMLDocument.h>
#include <LibWeb/HTML/Parser/HTMLParser.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Namespace.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
#include <LibWeb/SVG/AttributeNames.h>
#include <LibWeb/SVG/AttributeParser.h>
#include <LibWeb/SVG/Path.h>
#include <LibWeb/SVG/SVGAnimatedInteger.h>
#include <LibWeb/SVG/SVGAnimatedLength.h>
#include <LibWeb/SVG/SVGAnimatedNumber.h>
#include <LibWeb/SVG/SVGAnimatedNumberList.h>
#include <LibWeb/SVG/SVGAnimatedEnumeration.h>
#include <LibWeb/SVG/SVGAnimatedString.h>
#include <LibWeb/SVG/SVGAnimatedRect.h>
#include <LibWeb/SVG/SVGComponentTransferFunctionElement.h>
#include <LibWeb/SVG/SVGEllipseElement.h>
#include <LibWeb/SVG/SVGGeometryElement.h>
#include <LibWeb/SVG/SVGGraphicsElement.h>
#include <LibWeb/SVG/SVGLength.h>
#include <LibWeb/SVG/SVGLineElement.h>
#include <LibWeb/SVG/SVGNumberList.h>
#include <LibWeb/SVG/SVGRectElement.h>
#include <LibWeb/SVG/SVGSVGElement.h>
#include <LibWeb/SVG/SVGScriptElement.h>
#include <LibGfx/PathSkia.h>
#include <LibCore/EventLoop.h>
#include <core/SkPath.h>
#undef private
#undef protected
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web;

static std::string g_out;
static std::string str(StringView view) { return std::string(view.characters_without_null_termination(), view.length()); }
static std::string str(String const& s) { return str(s.bytes_as_string_view()); }
static std::string str(FlyString const& s) { return str(s.bytes_as_string_view()); }
static StringView sv(std::string const& s) { return StringView(s.data(), s.size()); }

static std::string bits(float value)
{
    u32 b;
    memcpy(&b, &value, 4);
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%08x", b);
    return buffer;
}

static std::string bits64(double value)
{
    u64 b;
    memcpy(&b, &value, 8);
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%016llx", (unsigned long long)b);
    return buffer;
}

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
    auto parser = HTML::HTMLParser::create(*document, sv(html), HTML::ParserScriptingMode::Disabled, "UTF-8"sv);
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

// The SkPath of a Gfx::Path: its verbs (MLQKCZ), its points' bits and its conic weights' bits.
static void put_path(Gfx::Path const& path)
{
    auto const& sk_path = static_cast<Gfx::PathImplSkia const&>(path.impl()).sk_path();
    std::string verbs = "verbs ";
    for (auto verb : sk_path.verbs()) {
        char const* letters = "MLQKCZ";
        verbs += letters[static_cast<int>(verb)];
    }
    g_out += verbs + "\n";
    std::string weights = "weights";
    for (auto weight : sk_path.conicWeights())
        weights += " " + bits(weight);
    std::string points = "points";
    for (auto point : sk_path.points())
        points += " " + bits(point.x()) + " " + bits(point.y());
    g_out += points + "\n" + weights + "\n";
}

static void put_floats(std::string const& label, Vector<float> const& values)
{
    g_out += label;
    for (auto value : values)
        g_out += " " + bits(value);
    g_out += "\n";
}

static void put_optional_style_value(std::string const& label, RefPtr<CSS::StyleValue const> const& value)
{
    g_out += label + " " + (value ? str(value->to_string(CSS::SerializationMode::Normal)) : std::string("none")) + "\n";
}

static void run(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    if (mode == "coord" || mode == "length" || mode == "poslen") {
        auto value = mode == "coord" ? SVG::AttributeParser::parse_coordinate(sv(args[1]))
            : mode == "length"      ? SVG::AttributeParser::parse_length(sv(args[1]))
                                    : SVG::AttributeParser::parse_positive_length(sv(args[1]));
        g_out += value.has_value() ? bits(*value) + "\n" : "none\n";
    } else if (mode == "integer") {
        auto value = SVG::AttributeParser::parse_integer(sv(args[1]));
        g_out += value.has_value() ? std::to_string(*value) + "\n" : "none\n";
    } else if (mode == "numpct") {
        auto value = SVG::AttributeParser::parse_number_percentage(sv(args[1]));
        if (value.has_value())
            g_out += bits(value->value()) + " " + (value->is_percentage() ? "%" : "n") + " " + bits(value->resolve_relative_to(250)) + "\n";
        else
            g_out += "none\n";
    } else if (mode == "viewbox") {
        auto value = SVG::AttributeParser::parse_viewbox(sv(args[1]));
        if (value.has_value())
            g_out += bits64(value->min_x) + " " + bits64(value->min_y) + " " + bits64(value->width) + " " + bits64(value->height) + "\n";
        else
            g_out += "none\n";
    } else if (mode == "par") {
        auto value = SVG::AttributeParser::parse_preserve_aspect_ratio(sv(args[1]));
        if (value.has_value())
            g_out += std::to_string(to_underlying(value->align)) + " " + std::to_string(to_underlying(value->meet_or_slice)) + "\n";
        else
            g_out += "none\n";
    } else if (mode == "units") {
        auto value = SVG::AttributeParser::parse_units(sv(args[1]));
        g_out += value.has_value() ? std::to_string(to_underlying(*value)) + "\n" : "none\n";
    } else if (mode == "spread") {
        auto value = SVG::AttributeParser::parse_spread_method(sv(args[1]));
        g_out += value.has_value() ? std::to_string(to_underlying(*value)) + "\n" : "none\n";
    } else if (mode == "table") {
        put_floats("table", SVG::AttributeParser::parse_table_values(sv(args[1])));
    } else if (mode == "points") {
        g_out += "points";
        for (auto point : SVG::AttributeParser::parse_points(sv(args[1])))
            g_out += " " + bits(point.x()) + " " + bits(point.y());
        g_out += "\n";
    } else if (mode == "transform") {
        auto list = SVG::AttributeParser::parse_transform(sv(args[1]));
        if (!list.has_value()) {
            g_out += "none\n";
            return;
        }
        for (auto& transform : *list) {
            transform.operation.visit(
                [&](SVG::Transform::Translate const& t) { g_out += "translate " + bits(t.x) + " " + bits(t.y) + "\n"; },
                [&](SVG::Transform::Scale const& t) { g_out += "scale " + bits(t.x) + " " + bits(t.y) + "\n"; },
                [&](SVG::Transform::Rotate const& t) { g_out += "rotate " + bits(t.a) + " " + bits(t.x) + " " + bits(t.y) + "\n"; },
                [&](SVG::Transform::SkewX const& t) { g_out += "skewX " + bits(t.a) + "\n"; },
                [&](SVG::Transform::SkewY const& t) { g_out += "skewY " + bits(t.a) + "\n"; },
                [&](SVG::Transform::Matrix const& t) { g_out += "matrix " + bits(t.a) + " " + bits(t.b) + " " + bits(t.c) + " " + bits(t.d) + " " + bits(t.e) + " " + bits(t.f) + "\n"; });
        }
        auto matrix = SVG::transform_from_transform_list(*list);
        g_out += "matrix " + bits(matrix.a()) + " " + bits(matrix.b()) + " " + bits(matrix.c()) + " " + bits(matrix.d()) + " " + bits(matrix.e()) + " " + bits(matrix.f()) + "\n";
    } else if (mode == "path") {
        auto path = SVG::AttributeParser::parse_path_data(sv(args[1]));
        for (auto& instruction : path.instructions()) {
            g_out += "instruction " + std::to_string(to_underlying(instruction.type)) + " " + (instruction.absolute ? "1" : "0");
            for (auto value : instruction.data)
                g_out += " " + bits(value);
            g_out += "\n";
        }
        g_out += "serialize " + str(path.serialize()) + "\n";
        put_path(path.to_gfx_path());
    } else if (mode == "shape") {
        auto document = parse(args[4]);
        auto& element = as<SVG::SVGGeometryElement>(*first_element(*document, args[1]));
        CSSPixelSize viewport { CSSPixels(std::stoi(args[2])), CSSPixels(std::stoi(args[3])) };
        put_path(element.get_path(viewport));
    } else if (mode == "svgroot") {
        auto document = parse(args[1]);
        auto& svg = as<SVG::SVGSVGElement>(*first_element(*document, "svg"));
        auto view_box = svg.view_box();
        g_out += "viewbox " + (view_box.has_value() ? bits64(view_box->min_x) + " " + bits64(view_box->min_y) + " " + bits64(view_box->width) + " " + bits64(view_box->height) : std::string("none")) + "\n";
        auto par = svg.preserve_aspect_ratio();
        g_out += "par " + (par.has_value() ? std::to_string(to_underlying(par->align)) + " " + std::to_string(to_underlying(par->meet_or_slice)) : std::string("none")) + "\n";
        auto active = svg.active_view_box();
        g_out += "active " + (active.has_value() ? bits64(active->min_x) + " " + bits64(active->min_y) + " " + bits64(active->width) + " " + bits64(active->height) : std::string("none")) + "\n";
        g_out += "nulled " + std::to_string(svg.view_box_for_bindings()->m_nulled) + "\n";
        put_optional_style_value("width", svg.width_style_value_from_attribute());
        put_optional_style_value("height", svg.height_style_value_from_attribute());
        auto metrics = SVG::SVGSVGElement::negotiate_natural_metrics(svg);
        g_out += "natural " + (metrics.width.has_value() ? std::to_string(metrics.width->raw_value()) : std::string("none"))
            + " " + (metrics.height.has_value() ? std::to_string(metrics.height->raw_value()) : std::string("none"))
            + " " + (metrics.aspect_ratio.has_value() ? std::to_string(metrics.aspect_ratio->numerator().raw_value()) + "/" + std::to_string(metrics.aspect_ratio->denominator().raw_value()) : std::string("none")) + "\n";
    } else if (mode == "xform") {
        auto document = parse(args[2]);
        auto& element = as<SVG::SVGGraphicsElement>(*first_element(*document, args[1]));
        auto matrix = element.element_transform();
        g_out += "matrix " + bits(matrix.a()) + " " + bits(matrix.b()) + " " + bits(matrix.c()) + " " + bits(matrix.d()) + " " + bits(matrix.e()) + " " + bits(matrix.f()) + "\n";
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
    } else if (mode == "role") {
        auto document = parse(args[2]);
        auto* element = first_element(*document, args[1]);
        auto role = element->default_role();
        g_out += role.has_value() ? std::to_string(to_underlying(*role)) + "\n" : "none\n";
    } else if (mode == "lengths") {
        auto document = parse(args[2]);
        auto* element = first_element(*document, args[1]);
        auto put = [&](char const* name, GC::Ref<SVG::SVGAnimatedLength> length) {
            g_out += std::string(name) + " " + bits(length->base_val()->value()) + " " + std::to_string(length->base_val()->unit_type()) + " " + bits(length->anim_val()->value()) + "\n";
        };
        if (auto* rect = as_if<SVG::SVGRectElement>(*element)) {
            put("x", rect->x());
            put("y", rect->y());
            put("width", rect->width());
            put("height", rect->height());
            put("rx", rect->rx());
            put("ry", rect->ry());
        } else if (auto* ellipse = as_if<SVG::SVGEllipseElement>(*element)) {
            put("cx", ellipse->cx());
            put("cy", ellipse->cy());
            put("rx", ellipse->rx());
            put("ry", ellipse->ry());
        } else if (auto* line = as_if<SVG::SVGLineElement>(*element)) {
            put("x1", line->x1());
            put("y1", line->y1());
            put("x2", line->x2());
            put("y2", line->y2());
        }
    } else if (mode == "href") {
        auto document = parse(args[1]);
        auto& script = as<SVG::SVGScriptElement>(*first_element(*document, "script"));
        g_out += "href " + str(script.href()->base_val()) + "\n";
        auto& svg = as<SVG::SVGElement>(*first_element(*document, "svg"));
        g_out += "class " + str(svg.class_name()->base_val()) + "\n";
    } else if (mode == "ctf") {
        auto document = parse("<!DOCTYPE html><body>");
        auto created = MUST(DOM::create_element(*document, "feFuncR"_fly_string, Namespace::SVG));
        auto* element = &as<SVG::SVGComponentTransferFunctionElement>(*created);
        for (size_t i = 1; i < args.size(); i++) {
            auto equals = args[i].find('=');
            auto name = FlyString::from_utf8_without_validation(ReadonlyBytes(args[i].data(), equals));
            auto value = MUST(String::from_utf8(sv(args[i].substr(equals + 1))));
            element->set_attribute_value(name, value);
        }
        g_out += "type " + std::to_string(element->type()->base_val()) + "\n";
        put_floats("table", element->table_float_values());
        g_out += "numbers " + bits(element->slope()->base_val()) + " " + bits(element->intercept()->base_val()) + " " + bits(element->amplitude()->base_val()) + " " + bits(element->exponent()->base_val()) + " " + bits(element->offset()->base_val()) + "\n";
        g_out += "colors";
        char buffer[4];
        for (auto byte : element->color_table()) {
            snprintf(buffer, sizeof(buffer), "%02x", byte);
            g_out += buffer;
        }
        g_out += "\n";
    } else if (mode == "animnum" || mode == "animint") {
        auto document = parse("<!DOCTYPE html><svg><g></g></svg>");
        auto& g = as<SVG::SVGElement>(*first_element(*document, "g"));
        if (args[1] != "-")
            g.set_attribute_value("stdDeviation"_fly_string, MUST(String::from_utf8(sv(args[1]))));
        DOM::QualifiedName name { "stdDeviation"_fly_string, {}, {} };
        if (mode == "animnum") {
            auto number = SVG::SVGAnimatedNumber::create(g.realm(), g, name, 2.f,
                args[2] == "1" ? SVG::SVGAnimatedNumber::SupportsSecondValue::Yes : SVG::SVGAnimatedNumber::SupportsSecondValue::No,
                args[3] == "second" ? SVG::SVGAnimatedNumber::ValueRepresented::Second : SVG::SVGAnimatedNumber::ValueRepresented::First);
            g_out += "base " + bits(number->base_val()) + "\n";
            number->set_base_val(7.5f);
        } else {
            auto integer = SVG::SVGAnimatedInteger::create(g.realm(), g, name, 2,
                args[2] == "1" ? SVG::SVGAnimatedInteger::SupportsSecondValue::Yes : SVG::SVGAnimatedInteger::SupportsSecondValue::No,
                args[3] == "second" ? SVG::SVGAnimatedInteger::ValueRepresented::Second : SVG::SVGAnimatedInteger::ValueRepresented::First);
            g_out += "base " + std::to_string(integer->base_val()) + "\n";
            integer->set_base_val(7);
        }
        g_out += "attribute " + str(g.get_attribute_value("stdDeviation"_fly_string)) + "\n";
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
