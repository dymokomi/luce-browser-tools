// Oracle for luce-browser-engine region r41 (CSS fonts: FontComputer, FontFace, FontFaceSet data, FontFeatureData,
// FontLoading, ParsedFontFace). Each case line of cases.txt is "<mode>\t<argument>[\t<argument>...]"; the oracle
// runs the reference build's code and prints one line, in the format the Luce test (css/tests_fonts) prints:
//
//   type       font_feature_value_type_from_string(arg)
//   features   a ComputedProperties built from the declarations in arg 1 (r40's oracle: initial values, the declared
//              values, StyleComputer::compute_value_of_property at 16px in 800x600), its font_feature_data(), the
//              shape features of to_shape_features over the @font-feature-values map in arg 2
//              ("type:name=v,v;..."), and the AK::Traits hashes of the data and of each map key
//   format     font_format_is_supported(arg); tech: font_tech_is_supported(FlyString)
//   load       requires_off_thread_vector_font_preparation and try_load_vector_font of a test font file (arg 1)
//              with the mime type essence arg 2 ("-" for none)
//   fontface   an @font-face rule with the descriptors arg, in a sheet of a document: is_valid() and every member
//              of its ParsedFontFace (ParsedFontFace::from_descriptors)
//   facedesc   a FontFace's set_<descriptor>_impl with the descriptor (arg 1) parsed from arg 2: the attribute,
//              the cached weight range, slope and width and the unicode ranges; facesetter: the public setter
//   match      FontComputer::compute_font_for_style_values (through ComputedProperties::computed_font_list) for the
//              declarations in arg 1, with the font faces of arg 2 registered (";"-separated
//              "family|min|max|slope|width|file|U+range,..."; faces after "||" are registered after the first
//              computation, which is then repeated)
//
// The font database holds the fonts of the engine's tests/libweb/fonts only (a SystemFontProvider over a
// PathFontProvider loading that directory), and the FontPlugin is in layout test mode, as Ladybird's test-web runs.
#define private public
#define protected public
#include <LibCore/EventLoop.h>
#include <LibGfx/Font/Font.h>
#include <LibGfx/Font/FontData.h>
#include <LibGfx/Font/FontDatabase.h>
#include <LibGfx/Font/PathFontProvider.h>
#include <LibGfx/Font/Typeface.h>
#include <LibGfx/FontCascadeList.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/CSSFontFaceDescriptors.h>
#include <LibWeb/CSS/CSSFontFaceRule.h>
#include <LibWeb/CSS/CSSRuleList.h>
#include <LibWeb/CSS/CSSStyleSheet.h>
#include <LibWeb/CSS/ComputedProperties.h>
#include <LibWeb/CSS/FontComputer.h>
#include <LibWeb/CSS/FontFace.h>
#include <LibWeb/CSS/FontFeatureData.h>
#include <LibWeb/CSS/FontLoading.h>
#include <LibWeb/CSS/ParsedFontFace.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/StyleComputer.h>
#include <LibWeb/CSS/StyleValues/ComputationContext.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/HTML/Scripting/TemporaryExecutionContext.h>
#include <LibWeb/HTML/TraversableNavigable.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <LibWeb/Platform/FontPlugin.h>
#include <LibWeb/WebIDL/Promise.h>
#undef private
#undef protected
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace Web::CSS;
using namespace Web;

static std::string g_fonts_directory = "/Users/sedov/Dev/luce_dev/luce-browser-engine-r41/tests/libweb/fonts";

static std::string to_std(StringView s) { return std::string(s.characters_without_null_termination(), s.length()); }
static std::string to_std(String const& s) { return to_std(s.bytes_as_string_view()); }
static std::string to_std(FlyString const& s) { return to_std(s.bytes_as_string_view()); }
static std::string num(double value) { return to_std(MUST(String::formatted("{}", value))); }
static std::string integer(long long value) { return std::to_string(value); }
template<typename E>
static std::string en(E value) { return std::to_string(static_cast<long long>(to_underlying(value))); }

