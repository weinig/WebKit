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

#include "config.h"
#include "StyleCrossfadeImage.h"

#include "AnimationUtilities.h"
#include "CSSCrossfadeValue.h"
#include "CrossfadeGeneratedImage.h"
#include "RenderElement.h"
#include <wtf/PointerComparison.h>

namespace WebCore {

StyleCrossfadeImage::StyleCrossfadeImage(RefPtr<StyleImage>&& from, RefPtr<StyleImage>&& to, double percentage, bool isPrefixed)
    : StyleGeneratedImage { Type::CrossfadeImage, StyleCrossfadeImage::isFixedSize }
    , m_from { WTFMove(from) }
    , m_to { WTFMove(to) }
    , m_percentage { percentage }
    , m_isPrefixed { isPrefixed }
    , m_inputImagesRegistered { false }
{
}

StyleCrossfadeImage::~StyleCrossfadeImage()
{
    if (!m_inputImagesRegistered)
        return;

    if (m_from)
        m_from->unregisterObserver(*this);
    if (m_to)
        m_to->unregisterObserver(*this);
}

bool StyleCrossfadeImage::operator==(const StyleImage& other) const
{
    return is<StyleCrossfadeImage>(other) && equals(downcast<StyleCrossfadeImage>(other));
}

bool StyleCrossfadeImage::equals(const StyleCrossfadeImage& other) const
{
    return equalInputImages(other) && m_percentage == other.m_percentage;
}

bool StyleCrossfadeImage::equalInputImages(const StyleCrossfadeImage& other) const
{
    return arePointingToEqualData(m_from, other.m_from) && arePointingToEqualData(m_to, other.m_to);
}

RefPtr<StyleCrossfadeImage> StyleCrossfadeImage::blend(const StyleCrossfadeImage& from, const BlendingContext& context) const
{
    ASSERT(equalInputImages(from));

    auto newPercentage = WebCore::blend(from.m_percentage, m_percentage, context);
    return StyleCrossfadeImage::create(m_from, m_to, newPercentage, from.m_isPrefixed && m_isPrefixed);
}

Ref<CSSValue> StyleCrossfadeImage::computedStyleValue(const RenderStyle& style) const
{
    auto fromComputedValue = m_from ? m_from->computedStyleValue(style) : static_reference_cast<CSSValue>(CSSPrimitiveValue::createIdentifier(CSSValueNone));
    auto toComputedValue = m_to ? m_to->computedStyleValue(style) : static_reference_cast<CSSValue>(CSSPrimitiveValue::createIdentifier(CSSValueNone));
    return CSSCrossfadeValue::create(WTFMove(fromComputedValue), WTFMove(toComputedValue), CSSPrimitiveValue::create(m_percentage, CSSUnitType::CSS_NUMBER), m_isPrefixed);
}

bool StyleCrossfadeImage::isPending() const
{
    if (m_from && m_from->isPending())
        return true;
    if (m_to && m_to->isPending())
        return true;
    return false;
}

void StyleCrossfadeImage::load(CachedResourceLoader& loader, const ResourceLoaderOptions& options)
{
    ASSERT(!m_inputImagesRegistered);

    if (m_from) {
        m_from->load(loader, options);
        m_from->registerObserver(*this);
    }

    if (m_to) {
        m_to->load(loader, options);
        m_to->registerObserver(*this);
    }

    m_inputImagesRegistered = true;
}

RefPtr<Image> StyleCrossfadeImage::image(const RenderElement* renderer, const FloatSize& size) const
{
    if (!renderer)
        return &Image::nullImage();

    if (size.isEmpty())
        return nullptr;

    if (!m_from || !m_to)
        return &Image::nullImage();

    auto fromImage = m_from->image(renderer, size);
    auto toImage = m_to->image(renderer, size);

    if (!fromImage || !toImage)
        return &Image::nullImage();

    return CrossfadeGeneratedImage::create(*fromImage, *toImage, m_percentage, fixedSize(*renderer), size);
}

bool StyleCrossfadeImage::knownToBeOpaque(const RenderElement& renderer) const
{
    if (m_from && !m_from->knownToBeOpaque(renderer))
        return false;
    if (m_to && !m_to->knownToBeOpaque(renderer))
        return false;
    return true;
}

FloatSize StyleCrossfadeImage::fixedSize(const RenderElement& renderer) const
{
    if (!m_from || !m_to)
        return { };

    auto fromImageSize = m_from->imageSize(&renderer, 1);
    auto toImageSize = m_to->imageSize(&renderer, 1);

    // Rounding issues can cause transitions between images of equal size to return
    // a different fixed size; avoid performing the interpolation if the images are the same size.
    if (fromImageSize == toImageSize)
        return fromImageSize;

    float percentage = m_percentage;
    float inversePercentage = 1 - percentage;

    return fromImageSize * inversePercentage + toImageSize * percentage;
}

void StyleCrossfadeImage::styleImageDidChange(const StyleImage&)
{
    if (!m_inputImagesRegistered)
        return;

    for (auto& client : clients().values())
        client->imageChanged(static_cast<WrappedImagePtr>(this));

    for (auto& observer : observers().values())
        observer->styleImageDidChange(*this);
}

} // namespace WebCore
