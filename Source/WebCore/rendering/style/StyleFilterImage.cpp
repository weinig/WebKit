/*
 * Copyright (C) 2022-2023 Apple Inc. All rights reserved.
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
#include "StyleFilterImage.h"

#include "BitmapImage.h"
#include "CSSFilter.h"
#include "CSSFilterImageValue.h"
#include "CSSValuePool.h"
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "ComputedStyleExtractor.h"
#include "HostWindow.h"
#include "ImageBuffer.h"
#include "LocalFrameView.h"
#include "NullGraphicsContext.h"
#include "RenderElement.h"
#include <wtf/PointerComparison.h>

namespace WebCore {

// MARK: - StyleFilterImage

StyleFilterImage::StyleFilterImage(Document* document, RefPtr<StyleImage>&& inputImage, FilterOperations&& filterOperations)
    : StyleGeneratedImage { Type::FilterImage, StyleFilterImage::isFixedSize }
    , m_inputImage { WTFMove(inputImage) }
    , m_filterOperations { WTFMove(filterOperations) }
    , m_document { document }
    , m_inputImageIsReady { false }
{
    if (m_inputImage)
        m_inputImage->addClient(*this);
}

StyleFilterImage::~StyleFilterImage()
{
    if (m_inputImage)
        m_inputImage->removeClient(*this);
}

bool StyleFilterImage::operator==(const StyleImage& other) const
{
    auto* otherFilterImage = dynamicDowncast<StyleFilterImage>(other);
    return otherFilterImage && equals(*otherFilterImage);
}

bool StyleFilterImage::equals(const StyleFilterImage& other) const
{
    return equalInputImages(other) && m_filterOperations == other.m_filterOperations;
}

bool StyleFilterImage::equalInputImages(const StyleFilterImage& other) const
{
    return arePointingToEqualData(m_inputImage, other.m_inputImage);
}

Ref<CSSValue> StyleFilterImage::computedStyleValue(const RenderStyle& style) const
{
    return CSSFilterImageValue::create(m_inputImage ? m_inputImage->computedStyleValue(style) : static_reference_cast<CSSValue>(CSSPrimitiveValue::create(CSSValueNone)), ComputedStyleExtractor::valueForFilter(style, m_filterOperations));
}

bool StyleFilterImage::isPending() const
{
    return m_inputImage ? m_inputImage->isPending() : false;
}

void StyleFilterImage::load(CachedResourceLoader& cachedResourceLoader, const ResourceLoaderOptions& options)
{
    if (m_inputImage)
        m_inputImage->load(cachedResourceLoader, options);

    for (auto& filterOperation : m_filterOperations) {
        // FIXME: StyleFilterImage needs to be able to track if these have finished loading.
        if (RefPtr referenceFilterOperation = dynamicDowncast<ReferenceFilterOperation>(filterOperation))
            referenceFilterOperation->loadExternalDocumentIfNeeded(cachedResourceLoader, options);
    }

    m_inputImageIsReady = true;
}

RefPtr<Image> StyleFilterImage::imageForRenderer(const RenderElement* client, const FloatSize& size, bool isForFirstLine) const
{
    if (!client)
        return &Image::nullImage();

    if (size.isEmpty())
        return nullptr;

    if (!m_inputImage || !m_document)
        return &Image::nullImage();

    auto image = m_inputImage->imageForRenderer(client, size, isForFirstLine);
    if (!image || image->isNull())
        return &Image::nullImage();

    auto& treeScopeForSVGReferences = const_cast<RenderElement*>(client)->treeScopeForSVGReferences();
    auto preferredFilterRenderingModes = m_document->preferredFilterRenderingModes();
    auto sourceImageRect = FloatRect { { }, size };

    auto cssFilter = CSSFilter::create(treeScopeForSVGReferences, m_filterOperations, preferredFilterRenderingModes, FloatSize { 1, 1 }, sourceImageRect, NullGraphicsContext());
    if (!cssFilter)
        return &Image::nullImage();

    cssFilter->setFilterRegion(sourceImageRect);

    auto hostWindow = (m_document->view() && m_document->view()->root()) ? m_document->view()->root()->hostWindow() : nullptr;

    auto sourceImage = ImageBuffer::create(size, RenderingPurpose::DOM, 1, DestinationColorSpace::SRGB(), ImageBufferPixelFormat::BGRA8, bufferOptionsForRendingMode(cssFilter->renderingMode()), hostWindow);
    if (!sourceImage)
        return &Image::nullImage();

    auto filteredImage = sourceImage->filteredNativeImage(*cssFilter, [&](GraphicsContext& context) {
        context.drawImage(*image, sourceImageRect);
    });
    if (!filteredImage)
        return &Image::nullImage();
    return BitmapImage::create(WTFMove(filteredImage));
}

bool StyleFilterImage::knownToBeOpaqueForRenderer(const RenderElement&) const
{
    return false;
}

LayoutSize StyleFilterImage::fixedSizeForRenderer(const RenderElement& client) const
{
    if (!m_inputImage)
        return { };

    return m_inputImage->imageSizeForRenderer(&client, 1);
}

// MARK: - StyleImageClient

void StyleFilterImage::styleImageChanged(StyleImage& image, const IntRect*)
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    for (auto entry : clients())
        entry.key.styleImageChanged(*this);
}

void StyleFilterImage::styleImageFinishedResourceLoad(StyleImage& image, CachedResource& resource)
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    for (auto entry : clients())
        entry.key.styleImageFinishedResourceLoad(*this, resource);
}

void StyleFilterImage::styleImageFinishedLoad(StyleImage& image)
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    // FIXME: This should also wait until any loads from FilterOperations are complete.

    for (auto entry : clients())
        entry.key.styleImageFinishedLoad(*this);
}

void StyleFilterImage::styleImageNeedsScheduledRenderingUpdate(StyleImage& image)
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    for (auto entry : clients())
        entry.key.styleImageNeedsScheduledRenderingUpdate(*this);
}

bool StyleFilterImage::styleImageCanDestroyDecodedData(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    for (auto entry : clients()) {
        if (!entry.key.styleImageCanDestroyDecodedData(const_cast<StyleFilterImage&>(*this)))
            return false;
    }
    return true;
}

bool StyleFilterImage::styleImageAnimationAllowed(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    for (auto entry : clients()) {
        if (!entry.key.styleImageAnimationAllowed(const_cast<StyleFilterImage&>(*this)))
            return false;
    }
    return true;
}

VisibleInViewportState StyleFilterImage::styleImageFrameAvailable(StyleImage& image, ImageAnimatingState animatingState, const IntRect* rect)
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    // FIXME: Should we delay this until filter operations have loaded?

    auto visibilityState = VisibleInViewportState::No;
    for (auto entry : clients()) {
        if (entry.key.styleImageFrameAvailable(*this, animatingState, rect) == VisibleInViewportState::Yes)
            visibilityState = VisibleInViewportState::Yes;
    }
    return visibilityState;
}

VisibleInViewportState StyleFilterImage::styleImageVisibleInViewport(StyleImage& image, const Document& document) const
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());
    ASSERT(m_inputImageIsReady);

    for (auto entry : clients()) {
        if (entry.key.styleImageVisibleInViewport(const_cast<StyleFilterImage&>(*this), document) == VisibleInViewportState::Yes)
            return VisibleInViewportState::Yes;
    }
    return VisibleInViewportState::No;
}

HashSet<Element*> StyleFilterImage::styleImageReferencingElements(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());

    HashSet<Element*> result;
    for (auto entry : clients())
        result.formUnion(entry.key.styleImageReferencingElements(const_cast<StyleFilterImage&>(*this)));
    return result;
}

ImageOrientation StyleFilterImage::styleImageOrientation(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());

    // FIXME: Implement.
    return StyleImageClient::styleImageOrientation(const_cast<StyleFilterImage&>(*this));
}

std::optional<LayoutSize> StyleFilterImage::styleImageOverrideImageSize(StyleImage& image) const
{
    ASSERT_UNUSED(image, &image == m_inputImage.get());

    // FIXME: Implement.
    return StyleImageClient::styleImageOverrideImageSize(const_cast<StyleFilterImage&>(*this));
}

} // namespace WebCore
