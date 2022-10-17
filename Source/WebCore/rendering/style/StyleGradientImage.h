/*
 * Copyright (C) 2022 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include "CSSGradientValue.h"
#include "StyleColor.h"
#include "StyleGeneratedImage.h"
#include <optional>
#include <variant>

namespace WebCore {

struct StyleGradientImageStop {
    StyleColor color;
    RefPtr<CSSPrimitiveValue> position;
};

class StyleGradientImage final : public StyleGeneratedImage {
public:
    using Stop = StyleGradientImageStop;

    struct Angle {
        float valueInDegrees;
    };

    struct SideOrCorner {
        enum Horizontal { Left, Right };
        enum Vertical { Top, Bottom };
        using Both = std::pair<Horizontal, Vertical>;
    };

    struct Position {
        // [ left | center | right ] || [ top | center | bottom ]
        struct Variant1 {
            
        };

        // [ left | center | right | <length-percentage> ]
        // [ top | center | bottom | <length-percentage> ]?
        struct Variant2 {
            
        };

        // [ [ left | right ] <length-percentage> ] &&
        // [ [ top | bottom ] <length-percentage> ]
        struct Variant3 {
            
        };
        
        std::variant<Variant1, Variant2, Variant3> value;
    };

    struct LinearData {
        std::variant<Angle, SideOrCorner::Horizontal, SideOrCorner::Vertical, SideOrCorner::Both> direction;
    }

    struct DeprecatedLinearData {
    
    };

    struct PrefixedLinearData {
    
    };

    struct LinearData {
        RefPtr<CSSPrimitiveValue> firstX;
        RefPtr<CSSPrimitiveValue> firstY;
        RefPtr<CSSPrimitiveValue> secondX;
        RefPtr<CSSPrimitiveValue> secondY;
        std::optional<float> angleInDegrees;
    };

    struct RadialData {
        RefPtr<CSSPrimitiveValue> firstX;
        RefPtr<CSSPrimitiveValue> firstY;
        RefPtr<CSSPrimitiveValue> secondX;
        RefPtr<CSSPrimitiveValue> secondY;
        RefPtr<CSSPrimitiveValue> firstRadius;
        RefPtr<CSSPrimitiveValue> secondRadius;
        RefPtr<CSSPrimitiveValue> shape;
        RefPtr<CSSPrimitiveValue> sizingBehavior;
        RefPtr<CSSPrimitiveValue> endHorizontalSize;
        RefPtr<CSSPrimitiveValue> endVerticalSize;
    };

    struct DeprecatedRadialData {
    
    };

    struct PrefixedRadialData {
    
    };

    struct ConicData {
        // https://drafts.csswg.org/css-values-4/#typedef-position
        //
        // <position> Determines the gradient center of the gradient. The <position> value
        // type (which is also used for background-position) is defined in [CSS-VALUES-3], and
        // is resolved using the center-point as the object area and the gradient box as the
        // positioning area. If this argument is omitted, it defaults to center.
        Position position;
        RefPtr<CSSPrimitiveValue> firstX;
        RefPtr<CSSPrimitiveValue> firstY;
        
        // https://drafts.csswg.org/css-values-4/#angle-value
        //
        // The entire gradient is rotated by this angle. If omitted, defaults to 0deg. The unit
        // identifier may be omitted if the <angle> is zero.
        Angle angle;
    };

    using Data = std::variant<LinearData, RadialData, ConicData>;

    static Ref<StyleGradientImage> create(Data data, CSSGradientRepeat repeat, CSSGradientType gradientType, CSSGradientColorInterpolationMethod colorInterpolationMethod, Vector<Stop> stops)
    {
        return adoptRef(*new StyleGradientImage(WTFMove(data), repeat, gradientType, colorInterpolationMethod, WTFMove(stops)));
    }
    virtual ~StyleGradientImage();

    bool operator==(const StyleImage&) const final;
    bool equals(const StyleGradientImage&) const;

    static constexpr bool isFixedSize = false;

private:
    explicit StyleGradientImage(Data&&, CSSGradientRepeat, CSSGradientType, CSSGradientColorInterpolationMethod, Vector<Stop>&&);

    Ref<CSSValue> computedStyleValue(const RenderStyle&) const final;
    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;
    RefPtr<Image> image(const RenderElement*, const FloatSize&) const final;
    bool knownToBeOpaque(const RenderElement&) const final;
    FloatSize fixedSize(const RenderElement&) const final;
    void didAddClient(RenderElement&) final { }
    void didRemoveClient(RenderElement&) final { }

    Ref<Gradient> createGradient(const LinearData&, const RenderElement&, const FloatSize&) const;
    Ref<Gradient> createGradient(const RadialData&, const RenderElement&, const FloatSize&) const;
    Ref<Gradient> createGradient(const ConicData&, const RenderElement&, const FloatSize&) const;

    template<typename GradientAdapter> GradientColorStops computeStops(GradientAdapter&, const CSSToLengthConversionData&, const RenderStyle&, float maxLengthForRepeat) const;

    bool hasColorDerivedFromElement() const;
    bool isCacheable() const;

    Data m_data;
    CSSGradientRepeat m_repeat;
    CSSGradientColorInterpolationMethod m_colorInterpolationMethod;
    Vector<Stop> m_stops;
    mutable std::optional<bool> m_hasColorDerivedFromElement;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleGradientImage, isGradientImage)
