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
#include "NullGraphicsContext.h"
#include "RenderElement.h"
#include <wtf/PointerComparison.h>

namespace WebCore {

// MARK: - StyleFilterImage

StyleFilterImage::StyleFilterImage(RefPtr<StyleImage>&& inputImage, FilterOperations&& filterOperations)
    : StyleGeneratedImage { Type::FilterImage, StyleFilterImage::isFixedSize }
    , m_inputImage { WTFMove(inputImage) }
    , m_filterOperations { WTFMove(filterOperations) }
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

    if (!m_inputImage)
        return &Image::nullImage();

    auto image = m_inputImage->imageForRenderer(client, size, isForFirstLine);
    if (!image || image->isNull())
        return &Image::nullImage();

    auto preferredFilterRenderingModes = client->page().preferredFilterRenderingModes();
    auto sourceImageRect = FloatRect { { }, size };

    auto cssFilter = CSSFilter::create(const_cast<RenderElement&>(*client), m_filterOperations, preferredFilterRenderingModes, FloatSize { 1, 1 }, sourceImageRect, NullGraphicsContext());
    if (!cssFilter)
        return &Image::nullImage();

    cssFilter->setFilterRegion(sourceImageRect);

    auto sourceImage = ImageBuffer::create(size, RenderingPurpose::DOM, 1, DestinationColorSpace::SRGB(), ImageBufferPixelFormat::BGRA8, bufferOptionsForRendingMode(cssFilter->renderingMode()), client->hostWindow());
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

void StyleFilterImage::styleImageChanged(StyleImage&, const IntRect*)
{
    if (!m_inputImageIsReady)
        return;

    for (auto entry : clients())
        entry.key.styleImageChanged(*this);
}

} // namespace WebCore