static std::vector<std::string> split(std::string const& text, char separator)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        auto end = text.find(separator, start);
        parts.push_back(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return parts;
}

static std::string trim(std::string text)
{
    while (!text.empty() && text.front() == ' ')
        text.erase(0, 1);
    while (!text.empty() && text.back() == ' ')
        text.pop_back();
    return text;
}

// mark: The page, documents and fonts ===========================================================

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

// The active document of a top-level traversable (it has a Window), made once: its realm makes the test documents.
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

static std::vector<GC::Root<DOM::Document>> g_documents;

// A fresh document without a Window (its own FontComputer), as the Luce test's document.
static DOM::Document& fresh_document()
{
    g_documents.push_back(GC::make_root(DOM::Document::create(window_document().realm())));
    return *g_documents.back();
}

// The SystemFontProvider of the oracle: the fonts of one directory (a PathFontProvider that is not one itself, so
// FontPlugin's constructor does not add the system's font directories).
class OracleFontProvider final : public Gfx::SystemFontProvider {
public:
    Gfx::PathFontProvider path;
    virtual StringView name() const override { return "Oracle"sv; }
    virtual RefPtr<Gfx::Font> get_font(FlyString const& family, float point_size, unsigned weight, unsigned width, unsigned slope, Optional<Gfx::FontVariationSettings> const& variations = {}, Optional<Gfx::ShapeFeatures> const& features = {}) override
    {
        return path.get_font(family, point_size, weight, width, slope, variations, features);
    }
    virtual void for_each_typeface_with_family_name(FlyString const& family_name, Function<void(Gfx::Typeface const&)> callback) override
    {
        path.for_each_typeface_with_family_name(family_name, move(callback));
    }
};

