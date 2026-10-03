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
// r56 (SVG II), cases_r56.txt:
//   grad <html>           every gradient: units, spread method, transform, the linear/radial geometry (through
//                         the href chain) and the offsets of the stops for_each_color_stop finds
//   pattern <html>        every pattern: units, content units, transform, x/y/width/height, the content element
//                         and the SVGAnimatedLength reflections
//   maskclip <html>       every mask and clipPath: units, content units, active viewBox; the masking area of
//                         a 100x50 target at 10,20
//   fe <html>             every filter and filter primitive: their attribute reflections
//   use <html>            every use: x, y, element_transform, the instance root, the shadow tree's children
//                         and its use-document-style-sheets flag; then the same after removing #ref
//   text <html>           every text, tspan and textPath: text_positioning, text_contents, getNumberOfChars,
//                         the x/y/dx/dy/rotate lists, the textPath's path or shape
//   a <html>              every SVG a: target, relList, the default tab index
//   image <html>          every image: x/y/width/height and the bounding box (no image data in phase 1)
//   fo <html>             every foreignObject: its placeholder lengths
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
#include <LibWeb/SVG/SVGAElement.h>
#include <LibWeb/SVG/SVGAnimatedLengthList.h>
#include <LibWeb/SVG/SVGClipPathElement.h>
#include <LibWeb/SVG/SVGFEBlendElement.h>
#include <LibWeb/SVG/SVGFEColorMatrixElement.h>
#include <LibWeb/SVG/SVGFEComponentTransferElement.h>
#include <LibWeb/SVG/SVGFECompositeElement.h>
#include <LibWeb/SVG/SVGFEDisplacementMapElement.h>
#include <LibWeb/SVG/SVGFEDropShadowElement.h>
#include <LibWeb/SVG/SVGFEFloodElement.h>
#include <LibWeb/SVG/SVGFEGaussianBlurElement.h>
#include <LibWeb/SVG/SVGFEImageElement.h>
#include <LibWeb/SVG/SVGFEMergeElement.h>
#include <LibWeb/SVG/SVGFEMergeNodeElement.h>
#include <LibWeb/SVG/SVGFEMorphologyElement.h>
#include <LibWeb/SVG/SVGFEOffsetElement.h>
#include <LibWeb/SVG/SVGFETurbulenceElement.h>
#include <LibWeb/SVG/SVGFilterElement.h>
#include <LibWeb/SVG/SVGForeignObjectElement.h>
#include <LibWeb/SVG/SVGGradientElement.h>
#include <LibWeb/SVG/SVGImageElement.h>
#include <LibWeb/SVG/SVGLengthList.h>
#include <LibWeb/SVG/SVGLinearGradientElement.h>
#include <LibWeb/SVG/SVGMaskElement.h>
#include <LibWeb/SVG/SVGPatternElement.h>
#include <LibWeb/SVG/SVGRadialGradientElement.h>
#include <LibWeb/SVG/SVGStopElement.h>
#include <LibWeb/SVG/SVGTextContentElement.h>
#include <LibWeb/SVG/SVGTextPathElement.h>
#include <LibWeb/SVG/SVGTextPositioningElement.h>
#include <LibWeb/SVG/SVGUseElement.h>
#include <LibWeb/SVG/SVGNumber.h>
#include <LibGfx/ImmutableBitmap.h>
#include <LibWeb/DOM/DOMTokenList.h>
#include <LibWeb/DOM/ShadowRoot.h>
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


// r56: the elements of a document in tree order whose class is T.
template<typename T>
static Vector<T*> all_of(DOM::Document& document)
{
    Vector<T*> found;
    document.for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
        if (auto* typed = as_if<T>(element))
            found.append(typed);
        return TraversalDecision::Continue;
    });
    return found;
}

static std::string numpct(SVG::NumberPercentage const& value)
{
    return bits(value.value()) + (value.is_percentage() ? "%" : "n");
}

