/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2021 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "StyleCachedImage.h"

#include "CSSImageValue.h"
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CachedResourceRequestInitiators.h"
#include "Document.h"
#include "Element.h"
#include "RenderElement.h"
#include "RenderView.h"

namespace WebCore {

Ref<StyleCachedImage> StyleCachedImage::create(ResolvedURL url, LoadedFromOpaqueSource loadedFromOpaqueSource, AtomString initiatorName, float scaleFactor)
{
    return adoptRef(*new StyleCachedImage(WTFMove(url), loadedFromOpaqueSource, WTFMove(initiatorName), scaleFactor));
}

Ref<StyleCachedImage> StyleCachedImage::create(URL url, LoadedFromOpaqueSource loadedFromOpaqueSource, AtomString initiatorName, float scaleFactor)
{
    return adoptRef(*new StyleCachedImage(makeResolvedURL(WTFMove(url)), loadedFromOpaqueSource, WTFMove(initiatorName), scaleFactor));
}

Ref<StyleCachedImage> StyleCachedImage::create(StyleCachedImage& other, float scaleFactor)
{
    if (other.m_scaleFactor == scaleFactor)
        return other;
    return adoptRef(*new StyleCachedImage(other, scaleFactor));
}

StyleCachedImage::StyleCachedImage(ResolvedURL&& url, LoadedFromOpaqueSource loadedFromOpaqueSource, AtomString&& initiatorName, float scaleFactor)
    : StyleImage { Type::CachedImage }
    , m_url { WTFMove(url) }
    , m_loadedFromOpaqueSource { loadedFromOpaqueSource }
    , m_initiatorName { WTFMove(initiatorName) }
    , m_isPending { true }
    , m_scaleFactor { scaleFactor }
{
}

StyleCachedImage::StyleCachedImage(StyleCachedImage& other, float scaleFactor)
    : StyleImage { Type::CachedImage }
    , m_url { other.m_url }
    , m_loadedFromOpaqueSource { other.m_loadedFromOpaqueSource }
    , m_initiatorName { other.m_initiatorName }
    , m_isPending { other.m_isPending }
    , m_scaleFactor { scaleFactor }
{
    ASSERT(other.m_scaleFactor != scaleFactor);
}

StyleCachedImage::~StyleCachedImage() = default;

bool StyleCachedImage::operator==(const StyleImage& other) const
{
    // FROM CSSImageValue.
    // return m_url == other.m_url;


    if (!is<StyleCachedImage>(other))
        return false;
    auto& otherCached = downcast<StyleCachedImage>(other);
    if (&otherCached == this)
        return true;
    if (m_scaleFactor != otherCached.m_scaleFactor)
        return false;
    if (m_cachedImage && m_cachedImage == otherCached.m_cachedImage)
        return true;
    return false;
}

URL StyleCachedImage::reresolvedURL(const Document& document) const
{
    if (CSSValue::isCSSLocalURL(m_url.resolvedURL.string()))
        return m_url.resolvedURL;

    // Re-resolving the URL is important for cases where resolvedURL is still not an absolute URL.
    // This can happen if there was no absolute base URL when the value was created, like a style from a document without a base URL.
    if (m_url.isLocalURL())
        return document.completeURL(m_url.specifiedURLString, URL());

    return document.completeURL(m_url.resolvedURL.string());
}

void StyleCachedImage::load(CachedResourceLoader& loader, const ResourceLoaderOptions& options)
{
    ASSERT(m_isPending);
    m_isPending = false;

    if (m_cachedImage)
        return;

    ResourceLoaderOptions loadOptions = options;
    loadOptions.loadedFromOpaqueSource = m_loadedFromOpaqueSource;
    
    CachedResourceRequest request(ResourceRequest(reresolvedURL(*loader.document())), loadOptions);
    if (m_initiatorName.isEmpty())
        request.setInitiator(cachedResourceRequestInitiators().css);
    else
        request.setInitiator(m_initiatorName);

    if (options.mode == FetchOptions::Mode::Cors) {
        ASSERT(loader.document());
        request.updateForAccessControl(*loader.document());
    }

    m_cachedImage = loader.requestImage(WTFMove(request)).value_or(nullptr);
}

CachedImage* StyleCachedImage::cachedImage() const
{
    return m_cachedImage.get();
}

Ref<CSSValue> StyleCachedImage::computedStyleValue(const RenderStyle&) const
{
    return CSSImageValue::create(m_url, m_loadedFromOpaqueSource, m_initiatorName);
}

bool StyleCachedImage::canRender(const RenderElement* renderer, float multiplier) const
{
    if (!m_cachedImage)
        return false;
    return m_cachedImage->canRender(renderer, multiplier);
}

bool StyleCachedImage::isPending() const
{
    return m_isPending;
}

bool StyleCachedImage::isLoaded() const
{
    if (!m_cachedImage)
        return false;
    return m_cachedImage->isLoaded();
}

bool StyleCachedImage::errorOccurred() const
{
    if (!m_cachedImage)
        return false;
    return m_cachedImage->errorOccurred();
}

FloatSize StyleCachedImage::imageSize(const RenderElement* renderer, float multiplier) const
{
    if (!m_cachedImage)
        return { };
    FloatSize size = m_cachedImage->imageSizeForRenderer(renderer, multiplier);
    size.scale(1 / m_scaleFactor);
    return size;
}

bool StyleCachedImage::imageHasRelativeWidth() const
{
    if (!m_cachedImage)
        return false;
    return m_cachedImage->imageHasRelativeWidth();
}

bool StyleCachedImage::imageHasRelativeHeight() const
{
    if (!m_cachedImage)
        return false;
    return m_cachedImage->imageHasRelativeHeight();
}

void StyleCachedImage::computeIntrinsicDimensions(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    if (!m_cachedImage)
        return;
    m_cachedImage->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
}

bool StyleCachedImage::usesImageContainerSize() const
{
    if (!m_cachedImage)
        return false;
    return m_cachedImage->usesImageContainerSize();
}

void StyleCachedImage::setContainerContextForRenderer(const RenderElement& renderer, const FloatSize& containerSize, float containerZoom)
{
    if (!m_cachedImage)
        return;
    m_cachedImage->setContainerContextForClient(renderer, LayoutSize(containerSize), containerZoom, imageURL());
}

void StyleCachedImage::addClient(RenderElement& renderer)
{
    ASSERT(!m_isPending);
    if (!m_cachedImage)
        return;
    m_cachedImage->addClient(renderer);
}

void StyleCachedImage::removeClient(RenderElement& renderer)
{
    ASSERT(!m_isPending);
    if (!m_cachedImage)
        return;
    m_cachedImage->removeClient(renderer);
}

bool StyleCachedImage::hasClient(RenderElement& renderer) const
{
    ASSERT(!m_isPending);
    if (!m_cachedImage)
        return false;
    return m_cachedImage->hasClient(renderer);
}

bool StyleCachedImage::hasImage() const
{
    if (!m_cachedImage)
        return false;
    return m_cachedImage->hasImage();
}

RefPtr<Image> StyleCachedImage::image(const RenderElement* renderer, const FloatSize&) const
{
    ASSERT(!m_isPending);
    if (!m_cachedImage)
        return nullptr;
    return m_cachedImage->imageForRenderer(renderer);
}

float StyleCachedImage::imageScaleFactor() const
{
    return m_scaleFactor;
}

bool StyleCachedImage::knownToBeOpaque(const RenderElement& renderer) const
{
    return m_cachedImage && m_cachedImage->currentFrameKnownToBeOpaque(&renderer);
}

bool StyleCachedImage::usesDataProtocol() const
{
    return imageURL().protocolIsData();
}

} // namespace WebCore
