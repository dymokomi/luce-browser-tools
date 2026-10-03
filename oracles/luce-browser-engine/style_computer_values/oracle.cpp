// Oracle for luce-browser-engine region r39 (StyleComputer's computed values): parses each case's
// value for its property with the reference build's Web::CSS parser (in the internal CSS realm),
// runs the StyleComputer function the case names and prints the result's serialization. Each case
// line is "<function>\t<property>\t<value>\t<argument>"; a line starting with "!" needs other
// regions' functions in the port (InitialValues, region r40), and gen_luce_cases.py leaves it out.
// "serialize" prints the parsed value itself: ShorthandStyleValue's serialization expands shorthands
// with StyleComputer::for_each_property_expanding_shorthands.
#define private public
#define protected public
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/CSS/StyleComputer.h>
#include <LibWeb/CSS/StyleValues/StyleValue.h>
#include <LibCore/EventLoop.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#undef private
#undef protected
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Web::CSS;
using namespace Web;

static std::string to_std(String const& s) { return std::string(s.bytes_as_string_view().characters_without_null_termination(), s.bytes_as_string_view().length()); }

static std::string run(std::vector<std::string> const& args)
{
    auto const& function = args[0];
    auto property = property_id_from_string(StringView(args[1].c_str(), args[1].size())).release_value();
    Parser::ParsingParams params { Web::internal_css_realm() };
    auto value = parse_css_value(params, StringView(args[2].c_str(), args[2].size()), property);
    if (!value)
        return "parse error";
    NonnullRefPtr<StyleValue const> v = *value;
    double argument = args.size() > 3 ? std::stod(args[3]) : 0;
    RefPtr<StyleValue const> result;
    if (function == "font_weight")
        result = StyleComputer::compute_font_weight(v, {});
    else if (function == "font_width")
        result = StyleComputer::compute_font_width(v);
    else if (function == "font_style")
        result = StyleComputer::compute_font_style(v);
    else if (function == "line_height")
        result = StyleComputer::compute_line_height(v, CSSPixels::nearest_value_for(argument));
    else if (function == "corner_shape")
        result = StyleComputer::compute_corner_shape(v);
    else if (function == "position_area")
        result = StyleComputer::compute_position_area(v);
    else if (function == "font_feature")
        result = StyleComputer::compute_font_feature_tag_value_list(v);
    else if (function == "animation_name")
        result = StyleComputer::compute_animation_name(v);
    else if (function == "math_depth")
        result = StyleComputer::compute_math_depth(v, {});
    else if (function == "font_size")
        result = StyleComputer::compute_font_size(v, static_cast<int>(argument), {});
    else if (function == "serialize")
        result = v;
    else if (function == "border_width")
        result = StyleComputer::compute_border_or_outline_width(v, argument);
    else
        return "unknown function";
    return to_std(result->to_string(SerializationMode::Normal));
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
        if (raw[0] == '!')
            raw = raw.substr(1);
        std::vector<std::string> args;
        size_t start = 0;
        while (true) {
            auto tab = raw.find('\t', start);
            args.push_back(raw.substr(start, tab == std::string::npos ? std::string::npos : tab - start));
            if (tab == std::string::npos)
                break;
            start = tab + 1;
        }
        std::cout << run(args) << "\n";
    }
}
