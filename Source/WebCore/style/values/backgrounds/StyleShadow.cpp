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

#include "config.h"
#include "StyleShadow.h"

#include "ColorBlending.h"
#include "RenderStyle.h"
#include "StylePrimitiveNumericTypes+Blending.h"
#include "StylePrimitiveNumericTypes+Conversions.h"
#include "StylePrimitiveNumericTypes+Evaluation.h"

namespace WebCore {
namespace Style {

// MARK: - Conversion

auto ToCSS<Shadow>::operator()(const Shadow& value, const RenderStyle& style) -> CSS::Shadow
{
    // auto color = cssValuePool.createColorValue(style.colorResolvingCurrentColor(currShadowData->color()));
    // auto x = adjustLengthForZoom(currShadowData->x(), style, adjust);
    // auto y = adjustLengthForZoom(currShadowData->y(), style, adjust);
    // auto blur = adjustLengthForZoom(currShadowData->radius(), style, adjust);
    // auto spread = propertyID == CSSPropertyTextShadow ? RefPtr<CSSPrimitiveValue>() : adjustLengthForZoom(currShadowData->spread(), style, adjust);
    // auto shadowStyle = propertyID == CSSPropertyTextShadow || currShadowData->style() == ShadowStyle::Normal ? RefPtr<CSSPrimitiveValue>() : CSSPrimitiveValue::create(CSSValueInset);

    return {
        .color = toCSS(value.color, style),
        .location = toCSS(value.location, style),
        .blur = toCSS(value.blur, style),
        .spread = toCSS(value.spread, style),
        .inset = toCSS(value.inset, style),
        .isWebkitBoxShadow = value.isWebkitBoxShadow,
    };
}

auto ToStyle<CSS::Shadow>::operator()(const CSS::Shadow& value, const BuilderState& state, const CSSCalcSymbolTable& symbolTable) -> Shadow
{
    //  auto color = shadowValue.color ? builderState.colorFromPrimitiveValue(*shadowValue.color) : Style::Color::currentColor();
    //  auto x = shadowValue.x->resolveAsLength<WebCore::Length>(conversionData);
    //  auto y = shadowValue.y->resolveAsLength<WebCore::Length>(conversionData);
    //  auto blur = shadowValue.blur ? shadowValue.blur->resolveAsLength<WebCore::Length>(conversionData) : WebCore::Length(0, LengthType::Fixed);
    //  auto spread = shadowValue.spread ? shadowValue.spread->resolveAsLength<WebCore::Length>(conversionData) : WebCore::Length(0, LengthType::Fixed);
    //  auto shadowStyle = shadowValue.style && shadowValue.style->valueID() == CSSValueInset ? ShadowStyle::Inset : ShadowStyle::Normal;

    return {
        .color = value.color ? toStyle(*value.color, state, symbolTable) : Color::currentColor(),
        .location = toStyle(value.location, state, symbolTable),
        .blur = value.blur ? toStyle(*value.blur, state, symbolTable) : Length<CSS::Nonnegative> { 0 },
        .spread = value.spread ? toStyle(*value.spread, state, symbolTable) : Length<> { 0 },
        .inset = toStyle(value.inset, state, symbolTable),
        .isWebkitBoxShadow = value.isWebkitBoxShadow,
    };
}

// MARK: - Blending

static inline std::optional<CSS::Keyword::Inset> blendInset(std::optional<CSS::Keyword::Inset> a, std::optional<CSS::Keyword::Inset> b, const BlendingContext& context)
{
    if (a == b)
        return b;

    auto aVal = !a ? 1.0 : 0.0;
    auto bVal = !b ? 1.0 : 0.0;

    auto result = WebCore::blend(aVal, bVal, context);
    return result > 0 ? std::nullopt : std::make_optional(CSS::Keyword::Inset { });
}

auto Blending<Shadow>::canBlend(const Shadow&, const Shadow&, const RenderStyle&, const RenderStyle&) -> bool
{
    return true;
}

auto Blending<Shadow>::blend(const Shadow& a, const Shadow& b, const RenderStyle& aStyle, const RenderStyle& bStyle, const BlendingContext& context) -> Shadow
{
    return {
        .color = WebCore::blend(aStyle.colorResolvingCurrentColor(a.color), bStyle.colorResolvingCurrentColor(b.color), context),
        .location = WebCore::Style::blend(a.location, b.location, context),
        .blur = WebCore::Style::blend(a.blur, b.blur, context),
        .spread = WebCore::Style::blend(a.spread, b.spread, context),
        .inset = blendInset(a.inset, b.inset, context),
        .isWebkitBoxShadow = b.isWebkitBoxShadow
    };
}

} // namespace Style
} // namespace WebCore
