/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2019 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

namespace WTF {
class TextStream;
}

namespace WebCore {

//enum class AbsoluteLengthUnit : uint8_t {
//    // https://drafts.csswg.org/css-values/#absolute-lengths
//    cm,     // centimeters          1cm = 96px/2.54
//    mm,     // millimeters          1mm = 1/10th of 1cm
//    Q,      // quarter-millimeters   1Q = 1/40th of 1cm
//    in,     // inches               1in = 2.54cm = 96px
//    pc,     // picas                1pc = 1/6th of 1in
//    pt,     // points               1pt = 1/72nd of 1in
//    px,     // pixels               1px = 1/96th of 1in
//};
//template<> struct UnitTraits<AbsoluteLengthUnit> {
//    static constexpr auto first = AbsoluteLengthUnit::cm;
//    static constexpr auto last = AbsoluteLengthUnit::px;
//};
//
//enum class FontRelativeLengthUnit : uint8_t {
//    em,     /* font size of the element */
//    ex,     /* x-height of the element’s font */
//    cap,    /* cap height (the nominal height of capital letters) of the element’s font */
//    ch,     /* typical character advance of a narrow glyph in the element’s font, as represented by the “0” (ZERO, U+0030) glyph */
//    ic,     /* typical character advance of a fullwidth glyph in the element’s font, as represented by the “水” (CJK water ideograph, U+6C34) glyph */
//    rem,    /* font size of the root element */
//    lh,     /* line height of the element */
//    rlh,    /* line height of the root element */
//};
//template<> struct UnitTraits<FontRelativeLengthUnit> {
//    static constexpr auto first = FontRelativeLengthUnit::em;
//    static constexpr auto last = FontRelativeLengthUnit::rlh;
//};
//
//enum class ViewportRelativeLengthUnit : uint8_t {
//    // https://drafts.csswg.org/css-values/#viewport-relative-lengths
//    vw,     /* 1% of viewport’s width */
//    vh,     /* 1% of viewport’s height */
//    vi,     /* 1% of viewport’s size in the root element’s inline axis */
//    vb,     /* 1% of viewport’s size in the root element’s block axis */
//    vmin,   /* 1% of viewport’s smaller dimension */
//    vmax,   /* 1% of viewport’s larger dimension */
//};
//template<> struct UnitTraits<ViewportRelativeLengthUnit> {
//    static constexpr auto first = ViewportRelativeLengthUnit::vw;
//    static constexpr auto last = ViewportRelativeLengthUnit::vmax;
//};
//
//enum class ContainerLengthUnit : uint8_t {
//    // https://drafts.csswg.org/css-conditional-5/#container-lengths
//    cqw,    // 1% of a query container’s width
//    cqh,    // 1% of a query container’s height
//    cqi,    // 1% of a query container’s inline size
//    cqb,    // 1% of a query container’s block size
//    cqmin,  // The smaller value of cqi or cqb
//    cqmax,  // The larger value of cqi or cqb
//};
//template<> struct UnitTraits<ContainerLengthUnit> {
//    static constexpr auto first = ContainerLengthUnit::cqw;
//    static constexpr auto last = ContainerLengthUnit::cqmax;
//};
//
//
//
//
//struct LengthUnitMetrics {
//    enum class Subsets : uint8_t {
//        AbsoluteLengthUnit          = 0,
//        FontRelativeLengthUnit      = 1,
//        ViewportRelativeLengthUnit  = 2,
//        ContainerLengthUnit         = 3,
//    };
//
//    static constexpr uint8_t SubsetSizes[] = {
//        UnitTraits<AbsoluteLengthUnit>::numberOfUnits,
//        UnitTraits<FontRelativeLengthUnit>::numberOfUnits,
//        UnitTraits<ViewportRelativeLengthUnit>::numberOfUnits,
//        UnitTraits<ContainerLengthUnit>::numberOfUnits,
//    };
//
//    static constexpr uint8_t SubsetOffsets[] = {
//        addSizesUpTo(SubsetSizes, Subsets::AbsoluteLengthUnit),
//        addSizesUpTo(SubsetSizes, Subsets::FontRelativeLengthUnit),
//        addSizesUpTo(SubsetSizes, Subsets::ViewportRelativeLengthUnit),
//        addSizesUpTo(SubsetSizes, Subsets::ContainerLengthUnit),
//    };
//
//    static constexpr uint8_t offsetFor(AbsoluteLengthUnit value)
//    {
//        return LengthUnitMetrics::SubsetOffset[LengthUnitMetrics::Subsets::AbsoluteLengthUnit] + value;
//    }
//    static constexpr uint8_t offsetFor(FontRelativeLengthUnit value)
//    {
//        return LengthUnitMetrics::SubsetOffset[LengthUnitMetrics::Subsets::FontRelativeLengthUnit] + value;
//    }
//    static constexpr uint8_t offsetFor(ViewportRelativeLengthUnit value)
//    {
//        return LengthUnitMetrics::SubsetOffset[LengthUnitMetrics::Subsets::ViewportRelativeLengthUnit] + value;
//    }
//    static constexpr uint8_t offsetFor(ContainerLengthUnit value)
//    {
//        return LengthUnitMetrics::SubsetOffset[LengthUnitMetrics::Subsets::ContainerLengthUnit] + value;
//    }
//};
//
//enum class LengthUnit : uint8_t {
//    cm  = LengthUnitMetrics::offsetFor(AbsoluteLengthUnit::cm),     // centimeters          1cm = 96px/2.54
//    mm  = LengthUnitMetrics::offsetFor(AbsoluteLengthUnit::mm),     // millimeters          1mm = 1/10th of 1cm
//    Q   = LengthUnitMetrics::offsetFor(AbsoluteLengthUnit::Q),      // quarter-millimeters   1Q = 1/40th of 1cm
//    In  = LengthUnitMetrics::offsetFor(AbsoluteLengthUnit::In),     // inches               1in = 2.54cm = 96px
//    pc  = LengthUnitMetrics::offsetFor(AbsoluteLengthUnit::pc),     // picas                1pc = 1/6th of 1in
//    pt  = LengthUnitMetrics::offsetFor(AbsoluteLengthUnit::pt),     // points               1pt = 1/72nd of 1in
//    px  = LengthUnitMetrics::offsetFor(AbsoluteLengthUnit::px),     // pixels               1px = 1/96th of 1in
//
//    // https://drafts.csswg.org/css-values/#relative-lengths
//    em,     // font size of the element
//    ex,     // x-height of the element’s font
//    cap,    // cap height (the nominal height of capital letters) of the element’s font
//    ch,     // typical character advance of a narrow glyph in the element’s font, as represented by the “0” (ZERO, U+0030) glyph
//    ic,     // typical character advance of a fullwidth glyph in the element’s font, as represented by the “水” (CJK water ideograph, U+6C34) glyph
//    rem,    // font size of the root element
//    lh,     // line height of the element
//    rlh,    // line height of the root element
//    vw,     // 1% of viewport’s width
//    vh,     // 1% of viewport’s height
//    vi,     // 1% of viewport’s size in the root element’s inline axis
//    vb,     // 1% of viewport’s size in the root element’s block axis
//    vmin,   // 1% of viewport’s smaller dimension
//    vmax,   // 1% of viewport’s larger dimension
//
//    // https://drafts.csswg.org/css-conditional-5/#container-lengths
//    cqw,    // 1% of a query container’s width
//    cqh,    // 1% of a query container’s height
//    cqi,    // 1% of a query container’s inline size
//    cqb,    // 1% of a query container’s block size
//    cqmin,  // The smaller value of cqi or cqb
//    cqmax,  // The larger value of cqi or cqb
//
//    // Non-standard
//    qem,    // Quirky em.
//};


// FIXME: No need to use all capitals and a CSS prefix on all these names. Should fix that.
enum class CSSUnitType : uint8_t {
    CSS_UNKNOWN,
    CSS_NUMBER,
    CSS_INTEGER,
    CSS_PERCENTAGE,
    CSS_EM,
    CSS_EX,
    CSS_PX,
    CSS_CM,
    CSS_MM,
    CSS_IN,
    CSS_PT,
    CSS_PC,
    CSS_DEG,
    CSS_RAD,
    CSS_GRAD,
    CSS_MS,
    CSS_S,
    CSS_HZ,
    CSS_KHZ,
    CSS_DIMENSION,
    CSS_STRING,
    CSS_URI,
    CSS_IDENT,
    CSS_ATTR,
    CSS_RGBCOLOR,

