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
        m_from->addClient(*this);
    if (m_to)
        m_to->addClient(*this);
}

StyleCrossfadeImage::~StyleCrossfadeImage()
{
    if (m_from)
        m_from->removeClient(*this);
    if (m_to)
        m_to->removeClient(*this);
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

    // FIXME: Why is this check here?
    if (!m_from || !m_from->cachedImage() || !m_to || !m_to->cachedImage())
        return nullptr;

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

RefPtr<Image> StyleCrossfadeImage::imageForRenderer(const RenderElement* client, const FloatSize& size, bool isForFirstLine) const
{
    if (!client)
        return &Image::nullImage();

    if (size.isEmpty())
        return nullptr;

    if (!m_from || !m_to)
        return &Image::nullImage();

    auto fromImage = m_from->imageForRenderer(client, size, isForFirstLine);
    auto toImage = m_to->imageForRenderer(client, size, isForFirstLine);

    if (!fromImage || !toImage)
        return &Image::nullImage();

    return CrossfadeGeneratedImage::create(*fromImage, *toImage, m_percentage, fixedSizeForRenderer(*client), size);
}

bool StyleCrossfadeImage::knownToBeOpaqueForRenderer(const RenderElement& client) const
{
    if (m_from && !m_from->knownToBeOpaqueForRenderer(client))
        return false;
    if (m_to && !m_to->knownToBeOpaqueForRenderer(client))
        return false;
    return true;
}

LayoutSize StyleCrossfadeImage::fixedSizeForRenderer(const RenderElement& client) const
{
    if (!m_from || !m_to)
        return { };

    auto fromImageSize = m_from->imageSizeForRenderer(&client, 1);
    auto toImageSize = m_to->imageSizeForRenderer(&client, 1);

    // Rounding issues can cause transitions between images of equal size to return
    // a different fixed size; avoid performing the interpolation if the images are the same size.
    if (fromImageSize == toImageSize)
        return fromImageSize;

    float percentage = m_percentage;
    float inversePercentage = 1 - percentage;

    return LayoutSize(fromImageSize * inversePercentage + toImageSize * percentage);
}

// MARK: - StyleImageClient

void StyleCrossfadeImage::styleImageChanged(StyleImage& image, const IntRect*)
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    for (auto entry : clients())
        entry.key.styleImageChanged(*this);
}

void StyleCrossfadeImage::styleImageFinishedResourceLoad(StyleImage& image, CachedResource& resource)
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    for (auto entry : clients())
        entry.key.styleImageFinishedResourceLoad(*this, resource);
}

void StyleCrossfadeImage::styleImageFinishedLoad(StyleImage& image)
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    // FIXME: Send when all non-null have finished.
}

void StyleCrossfadeImage::styleImageNeedsScheduledRenderingUpdate(StyleImage& image)
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    for (auto entry : clients())
        entry.key.styleImageNeedsScheduledRenderingUpdate(*this);
}

bool StyleCrossfadeImage::styleImageCanDestroyDecodedData(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    // FIXME: Implement.
    return false;
}

bool StyleCrossfadeImage::styleImageAnimationAllowed(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    // FIXME: Implement.
    return false;
}

VisibleInViewportState StyleCrossfadeImage::styleImageFrameAvailable(StyleImage& image, ImageAnimatingState, const IntRect*)
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    // FIXME: Implement.
    return VisibleInViewportState::No;
}

VisibleInViewportState StyleCrossfadeImage::styleImageVisibleInViewport(StyleImage& image, const Document&) const
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());
    ASSERT(m_inputImagesAreReady);

    // FIXME: Implement.
    return VisibleInViewportState::No;
}

HashSet<Element*> StyleCrossfadeImage::styleImageReferencingElements(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());

    HashSet<Element*> result;
    for (auto entry : clients())
        result.formUnion(entry.key.styleImageReferencingElements(const_cast<StyleCrossfadeImage&>(*this)));
    return result;
}

ImageOrientation StyleCrossfadeImage::styleImageOrientation(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());

    // FIXME: Implement.
    return StyleImageClient::styleImageOrientation(const_cast<StyleCrossfadeImage&>(*this));
}

std::optional<LayoutSize> StyleCrossfadeImage::styleImageOverrideImageSize(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_from.get() || &image == m_to.get());

    // FIXME: Implement.
    return StyleImageClient::styleImageOverrideImageSize(const_cast<StyleCrossfadeImage&>(*this));
}

} // namespace WebCore
