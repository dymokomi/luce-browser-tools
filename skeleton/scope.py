"""Phase-1 and phase-2 scope: the regions of DESIGN.md §8.2 as file-stem patterns, and their packages/modules.

A *stem* is a donor-relative path without extension (`Libraries/LibWeb/DOM/Node`,
`Build/Libraries/LibWeb/CSS/PropertyID`, `AK/Vector`). The first region whose pattern matches a
stem owns it. Stems that match no region are out of scope: their declarations appear only when
the closure pulls them in (types only).
"""
import fnmatch

# Modules in dependency order; `SEES` is what each module may import.
MODULES = ['ak', 'gc', 'web_unicode', 'text_codec', 'web_url', 'web_infra',
           'css_syntax', 'css_data', 'html_syntax',
           'gfx', 'web_fonts', 'raster', 'display_list', 'web']
PACKAGE_OF = {'ak': 'foundation', 'gc': 'foundation', 'web_unicode': 'foundation', 'text_codec': 'foundation',
              'web_url': 'foundation', 'web_infra': 'foundation', 'css_syntax': 'css', 'css_data': 'css',
              'html_syntax': 'html', 'gfx': 'render', 'web_fonts': 'render', 'raster': 'render',
              'display_list': 'render', 'web': 'engine'}
# Modules the generator never writes: another agent owns them (the tiny-skia port, DESIGN.md §7.3).
FOREIGN_MODULES = {'raster'}
PACKAGES = ['foundation', 'css', 'html', 'render', 'engine']
PACKAGE_DEPS = {'foundation': [], 'css': ['foundation'], 'html': ['foundation'],
                'render': ['foundation', 'css'], 'engine': ['foundation', 'css', 'html', 'render']}
FOUNDATION = ['ak', 'gc', 'web_unicode', 'text_codec', 'web_url', 'web_infra']
SEES = {
    'ak': [], 'gc': ['ak'], 'web_unicode': ['ak', 'gc'], 'text_codec': ['ak', 'gc', 'web_unicode'],
    'web_url': ['ak', 'gc', 'web_unicode', 'text_codec'],
    'web_infra': ['ak', 'gc', 'web_unicode', 'text_codec', 'web_url'],
    'css_syntax': FOUNDATION, 'css_data': FOUNDATION + ['css_syntax'], 'html_syntax': FOUNDATION,
    'gfx': FOUNDATION + ['css_syntax', 'css_data'],
    'web_fonts': FOUNDATION + ['css_syntax', 'css_data', 'gfx'],
    'raster': FOUNDATION + ['css_syntax', 'css_data', 'gfx', 'web_fonts'],
    'display_list': FOUNDATION + ['css_syntax', 'css_data', 'gfx', 'web_fonts', 'raster'],
    'web': MODULES[:-1],
}

W = 'Libraries/LibWeb/'
G = 'Build/Libraries/LibWeb/'


def _w(*names):
    return [W + n for n in names]


