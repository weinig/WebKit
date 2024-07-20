/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2023 Apple Inc. All rights reserved.
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
#include "ReferencedSVGResources.h"
#include "RenderElement.h"
#include "RenderSVGResourceMasker.h"
#include "RenderView.h"
#include "SVGImage.h"
#include "SVGMaskElement.h"
#include "SVGResourceImage.h"
#include "SVGSVGElement.h"
#include "SVGURIReference.h"

namespace WebCore {

Ref<StyleCachedImage> StyleCachedImage::create(Ref<CSSImageValue> cssValue, float scaleFactor)
{
    return adoptRef(*new StyleCachedImage(WTFMove(cssValue), scaleFactor));
}

Ref<StyleCachedImage> StyleCachedImage::create(CachedResourceHandle<CachedImage> cachedImage)
{
    return adoptRef(*new StyleCachedImage(WTFMove(cachedImage)));
}

Ref<StyleCachedImage> StyleCachedImage::copyOverridingScaleFactor(StyleCachedImage& other, float scaleFactor)
{
    if (other.m_scaleFactor == scaleFactor)
        return other;
    return StyleCachedImage::create(other.m_cssValue, scaleFactor);
}

StyleCachedImage::StyleCachedImage(Ref<CSSImageValue>&& cssValue, float scaleFactor)
    : StyleImage { Type::CachedImage }
    , m_cssValue { WTFMove(cssValue) }
    , m_scaleFactor { scaleFactor }
    , m_forceAllClientsWaitingForAsyncDecoding { false }
{
    m_cachedImage = m_cssValue->cachedImage();
    if (m_cachedImage)
        m_isPending = false;
}

StyleCachedImage::StyleCachedImage(CachedResourceHandle<CachedImage> cachedImage)
    : StyleCachedImage { CSSImageValue::create(WTFMove(cachedImage)), 1 }
{
}

StyleCachedImage::~StyleCachedImage()
{
    //    if (m_cachedImage && m_cachedImage->svgImageCache())
    //        m_cachedImage->svgImageCache()
}

bool StyleCachedImage::operator==(const StyleImage& other) const
{
    auto* otherCachedImage = dynamicDowncast<StyleCachedImage>(other);
    return otherCachedImage && equals(*otherCachedImage);
}

bool StyleCachedImage::equals(const StyleCachedImage& other) const
{
    if (&other == this)
        return true;
    if (m_scaleFactor != other.m_scaleFactor)
        return false;
    if (m_cssValue.ptr() == other.m_cssValue.ptr() || m_cssValue->equals(other.m_cssValue.get()))
        return true;
    if (m_cachedImage && m_cachedImage == other.m_cachedImage)
        return true;
    return false;
}

URL StyleCachedImage::imageURL() const
{
    return m_cssValue->imageURL();
}

URL StyleCachedImage::reresolvedURL(const Document& document) const
{
    return m_cssValue->reresolvedURL(document);
}

void StyleCachedImage::load(CachedResourceLoader& loader, const ResourceLoaderOptions& options)
{
    ASSERT(m_isPending);
    m_isPending = false;
    m_cachedImage = m_cssValue->loadImage(loader, options);
    if (m_cachedImage)
        m_cachedImage->addClient(*this);
}

CachedImage* StyleCachedImage::cachedImage() const
{
    return m_cachedImage.get();
}

Ref<CSSValue> StyleCachedImage::computedStyleValue(const RenderStyle&) const
{
    return m_cssValue.copyRef();
}

bool StyleCachedImage::isPending() const
{
    return m_isPending;
}

bool StyleCachedImage::isLoadedForRenderer(const RenderElement* client) const
{
    if (isRenderSVGResource(client))
        return true;
    return m_cachedImage && m_cachedImage->isLoaded();
}

bool StyleCachedImage::errorOccurred() const
{
    return m_cachedImage && m_cachedImage->errorOccurred();
}

bool StyleCachedImage::canRenderForRenderer(const RenderElement* client, float multiplier) const
{
    if (isRenderSVGResource(client))
        return true;
    return m_cachedImage && !m_cachedImage->errorOccurred() && !imageSizeForRenderer(client, multiplier, StyleImageSizeType::Used).isEmpty();
}

RefPtr<Image> StyleCachedImage::imageForRenderer(const RenderElement* client, const FloatSize&, bool) const
{
    ASSERT(!m_isPending);

    if (auto renderSVGResource = this->renderSVGResource(client))
        return SVGResourceImage::create(*renderSVGResource, reresolvedURL(client->document()));

    if (auto renderSVGResource = this->legacyRenderSVGResource(client))
        return SVGResourceImage::create(*renderSVGResource, reresolvedURL(client->document()));

    // Explicitly check for m_cachedImage here to return `nullptr`, instead of `Image::nullImage()`.
    if (!m_cachedImage)
        return nullptr;

    RefPtr image = m_cachedImage->image();
    if (image->drawsSVGImage()) {
        auto svgImage = m_cachedImage->svgImageCache()->imageForRenderer(client);
        if (svgImage != &Image::nullImage())
            return image;
    }

    return image.get();
}

bool StyleCachedImage::knownToBeOpaque() const
{
    // FIXME: Handle SVGResource cases.

    if (!m_cachedImage)
        return false;

    RefPtr image = m_cachedImage->rawImage();
    if (!image)
        return false;

    return image->currentFrameKnownToBeOpaque();
}

LayoutSize StyleCachedImage::imageSizeForRenderer(const RenderElement* client, float multiplier, StyleImageSizeType) const
{
    if (isRenderSVGResource(client))
        return m_containerSize;

    auto imageSize = unclampedImageSizeForRenderer(client, multiplier, StyleImageSizeType::Used);
    if (imageSize.isEmpty() || multiplier == 1.0f)
        return imageSize.scaledDown(m_scaleFactor);

    // Don't let images that have a width/height >= 1 shrink below 1 when zoomed.
    LayoutSize minimumSize(imageSize.width() > 0 ? 1 : 0, imageSize.height() > 0 ? 1 : 0);
    imageSize.clampToMinimumSize(minimumSize);

    ASSERT(multiplier != 1.0f || (imageSize.width().fraction() == 0.0f && imageSize.height().fraction() == 0.0f));
    return imageSize.scaledDown(m_scaleFactor);
}

NaturalDimensions StyleCachedImage::naturalDimensions() const
{
    if (!m_cachedImage)
        return NaturalDimensions::none();

    auto image = m_cachedImage->rawImage();
    if (!image)
        return NaturalDimensions::none();

    // FIXME: Implement.
    return NaturalDimensions::none();
}

bool StyleCachedImage::imageHasRelativeWidth() const
{
    return m_cachedImage && m_cachedImage->imageHasRelativeWidth();
}

bool StyleCachedImage::imageHasRelativeHeight() const
{
    return m_cachedImage && m_cachedImage->imageHasRelativeHeight();
}

void StyleCachedImage::computeIntrinsicDimensionsForRenderer(const RenderElement* client, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    // In case of an SVG resource, we should return the container size.
    if (isRenderSVGResource(client)) {
        FloatSize size = floorSizeToDevicePixels(m_containerSize, client ? client->document().deviceScaleFactor() : 1);
        intrinsicWidth = Length(size.width(), LengthType::Fixed);
        intrinsicHeight = Length(size.height(), LengthType::Fixed);
        intrinsicRatio = size;
        return;
    }

    if (!m_cachedImage)
        return;

    m_cachedImage->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
}

bool StyleCachedImage::usesImageContainerSize() const
{
    return m_cachedImage ? m_cachedImage->usesImageContainerSize() : false;
}

void StyleCachedImage::setContainerContextForRenderer(const RenderElement& client, const LayoutSize& containerSize, float containerZoom, const URL&)
{
    m_containerSize = containerSize;

    if (containerSize.isEmpty())
        return;

    ASSERT(containerZoom);
    auto image = m_cachedImage ? m_cachedImage->rawImage() : nullptr;;
    if (!image) {
        m_pendingContainerContextRequests.set(client, ContainerContext { containerSize, containerZoom, imageURL() });
        return;
    }

    if (image->drawsSVGImage())
        m_cachedImage->svgImageCache()->setContainerContextForRenderer(client, containerSize, containerZoom, imageURL());
    else
        image->setContainerSize(containerSize);
}

bool StyleCachedImage::isClientWaitingForAsyncDecoding(const StyleImageClient& client) const
{
    return m_forceAllClientsWaitingForAsyncDecoding
        || m_clientsWaitingForAsyncDecoding.contains(const_cast<StyleImageClient&>(client));
}

void StyleCachedImage::addClientWaitingForAsyncDecoding(StyleImageClient& client)
{
    if (m_forceAllClientsWaitingForAsyncDecoding || m_clientsWaitingForAsyncDecoding.contains(client))
        return;

    if (!m_clients.contains(client)) {
        // If the <html> element does not have its own background specified, painting the root box
        // renderer uses the style of the <body> element, see RenderView::rendererForRootBackground().
        // In this case, the client we are asked to add is the root box renderer. Since we can't add
        // a client to m_clientsWaitingForAsyncDecoding unless it is one of the m_clients, we are going
        // to cancel the repaint optimization we do in CachedImage::imageFrameAvailable() by adding
        // all the m_clients to m_clientsWaitingForAsyncDecoding.
        m_forceAllClientsWaitingForAsyncDecoding = true;
        if (m_cachedImage)
            m_cachedImage->setForceAllClientsWaitingForAsyncDecoding(true);
    } else {
        m_clientsWaitingForAsyncDecoding.add(client);

        if (m_cachedImage)
            m_cachedImage->addClientWaitingForAsyncDecoding(*this);
    }
}

void StyleCachedImage::removeAllClientsWaitingForAsyncDecoding()
{
    m_clientsWaitingForAsyncDecoding.clear();
    m_forceAllClientsWaitingForAsyncDecoding = false;

    if (m_cachedImage)
        m_cachedImage->removeAllClientsWaitingForAsyncDecoding();
}

void StyleCachedImage::addClient(StyleImageClient& client)
{
    ASSERT(!m_isPending);
    m_clients.add(client);
}

void StyleCachedImage::removeClient(StyleImageClient& client)
{
    ASSERT(!m_isPending);
    if (m_clients.remove(client)) {
        m_clientsWaitingForAsyncDecoding.remove(client);
        for (auto entry : m_clients)
            entry.key.styleImageClientRemoved(*this);
    }
}

bool StyleCachedImage::hasClient(StyleImageClient& client) const
{
    ASSERT(!m_isPending);
    return m_clients.contains(client);
}

bool StyleCachedImage::hasImage() const
{
    return m_cachedImage && m_cachedImage->hasImage();
}

Image* StyleCachedImage::image() const
{
    return m_cachedImage ? m_cachedImage->rawImage() : nullptr;
}

float StyleCachedImage::imageScaleFactor() const
{
    return m_scaleFactor;
}

bool StyleCachedImage::usesDataProtocol() const
{
    return m_cssValue->imageURL().protocolIsData();
}

// MARK: - CachedImageClient

void StyleCachedImage::notifyFinished(CachedResource& resource, const NetworkLoadMetrics&, LoadWillContinueInAnotherProcess)
{
    for (auto entry : m_clients)
        entry.key.styleImageFinishedResourceLoad(*this, resource);
    for (auto entry : m_clients)
        entry.key.styleImageFinishedLoad(*this);
}

bool StyleCachedImage::canDestroyDecodedData(CachedImage& cachedImage) const
{
    ASSERT_UNUSED(cachedImage, cachedImage == m_cachedImage);

    for (auto entry : m_clients) {
        if (!entry.key.styleImageCanDestroyDecodedData(const_cast<StyleCachedImage&>(*this)))
            return false;
    }
    return true;
}

bool StyleCachedImage::allowsAnimation(CachedImage& cachedImage) const
{
    ASSERT_UNUSED(cachedImage, cachedImage == m_cachedImage);

    for (auto entry : m_clients) {
        if (entry.key.styleImageAnimationAllowed(const_cast<StyleCachedImage&>(*this)))
            return true;
    }
    return false;
}

void StyleCachedImage::imageCreated(CachedImage& cachedImage, const Image& image)
{
    ASSERT_UNUSED(cachedImage, cachedImage == m_cachedImage);

    // Send queued container size requests.
    if (image.usesContainerSize()) {
        bool useSVGCache = image.drawsSVGImage();
        for (auto [client, context] : m_pendingContainerContextRequests) {
            if (useSVGCache)
                m_cachedImage->svgImageCache()->setContainerContextForRenderer(client, context.containerSize, context.containerZoom, context.imageURL);
            else
                const_cast<Image&>(image).setContainerSize(context.containerSize);
        }
    }

    m_pendingContainerContextRequests.clear();
    m_clientsWaitingForAsyncDecoding.clear();
    m_forceAllClientsWaitingForAsyncDecoding = false;
}

void StyleCachedImage::imageChanged(CachedImage& cachedImage, const IntRect* rect)
{
    ASSERT_UNUSED(cachedImage, &cachedImage == m_cachedImage.get());

    for (auto entry : m_clients)
        entry.key.styleImageChanged(*this, rect);
}

void StyleCachedImage::scheduleRenderingUpdateForImage(CachedImage& cachedImage)
{
    ASSERT_UNUSED(cachedImage, &cachedImage == m_cachedImage.get());

    for (auto entry : m_clients)
        entry.key.styleImageNeedsScheduledRenderingUpdate(*this);
}

VisibleInViewportState StyleCachedImage::imageFrameAvailable(CachedImage& cachedImage, ImageAnimatingState animatingState, const IntRect* changeRect, DecodingStatus decodingStatus)
{
    ASSERT_UNUSED(cachedImage, &cachedImage == m_cachedImage.get());

    VisibleInViewportState visibleState = VisibleInViewportState::No;

    for (auto entry : m_clients) {
        if (animatingState == ImageAnimatingState::No && !m_forceAllClientsWaitingForAsyncDecoding && !m_clientsWaitingForAsyncDecoding.contains(entry.key))
            continue;
        if (entry.key.styleImageFrameAvailable(*this, animatingState, changeRect) == VisibleInViewportState::Yes)
            visibleState = VisibleInViewportState::Yes;
    }

    if (decodingStatus != DecodingStatus::Partial) {
        m_clientsWaitingForAsyncDecoding.clear();
        m_forceAllClientsWaitingForAsyncDecoding = false;
    }

    return visibleState;
}

VisibleInViewportState StyleCachedImage::imageVisibleInViewport(CachedImage& cachedImage, const Document& document) const
{
    ASSERT_UNUSED(cachedImage, &cachedImage == m_cachedImage.get());

    for (auto entry : m_clients) {
        if (entry.key.styleImageVisibleInViewport(const_cast<StyleCachedImage&>(*this), document) == VisibleInViewportState::Yes)
            return VisibleInViewportState::Yes;
    }
    return VisibleInViewportState::No;
}


// MARK: - Internal

LegacyRenderSVGResourceContainer* StyleCachedImage::uncheckedRenderSVGResource(TreeScope& treeScope, const AtomString& fragment) const
{
    auto renderSVGResource = ReferencedSVGResources::referencedRenderResource(treeScope, fragment);
    m_isRenderSVGResource = renderSVGResource != nullptr;
    return renderSVGResource;
}

LegacyRenderSVGResourceContainer* StyleCachedImage::uncheckedRenderSVGResource(const RenderElement* client) const
{
    if (!client)
        return nullptr;

    if (imageURL().string().find('#') == notFound) {
        m_isRenderSVGResource = false;
        return nullptr;
    }

    Ref document = client->document();
    auto reresolvedURL = this->reresolvedURL(document);

    if (!m_cachedImage) {
        auto fragmentIdentifier = SVGURIReference::fragmentIdentifierFromIRIString(reresolvedURL.string(), document);
        return uncheckedRenderSVGResource(client->treeScopeForSVGReferences(), fragmentIdentifier);
    }

    auto image = dynamicDowncast<SVGImage>(m_cachedImage->image());
    if (!image)
        return nullptr;

    auto rootElement = image->rootElement();
    if (!rootElement)
        return nullptr;

    auto fragmentIdentifier = reresolvedURL.fragmentIdentifier();
    return uncheckedRenderSVGResource(rootElement->treeScopeForSVGReferences(), fragmentIdentifier.toAtomString());
}

LegacyRenderSVGResourceContainer* StyleCachedImage::legacyRenderSVGResource(const RenderElement* client) const
{
    if (m_isRenderSVGResource && !*m_isRenderSVGResource)
        return nullptr;
    return uncheckedRenderSVGResource(client);
}

RenderSVGResourceContainer* StyleCachedImage::renderSVGResource(const RenderElement* client) const
{
    if (m_isRenderSVGResource)
        return nullptr;

    if (!client)
        return nullptr;

    if (imageURL().string().find('#') == notFound)
        return nullptr;

    if (!m_cachedImage) {
        if (RefPtr referencedMaskElement = ReferencedSVGResources::referencedMaskElement(client->treeScopeForSVGReferences(), *this)) {
            if (auto* referencedMaskerRenderer = dynamicDowncast<RenderSVGResourceMasker>(referencedMaskElement->renderer()))
                return referencedMaskerRenderer;
        }
        return nullptr;
    }

    auto image = dynamicDowncast<SVGImage>(m_cachedImage->image());
    if (!image)
        return nullptr;

    auto rootElement = image->rootElement();
    if (!rootElement)
        return nullptr;

    Ref document = client->document();
    auto reresolvedURL = this->reresolvedURL(document);
    auto fragmentIdentifier = reresolvedURL.fragmentIdentifier();
    if (RefPtr referencedMaskElement = ReferencedSVGResources::referencedMaskElement(rootElement->treeScopeForSVGReferences(), fragmentIdentifier.toAtomString())) {
        if (auto* referencedMaskerRenderer = dynamicDowncast<RenderSVGResourceMasker>(referencedMaskElement->renderer()))
            return referencedMaskerRenderer;
    }

    return nullptr;
}

bool StyleCachedImage::isRenderSVGResource(const RenderElement* client) const
{
    return renderSVGResource(client) || legacyRenderSVGResource(client);
}

LayoutSize StyleCachedImage::unclampedImageSizeForRenderer(const RenderElement* client, float multiplier, StyleImageSizeType sizeType) const
{
    if (!m_cachedImage)
        return { };

    RefPtr image = m_cachedImage->rawImage();
    if (!image)
        return { };

    auto imageSize = [&] {
        if (client) {
            if (auto overrideImageSize = client->styleImageOverrideImageSize(const_cast<StyleCachedImage&>(*this)))
                return *overrideImageSize;
        }

        if (image->drawsSVGImage() && sizeType == StyleImageSizeType::Used)
            return m_cachedImage->svgImageCache()->imageSizeForRenderer(client);

        return LayoutSize(image->size(client ? client->styleImageOrientation(const_cast<StyleCachedImage&>(*this)) : ImageOrientation(ImageOrientation::Orientation::FromImage)));
    }();

    if (imageSize.isEmpty() || multiplier == 1.0f)
        return imageSize;

    float widthScale = image->hasRelativeWidth() ? 1.0f : multiplier;
    float heightScale = image->hasRelativeHeight() ? 1.0f : multiplier;
    return imageSize.scaled(widthScale, heightScale);
}

} // namespace WebCore
