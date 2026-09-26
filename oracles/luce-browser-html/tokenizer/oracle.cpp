/*
 * Local oracle for luce-browser-html region r20: runs Ladybird's HTMLTokenizer (reference build)
 * over a list of cases and prints every token with all its fields, in the format that
 * tests_html_tokenizer_cases.lucb's dump functions print. Not committed anywhere.
 *
 * Input (stdin): one case per line: MODE<TAB>STATE<TAB>INPUT, INPUT with \n \r \t \0 \\ escapes.
 *   MODE "whole":  HTMLTokenizer(input, "UTF-8"); after the first token, if STATE != "-",
 *                  switch_to(STATE) (by number).
 * Stops at the EOF token.
 * Output: for each case, "== <n>" then one line per token, then "--".
 */
// Local oracle only: open Badge's constructor so that the oracle can call parser_did_run.
#include <AK/Noncopyable.h>
#include <AK/Platform.h>
#define private public
#include <AK/Badge.h>
#undef private
#include <AK/ByteString.h>
#include <AK/StringBuilder.h>
#include <AK/Utf8View.h>
#include <LibWeb/HTML/Parser/HTMLTokenizer.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <iostream>

using namespace Web::HTML;

static std::string unescape(std::string const& s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char c = s[++i];
            if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else if (c == '0') out += '\0';
            else if (c == 'f') out += '\f';
            else out += c;
        } else {
            out += s[i];
        }
    }
    return out;
}

static void escape_into(StringBuilder& b, StringView v)
{
    for (auto c : v) {
        if (c == '\n') b.append("\\n"sv);
        else if (c == '\r') b.append("\\r"sv);
        else if (c == '\t') b.append("\\t"sv);
        else if (c == '\0') b.append("\\0"sv);
        else if (c == '\\') b.append("\\\\"sv);
        else b.append(c);
    }
}

static void pos(StringBuilder& b, HTMLToken::Position const& p)
{
    b.appendff("{}:{}", p.line, p.column);
}

static void dump(HTMLToken& t)
{
    StringBuilder b;
    switch (t.type()) {
    case HTMLToken::Type::DOCTYPE: {
        auto& d = t.doctype_data();
        b.append("DOCTYPE name="sv);
        if (d.missing_name) b.append("<missing>"sv); else escape_into(b, d.name);
        b.append(" public="sv);
        if (d.missing_public_identifier) b.append("<missing>"sv); else escape_into(b, d.public_identifier);
        b.append(" system="sv);
        if (d.missing_system_identifier) b.append("<missing>"sv); else escape_into(b, d.system_identifier);
        b.appendff(" quirks={}", d.force_quirks ? 1 : 0);
        break;
    }
    case HTMLToken::Type::StartTag:
    case HTMLToken::Type::EndTag:
        b.append(t.type() == HTMLToken::Type::StartTag ? "StartTag "sv : "EndTag "sv);
        escape_into(b, t.tag_name());
        b.appendff(" self_closing={} dup={} attributes={}", t.is_self_closing() ? 1 : 0, t.had_duplicate_attribute() ? 1 : 0, t.attribute_count());
        t.for_each_attribute([&](auto& a) {
            b.append(" ["sv);
            escape_into(b, a.local_name);
            b.append("="sv);
            escape_into(b, a.value);
            b.append(" "sv);
            pos(b, a.name_start_position);
            b.append("-"sv);
            pos(b, a.name_end_position);
            b.append(" "sv);
            pos(b, a.value_start_position);
            b.append("-"sv);
            pos(b, a.value_end_position);
            b.append("]"sv);
            return IterationDecision::Continue;
        });
        break;
    case HTMLToken::Type::Comment:
        b.append("Comment "sv);
        escape_into(b, t.comment());
        break;
    case HTMLToken::Type::Character:
        b.appendff("Character U+{:04X}", t.code_point());
        break;
    case HTMLToken::Type::EndOfFile:
        b.append("EOF"sv);
        break;
    default:
        b.append("Invalid"sv);
    }
    b.append(" @"sv);
    pos(b, t.start_position());
    b.append("-"sv);
    pos(b, t.end_position());
    outln("{}", b.string_view());
}

static void drain(HTMLTokenizer& t, HTMLTokenizer::StopAtInsertionPoint stop)
{
    while (true) {
        auto token = t.next_token(stop);
        if (!token.has_value()) {
            outln("(none)");
            return;
        }
        dump(token.value());
        if (token->is_end_of_file())
            return;
    }
}

static void drain_one(HTMLTokenizer& t)
{
    auto token = t.next_token();
    if (!token.has_value())
        outln("(none)");
    else
        dump(token.value());
}

static void show(HTMLTokenizer& t)
{
    StringBuilder b;
    b.append("unparsed="sv);
    escape_into(b, t.unparsed_input());
    b.appendff(" defined={} reached={} closed={} eof_inserted={} blocked={}", t.is_insertion_point_defined() ? 1 : 0, t.is_insertion_point_reached() ? 1 : 0, t.is_input_stream_closed() ? 1 : 0, t.is_eof_inserted() ? 1 : 0, t.is_blocked() ? 1 : 0);
    outln("{}", b.string_view());
}