# (region, name, module, [patterns])
REGIONS = [
    # ---- foundation ------------------------------------------------------------------------
    ('r01', 'ak_core', 'ak', ['AK/' + n for n in (
        'Types', 'Assertions', 'Checked', 'SaturatingMath', 'IntegralMath', 'Math', 'NumericLimits',
        'Traits', 'HashFunctions', 'StringHash', 'QuickSort', 'BinarySearch', 'InsertionSort', 'Span',
        'Array', 'Vector', 'HashTable', 'HashMap', 'Optional', 'Variant', 'Function', 'Error',
        'IterationDecision', 'Endian', 'BitCast', 'Enumerate', 'StdLibExtras', 'Concepts', 'Badge',
        'NonnullOwnPtr', 'OwnPtr', 'RefPtr', 'NonnullRefPtr', 'RefCounted', 'AtomicRefCounted', 'Weakable',
        'WeakPtr', 'NonnullRawPtr', 'TemporaryChange', 'ScopeGuard', 'Try', 'Tuple', 'ByteBuffer',
        'FixedArray', 'CircularBuffer', 'CircularQueue', 'Queue', 'Stack', 'IntrusiveList', 'RedBlackTree',
        'BinaryHeap', 'Trie', 'DistinctNumeric', 'EnumBits', 'Time', 'Bitmap', 'BitmapView', 'Memory',
        'Find', 'AnyOf', 'AllOf', 'Iterator', 'ReverseIterator', 'Atomic', 'StackInfo', 'Platform',
        'DefaultDelete', 'Noncopyable', 'NeverDestroyed', 'Singleton', 'ScopedValueRollback', 'TypeCasts',
        'MaybeOwned', 'SegmentedVector', 'DoublyLinkedList', 'SinglyLinkedList', 'Result', 'Random',
        'IDAllocator', 'COWVector', 'CopyOnWrite', 'FixedPoint', 'UFixedBigInt', 'BumpAllocator',
        'IntrusiveRedBlackTree', 'IntrusiveDetails', 'Stream', 'MemoryStream', 'BufferedStream',
        'LEB128', 'Debug', 'kmalloc', 'Forward', 'TypedTransfer', 'Diagnostics')]),
    ('r02', 'ak_text', 'ak', ['AK/' + n for n in (
        'StringBase', 'StringData', 'String', 'FlyString', 'StringView', 'StringBuilder', 'StringUtils',
        'StringConversions', 'Utf8View', 'Utf16View', 'Utf32View', 'Utf16String', 'Utf16StringBase',
        'Utf16StringData', 'Utf16FlyString', 'ByteString', 'ByteStringImpl', 'GenericLexer',
        'CharacterTypes', 'UnicodeUtils', 'Base64', 'Hex', 'LexicalPath', 'SourceLocation',
        'StringNumber', 'IPv4Address', 'IPv6Address')]),
    ('r03', 'ak_format', 'ak', ['AK/' + n for n in (
        'Format', 'NumberFormat', 'FloatingPoint', 'JsonValue', 'JsonObject', 'JsonArray', 'JsonParser',
        'JsonObjectSerializer', 'JsonArraySerializer', 'CheckedFormatString', 'GenericShorthands')]),
    ('r03b', 'web_infra', 'web_infra', _w('Infra/Strings', 'Infra/CharacterTypes', 'Infra/ByteSequences',
                                          'Infra/Types')),
    ('r04', 'gc', 'gc', ['Libraries/LibGC/*']),
    ('r05', 'web_url', 'web_url', ['Libraries/LibURL/*', 'Build/Libraries/LibURL/*']),
    ('r06', 'text_codec', 'text_codec', ['Libraries/LibTextCodec/*', 'Build/Libraries/LibTextCodec/*']),
    ('r07', 'web_unicode', 'web_unicode', ['Libraries/LibUnicode/' + n for n in (
        'Segmenter', 'CharacterTypes', 'Normalize', 'Forward', 'IDNA', 'Utf16String', 'String',
        'Locale')]),
    # ---- css -------------------------------------------------------------------------------
    ('r30a', 'css_syntax', 'css_syntax', _w(
        'CSS/Parser/Tokenizer', 'CSS/Parser/Token', 'CSS/Parser/ComponentValue', 'CSS/Parser/TokenStream',
        'CSS/Parser/Types', 'CSS/Number', 'CSS/CharacterTypes', 'CSS/SerializationMode')),
    ('r12c', 'css_data', 'css_data', [G + 'CSS/' + n for n in (
        'PropertyID', 'Keyword', 'Enums', 'Units', 'PseudoClass', 'PseudoElement', 'MediaFeatureID',
        'DescriptorID', 'EnvironmentVariable', 'MathFunctions', 'TransformFunctions')]),
    # ---- html ------------------------------------------------------------------------------
    ('r20', 'html_tokenizer', 'html_syntax', _w('HTML/Parser/HTMLTokenizer', 'HTML/Parser/HTMLToken',
                                                'HTML/Parser/Entities') +
     [G + 'HTML/Parser/NamedCharacterReferences']),
    # ---- render ----------------------------------------------------------------------------
    ('r08', 'gfx_geometry', 'gfx', _w('PixelUnits') + ['Libraries/LibGfx/' + n for n in (
        'Point', 'Size', 'Rect', 'Line', 'Quad', 'AffineTransform', 'Matrix', 'Matrix3x3', 'Matrix4x4',
        'Vector2', 'Vector3', 'Vector4', 'VectorN', 'BoundingBox', 'Orientation', 'WindingRule',
        'ScalingMode', 'LineStyle', 'CompositingAndBlendingOperator', 'Color', 'ColorConversion',
        'InterpolationColorSpace', 'Palette', 'Cursor', 'TextAlignment', 'TextAttributes', 'Triangle',
        'Forward', 'ColorSpace', 'MorphologyOperator')]),
    ('r09', 'web_fonts', 'web_fonts', ['Libraries/LibGfx/Font/*', 'Libraries/LibGfx/FontCascadeList',
                                       'Libraries/LibGfx/TextLayout', 'Libraries/LibGfx/ShapeFeature']),
    ('r10', 'gfx_paint', 'gfx', ['Libraries/LibGfx/' + n for n in (
        'Path', 'PaintStyle', 'Filter', 'FilterImpl', 'Gradients', 'Bitmap', 'ImmutableBitmap',
        'PaintingSurface', 'Painter', 'ShareableBitmap', 'BitmapExportResult', 'BitmapSequence',
        'GradientPainting', 'FourCC')]),
    ('r51a', 'display_list', 'display_list', _w(
        'Painting/DisplayList', 'Painting/DisplayListRecorder', 'Painting/DisplayListCommand',
        'Painting/AccumulatedVisualContext', 'Painting/ScrollFrame', 'Painting/ScrollState',
        'Painting/PaintStyle', 'Painting/GradientData', 'Painting/BorderRadiiData', 'Painting/BordersData',
        'Painting/ExternalContentSource', 'Painting/ShouldAntiAlias', 'Painting/DisplayListPlayer*')),
    # ---- engine ----------------------------------------------------------------------------
    ('r13', 'top_level', 'web', _w('Dump', 'Namespace', 'Forward', 'InvalidateDisplayList',
                                   'GraphemeEdgeTracker', 'TraversalDecision', 'TraversalOrder', 'TreeNode',
                                   'MimeSniff/*', 'Platform/*', 'Infra/*', 'Export')),
    ('r14', 'bindings_webidl', 'web', _w('Bindings/*', 'WebIDL/*')),
    ('r15', 'dom_node', 'web', _w('DOM/Node', 'DOM/ParentNode', 'DOM/ChildNode', 'DOM/NonElementParentNode',
                                  'DOM/NonDocumentTypeChildNode', 'DOM/NodeOperations', 'DOM/NodeType')),
    ('r16', 'dom_element', 'web', _w('DOM/Element', 'DOM/ElementFactory')),
    ('r17', 'dom_document', 'web', _w('DOM/Document', 'DOM/DocumentLoading')),
    ('r19', 'dom_events', 'web', _w('DOM/Event', 'DOM/EventTarget', 'DOM/EventDispatcher', 'DOM/CustomEvent',
                                    'DOM/DOMEventListener', 'DOM/IDLEventListener', 'DOM/AbortController',
                                    'DOM/AbortSignal')),
    ('r18', 'dom_leaves', 'web', _w('DOM/*')),
    ('r21', 'html_tree_construction', 'web', _w('HTML/Parser/*')),
    ('r22', 'html_element', 'web', _w('HTML/HTMLElement', 'HTML/HTMLOrSVGElement', 'HTML/GlobalEventHandlers',
                                      'HTML/WindowEventHandlers', 'HTML/AttributeNames', 'HTML/TagNames',
                                      'HTML/EventNames', 'HTML/Numbers', 'HTML/Dates', 'HTML/Focus',
                                      'HTML/DOMStringMap', 'HTML/LazyLoadingElement',
                                      'HTML/HTMLHyperlinkElementUtils', 'HTML/HTMLUnknownElement')),
    ('r23', 'html_forms', 'web', _w('HTML/FormAssociatedElement', 'HTML/FormControlInfrastructure',
                                    'HTML/AutocompleteElement', 'HTML/ValidityState', 'HTML/ElementInternals',
                                    'HTML/HTMLFormElement', 'HTML/HTMLLabelElement', 'HTML/HTMLFieldSetElement',
                                    'HTML/HTMLLegendElement', 'HTML/HTMLOutputElement',
                                    'HTML/HTMLFormControlsCollection')),
    ('r24', 'html_input', 'web', _w('HTML/HTMLInputElement')),
    ('r25', 'html_controls', 'web', _w('HTML/HTMLSelectElement', 'HTML/HTMLOptionElement',
                                       'HTML/HTMLOptGroupElement', 'HTML/HTMLSelectedContentElement',
                                       'HTML/HTMLTextAreaElement', 'HTML/HTMLButtonElement',
                                       'HTML/HTMLMeterElement', 'HTML/HTMLProgressElement',
                                       'HTML/HTMLDataListElement', 'HTML/HTMLOptionsCollection')),
    ('r26', 'html_structure', 'web', _w('HTML/HTMLTable*', 'HTML/HTMLLIElement', 'HTML/HTMLOListElement',
                                        'HTML/HTMLUListElement', 'HTML/HTMLDListElement', 'HTML/HTMLMenuElement',
                                        'HTML/HTMLDirectoryElement', 'HTML/HTMLDetailsElement',
                                        'HTML/HTMLSummaryElement', 'HTML/HTMLDialogElement',
                                        'HTML/HTMLSlotElement', 'HTML/HTMLTemplateElement',
                                        'HTML/HTMLImageElement', 'HTML/HTMLPictureElement',
                                        'HTML/HTMLSourceElement', 'HTML/SourceSet', 'HTML/HTMLLinkElement',
                                        'HTML/HTMLStyleElement', 'HTML/HTMLMetaElement', 'HTML/HTMLBaseElement',
                                        'HTML/HTMLTitleElement', 'HTML/ImageRequest',
                                        'HTML/SharedResourceRequest', 'HTML/DecodedImageData',
                                        'HTML/AnimatedDecodedImageData', 'HTML/ListOfAvailableImages',
                                        'HTML/CORSSettingAttribute')),
    ('r27', 'html_elements_rest', 'web', _w('HTML/HTML*Element', 'HTML/MediaControls*', 'HTML/HTMLDocument')),
    ('r28', 'html_browsing', 'web', _w('HTML/Window', 'HTML/WindowProxy', 'HTML/Navigable', 'HTML/TraversableNavigable',
                                       'HTML/NavigableContainer*', 'HTML/BrowsingContext*', 'HTML/DocumentState',
                                       'HTML/SessionHistoryEntry', 'HTML/NavigationParams', 'HTML/PolicyContainers',
                                       'HTML/SandboxingFlagSet', 'HTML/CrossOrigin/*', 'HTML/DocumentReadyState',
                                       'HTML/VisibilityState', 'HTML/TokenizedFeatures', 'HTML/HistoryHandlingBehavior',
                                       'HTML/NavigationType', 'HTML/Origin*', 'HTML/EmbedderPolicy',
                                       'HTML/ActivateTab', 'HTML/POSTResource', 'HTML/SessionHistoryTraversalQueue',
                                       'HTML/Navigator*', 'Page/*')),
    ('r29', 'html_event_loop', 'web', _w('HTML/EventLoop/*', 'HTML/Scripting/Environments',
                                         'HTML/Scripting/Agent', 'HTML/Scripting/SimilarOriginWindowAgent',
                                         'HTML/Scripting/SerializedEnvironmentSettingsObject',
                                         'HTML/Scripting/TemporaryExecutionContext',
                                         'HTML/Scripting/WindowEnvironmentSettingsObject',
                                         'HTML/Scripting/ExceptionReporter',
                                         'HTML/AnimationFrameCallbackDriver')),
    ('r30b', 'css_parser_core', 'web', _w('CSS/Parser/Parser', 'CSS/Parser/Helpers', 'CSS/Parser/ErrorReporter',
                                          'CSS/Parser/RuleContext', 'CSS/Serialize')),
    ('r31', 'css_rule_parsing', 'web', _w('CSS/Parser/RuleParsing', 'CSS/Parser/SelectorParsing',
                                          'CSS/Parser/MediaParsing', 'CSS/Parser/DescriptorParsing',
                                          'CSS/Parser/SyntaxParsing', 'CSS/Parser/Syntax*')),
    ('r32', 'css_value_parsing', 'web', _w('CSS/Parser/ValueParsing', 'CSS/Parser/GradientParsing')),
    ('r33', 'css_property_parsing', 'web', _w('CSS/Parser/*')),
    ('r35', 'style_values_2', 'web', _w('CSS/StyleValues/Calculated*', 'CSS/NumericType', 'CSS/StyleValues/Transformation*',
                                        'CSS/StyleValues/BasicShape*', 'CSS/StyleValues/Filter*',
                                        'CSS/StyleValues/Easing*', 'CSS/StyleValues/Counter*',
                                        'CSS/StyleValues/Font*', 'CSS/StyleValues/Grid*')),
    ('r34', 'style_values_1', 'web', _w('CSS/StyleValues/*')),
    ('r37', 'css_sheets', 'web', _w('CSS/CSSStyleSheet', 'CSS/CSSRule', 'CSS/CSSRuleList', 'CSS/CSSStyleRule',
                                    'CSS/CSSImportRule', 'CSS/CSSMediaRule', 'CSS/CSSSupportsRule',
                                    'CSS/CSSConditionRule', 'CSS/CSSGroupingRule', 'CSS/CSSFontFaceRule',
                                    'CSS/CSSLayer*', 'CSS/CSSNamespaceRule', 'CSS/CSSNestedDeclarations',
                                    'CSS/CSSPropertyRule', 'CSS/CSSContainerRule', 'CSS/CSSPageRule',
                                    'CSS/CSSMarginRule', 'CSS/CSSCounterStyleRule', 'CSS/CSSFontFeatureValues*',
                                    'CSS/CSSFunction*', 'CSS/StyleSheet', 'CSS/StyleSheetList',
                                    'CSS/StyleSheetIdentifier', 'CSS/MediaList', 'CSS/CSSStyleProperties',
                                    'CSS/CSSStyleDeclaration', 'CSS/CSSDescriptors', 'CSS/CSSFontFaceDescriptors',
                                    'CSS/CSSPageDescriptors', 'CSS/Descriptor', 'CSS/CSS')
     + [G + 'CSS/GeneratedCSSStyleProperties']),
    ('r38', 'selectors', 'web', _w('CSS/Selector', 'CSS/SelectorEngine', 'CSS/PageSelector', 'CSS/PseudoClassBitmap')),
    ('r39', 'style_computer', 'web', _w('CSS/StyleComputer', 'CSS/CascadedProperties', 'CSS/StyleScope',
                                        'CSS/CustomPropertyData', 'CSS/CountersSet', 'CSS/CascadeOrigin',
                                        'CSS/CustomPropertyRegistration')),
    ('r40', 'computed_properties', 'web', _w('CSS/ComputedProperties', 'CSS/ComputedValues', 'CSS/StyleProperty')),
    ('r41', 'css_fonts', 'web', _w('CSS/FontComputer', 'CSS/FontFace', 'CSS/FontFaceSet', 'CSS/ParsedFontFace',
                                   'CSS/FontFeatureData', 'CSS/FontLoading')),
    ('r42', 'css_media_invalidation', 'web', _w('CSS/MediaQuery', 'CSS/MediaQueryList', 'CSS/BooleanExpression',
                                                'CSS/Supports', 'CSS/ContainerQuery', 'CSS/Preferred*',
                                                'CSS/Screen', 'CSS/VisualViewport', 'CSS/Invalidation/*',
                                                'CSS/InvalidationSet', 'CSS/StyleInvalidation*',
                                                'CSS/StyleSheetInvalidation')),
    ('r36', 'css_values_units', 'web', _w('CSS/Length', 'CSS/Angle', 'CSS/Time', 'CSS/Frequency', 'CSS/Flex',
                                          'CSS/Percentage*', 'CSS/Size', 'CSS/Display', 'CSS/GridTrack*',
                                          'CSS/LengthBox', 'CSS/EdgeRect', 'CSS/Clip', 'CSS/Filter', 'CSS/Sizing',
                                          'CSS/URL', 'CSS/Fetch', 'CSS/SystemColor', 'CSS/CounterStyle*',
                                          'CSS/ValueType', 'CSS/ColorFunctionDescriptor', 'CSS/ColorInterpolation',
                                          'CSS/Ratio', 'CSS/Resolution', 'CSS/NumericRange', 'CSS/ColumnCount',
                                          'CSS/CalculationResolutionContext', 'CSS/PropertyName*',
                                          'CSS/DescriptorNameAndID', 'CSS/Enums', 'CSS/Keyword')
     + [G + 'CSS/*']),
    ('r43', 'layout_tree', 'web', _w('Layout/Node', 'Layout/Box', 'Layout/BlockContainer', 'Layout/InlineNode',
                                     'Layout/TextNode', 'Layout/BreakNode', 'Layout/Viewport', 'Layout/ReplacedBox',
                                     'Layout/ImageBox', 'Layout/ListItem*', 'Layout/TreeBuilder',
                                     'Layout/CheckBox', 'Layout/RadioButton', 'Layout/FieldSetBox',
                                     'Layout/LegendBox', 'Layout/TextInputBox', 'Layout/TextAreaBox',
                                     'Layout/ImageProvider', 'Layout/VideoBox', 'Layout/AudioBox',
                                     'Layout/CanvasBox', 'Layout/NavigableContainerViewport', 'Layout/Label*',
                                     'Layout/BoxModelMetrics')),
    ('r44', 'layout_formatting', 'web', _w('Layout/FormattingContext', 'Layout/BlockFormattingContext',
                                           'Layout/LayoutState', 'Layout/AvailableSpace',
                                           'Layout/ReplacedWithChildrenFormattingContext')),
    ('r45', 'layout_inline', 'web', _w('Layout/InlineFormattingContext', 'Layout/InlineLevelIterator',
                                       'Layout/LineBuilder', 'Layout/LineBox', 'Layout/LineBoxFragment')),
    ('r46', 'layout_flex', 'web', _w('Layout/FlexFormattingContext')),
    ('r47', 'layout_grid', 'web', _w('Layout/GridFormattingContext')),
    ('r48', 'layout_table', 'web', _w('Layout/TableFormattingContext', 'Layout/TableGrid', 'Layout/TableWrapper')),
    ('r49', 'layout_svg', 'web', _w('Layout/SVG*')),
    ('r43b', 'layout_rest', 'web', _w('Layout/*')),
    ('r51b', 'paint_recording', 'web', _w('Painting/DisplayListRecordingContext',
                                          'Painting/BorderRadiusCornerClipper', 'Painting/Scrollbar',
                                          'Painting/ChromeMetrics', 'Painting/DevicePixelConverter',
                                          'Painting/ChromeWidget', 'Painting/ResizeHandle')),
    ('r52', 'paint_painters', 'web', _w('Painting/BorderPainting', 'Painting/BackgroundPainting',
                                        'Painting/ShadowPainting', 'Painting/GradientPainting',
                                        'Painting/TableBordersPainting', 'Painting/BoxModelMetrics',
                                        'Painting/ResolvedCSSFilter', 'Painting/Blending', 'Painting/ShadowData',
                                        'Painting/InputColors')),
    ('r53', 'paint_svg_replaced', 'web', _w('Painting/SVG*', 'Painting/ImagePaintable', 'Painting/MarkerPaintable',
                                            'Painting/CheckBoxPaintable', 'Painting/RadioButtonPaintable',
                                            'Painting/FieldSetPaintable', 'Painting/VideoPaintable',
                                            'Painting/CanvasPaintable',
                                            'Painting/NavigableContainerViewportPaintable')),
    ('r50', 'paintables', 'web', _w('Painting/*')),
    ('r56', 'svg_2', 'web', _w('SVG/SVG*Gradient*', 'SVG/SVGStop*', 'SVG/SVGPattern*', 'SVG/SVGMask*',
                               'SVG/SVGClipPath*', 'SVG/SVGUse*', 'SVG/SVGSymbol*', 'SVG/SVGText*', 'SVG/SVGTSpan*',
                               'SVG/SVGForeignObject*', 'SVG/SVGFE*', 'SVG/SVGFilter*', 'SVG/SVGDecodedImageData',
                               'SVG/SVGImage*', 'SVG/SVGA*')),
    ('r55', 'svg_1', 'web', _w('SVG/*')),
    ('r13b', 'mathml_aria_misc', 'web', _w('MathML/*', 'ARIA/*', 'XLink/*') + [G + 'ARIA/*', G + 'MathML/*']),
]