static void put_optional_matrix(std::string const& label, Optional<Gfx::AffineTransform> const& matrix)
{
    if (!matrix.has_value()) {
        g_out += label + " none\n";
        return;
    }
    g_out += label + " " + bits(matrix->a()) + " " + bits(matrix->b()) + " " + bits(matrix->c()) + " " + bits(matrix->d()) + " " + bits(matrix->e()) + " " + bits(matrix->f()) + "\n";
}

static void put_view_box_line(std::string const& label, Optional<SVG::ViewBox> const& view_box)
{
    g_out += label + " " + (view_box.has_value() ? bits64(view_box->min_x) + " " + bits64(view_box->min_y) + " " + bits64(view_box->width) + " " + bits64(view_box->height) : std::string("none")) + "\n";
}

static void put_animated_length(std::string const& label, GC::Ref<SVG::SVGAnimatedLength> length)
{
    g_out += label + " " + bits(length->base_val()->value()) + " " + std::to_string(length->base_val()->unit_type()) + " " + bits(length->anim_val()->value()) + "\n";
}

static void put_string(std::string const& label, String const& value)
{
    g_out += label + " [" + str(value) + "]\n";
}

static void put_number(std::string const& label, float value)
{
    g_out += label + " " + bits(value) + "\n";
}

static std::string element_name(DOM::Element const* element)
{
    if (!element)
        return "none";
    auto id = element->get_attribute(HTML::AttributeNames::id);
    return str(element->local_name()) + (id.has_value() ? "#" + str(*id) : std::string());
}

template<typename T>
static void put_fpsa(T& element)
{
    put_animated_length("  x", element.x());
    put_animated_length("  y", element.y());
    put_animated_length("  width", element.width());
    put_animated_length("  height", element.height());
    put_string("  result", element.result()->base_val());
}

