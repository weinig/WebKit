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
#include "CSSValuePool.h"
#include "CachedImage.h"
#include "CachedResourceLoader.h"
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
    , m_inputImagesAreReady { false }
{
    if (m_from)
        m_from->addStyleImageClient(*this);
    if (m_to)
        m_to->addStyleImageClient(*this);
}

StyleCrossfadeImage::~StyleCrossfadeImage()
{
    if (m_from)
        m_from->removeStyleImageClient(*this);
    if (m_to)
        m_to->removeStyleImageClient(*this);
}

bool StyleCrossfadeImage::operator==(const StyleImage& other) const
{
    auto* otherCrossfadeImage = dynamicDowncast<StyleCrossfadeImage>(other);
    return otherCrossfadeImage && equals(*otherCrossfadeImage);
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

    //    if (!m_cachedToImage || !m_cachedFromImage)
    //        return nullptr;

    auto newPercentage = WebCore::blend(from.m_percentage, m_percentage, context);
    return StyleCrossfadeImage::create(m_from, m_to, newPercentage, from.m_isPrefixed && m_isPrefixed);
}

Ref<CSSValue> StyleCrossfadeImage::computedStyleValue(const RenderStyle& style) const
{
    auto fromComputedValue = m_from ? m_from->computedStyleValue(style) : static_reference_cast<CSSValue>(CSSPrimitiveValue::create(CSSValueNone));
    auto toComputedValue = m_to ? m_to->computedStyleValue(style) : static_reference_cast<CSSValue>(CSSPrimitiveValue::create(CSSValueNone));
    return CSSCrossfadeValue::create(WTFMove(fromComputedValue), WTFMove(toComputedValue), CSSPrimitiveValue::create(m_percentage), m_isPrefixed);
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
    if (m_from) {
        if (m_from->isPending())
            m_from->load(loader, options);
    }

    if (m_to) {
        if (m_to->isPending())
            m_to->load(loader, options);
    }

    m_inputImagesAreReady = true;
}

bool StyleCrossfadeImage::isLoaded(const RenderElement* renderer) const
{
    if (m_from && !m_from->isLoaded(renderer))
        return false;
    if (m_to && !m_to->isLoaded(renderer))
        return false;
    return true;
}

bool StyleCrossfadeImage::errorOccurred() const
{
    if (m_from && m_from->errorOccurred())
        return true;
    if (m_to && m_to->errorOccurred())
        return true;
    return false;
}

RefPtr<Image> StyleCrossfadeImage::image(const RenderElement* renderer, const FloatSize& size, bool isForFirstLine) const
{
    if (!renderer)
        return &Image::nullImage();

    if (size.isEmpty())
        return nullptr;

    if (!m_from || !m_to)
        return &Image::nullImage();

    auto fromImage = m_from->image(renderer, size, isForFirstLine);
    auto toImage = m_to->image(renderer, size, isForFirstLine);

    if (!fromImage || !toImage)
        return &Image::nullImage();

    return CrossfadeGeneratedImage::create(*fromImage, *toImage, m_percentage, fixedSize(*renderer), size);
}

bool StyleCrossfadeImage::canRender(const RenderElement* renderer, float multiplier) const
{
    if (m_from && !m_from->canRender(renderer, multiplier))
        return false;
    if (m_to && !m_to->canRender(renderer, multiplier))
        return false;
    return true;
}

void StyleCrossfadeImage::setContainerContextForRenderer(const RenderElement& renderer, const FloatSize& containerSize, float containerZoom)
{
    if (m_from)
        m_from->setContainerContextForRenderer(renderer, containerSize, containerZoom);
    if (m_to)
        m_to->setContainerContextForRenderer(renderer, containerSize, containerZoom);

    StyleGeneratedImage::setContainerContextForRenderer(renderer, containerSize, containerZoom);
}

bool StyleCrossfadeImage::knownToBeOpaque(const RenderElement& renderer) const
{
    if (m_from && !m_from->knownToBeOpaque(renderer))
        return false;
    if (m_to && !m_to->knownToBeOpaque(renderer))
        return false;
    return true;
}

bool StyleCrossfadeImage::isClientWaitingForAsyncDecoding(RenderElement& renderer) const
{
    return (m_from && m_from->isClientWaitingForAsyncDecoding(renderer))
        || (m_to && m_to->isClientWaitingForAsyncDecoding(renderer));
}

void StyleCrossfadeImage::addClientWaitingForAsyncDecoding(RenderElement& renderer)
{
    if (m_from)
        m_from->addClientWaitingForAsyncDecoding(renderer);
    if (m_to)
        m_to->addClientWaitingForAsyncDecoding(renderer);
}

void StyleCrossfadeImage::removeAllClientsWaitingForAsyncDecoding()
{
    if (m_from)
        m_from->removeAllClientsWaitingForAsyncDecoding();
    if (m_to)
        m_to->removeAllClientsWaitingForAsyncDecoding();
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

void StyleCrossfadeImage::styleImageChanged(StyleImage&, const IntRect*)
{
    if (!m_inputImagesAreReady)
        return;

    notifyClientsOfChange();
}

} // namespace WebCore