# Phase 2 (DESIGN.md §1.2, §8.2 "Phase 2"): loading. What P2's features reach from the ported phase-1
# code: Fetch's infrastructure, fetching and HTTP layer (with LibHTTP's header list), the Loader,
# Content Security Policy, the image, preload and CORS-request paths, the XML document builder and
# referrer policy / secure contexts. The JS-facing Fetch API (Fetch/Body, BodyInit, Headers,
# Request, Response, FetchMethod, Enums) and script fetching (HTML/Scripting/Fetching) are phase 3;
# blob URLs, storage and file lists only scripting or user input reach. The XML parser itself is
# luce-xml's and the image decoders are luce-png/luce-jpeg/... (§7.0): only the builder and the
# ImageCodecPlugin seam (ported in phase 1) belong to the engine.
P2_REGIONS = [
    ('p2a', 'fetch_infrastructure', 'web', _w(
        'Fetch/Infrastructure/ConnectionTimingInfo', 'Fetch/Infrastructure/FetchAlgorithms',
        'Fetch/Infrastructure/FetchController', 'Fetch/Infrastructure/FetchParams',
        'Fetch/Infrastructure/FetchRecord', 'Fetch/Infrastructure/FetchTimingInfo', 'Fetch/Infrastructure/HTTP',
        'Fetch/Infrastructure/IncrementalReadLoopReadRequest', 'Fetch/Infrastructure/MimeTypeBlocking',
        'Fetch/Infrastructure/NetworkPartitionKey', 'Fetch/Infrastructure/NoSniffBlocking',
        'Fetch/Infrastructure/PortBlocking', 'Fetch/Infrastructure/RequestOrResponseBlocking',
        'Fetch/Infrastructure/Task', 'Fetch/Infrastructure/URL', 'SecureContexts/*', 'ReferrerPolicy/*')),
    ('p2b', 'fetch_http', 'web', _w('Fetch/Infrastructure/HTTP/*') + [
        'Libraries/LibHTTP/' + n for n in ('HeaderList', 'Header', 'HTTP', 'Method', 'Status')]),
    ('p2c', 'fetch_fetching', 'web', _w('Fetch/Fetching/*')),
    # (LibCore's Resource and Promise are seams the Loader and navigation call; LibCore is not
    # ported, its closure stubs are implemented here.)
    ('p2d', 'loader_resources', 'web', _w('Loader/*', 'HTML/PotentialCORSRequest', 'HTML/PreloadEntry',
                                          'HTML/NavigationObserver', 'HTML/BitmapDecodedImageData',
                                          'XML/XMLDocumentBuilder') +
     ['Libraries/LibCore/Resource', 'Libraries/LibCore/Promise']),
    ('p2e', 'csp_policy', 'web', _w(
        'ContentSecurityPolicy/BlockingAlgorithms', 'ContentSecurityPolicy/Policy',
        'ContentSecurityPolicy/PolicyList', 'ContentSecurityPolicy/SerializedPolicy',
        'ContentSecurityPolicy/Violation', 'ContentSecurityPolicy/SecurityPolicyViolationEvent',
        'ContentSecurityPolicy/Directives/Directive', 'ContentSecurityPolicy/Directives/DirectiveFactory',
        'ContentSecurityPolicy/Directives/Names', 'ContentSecurityPolicy/Directives/KeywordSources',
        'ContentSecurityPolicy/Directives/KeywordTrustedTypes',
        'ContentSecurityPolicy/Directives/SerializedDirective')),
    ('p2f', 'csp_directives', 'web', _w('ContentSecurityPolicy/Directives/*')),
]
P1_REGIONS = list(REGIONS)
P1_REGION_IDS = {r[0] for r in P1_REGIONS}
REGIONS = P1_REGIONS + P2_REGIONS
PHASE = 2
# Libraries outside LibWeb's core whose files a phase's regions port (their types are full).
PHASE_LIBS = {1: set(), 2: {'LibHTTP'}}