using Stop = HTMLTokenizer::StopAtInsertionPoint;

static void scenarios()
{
    {
        outln("== streaming");
        HTMLTokenizer t;
        t.append_to_input_stream("<p cla"sv);
        drain(t, Stop::No);
        show(t);
        t.append_to_input_stream("ss=x>a\r"sv);
        drain(t, Stop::No);
        t.append_to_input_stream("\nb&no"sv);
        drain(t, Stop::No);
        t.append_to_input_stream("t;c&notin"sv);
        drain(t, Stop::No);
        t.append_to_input_stream("d\xc3\xa9"sv);
        drain(t, Stop::No);
        show(t);
        t.close_input_stream();
        drain(t, Stop::No);
        show(t);
        outln("--");
    }
    {
        outln("== insertion point");
        HTMLTokenizer t;
        t.append_to_input_stream("<a>12<b>tail</b>"sv);
        drain_one(t);
        t.update_insertion_point();
        show(t);
        t.insert_input_at_insertion_point("<i>w&amp</i>"sv);
        show(t);
        drain(t, Stop::Yes);
        show(t);
        t.store_old_insertion_point();
        t.undefine_insertion_point();
        show(t);
        drain(t, Stop::No);
        t.restore_old_insertion_point();
        show(t);
        t.close_input_stream();
        drain(t, Stop::No);
        show(t);
        t.parser_did_run(Badge<HTMLParser> {});
        show(t);
        outln("--");
    }
    {
        outln("== nested insertion points");
        HTMLTokenizer t;
        t.append_to_input_stream("x<y>z"sv);
        drain_one(t);
        drain_one(t);
        t.update_insertion_point();
        t.store_old_insertion_point();
        t.insert_input_at_insertion_point("AB"sv);
        show(t);
        t.store_old_insertion_point();
        t.insert_input_at_insertion_point("\xe2\x82\xac\r"sv);
        show(t);
        drain(t, Stop::Yes);
        t.insert_input_at_insertion_point("\n"sv);
        drain(t, Stop::Yes);
        t.restore_old_insertion_point();
        show(t);
        t.restore_old_insertion_point();
        show(t);
        t.insert_eof();
        t.set_blocked(true);
        show(t);
        t.undefine_insertion_point();
        drain(t, Stop::No);
        outln("--");
    }
    {
        outln("== abort");
        HTMLTokenizer t { "<a>b"sv, "UTF-8"sv };
        drain_one(t);
        t.abort();
        drain(t, Stop::No);
        outln("--");
    }
    {
        outln("== windows-1252");
        HTMLTokenizer t { "<p title=\x93q\x94>\x80\xe9"sv, "windows-1252"sv };
        drain(t, Stop::No);
        StringBuilder b;
        b.append("source="sv);
        escape_into(b, t.source());
        outln("{}", b.string_view());
        outln("--");
    }
}

int main(int argc, char** argv)
{
    if (argc > 2) {
        // --matcher WORD: feed WORD's code points to a NamedCharacterReferenceMatcher.
        NamedCharacterReferenceMatcher matcher;
        for (auto c : Utf8View { StringView { argv[2], strlen(argv[2]) } }) {
            bool consumed = matcher.try_consume_code_point(c);
            auto cps = matcher.code_points();
            outln("{:x} consumed={} overconsumed={} semicolon={} match={}", c, consumed ? 1 : 0, matcher.overconsumed_code_points(), matcher.last_match_ends_with_semicolon() ? 1 : 0,
                cps.has_value() ? ByteString::formatted("{:x}/{}", cps->first, named_character_reference_second_codepoint_value(cps->second).value_or(0)) : ByteString("none"sv));
        }
        return 0;
    }
    if (argc > 1) {
        scenarios();
        return 0;
    }
    std::string line;
    int n = 0;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        auto t1 = line.find('\t');
        auto t2 = line.find('\t', t1 + 1);
        std::string mode = line.substr(0, t1);
        std::string state = line.substr(t1 + 1, t2 - t1 - 1);
        std::string input = unescape(line.substr(t2 + 1));
        outln("== {}", n++);
        HTMLTokenizer tokenizer { StringView { input.data(), input.size() }, "UTF-8"sv };
        bool first = true;
        while (true) {
            auto token = tokenizer.next_token();
            if (!token.has_value())
                break;
            dump(token.value());
            // Stop after the EOF token: asking again re-runs the EOF branch of the current state,
            // which in several states (Comment, DOCTYPE identifiers, ...) touches the moved-from
            // current token and fails a VERIFY in the donor.
            if (token->is_end_of_file())
                break;
            if (first && state != "-")
                tokenizer.switch_to((HTMLTokenizer::State)std::stoi(state));
            first = false;
        }
        outln("--");
    }
    return 0;
}
