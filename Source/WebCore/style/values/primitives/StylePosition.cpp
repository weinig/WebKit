/*
 * Copyright (C) 2024-2025 Samuel Weinig <sam@webkit.org>
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
#include "StylePosition.h"

#include "BoxSides.h"
#include "CalculationCategory.h"
#include "CalculationTree.h"
#include "LengthPoint.h"
#include "StylePrimitiveNumericTypes+Blending.h"
#include "StylePrimitiveNumericTypes+Conversions.h"
#include "StylePrimitiveNumericTypes+Evaluation.h"

namespace WebCore {
namespace Style {

using namespace CSS::Literals;

enum XAxisPhysicalKeywords : bool { Left, Right };
enum YAxisPhysicalKeywords : bool { Top, Bottom };

static XAxisPhysicalKeywords mapXStart(WritingMode writingMode)
{
    if (writingMode.isHorizontal()) {
        if (writingMode.isBidiLTR()) {
            // horizontal / left-to-right: `x-start` maps to `left`
            return XAxisPhysicalKeywords::Left;
        } else {
            // horizontal / right-to-left: `x-start` maps to `right`
            return XAxisPhysicalKeywords::Right;
        }
    } else {
        if (!writingMode.isBlockFlipped()) {
            // vertical / left-to-right: `x-start` maps to `left`
            return XAxisPhysicalKeywords::Left;
        } else {
            // vertical / right-to-left: `x-start` maps to `right`
            return XAxisPhysicalKeywords::Right;
        }
    }
}

static XAxisPhysicalKeywords mapXEnd(WritingMode writingMode)
{
    if (writingMode.isHorizontal()) {
        if (writingMode.isBidiLTR()) {
            // horizontal / left-to-right: `x-end` maps to `right`
            return XAxisPhysicalKeywords::Right;
        } else {
            // horizontal / right-to-left: `x-end` maps to `left`
            return XAxisPhysicalKeywords::Left;
        }
    } else {
        if (!writingMode.isBlockFlipped()) {
            // vertical / left-to-right: `x-end` maps to `right`
            return XAxisPhysicalKeywords::Right;
        } else {
            // vertical / right-to-left: `x-end` maps to `left`
            return XAxisPhysicalKeywords::Left;
        }
    }
}

static YAxisPhysicalKeywords mapYStart(WritingMode writingMode)
{
    if (writingMode.isHorizontal()) {
        if (!writingMode.isBlockFlipped()) {
            // horizontal / top-to-bottom: `y-start` maps to `top`
            return YAxisPhysicalKeywords::Top;
        } else {
            // horizontal / bottom-to-top: `y-start` maps to `bottom`
            return YAxisPhysicalKeywords::Bottom;
        }
    } else {
        // vertical: `y-start` maps to `top`
        return YAxisPhysicalKeywords::Top;
    }
}

static YAxisPhysicalKeywords mapYEnd(WritingMode writingMode)
{
    if (writingMode.isHorizontal()) {
        if (!writingMode.isBlockFlipped()) {
            // horizontal / top-to-bottom: `y-end` maps to `bottom`
            return YAxisPhysicalKeywords::Bottom;
        } else {
            // horizontal / bottom-to-top: `y-end` maps to `top`
            return YAxisPhysicalKeywords::Top;
        }
    } else {
        // vertical: `y-end` maps to `bottom`
        return YAxisPhysicalKeywords::Bottom;
    }
}

// MARK: Core Keyword Resolution

static auto resolveKeyword(CSS::Keyword::Top, const BuilderState&) -> LengthPercentage<>
{
    return 0_css_percentage;
}

static auto resolveKeyword(CSS::Keyword::Top, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return toStyle(length, state);
}

static auto resolveKeyword(CSS::Keyword::Right, const BuilderState&) -> LengthPercentage<>
{
    return 100_css_percentage;
}

static auto resolveKeyword(CSS::Keyword::Right, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return reflect(toStyle(length, state));
}

static auto resolveKeyword(CSS::Keyword::Bottom, const BuilderState&) -> LengthPercentage<>
{
    return 100_css_percentage;
}

static auto resolveKeyword(CSS::Keyword::Bottom, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return reflect(toStyle(length, state));
}

static auto resolveKeyword(CSS::Keyword::Left, const BuilderState&) -> LengthPercentage<>
{
    return 0_css_percentage;
}

static auto resolveKeyword(CSS::Keyword::Left, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return toStyle(length, state);
}

static auto resolveKeyword(CSS::Keyword::Center, const BuilderState&) -> LengthPercentage<>
{
    return 50_css_percentage;
}

// MARK: Mapped value resolution

template<typename... Args> static auto resolveKeyword(XAxisPhysicalKeywords keyword, Args&&... args) -> LengthPercentage<>
{
    switch (keyword) {
    case XAxisPhysicalKeywords::Right:
        return resolveKeyword(CSS::Keyword::Right { }, std::forward<Args>(args)...);
    case XAxisPhysicalKeywords::Left:
        return resolveKeyword(CSS::Keyword::Left { }, std::forward<Args>(args)...);
    }
    ASSERT_NOT_REACHED();
    return 0_css_percentage;
}

template<typename... Args> static auto resolveKeyword(YAxisPhysicalKeywords keyword, Args&&... args) -> LengthPercentage<>
{
    switch (keyword) {
    case YAxisPhysicalKeywords::Top:
        return resolveKeyword(CSS::Keyword::Top { }, std::forward<Args>(args)...);
    case YAxisPhysicalKeywords::Bottom:
        return resolveKeyword(CSS::Keyword::Bottom { }, std::forward<Args>(args)...);
    }
    ASSERT_NOT_REACHED();
    return 0_css_percentage;
}

template<typename... Args> static auto resolveKeyword(BoxSide boxSide, Args&&... args) -> LengthPercentage<>
{
    switch (boxSide) {
    case BoxSide::Top:
        return resolveKeyword(CSS::Keyword::Top { }, std::forward<Args>(args)...);
    case BoxSide::Right:
        return resolveKeyword(CSS::Keyword::Right { }, std::forward<Args>(args)...);
    case BoxSide::Bottom:
        return resolveKeyword(CSS::Keyword::Bottom { }, std::forward<Args>(args)...);
    case BoxSide::Left:
        return resolveKeyword(CSS::Keyword::Left { }, std::forward<Args>(args)...);
    }
    ASSERT_NOT_REACHED();
    return 0_css_percentage;
}

// MARK: Mapping resolvers (<position-two>)

static auto resolveKeyword(CSS::Keyword::XStart, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapXStart(state.style().writingMode()), state);
}

static auto resolveKeyword(CSS::Keyword::XEnd, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapXEnd(state.style().writingMode()), state);
}

static auto resolveKeyword(CSS::Keyword::YStart, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapYStart(state.style().writingMode()), state);
}

static auto resolveKeyword(CSS::Keyword::YEnd, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapYEnd(state.style().writingMode()), state);
}

static auto resolveKeyword(CSS::Keyword::BlockStart, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::BlockStart), state);
}

static auto resolveKeyword(CSS::Keyword::BlockEnd, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::BlockEnd), state);
}

static auto resolveKeyword(CSS::Keyword::InlineStart, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::InlineStart), state);
}

static auto resolveKeyword(CSS::Keyword::InlineEnd, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::InlineEnd), state);
}

// MARK: Mapping resolvers (<position-four>)

static auto resolveKeyword(CSS::Keyword::XStart, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapXStart(state.style().writingMode()), length, state);
}

static auto resolveKeyword(CSS::Keyword::XEnd, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapXEnd(state.style().writingMode()), length, state);
}

static auto resolveKeyword(CSS::Keyword::YStart, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapYStart(state.style().writingMode()), length, state);
}

static auto resolveKeyword(CSS::Keyword::YEnd, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapYEnd(state.style().writingMode()), length, state);
}

static auto resolveKeyword(CSS::Keyword::BlockStart, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::BlockStart), length, state);
}

static auto resolveKeyword(CSS::Keyword::BlockEnd, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::BlockEnd), length, state);
}

static auto resolveKeyword(CSS::Keyword::InlineStart, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::InlineStart), length, state);
}

static auto resolveKeyword(CSS::Keyword::InlineEnd, const CSS::LengthPercentage<>& length, const BuilderState& state) -> LengthPercentage<>
{
    return resolveKeyword(mapSideLogicalToPhysical(state.style().writingMode(), LogicalBoxSide::InlineEnd), length, state);
}

// MARK: Horizontal/Vertical

static auto resolve(const CSS::TwoComponentPositionHorizontal& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](auto keyword) {
            return resolveKeyword(keyword, state);
        },
        [&](const CSS::LengthPercentage<>& value) {
            return toStyle(value, state);
        }
    );
}

static auto resolve(const CSS::TwoComponentPositionVertical& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](auto keyword) {
            return resolveKeyword(keyword, state);
        },
        [&](const CSS::LengthPercentage<>& value) {
            return toStyle(value, state);
        }
    );
}

static auto resolve(const CSS::ThreeComponentPositionHorizontal& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](auto keyword) {
            return resolveKeyword(keyword, state);
        }
    );
}

static auto resolve(const CSS::ThreeComponentPositionVertical& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](auto keyword) {
            return resolveKeyword(keyword, state);
        }
    );
}

static auto resolve(const CSS::FourComponentPositionHorizontal& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(get<0>(value.offset),
        [&](auto keyword) {
            return resolveKeyword(keyword, get<1>(value.offset), state);
        }
    );
}

static auto resolve(const CSS::FourComponentPositionVertical& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(get<0>(value.offset),
        [&](auto keyword) {
            return resolveKeyword(keyword, get<1>(value.offset), state);
        }
    );
}

// MARK: Block/Inline

static auto resolve(const CSS::TwoComponentPositionBlock& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](auto keyword) {
            return resolveKeyword(keyword, state);
        }
    );
}

static auto resolve(const CSS::TwoComponentPositionInline& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](auto keyword) {
            return resolveKeyword(keyword, state);
        }
    );
}

static auto resolve(const CSS::FourComponentPositionBlock& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(get<0>(value.offset),
        [&](auto keyword) {
            return resolveKeyword(keyword, get<1>(value.offset), state);
        }
    );
}

static auto resolve(const CSS::FourComponentPositionInline& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(get<0>(value.offset),
        [&](auto keyword) {
            return resolveKeyword(keyword, get<1>(value.offset), state);
        }
    );
}

// MARK: Start/End

static auto resolveMappingBlock(const CSS::TwoComponentPositionLogical& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](CSS::Keyword::Start) {
            return resolveKeyword(CSS::Keyword::BlockStart { }, state);
        },
        [&](CSS::Keyword::End) {
            return resolveKeyword(CSS::Keyword::BlockEnd { }, state);
        },
        [&](CSS::Keyword::Center) {
            return resolveKeyword(CSS::Keyword::Center { }, state);
        }
    );
}

static auto resolveMappingInline(const CSS::TwoComponentPositionLogical& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(value.offset,
        [&](CSS::Keyword::Start) {
            return resolveKeyword(CSS::Keyword::InlineStart { }, state);
        },
        [&](CSS::Keyword::End) {
            return resolveKeyword(CSS::Keyword::InlineEnd { }, state);
        },
        [&](CSS::Keyword::Center) {
            return resolveKeyword(CSS::Keyword::Center { }, state);
        }
    );
}

static auto resolveMappingBlock(const CSS::FourComponentPositionLogical& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(get<0>(value.offset),
        [&](CSS::Keyword::Start) {
            return resolveKeyword(CSS::Keyword::BlockStart { }, get<1>(value.offset), state);
        },
        [&](CSS::Keyword::End) {
            return resolveKeyword(CSS::Keyword::BlockEnd { }, get<1>(value.offset), state);
        }
    );
}

static auto resolveMappingInline(const CSS::FourComponentPositionLogical& value, const BuilderState& state) -> LengthPercentage<>
{
    return WTF::switchOn(get<0>(value.offset),
        [&](CSS::Keyword::Start) {
            return resolveKeyword(CSS::Keyword::InlineStart { }, get<1>(value.offset), state);
        },
        [&](CSS::Keyword::End) {
            return resolveKeyword(CSS::Keyword::InlineEnd { }, get<1>(value.offset), state);
        }
    );
}

auto ToStyle<CSS::TwoComponentPositionHorizontal>::operator()(const CSS::TwoComponentPositionHorizontal& value, const BuilderState& state) -> TwoComponentPositionHorizontal
{
    return { resolve(value, state) };
}

auto ToStyle<CSS::TwoComponentPositionVertical>::operator()(const CSS::TwoComponentPositionVertical& value, const BuilderState& state) -> TwoComponentPositionVertical
{
    return { resolve(value, state) };
}

template<typename T> concept IsLogicalComponent =
       std::same_as<T, CSS::TwoComponentPositionLogical>
    || std::same_as<T, CSS::FourComponentPositionLogical>;

template<typename T> concept IsStartEndComponents =
       std::same_as<T, CSS::TwoComponentPositionStartEnd>
    || std::same_as<T, CSS::ThreeComponentPositionStartEndLengthFirst>
    || std::same_as<T, CSS::ThreeComponentPositionStartEndLengthSecond>
    || std::same_as<T, CSS::FourComponentPositionStartEnd>;

// MARK: <position> conversion

auto ToCSS<Position>::operator()(const Position& value, const RenderStyle& style) -> CSS::Position
{
    return CSS::TwoComponentPositionHorizontalVertical { { toCSS(value.x(), style) }, { toCSS(value.y(), style) } };
}

auto ToStyle<CSS::Position>::operator()(const CSS::Position& position, const BuilderState& state) -> Position
{
    return WTF::switchOn(position,
        [&](const auto& components) {
            return Position {
                resolve(get<0>(components), state),
                resolve(get<1>(components), state),
            };
        },
        [&]<IsStartEndComponents T>(const T& components) {
            return Position {
                resolveMappingBlock(get<0>(components), state),
                resolveMappingInline(get<1>(components), state),
            };
        }
    );
}

// MARK: <position-x> conversion

auto ToCSS<PositionX>::operator()(const PositionX& value, const RenderStyle& style) -> CSS::PositionX
{
    return CSS::TwoComponentPositionHorizontal { toCSS(value.value, style) };
}

auto ToStyle<CSS::PositionX>::operator()(const CSS::PositionX& positionX, const BuilderState& state) -> PositionX
{
    return WTF::switchOn(positionX,
        [&](const auto& value) {
            return PositionX { resolve(value, state) };
        },
        [&]<IsLogicalComponent T>(const T& value) {
            return PositionX { resolveMappingBlock(value, state) };
        }
    );
}

// MARK: <position-y> conversion

auto ToCSS<PositionY>::operator()(const PositionY& value, const RenderStyle& style) -> CSS::PositionY
{
    return CSS::TwoComponentPositionVertical { toCSS(value.value, style) };
}

auto ToStyle<CSS::PositionY>::operator()(const CSS::PositionY& positionY, const BuilderState& state) -> PositionY
{
    return WTF::switchOn(positionY,
        [&](const auto& value) {
            return PositionY { resolve(value, state) };
        },
        [&]<IsLogicalComponent T>(const T& value) {
            return PositionY { resolveMappingInline(value, state) };
        }
    );
}

// MARK: - Evaluation

auto Evaluation<Position>::operator()(const Position& position, FloatSize referenceBox) -> FloatPoint
{
    return evaluate(position.value, referenceBox);
}

// MARK: - Platform

static auto toPlatform(const LengthPercentage<>& length) -> WebCore::Length
{
    return WTF::switchOn(length,
        [](const LengthPercentage<>::Dimension& dimension) {
            return WebCore::Length { dimension.value, WebCore::LengthType::Fixed };
        },
        [](const LengthPercentage<>::Percentage& percentage) {
            return WebCore::Length { percentage.value, WebCore::LengthType::Percent };
        },
        [](const LengthPercentage<>::Calc& calc) {
            return WebCore::Length { calc.protectedCalculation() };
        }
    );
}

auto toPlatform(const Position& position) -> WebCore::LengthPoint
{
    return { toPlatform(position.x()), toPlatform(position.y()) };
}

auto toPlatform(const PositionX& positionX) -> WebCore::Length
{
    return toPlatform(positionX.value);
}

auto toPlatform(const PositionY& positionY) -> WebCore::Length
{
    return toPlatform(positionY.value);
}

} // namespace CSS
} // namespace WebCore