def phase_of(region_id):
    """1 or 2: the phase a region belongs to."""
    return 1 if region_id in P1_REGION_IDS else 2


def set_phase(phase):
    """Plan as of a phase: phase 1's regions only, or phase 1's and phase 2's (the default). merge.py
    compares the two plans to find what phase 2 adds."""
    global REGIONS, PHASE
    PHASE = phase
    REGIONS = P1_REGIONS + (P2_REGIONS if phase >= 2 else [])
    _cache.clear()


# Declaration moves of DESIGN.md §4.1.3: C++ qualified name -> module that owns it.
MOVES = {
    'Web::CSS::Important': 'css_syntax',
    'Web::CSS::BorderData': 'display_list',
    'Web::CSS::ColorInterpolationMethodStyleValue::PolarColorInterpolationMethod': 'display_list',
    'Web::Painting::CornerClip': 'display_list',
}

# Field retypings of DESIGN.md §4.1.3 (a lower package's field that pointed upward):
# (C++ class, field) -> [(Luce field, Luce type, modules the type names)].
FIELD_CUTS = {
    ('Web::HTML::HTMLTokenizer', 'm_parser'): [
        ('m_adjusted_current_node_is_foreign', '(func(void*) -> bool)?', []),
        ('m_parser_context', 'void*?', [])],
    ('Web::Painting::ScrollFrame', 'm_paintable_box'): [('m_paintable_box', 'gc.Weak[gc.Cell]', ['gc'])],
}