    CSS_VW,
    CSS_VH,
    CSS_VMIN,
    CSS_VMAX,
    CSS_VB,
    CSS_VI,
    CSS_SVW,
    CSS_SVH,
    CSS_SVMIN,
    CSS_SVMAX,
    CSS_SVB,
    CSS_SVI,
    CSS_LVW,
    CSS_LVH,
    CSS_LVMIN,
    CSS_LVMAX,
    CSS_LVB,
    CSS_LVI,
    CSS_DVW,
    CSS_DVH,
    CSS_DVMIN,
    CSS_DVMAX,
    CSS_DVB,
    CSS_DVI,
    FirstViewportCSSUnitType = CSS_VW,
    LastViewportCSSUnitType = CSS_DVI,

    CSS_CQW,
    CSS_CQH,
    CSS_CQI,
    CSS_CQB,
    CSS_CQMIN,
    CSS_CQMAX,

    CSS_DPPX,
    CSS_X,
    CSS_DPI,
    CSS_DPCM,
    CSS_FR,
    CSS_Q,
    CSS_LH,
    CSS_RLH,

    CustomIdent,

    CSS_TURN,
    CSS_REM,
    CSS_REX,
    CSS_CAP,
    CSS_RCAP,
    CSS_CH,
    CSS_RCH,
    CSS_IC,
    CSS_RIC,

    CSS_CALC,
    CSS_CALC_PERCENTAGE_WITH_NUMBER,
    CSS_CALC_PERCENTAGE_WITH_LENGTH,

    CSS_ANCHOR,

    CSS_FONT_FAMILY,

    CSS_UNRESOLVED_COLOR,

    CSS_PROPERTY_ID,
    CSS_VALUE_ID,
    
    // This value is used to handle quirky margins in reflow roots (body, td, and th) like WinIE.
    // The basic idea is that a stylesheet can use the value __qem (for quirky em) instead of em.
    // When the quirky value is used, if you're in quirks mode, the margin will collapse away
    // inside a table cell. This quirk is specified in the HTML spec but our impl is different.
    CSS_QUIRKY_EM

    // Note that CSSValue allocates 7 bits for m_primitiveUnitType, so there can be no value here > 127.
};

enum class CSSUnitCategory : uint8_t {
    Number,
    Percent,
    AbsoluteLength,
    FontRelativeLength,
    ViewportPercentageLength,
    Angle,
    Time,
    Frequency,
    Resolution,
    Flex,
    Other
};

CSSUnitCategory unitCategory(CSSUnitType);
CSSUnitType canonicalUnitTypeForCategory(CSSUnitCategory);
CSSUnitType canonicalUnitTypeForUnitType(CSSUnitType);

WTF::TextStream& operator<<(WTF::TextStream&, CSSUnitCategory);
WTF::TextStream& operator<<(WTF::TextStream&, CSSUnitType);

} // namespace WebCore
