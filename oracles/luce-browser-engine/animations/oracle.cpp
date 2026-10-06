// Oracle for luce-browser-engine region p4a (animations and transitions): parses easing functions with the reference
// build's Animations::AnimationEffect::parse_easing_string (CSS::EasingFunction::from_style_value, linear()
// canonicalization), prints their serialization, and evaluates them (EasingFunction::evaluate_at) on a grid of input
// progress values, with and without the before flag. Doubles are printed as their bits; the Luce test
// (web/css/tests_easing_function) prints the same dump and compares it with this output (expected.txt) within 64 ulps.
#include <LibCore/EventLoop.h>
#include <LibWeb/Animations/AnimationEffect.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/EasingFunction.h>
#include <LibWeb/Platform/EventLoopPlugin.h>
#include <cstdio>
#include <cstring>
#include <string>

using namespace Web;

static std::string g_out;

static void hex(double value)
{
    u64 bits;
    memcpy(&bits, &value, sizeof(bits));
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(bits));
    g_out += buffer;
}

// The easing functions of the dump, as CSS text (the Luce test has the same list).
static char const* const s_easings[] = {
    "linear",
    "ease",
    "ease-in",
    "ease-out",
    "ease-in-out",
    "cubic-bezier(0.1, 0.7, 1, 0.1)",
    "cubic-bezier(0.8, 0, 1, 1)",
    "cubic-bezier(0, 1.5, 1, -0.5)",
    "cubic-bezier(1, 0, 0, 1)",
    "cubic-bezier(0, 0, 0, 0)",
    "cubic-bezier(1, 1, 1, 1)",
    "cubic-bezier(0.3, -0.4, 0.7, 1.4)",
    "steps(4)",
    "steps(4, start)",
    "steps(4, end)",
    "steps(3, jump-none)",
    "steps(5, jump-both)",
    "steps(1, jump-start)",
    "steps(2, jump-end)",
    "step-start",
    "step-end",
    "linear(0, 0.25, 1)",
    "linear(0, 0.25 75%, 1)",
    "linear(0 0%, 1 50%, 0 100%)",
    "linear(0, 1 25% 75%, 0)",
    "linear(0.5)",
    "linear(0 20%, 0.5 10%, 1)",
    "linear(0, 0.3, 0.6 40%, 1)",
    "linear(0, 0.1, 0.2, 0.9 90%, 1)",
    "linear(-0.5, 1.5 60%, 1)",
};

// The input progress values every easing function is evaluated at.
static double const s_inputs[] = { -0.5, -0.1, 0.0, 0.1, 0.2, 0.25, 1.0 / 3.0, 0.4, 0.5, 0.6, 0.75, 0.9, 0.999, 1.0, 1.1, 1.5 };

int main()
{
    Core::EventLoop event_loop;
    Web::Platform::EventLoopPlugin::install(*new Web::Platform::EventLoopPlugin);
    Web::Bindings::initialize_main_thread_vm(Web::Bindings::AgentType::SimilarOriginWindow);

    for (auto const* text : s_easings) {
        g_out += "easing ";
        g_out += text;
        g_out += "\n";
        auto easing = Animations::AnimationEffect::parse_easing_string(StringView { text, strlen(text) });
        if (!easing.has_value()) {
            g_out += "invalid\n";
            continue;
        }
        auto string = easing->to_string();
        g_out += "string ";
        g_out += std::string(string.bytes_as_string_view().characters_without_null_termination(), string.bytes().size());
        g_out += "\n";
        for (auto input : s_inputs) {
            for (int before_flag = 0; before_flag < 2; before_flag++) {
                hex(input);
                g_out += before_flag ? " 1 " : " 0 ";
                hex(easing->evaluate_at(input, before_flag));
                g_out += "\n";
            }
        }
    }
    fwrite(g_out.data(), 1, g_out.size(), stdout);
    return 0;
}