static void run_r56(std::vector<std::string> const& args)
{
    auto const& mode = args[0];
    auto document = parse(args[1]);
    if (mode == "grad") {
        for (auto* gradient : all_of<SVG::SVGGradientElement>(*document)) {
            g_out += element_name(gradient) + " units " + std::to_string(to_underlying(gradient->gradient_units())) + " spread " + std::to_string(to_underlying(gradient->spread_method())) + "\n";
            put_optional_matrix("  transform", gradient->gradient_transform());
            if (auto* linear = as_if<SVG::SVGLinearGradientElement>(*gradient))
                g_out += "  linear " + numpct(linear->start_x()) + " " + numpct(linear->start_y()) + " " + numpct(linear->end_x()) + " " + numpct(linear->end_y()) + "\n";
            if (auto* radial = as_if<SVG::SVGRadialGradientElement>(*gradient))
                g_out += "  radial " + numpct(radial->start_circle_x()) + " " + numpct(radial->start_circle_y()) + " " + numpct(radial->start_circle_radius()) + " " + numpct(radial->end_circle_x()) + " " + numpct(radial->end_circle_y()) + " " + numpct(radial->end_circle_radius()) + "\n";
            g_out += "  stops";
            gradient->for_each_color_stop([&](SVG::SVGStopElement& stop) {
                g_out += " " + bits(stop.stop_offset());
            });
            g_out += "\n";
            g_out += "  href " + str(gradient->href()->base_val()) + "\n";
        }
    } else if (mode == "pattern") {
        for (auto* pattern : all_of<SVG::SVGPatternElement>(*document)) {
            g_out += element_name(pattern) + " units " + std::to_string(to_underlying(pattern->pattern_units())) + " content " + std::to_string(to_underlying(pattern->pattern_content_units())) + "\n";
            put_optional_matrix("  transform", pattern->pattern_transform());
            g_out += "  rect " + numpct(pattern->pattern_x()) + " " + numpct(pattern->pattern_y()) + " " + numpct(pattern->pattern_width()) + " " + numpct(pattern->pattern_height()) + "\n";
            g_out += "  content " + element_name(pattern->pattern_content_element().ptr()) + "\n";
            put_animated_length("  x", pattern->x());
            put_animated_length("  y", pattern->y());
            put_animated_length("  width", pattern->width());
            put_animated_length("  height", pattern->height());
            put_view_box_line("  viewbox", pattern->view_box());
        }
    } else if (mode == "maskclip") {
        CSSPixelRect target { 10, 20, 100, 50 };
        for (auto* mask : all_of<SVG::SVGMaskElement>(*document)) {
            g_out += element_name(mask) + " units " + std::to_string(to_underlying(mask->mask_units())) + " content " + std::to_string(to_underlying(mask->mask_content_units())) + "\n";
            put_view_box_line("  active", mask->active_view_box());
            auto area = mask->resolve_masking_area(target);
            g_out += "  area " + std::to_string(area.x().raw_value()) + " " + std::to_string(area.y().raw_value()) + " " + std::to_string(area.width().raw_value()) + " " + std::to_string(area.height().raw_value()) + "\n";
        }
        for (auto* clip : all_of<SVG::SVGClipPathElement>(*document)) {
            g_out += element_name(clip) + " units " + std::to_string(to_underlying(clip->clip_path_units())) + "\n";
            put_view_box_line("  active", clip->active_view_box());
        }
    } else if (mode == "fe") {
        for (auto* filter : all_of<SVG::SVGFilterElement>(*document)) {
            g_out += element_name(filter) + " units " + std::to_string(filter->filter_units()->base_val()) + " primitive " + std::to_string(filter->primitive_units()->base_val()) + "\n";
            put_animated_length("  x", filter->x());
            put_animated_length("  width", filter->width());
        }
        for (auto* element : all_of<SVG::SVGElement>(*document)) {
            if (auto* blend = as_if<SVG::SVGFEBlendElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", blend->in1()->base_val());
                put_string("  in2", blend->in2()->base_val());
                g_out += "  mode " + std::to_string(to_underlying(blend->mode())) + " " + std::to_string(blend->mode_for_bindings()->base_val()) + "\n";
                put_fpsa(*blend);
            } else if (auto* matrix = as_if<SVG::SVGFEColorMatrixElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", matrix->in1()->base_val());
                g_out += "  type " + std::to_string(matrix->type()->base_val()) + "\n";
                put_string("  values", matrix->values()->base_val());
                put_fpsa(*matrix);
            } else if (auto* transfer = as_if<SVG::SVGFEComponentTransferElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", transfer->in1()->base_val());
                put_fpsa(*transfer);
            } else if (auto* composite = as_if<SVG::SVGFECompositeElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", composite->in1()->base_val());
                put_string("  in2", composite->in2()->base_val());
                g_out += "  k " + bits(composite->k1()->base_val()) + " " + bits(composite->k2()->base_val()) + " " + bits(composite->k3()->base_val()) + " " + bits(composite->k4()->base_val()) + "\n";
                g_out += "  operator " + std::to_string(to_underlying(composite->operator_())) + " " + std::to_string(composite->operator_for_bindings()->base_val()) + "\n";
                put_fpsa(*composite);
            } else if (auto* displacement = as_if<SVG::SVGFEDisplacementMapElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", displacement->in1()->base_val());
                put_string("  in2", displacement->in2()->base_val());
                put_number("  scale", displacement->scale()->base_val());
                g_out += "  channels " + std::to_string(displacement->x_channel_selector()->base_val()) + " " + std::to_string(displacement->y_channel_selector()->base_val()) + "\n";
                put_fpsa(*displacement);
            } else if (auto* shadow = as_if<SVG::SVGFEDropShadowElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", shadow->in1()->base_val());
                g_out += "  d " + bits(shadow->dx()->base_val()) + " " + bits(shadow->dy()->base_val()) + "\n";
                g_out += "  std " + bits(shadow->std_deviation_x()->base_val()) + " " + bits(shadow->std_deviation_y()->base_val()) + "\n";
                put_fpsa(*shadow);
                shadow->set_std_deviation(1.25f, 0.1f);
                g_out += "  set [" + str(shadow->get_attribute_value("stdDeviation"_fly_string)) + "] " + bits(shadow->std_deviation_x()->base_val()) + " " + bits(shadow->std_deviation_y()->base_val()) + "\n";
            } else if (auto* flood = as_if<SVG::SVGFEFloodElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_fpsa(*flood);
            } else if (auto* blur = as_if<SVG::SVGFEGaussianBlurElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", blur->in1()->base_val());
                g_out += "  std " + bits(blur->std_deviation_x()->base_val()) + " " + bits(blur->std_deviation_y()->base_val()) + " edge " + std::to_string(blur->edge_mode()->base_val()) + "\n";
                put_fpsa(*blur);
            } else if (auto* image = as_if<SVG::SVGFEImageElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  href", image->href()->base_val());
                g_out += "  bitmap " + std::string(image->current_image_bitmap() ? "yes" : "none") + "\n";
                put_fpsa(*image);
            } else if (auto* merge = as_if<SVG::SVGFEMergeElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_fpsa(*merge);
            } else if (auto* merge_node = as_if<SVG::SVGFEMergeNodeElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", merge_node->in1()->base_val());
            } else if (auto* morphology = as_if<SVG::SVGFEMorphologyElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", morphology->in1()->base_val());
                g_out += "  operator " + std::to_string(to_underlying(morphology->morphology_operator())) + " " + std::to_string(morphology->operator_for_bindings()->base_val()) + "\n";
                g_out += "  radius " + bits(morphology->radius_x()->base_val()) + " " + bits(morphology->radius_y()->base_val()) + "\n";
                put_fpsa(*morphology);
            } else if (auto* offset = as_if<SVG::SVGFEOffsetElement>(*element)) {
                g_out += element_name(element) + "\n";
                put_string("  in", offset->in1()->base_val());
                g_out += "  d " + bits(offset->dx()->base_val()) + " " + bits(offset->dy()->base_val()) + "\n";
                put_fpsa(*offset);
            } else if (auto* turbulence = as_if<SVG::SVGFETurbulenceElement>(*element)) {
                g_out += element_name(element) + "\n";
                g_out += "  frequency " + bits(turbulence->base_frequency_x()->base_val()) + " " + bits(turbulence->base_frequency_y()->base_val()) + "\n";
                g_out += "  octaves " + std::to_string(turbulence->num_octaves()->base_val()) + " seed " + bits(turbulence->seed()->base_val()) + "\n";
                g_out += "  stitch " + std::to_string(turbulence->stitch_tiles()->base_val()) + " type " + std::to_string(turbulence->type()->base_val()) + "\n";
                put_fpsa(*turbulence);
            }
        }
    } else if (mode == "use") {
        auto put_uses = [&](char const* label) {
            for (auto* use : all_of<SVG::SVGUseElement>(*document)) {
                g_out += std::string(label) + " " + element_name(use) + "\n";
                put_animated_length("  x", use->x());
                put_animated_length("  y", use->y());
                auto matrix = use->element_transform();
                g_out += "  matrix " + bits(matrix.a()) + " " + bits(matrix.b()) + " " + bits(matrix.c()) + " " + bits(matrix.d()) + " " + bits(matrix.e()) + " " + bits(matrix.f()) + "\n";
                g_out += "  instance " + element_name(use->instance_root().ptr()) + "\n";
                auto shadow = use->shadow_root();
                g_out += "  shadow " + std::to_string(shadow->child_count()) + " sheets " + std::to_string(shadow->uses_document_style_sheets()) + "\n";
                shadow->for_each_in_subtree_of_type<DOM::Element>([&](DOM::Element& element) {
                    auto width = element.get_attribute(HTML::AttributeNames::width);
                    g_out += "    " + element_name(&element) + (width.has_value() ? " width=" + str(*width) : std::string()) + "\n";
                    return TraversalDecision::Continue;
                });
            }
        };
        put_uses("use");
        if (auto ref = document->get_element_by_id("ref"_fly_string)) {
            ref->remove();
            put_uses("removed");
        }
    } else if (mode == "text") {
        for (auto* text : all_of<SVG::SVGTextContentElement>(*document)) {
            g_out += element_name(text) + " chars " + std::to_string(MUST(text->get_number_of_chars())) + " [" + str(text->text_contents().to_utf8()) + "]\n";
            if (auto* positioning = as_if<SVG::SVGTextPositioningElement>(*text)) {
                auto put_positions = [&](char const* name, Vector<SVG::TextPositioning::Position> const& positions) {
                    g_out += std::string("  ") + name;
                    for (auto const& position : positions) {
                        position.visit(
                            [&](CSS::Number const& number) { g_out += " n" + bits64(number.value()); },
                            [&](CSS::LengthPercentage const& length_percentage) { g_out += " l" + str(length_percentage.to_string(CSS::SerializationMode::Normal)); });
                    }
                    g_out += "\n";
                };
                auto positioning_values = positioning->text_positioning();
                put_positions("x", positioning_values.x);
                put_positions("y", positioning_values.y);
                put_positions("dx", positioning_values.dx);
                put_positions("dy", positioning_values.dy);
                auto put_list = [&](char const* name, GC::Ref<SVG::SVGAnimatedLengthList> list) {
                    g_out += std::string("  list ") + name;
                    for (auto const& item : list->base_val()->items())
                        g_out += " " + bits(item->value()) + "/" + std::to_string(item->unit_type());
                    g_out += "\n";
                };
                put_list("x", positioning->x());
                put_list("y", positioning->y());
                put_list("dx", positioning->dx());
                put_list("dy", positioning->dy());
                g_out += "  rotate";
                for (auto const& item : positioning->rotate()->base_val()->items())
                    g_out += " " + bits(item->value());
                g_out += "\n";
            }
            if (auto* text_path = as_if<SVG::SVGTextPathElement>(*text))
                g_out += "  path " + element_name(text_path->path_or_shape().ptr()) + "\n";
        }
    } else if (mode == "a") {
        for (auto* a : all_of<SVG::SVGAElement>(*document)) {
            g_out += element_name(a) + "\n";
            put_string("  target", a->target()->base_val());
            g_out += "  rel " + std::to_string(a->rel_list()->length()) + " [" + str(a->rel_list()->value()) + "]\n";
            g_out += "  tabindex " + std::to_string(a->default_tab_index_value()) + " activation " + std::to_string(a->has_activation_behavior()) + "\n";
            put_string("  href", a->href()->base_val());
        }
    } else if (mode == "image") {
        for (auto* image : all_of<SVG::SVGImageElement>(*document)) {
            g_out += element_name(image) + "\n";
            put_animated_length("  x", image->x());
            put_animated_length("  y", image->y());
            put_animated_length("  width", image->width());
            put_animated_length("  height", image->height());
            auto box = image->bounding_box();
            g_out += "  box " + bits(box.x()) + " " + bits(box.y()) + " " + bits(box.width()) + " " + bits(box.height()) + "\n";
            g_out += "  available " + std::to_string(image->is_image_available()) + "\n";
        }
    } else if (mode == "fo") {
        for (auto* fo : all_of<SVG::SVGForeignObjectElement>(*document)) {
            g_out += element_name(fo) + "\n";
            put_animated_length("  x", fo->x());
            put_animated_length("  height", fo->height());
        }
    } else {
        g_out += "unknown mode\n";
    }
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
    } else if (mode == "grad" || mode == "pattern" || mode == "maskclip" || mode == "fe" || mode == "use" || mode == "text" || mode == "a" || mode == "image" || mode == "fo") {
        run_r56(args);
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