static ByteBuffer read_font_file(std::string const& name)
{
    std::ifstream file(g_fonts_directory + "/" + name, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    VERIFY(!bytes.empty());
    return MUST(ByteBuffer::copy(bytes.data(), bytes.size()));
}

static NonnullRefPtr<Gfx::Typeface const> typeface_of_file(std::string const& name)
{
    return MUST(try_load_vector_font(read_font_file(name)));
}

// mark: Printing ================================================================================

// A font: its family, slope, typeface width and point size, the variation axes (sorted) and the shape features
// it was made with. (The weight is left out: a variable font's instance reports the "wght" it was made with, and
// luce-browser-render does not instance variable fonts yet; the axes say what the font computer asked for.)
static std::string font(Gfx::Font const& font)
{
    std::string out = to_std(font.family()) + "/" + integer(font.slope()) + "/" + integer(font.typeface().width()) + "/" + num(static_cast<double>(font.point_size())) + "<";
    bool first = true;
    for (auto const& axis : font.m_font_variation_settings.to_sorted_list()) {
        out += (first ? "" : ",") + std::string(axis.tag.cc, 4) + "=" + num(static_cast<double>(axis.value));
        first = false;
    }
    out += ">[";
    first = true;
    for (auto const& feature : font.m_shape_features) {
        out += (first ? "" : ",") + std::string(feature.tag, 4) + "=" + integer(feature.value);
        first = false;
    }
    return out + "]";
}

static std::string font_list(Gfx::FontCascadeList const& list)
{
    std::string out = "[";
    bool first = true;
    list.for_each_font_entry([&](Gfx::FontCascadeList::Entry const& entry) {
        if (!first)
            out += " ";
        first = false;
        out += font(*entry.font);
        if (entry.range_data.has_value()) {
            out += "{" + to_std(entry.range_data->enclosing_range.to_string()) + ":" + integer(entry.range_data->unicode_ranges.size()) + "}";
        }
    });
    out += "] pending " + integer(list.m_pending_faces.size());
    if (list.m_last_resort_font)
        out += " last " + font(*list.m_last_resort_font);
    return out;
}

static std::string optional_percentage(Optional<Percentage> const& value)
{
    return value.has_value() ? num(value->value()) : "-";
}

static std::string optional_fly(Optional<FlyString> const& value)
{
    return value.has_value() ? to_std(*value) : "-";
}

// mark: ComputedProperties (r40's oracle) =======================================================

static Length::FontMetrics font_metrics()
{
    Gfx::FontPixelMetrics metrics;
    metrics.size = 16;
    metrics.x_height = 7.5f;
    metrics.advance_of_ascii_zero = 9.25f;
    metrics.ascent = 12.796875f;
    return Length::FontMetrics(CSSPixels(16), metrics, CSSPixels(18.5));
}

static Length::FontMetrics root_font_metrics()
{
    Gfx::FontPixelMetrics metrics;
    metrics.size = 20;
    metrics.x_height = 8.0f;
    metrics.advance_of_ascii_zero = 10.0f;
    metrics.ascent = 15.0f;
    return Length::FontMetrics(CSSPixels(20), metrics, CSSPixels(22));
}

static GC::Ref<ComputedProperties> build(std::string const& declarations)
{
    std::map<PropertyID, NonnullRefPtr<StyleValue const>> specified;
    for (auto i = to_underlying(first_longhand_property_id); i <= to_underlying(last_longhand_property_id); ++i)
        specified.emplace(static_cast<PropertyID>(i), property_initial_value(static_cast<PropertyID>(i)));
    Parser::ParsingParams params { Web::internal_css_realm() };
    for (auto const& declaration : split(declarations, ';')) {
        auto colon = declaration.find(':');
        if (colon == std::string::npos)
            continue;
        auto name = trim(declaration.substr(0, colon));
        auto value = trim(declaration.substr(colon + 1));
        auto id = property_id_from_string(StringView(name.c_str(), name.size())).release_value();
        auto parsed = parse_css_value(params, StringView(value.c_str(), value.size()), id);
        if (!parsed) {
            std::cerr << "PARSE " << name << ": " << value << std::endl;
            VERIFY_NOT_REACHED();
        }
        specified.insert_or_assign(id, *parsed);
    }
    ComputationContext context { .length_resolution_context = Length::ResolutionContext { CSSPixelRect(0, 0, 800, 600), font_metrics(), root_font_metrics() } };
    auto style = Web::Bindings::main_thread_vm().heap().allocate<ComputedProperties>();
    for (auto& [id, value] : specified) {
        auto computed = StyleComputer::compute_value_of_property(id, value, [&](PropertyID other) { return specified.at(other); }, context, 1.0);
        style->set_property(id, computed);
    }
    return style;
}

// mark: The modes ===============================================================================

static Optional<FontFeatureValueType> value_type(std::string const& name)
{
    return font_feature_value_type_from_string(FlyString::from_utf8_without_validation(StringView(name.c_str(), name.size()).bytes()));
}

static std::string run_features(std::string const& declarations, std::string const& values_spec)
{
    HashMap<FontFeatureValueKey, Vector<u32>> values;
    std::string key_hashes;
    if (!values_spec.empty()) {
        for (auto const& entry : split(values_spec, ';')) {
            auto colon = entry.find(':');
            auto equals = entry.find('=');
            auto type = value_type(entry.substr(0, colon)).release_value();
            auto name = entry.substr(colon + 1, equals - colon - 1);
            Vector<u32> numbers;
            for (auto const& number : split(entry.substr(equals + 1), ','))
                numbers.append(static_cast<u32>(std::stoul(number)));
            FontFeatureValueKey key { type, FlyString::from_utf8_without_validation(StringView(name.c_str(), name.size()).bytes()) };
            key_hashes += " " + integer(Traits<FontFeatureValueKey>::hash(key));
            values.set(key, numbers);
        }
    }
    auto style = build(declarations);
    auto data = style->font_feature_data();
    std::string out;
    for (auto const& feature : data.to_shape_features(values)) {
        if (!out.empty())
            out += ",";
        out += std::string(feature.tag, 4) + "=" + integer(feature.value);
    }
    out += " | hash " + integer(Traits<FontFeatureData>::hash(data)) + " | keys" + key_hashes;
    return out;
}

static std::string run_load(std::string const& file, std::string const& mime)
{
    auto bytes = read_font_file(file);
    Optional<ByteString> mime_type_essence;
    if (mime != "-")
        mime_type_essence = ByteString(StringView(mime.c_str(), mime.size()));
    std::string out = "off-thread " + integer(requires_off_thread_vector_font_preparation(bytes, mime_type_essence) ? 1 : 0);
    if (requires_off_thread_vector_font_preparation(bytes, mime_type_essence))
        return out;
    auto result = try_load_vector_font(bytes, mime_type_essence);
    if (result.is_error())
        return out + " error " + to_std(result.error().string_literal());
    auto typeface = result.release_value();
    return out + " " + to_std(typeface->family()) + " " + integer(typeface->weight());
}

static std::string run_fontface(std::string const& descriptors)
{
    auto& document = fresh_document();
    auto text = "@font-face { " + descriptors + " }";
    auto sheet = parse_css_stylesheet(Parser::ParsingParams { document }, StringView(text.c_str(), text.size()));
    sheet->m_owning_documents_or_shadow_roots.set(&document);
    if (sheet->rules().length() == 0)
        return "no rule";
    auto& rule = as<CSSFontFaceRule>(*sheet->rules().item(0));
    std::string out = "valid " + integer(rule.is_valid() ? 1 : 0);
    auto face = rule.font_face();
    out += " family " + to_std(face.font_family());
    out += " weight " + (face.weight().has_value() ? integer(face.weight()->min) + "-" + integer(face.weight()->max) : std::string("-"));
    out += " slope " + (face.slope().has_value() ? integer(*face.slope()) : std::string("-"));
    out += " width " + (face.width().has_value() ? integer(*face.width()) : std::string("-"));
    out += " sources [";
    bool first = true;
    for (auto const& source : face.sources()) {
        if (!first)
            out += " ";
        first = false;
        source.local_or_url.visit(
            [&](FlyString const& local) { out += "local(" + to_std(local) + ")"; },
            [&](Web::CSS::URL const& url) { out += "url(" + to_std(url.to_string()) + ")"; });
        out += "/" + optional_fly(source.format) + "/";
        for (size_t i = 0; i < source.tech.size(); ++i)
            out += (i ? "," : "") + en(source.tech[i]);
    }
    out += "] ranges [";
    for (size_t i = 0; i < face.unicode_ranges().size(); ++i)
        out += (i ? " " : "") + to_std(face.unicode_ranges()[i].to_string());
    out += "] ascent " + optional_percentage(face.ascent_override()) + " descent " + optional_percentage(face.descent_override()) + " line-gap " + optional_percentage(face.line_gap_override());
    out += " display " + en(face.font_display()) + " named " + optional_fly(face.font_named_instance()) + " language " + optional_fly(face.font_language_override());
    out += " features ";
    if (auto settings = face.font_feature_settings(); settings.has_value()) {
        out += "[";
        bool first_setting = true;
        for (auto const& [tag, value] : *settings) {
            out += (first_setting ? "" : ",") + to_std(tag) + ":" + integer(value);
            first_setting = false;
        }
        out += "]";
    } else {
        out += "-";
    }
    out += " variations ";
    if (auto settings = face.font_variation_settings(); settings.has_value()) {
        out += "[";
        bool first_setting = true;
        for (auto const& [tag, value] : *settings) {
            out += (first_setting ? "" : ",") + to_std(tag) + ":" + num(value);
            first_setting = false;
        }
        out += "]";
    } else {
        out += "-";
    }
    return out;
}

static GC::Ref<FontFace> make_face()
{
    auto& realm = window_document().realm();
    HTML::TemporaryExecutionContext context { realm };
    return realm.create<FontFace>(realm, WebIDL::create_promise(realm));
}

static std::string face_state(FontFace const& face, String const& attribute)
{
    return to_std(attribute) + " | weight " + integer(face.m_cached_weight_range.min) + "-" + integer(face.m_cached_weight_range.max) + " slope " + integer(face.m_cached_slope) + " width " + integer(face.m_cached_width) + " ranges " + integer(face.m_unicode_ranges.size());
}

static std::string run_facedesc(std::string const& descriptor, std::string const& value, bool public_setter)
{
    auto face = make_face();
    auto string = MUST(String::from_utf8(StringView(value.c_str(), value.size())));
    static std::map<std::string, std::pair<DescriptorID, int>> const descriptors = {
        { "family", { DescriptorID::FontFamily, 0 } },
        { "style", { DescriptorID::FontStyle, 1 } },
        { "weight", { DescriptorID::FontWeight, 2 } },
        { "stretch", { DescriptorID::FontWidth, 3 } },
        { "unicode-range", { DescriptorID::UnicodeRange, 4 } },
        { "feature-settings", { DescriptorID::FontFeatureSettings, 5 } },
        { "variation-settings", { DescriptorID::FontVariationSettings, 6 } },
        { "display", { DescriptorID::FontDisplay, 7 } },
        { "ascent-override", { DescriptorID::AscentOverride, 8 } },
        { "descent-override", { DescriptorID::DescentOverride, 9 } },
        { "line-gap-override", { DescriptorID::LineGapOverride, 10 } },
    };
    auto [id, index] = descriptors.at(descriptor);
    if (public_setter) {
        auto call = [&]() -> WebIDL::ExceptionOr<void> {
            switch (index) {
            case 0: return face->set_family(string);
            case 1: return face->set_style(string);
            case 2: return face->set_weight(string);
            case 3: return face->set_stretch(string);
            case 4: return face->set_unicode_range(string);
            case 5: return face->set_feature_settings(string);
            case 6: return face->set_variation_settings(string);
            case 7: return face->set_display(string);
            case 8: return face->set_ascent_override(string);
            case 9: return face->set_descent_override(string);
            default: return face->set_line_gap_override(string);
            }
        };
        auto result = call();
        if (result.is_exception())
            return "exception";
    } else {
        auto parsed = parse_css_descriptor(Parser::ParsingParams(), AtRuleID::FontFace, DescriptorNameAndID::from_id(id), string);
        if (!parsed)
            return "parse error";
        auto value_ref = parsed.release_nonnull();
        switch (index) {
        case 0: face->set_family_impl(value_ref); break;
        case 1: face->set_style_impl(value_ref); break;
        case 2: face->set_weight_impl(value_ref); break;
        case 3: face->set_stretch_impl(value_ref); break;
        case 4: face->set_unicode_range_impl(value_ref); break;
        case 5: face->set_feature_settings_impl(value_ref); break;
        case 6: face->set_variation_settings_impl(value_ref); break;
        case 7: face->set_display_impl(value_ref); break;
        case 8: face->set_ascent_override_impl(value_ref); break;
        case 9: face->set_descent_override_impl(value_ref); break;
        default: face->set_line_gap_override_impl(value_ref); break;
        }
    }
    String attribute;
    switch (index) {
    case 0: attribute = face->family(); break;
    case 1: attribute = face->style(); break;
    case 2: attribute = face->weight(); break;
    case 3: attribute = face->stretch(); break;
    case 4: attribute = face->unicode_range(); break;
    case 5: attribute = face->feature_settings(); break;
    case 6: attribute = face->variation_settings(); break;
    case 7: attribute = face->display(); break;
    case 8: attribute = face->ascent_override(); break;
    case 9: attribute = face->descent_override(); break;
    default: attribute = face->line_gap_override(); break;
    }
    return face_state(*face, attribute);
}

// "family|min|max|slope|width|file|U+range,...": a loaded FontFace (its typeface parsed from the file), registered
// with the document's FontComputer.
static void register_face(DOM::Document& document, std::string const& spec)
{
    auto fields = split(spec, '|');
    auto face = make_face();
    face->m_family = MUST(String::from_utf8(StringView(fields[0].c_str(), fields[0].size())));
    face->m_cached_weight_range = { std::stoi(fields[1]), std::stoi(fields[2]) };
    face->m_cached_slope = std::stoi(fields[3]);
    face->m_cached_width = std::stoi(fields[4]);
    face->m_parsed_font = typeface_of_file(fields[5]);
    face->m_unicode_ranges.clear();
    if (fields.size() > 6) {
        for (auto const& range : split(fields[6], ',')) {
            auto dash = range.find('-');
            auto min = std::stoul(range.substr(2, dash - 2), nullptr, 16);
            auto max = std::stoul(range.substr(dash + 1), nullptr, 16);
            face->m_unicode_ranges.append(Gfx::UnicodeRange { static_cast<u32>(min), static_cast<u32>(max) });
        }
    } else {
        face->m_unicode_ranges.append(Gfx::UnicodeRange { 0, 0x10FFFF });
    }
    face->m_status = Bindings::FontFaceLoadStatus::Loaded;
    document.font_computer().register_font_face(face);
}

static std::string run_match(std::string const& declarations, std::string const& faces)
{
    auto& document = fresh_document();
    auto groups = faces.find("||");
    auto before = faces.substr(0, groups);
    for (auto const& spec : split(before, ';')) {
        if (!spec.empty())
            register_face(document, spec);
    }
    auto style = build(declarations);
    auto out = font_list(*style->computed_font_list(document.font_computer()));
    if (groups == std::string::npos)
        return out;
    for (auto const& spec : split(faces.substr(groups + 2), ';')) {
        if (!spec.empty())
            register_face(document, spec);
    }
    auto again = build(declarations);
    return out + " >> " + font_list(*again->computed_font_list(document.font_computer()));
}

static std::string run(std::vector<std::string> const& fields)
{
    auto const& mode = fields[0];
    auto arg = [&](size_t i) { return i < fields.size() ? fields[i] : std::string(); };
    if (mode == "type") {
        auto type = value_type(arg(1));
        return type.has_value() ? en(*type) : "-";
    }
    if (mode == "features")
        return run_features(arg(1), arg(2));
    if (mode == "format")
        return integer(font_format_is_supported(FlyString::from_utf8_without_validation(StringView(arg(1).c_str(), arg(1).size()).bytes())) ? 1 : 0);
    if (mode == "tech")
        return integer(font_tech_is_supported(FlyString::from_utf8_without_validation(StringView(arg(1).c_str(), arg(1).size()).bytes())) ? 1 : 0);
    if (mode == "load")
        return run_load(arg(1), arg(2));
    if (mode == "fontface")
        return run_fontface(arg(1));
    if (mode == "facedesc")
        return run_facedesc(arg(1), arg(2), false);
    if (mode == "facesetter")
        return run_facedesc(arg(1), arg(2), true);
    if (mode == "match")
        return run_match(arg(1), arg(2));
    return "unknown mode";
}

int main(int argc, char** argv)
{
    Core::EventLoop event_loop;
    Web::Platform::EventLoopPlugin::install(*new Web::Platform::EventLoopPlugin);
    auto& provider = static_cast<OracleFontProvider&>(Gfx::FontDatabase::the().install_system_font_provider(make<OracleFontProvider>()));
    provider.path.load_all_fonts_from_uri(MUST(String::formatted("file://{}", StringView(g_fonts_directory.c_str(), g_fonts_directory.size()))));
    Web::Platform::FontPlugin::install(*new Web::Platform::FontPlugin(true, &provider));
    Web::Bindings::initialize_main_thread_vm(Web::Bindings::AgentType::SimilarOriginWindow);
    std::ifstream in(argc > 1 ? argv[1] : "cases.txt");
    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty() || raw[0] == '#')
            continue;
        if (raw[0] == '!')
            raw = raw.substr(1);
        std::cerr << raw << std::endl;
        std::cout << run(split(raw, '\t')) << std::endl;
    }
}