# Out-of-scope LibWeb directories whose types the closure may still pull in (types only).
LATER_TYPES = ['Libraries/LibWeb/*', 'Build/Libraries/LibWeb/*']

# Default module of a library, used for closure-pulled out-of-scope declarations.
LIB_MODULE = [
    ('AK/', 'ak'), ('Libraries/LibGC/', 'gc'), ('Libraries/LibUnicode/', 'web_unicode'),
    ('Libraries/LibTextCodec/', 'text_codec'), ('Build/Libraries/LibTextCodec/', 'text_codec'),
    ('Libraries/LibURL/', 'web_url'), ('Build/Libraries/LibURL/', 'web_url'),
    ('Libraries/LibGfx/Font/', 'web_fonts'), ('Libraries/LibGfx/', 'gfx'), ('Libraries/LibHTTP/', 'web'),
]

_cache = {}


def stem(path):
    for ext in ('.cpp', '.h', '.mm'):
        if path.endswith(ext):
            return path[:-len(ext)]
    return path


def region_of(path):
    """(region, name, module) of a donor-relative file, or None when out of scope."""
    s = stem(path)
    r = _cache.get(s)
    if r is not None:
        return r or None
    for region, name, module, pats in REGIONS:
        for p in pats:
            if fnmatch.fnmatchcase(s, p):
                _cache[s] = (region, name, module)
                return _cache[s]
    _cache[s] = ()
    return None


def default_module(path):
    """Module for a closure-pulled declaration of an out-of-scope file."""
    r = region_of(path)
    if r:
        return r[2]
    for prefix, module in LIB_MODULE:
        if path.startswith(prefix):
            return module
    return 'web'


def regions_of_package(package):
    return [(r, n, m) for r, n, m, _ in REGIONS if PACKAGE_OF[m] == package]
