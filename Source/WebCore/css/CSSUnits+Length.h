/*
 * Copyright (C) 2024 Samuel Weinig <sam@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

namespace WTF {
class TextStream;
}

namespace WebCore {

enum class CSSUnitType : uint8_t;

namespace CSS {

template<typename> struct UnitTypeTraits;

namespace Dimension {

struct LengthDefinition {
    // Absolute
    struct cm { };     // centimeters          1cm = 96px/2.54
    struct mm { };     // millimeters          1mm = 1/10th of 1cm
    struct Q { };      // quarter-millimeters   1Q = 1/40th of 1cm
    struct in { };     // inches               1in = 2.54cm = 96px
    struct pc { };     // picas                1pc = 1/6th of 1in
    struct pt { };     // points               1pt = 1/72nd of 1in
    struct px { };     // pixels               1px = 1/96th of 1in
    // Font Relative
    struct em { };     // font size of the element
    struct ex { };     // x-height of the element’s font
    struct cap { };    // cap height (the nominal height of capital letters) of the element’s font
    struct ch { };     // typical character advance of a narrow glyph in the element’s font, as represented by the “0” (ZERO, U+0030) glyph
    struct ic { };     // typical character advance of a fullwidth glyph in the element’s font, as represented by the “水” (CJK water ideograph, U+6C34) glyph
    struct rem { };    // font size of the root element
    struct lh { };     // line height of the element
    struct rlh { };    // line height of the root element
    // Viewport Relative
    struct vw { };     // 1% of viewport’s width
    struct vh { };     // 1% of viewport’s height
    struct vi { };     // 1% of viewport’s size in the root element’s inline axis
    struct vb { };     // 1% of viewport’s size in the root element’s block axis
    struct vmin { };   // 1% of viewport’s smaller dimension
    struct vmax { };   // 1% of viewport’s larger dimension
    // Container Relative
    struct cqw { };    // 1% of a query container’s width
    struct cqh { };    // 1% of a query container’s height
    struct cqi { };    // 1% of a query container’s inline size
    struct cqb { };    // 1% of a query container’s block size
    struct cqmin { };  // The smaller value of cqi or cqb
    struct cqmax { };  // The larger value of cqi or cqb
    // Non-standard
    struct _qem { };   // Quirky em.
};

} // namespace Dimension

namespace Value {

template<typename T> struct Quantity {
    using type = T;
    double value;
};

using Length = std::variant<
    Quantity<Dimension::Length::cm>,     // centimeters          1cm = 96px/2.54
    Quantity<Dimension::Length::mm>,     // millimeters          1mm = 1/10th of 1cm
    Quantity<Dimension::Length::Q>,      // quarter-millimeters   1Q = 1/40th of 1cm
    Quantity<Dimension::Length::In>,     // inches               1in = 2.54cm = 96px
    Quantity<Dimension::Length::pc>,     // picas                1pc = 1/6th of 1in
    Quantity<Dimension::Length::pt>,     // points               1pt = 1/72nd of 1in
    Quantity<Dimension::Length::px>,     // pixels               1px = 1/96th of 1in

    // Font Relative
    Quantity<Dimension::Length::em>,     // font size of the element
    Quantity<Dimension::Length::ex>,     // x-height of the element’s font
    Quantity<Dimension::Length::cap>,    // cap height (the nominal height of capital letters) of the element’s font
    Quantity<Dimension::Length::ch>,     // typical character advance of a narrow glyph in the element’s font, as represented by the “0” (ZERO, U+0030) glyph
    Quantity<Dimension::Length::ic>,     // typical character advance of a fullwidth glyph in the element’s font, as represented by the “水” (CJK water ideograph, U+6C34) glyph
    Quantity<Dimension::Length::rem>,    // font size of the root element
    Quantity<Dimension::Length::lh>,     // line height of the element
    Quantity<Dimension::Length::rlh>,    // line height of the root element

    // Viewport Relative
    Quantity<Dimension::Length::vw>,     // 1% of viewport’s width
    Quantity<Dimension::Length::vh>,     // 1% of viewport’s height
    Quantity<Dimension::Length::vi>,     // 1% of viewport’s size in the root element’s inline axis
    Quantity<Dimension::Length::vb>,     // 1% of viewport’s size in the root element’s block axis
    Quantity<Dimension::Length::vmin>,   // 1% of viewport’s smaller dimension
    Quantity<Dimension::Length::vmax>,   // 1% of viewport’s larger dimension

    // Container Relative
    Quantity<Dimension::Length::cqw>,    // 1% of a query container’s width
    Quantity<Dimension::Length::cqh>,    // 1% of a query container’s height
    Quantity<Dimension::Length::cqi>,    // 1% of a query container’s inline size
    Quantity<Dimension::Length::cqb>,    // 1% of a query container’s block size
    Quantity<Dimension::Length::cqmin>,  // The smaller value of cqi or cqb
    Quantity<Dimension::Length::cqmax>,  // The larger value of cqi or cqb

    // Non-standard
    Quantity<Dimension::Length::_qem>,   // Quirky em.

    // calc()
    calc<Dimension::Length>
>;

// Resolution for style building.
Style::Length resolveToStyle(const Length&, Style::BuilderState&);

// Serialization.
void serializationForCSS(StringBuilder&, const Length&);

// For compatibility with CSSPrimitiveValue
CSSUnitType primitiveType(const Length&);
Ref<CSSPrimitiveValue> createCSSPrimitiveValue(const Length&);

template<typename T> T computeLength(const Length&, const CSSToLengthConversionData&;
template<int> WebCore::Length convertToLength(const Length&, const CSSToLengthConversionData&;
bool convertingToLengthHasRequiredConversionData(const Length&, int lengthConversion, const CSSToLengthConversionData&;

WTF::TextStream& operator<<(WTF::TextStream&, const Length&);

} // namespace Value

enum class AbsoluteLengthUnitType : uint8_t {
    // https://drafts.csswg.org/css-values/#absolute-lengths
    cm,     // centimeters          1cm = 96px/2.54
    mm,     // millimeters          1mm = 1/10th of 1cm
    Q,      // quarter-millimeters   1Q = 1/40th of 1cm
    in,     // inches               1in = 2.54cm = 96px
    pc,     // picas                1pc = 1/6th of 1in
    pt,     // points               1pt = 1/72nd of 1in
    px,     // pixels               1px = 1/96th of 1in
};
template<> struct UnitTypeTraits<AbsoluteLengthUnitType> {
    static constexpr auto first = AbsoluteLengthUnitType::cm;
    static constexpr auto last = AbsoluteLengthUnitType::px;
};
CSSUnitType toCSSUnitType(AbsoluteLengthUnitType);
LengthUnitType toCSSLengthUnitType(AbsoluteLengthUnitType);

enum class FontRelativeLengthUnitType : uint8_t {
    em,     // font size of the element
    ex,     // x-height of the element’s font
    cap,    // cap height (the nominal height of capital letters) of the element’s font
    ch,     // typical character advance of a narrow glyph in the element’s font, as represented by the “0” (ZERO, U+0030) glyph
    ic,     // typical character advance of a fullwidth glyph in the element’s font, as represented by the “水” (CJK water ideograph, U+6C34) glyph
    rem,    // font size of the root element
    lh,     // line height of the element
    rlh,    // line height of the root element
};
template<> struct UnitTypeTraits<FontRelativeLengthUnitType> {
    static constexpr auto first = FontRelativeLengthUnitType::em;
    static constexpr auto last = FontRelativeLengthUnitType::rlh;
};
CSSUnitType toCSSUnitType(FontRelativeLengthUnitType);
LengthUnitType toCSSLengthUnitType(FontRelativeLengthUnitType);

enum class ViewportRelativeLengthUnitType : uint8_t {
    // https://drafts.csswg.org/css-values/#viewport-relative-lengths
    vw,     // 1% of viewport’s width
    vh,     // 1% of viewport’s height
    vi,     // 1% of viewport’s size in the root element’s inline axis
    vb,     // 1% of viewport’s size in the root element’s block axis
    vmin,   // 1% of viewport’s smaller dimension
    vmax,   // 1% of viewport’s larger dimension
};
template<> struct UnitTypeTraits<ViewportRelativeLengthUnitType> {
    static constexpr auto first = ViewportRelativeLengthUnitType::vw;
    static constexpr auto last = ViewportRelativeLengthUnitType::vmax;
};
CSSUnitType toCSSUnitType(ViewportRelativeLengthUnitType);
LengthUnitType toCSSLengthUnitType(ViewportRelativeLengthUnitType);

enum class ContainerLengthUnitType : uint8_t {
    // https://drafts.csswg.org/css-conditional-5/#container-lengths
    cqw,    // 1% of a query container’s width
    cqh,    // 1% of a query container’s height
    cqi,    // 1% of a query container’s inline size
    cqb,    // 1% of a query container’s block size
    cqmin,  // The smaller value of cqi or cqb
    cqmax,  // The larger value of cqi or cqb
};
template<> struct UnitTypeTraits<ContainerLengthUnitType> {
    static constexpr auto first = ContainerLengthUnitType::cqw;
    static constexpr auto last = ContainerLengthUnitType::cqmax;
};
CSSUnitType toCSSUnitType(AbsoluteLengthUnit);
LengthUnitType toCSSLengthUnitType(AbsoluteLengthUnit);

enum class QuirkyLengthUnitType : uint8_t {
    _qem,   // Non-standard
};
template<> struct UnitTypeTraits<QuirkyLengthUnitType> {
    static constexpr auto first = QuirkyLengthUnitType::_qem;
    static constexpr auto last = QuirkyLengthUnitType::_qem;
};
CSSUnitType toCSSUnitType(QuirkyLengthUnitType);
LengthUnitType toCSSLengthUnitType(QuirkyLengthUnitType);

enum class LengthUnitType : uint8_t {
    // Absolute Relative
    cm      // centimeters          1cm = 96px/2.54
    mm      // millimeters          1mm = 1/10th of 1cm
    Q       // quarter-millimeters   1Q = 1/40th of 1cm
    In      // inches               1in = 2.54cm = 96px
    pc      // picas                1pc = 1/6th of 1in
    pt      // points               1pt = 1/72nd of 1in
    px      // pixels               1px = 1/96th of 1in

    // Font Relative
    em,     // font size of the element
    ex,     // x-height of the element’s font
    cap,    // cap height (the nominal height of capital letters) of the element’s font
    ch,     // typical character advance of a narrow glyph in the element’s font, as represented by the “0” (ZERO, U+0030) glyph
    ic,     // typical character advance of a fullwidth glyph in the element’s font, as represented by the “水” (CJK water ideograph, U+6C34) glyph
    rem,    // font size of the root element
    lh,     // line height of the element
    rlh,    // line height of the root element

    // Viewport Relative
    vw,     // 1% of viewport’s width
    vh,     // 1% of viewport’s height
    vi,     // 1% of viewport’s size in the root element’s inline axis
    vb,     // 1% of viewport’s size in the root element’s block axis
    vmin,   // 1% of viewport’s smaller dimension
    vmax,   // 1% of viewport’s larger dimension

    // Container Relative
    cqw,    // 1% of a query container’s width
    cqh,    // 1% of a query container’s height
    cqi,    // 1% of a query container’s inline size
    cqb,    // 1% of a query container’s block size
    cqmin,  // The smaller value of cqi or cqb
    cqmax,  // The larger value of cqi or cqb

    // Non-standard
    qem,    // Quirky em.
};
template<> struct UnitTypeTraits<LengthUnitType> {
    static constexpr auto first = LengthUnitType::cm;
    static constexpr auto last = LengthUnitType::_qem;

    static constexpr auto canonical = LengthUnitType::px;
};
CSSUnitType toCSSUnitType(LengthUnitType);




} // namespace CSS
} // namespace WebCore
