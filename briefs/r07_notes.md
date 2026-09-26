# r07 (web_unicode) final report — source for docs/regions/r07.md

port/r07 c684dd3. 39 web_unicode tests pass; test.sh regenerates and compares the generated tables.

## What replaces ICU
- `tools/gen_ucd/` (Luce program) reads `data/ucd/` (ppucd.txt, nfc.txt, uts46.txt, emoji-sequence files; Unicode 17.0.0) and `data/icu/` (locale lists dumped from ICU 78.2: available locales, likely subtags, RTL scripts), license files beside both.
- Generated tables `generated_ucd_properties`, `_casing`, `_emoji`, `_idna`, `generated_icu_locales` (hex strings, ~850 KB), ICU numbering for categories/properties/scripts.
- Hand-written: `ucd` (property lookup); `casing` (full case mapping, titlecasing, closure, halfwidth-fullwidth; tr/lt/nl/hy rules); `break_iterator`, `break_grapheme`, `break_word`, `break_line`, `break_sentence` (ICU rule files incl. word rule statuses and navigation semantics); `text_buffers`; `idna`, `idna_2` (from ICU uts46.cpp/punycode.cpp); `normalize` via luce-std `unicode.normalize`.

## Verified locally against ICU 78.2 (harness not committed)
All per-code-point properties for every code point; grapheme/line/sentence boundaries on 60k random strings; word boundaries/statuses except Han/Kana/Thai (ICU dictionaries); casing root/tr/lt/nl + titlecase + fullwidth on 30k strings; IDNA on ~26k inputs in both LibURL option sets.

## Tests
UCD GraphemeBreakTest/WordBreakTest/SentenceBreakTest/LineBreakTest pass (dictionary word cases skipped like ICU). IdnaTestV2 toAsciiN passes (lone A4_2 on trailing root dot = success, as ICU). Ported TestUnicodeCharacterTypes, TestSegmenter, TestIDNA, TestUnicodeNormalization, subtag/type-identifier cases of TestLocale; casing tests pinned to ICU. Tests needing AK Strings run through cores (`segment_utf8`, `normalize_utf8`, `uts46_name_to_ascii_utf8`, `case_insensitive_ranges`).
NOT ported yet: TestLocale parse and canonicalize cases (need ak String/Vector) — add after r01/r02 merge.

## Changes other regions must know
- LibUnicode-defined AK::String/Utf16String methods (to_lowercase, to_uppercase, to_titlecase, to_casefold, to_fullwidth, equals_ignoring_case, find_byte_offset_ignoring_case, trim_whitespace) are `web_unicode.string_*` / `web_unicode.utf16_string_*` (ak can't import web_unicode). namemap updated; `ak/stub_r07_web_unicode.lucb` + ORDER entry removed.
- `locale_id_remove_extension_type` / `locale_id_for_each_extension_of_type` take `ExtensionKind` instead of a type parameter; callback `Function1[Extension*, IterationDecision]`.
- Internal Locale parsers use a local `LocaleLexer` (ak's `GenericLexer[u8]` had an unmapped input field at the time — re-check against r02's GenericLexer and switch if faithful).
- `char_direction_to_bidi_class` takes `u32`; `apply_extensions_to_locale` takes `LocaleId*`s; `ranges_equal_ignoring_case` takes code-point spans.
- Hand-edited types: `SegmenterImpl` holds `BreakIterator*` + new `SegmentedText` enum; `ResolvedLocale` holds byte spans; GENERAL_CATEGORY_*/PROPERTY_* are `let` constants; ICU-only removed (PropertyName, ADDITIONAL_NAME, case-closure caches, `types_external`, `types_generated_{brkiter,uniset,locid}`). ak's `EmptyOrStringOrIcu78UnicodeString` and `Icu78UnicodeString` now unused — drop them.
- String paths of Segmenter/String/Utf16String/Normalize and all of Locale call ak String/StringBuilder/Vector/HashTable (trapped before r01/r02). Utf16View/Utf32View segmenter paths work.

## Deviations from ICU
No dictionary segmentation (CJK/SEA); no Greek-specific uppercasing; locale canonicalization syntax-only (no CLDR aliases); likely subtags = dumped table + UTS #35; segmenter locales use root rules; IDNA ContextO checks not ported (Ladybird never enables); ICU data quirks kept (ccc from nfc.txt; U+2985/U+2986 not fullwidth-mapped).
