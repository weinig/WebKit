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
#include "StyleLocalSVGResourceImage.h"

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

Ref<StyleLocalSVGResourceImage> StyleLocalSVGResourceImage::create(Ref<CSSImageValue> cssValue, float scaleFactor)
{
    return adoptRef(*new StyleLocalSVGResourceImage(WTFMove(cssValue), scaleFactor));
}

Ref<StyleLocalSVGResourceImage> StyleLocalSVGResourceImage::copyOverridingScaleFactor(StyleLocalSVGResourceImage& other, float scaleFactor)
{
    if (other.m_scaleFactor == scaleFactor)
        return other;
    return StyleLocalSVGResourceImage::create(other.m_cssValue, scaleFactor);
}

StyleLocalSVGResourceImage::StyleLocalSVGResourceImage(Ref<CSSImageValue>&& cssValue, float scaleFactor)
    : StyleImage { Type::CachedImage }
    , m_cssValue { WTFMove(cssValue) }
    , m_scaleFactor { scaleFactor }
    , m_forceAllClientsWaitingForAsyncDecoding { false }
{
    m_cachedImage = m_cssValue->cachedImage();
    if (m_cachedImage)
        m_isPending = false;
}

StyleLocalSVGResourceImage::StyleLocalSVGResourceImage(CachedResourceHandle<CachedImage> cachedImage)
    : StyleLocalSVGResourceImage { CSSImageValue::create(WTFMove(cachedImage)), 1 }
{
}

StyleLocalSVGResourceImage::~StyleLocalSVGResourceImage() = default;

bool StyleLocalSVGResourceImage::operator==(const StyleImage& other) const
{
    auto* otherCachedImage = dynamicDowncast<StyleLocalSVGResourceImage>(other);
    return otherCachedImage && equals(*otherCachedImage);
}

bool StyleLocalSVGResourceImage::equals(const StyleLocalSVGResourceImage& other) const
{
    if (&other == this)
        return true;
    if (m_scaleFactor != other.m_scaleFactor)
        return false;
    if (m_cssValue.ptr() == other.m_cssValue.ptr() || m_cssValue->equals(other.m_cssValue.get()))
        return true;
    return false;
}

URL StyleLocalSVGResourceImage::imageURL() const
{
    return m_cssValue->imageURL();
}

//URL StyleLocalSVGResourceImage::reresolvedURL(const Document& document) const
//{
//    return m_cssValue->reresolvedURL(document);
//}

void StyleLocalSVGResourceImage::load(CachedResourceLoader&, const ResourceLoaderOptions&)
{
}

Ref<CSSValue> StyleLocalSVGResourceImage::computedStyleValue(const RenderStyle&) const
{
    return m_cssValue.copyRef();
}

bool StyleLocalSVGResourceImage::isPending() const
{
    return false;
}

bool StyleLocalSVGResourceImage::isLoaded() const
{
    return m_isRenderSVGResource;
}

bool StyleLocalSVGResourceImage::errorOccurred() const
{
    return false;
}

bool StyleLocalSVGResourceImage::canRender() const
{
    return m_isRenderSVGResource;
}

RefPtr<Image> StyleLocalSVGResourceImage::imageForContext(const StyleImageSizingContext& context, const FloatSize&, bool) const
{
    if (auto renderSVGResource = this->renderSVGResource(context))
        return SVGResourceImage::create(*renderSVGResource, imageURL());

    if (auto renderSVGResource = this->legacyRenderSVGResource(context))
        return SVGResourceImage::create(*renderSVGResource, imageURL());

    return nullptr;
}

bool StyleLocalSVGResourceImage::knownToBeOpaque() const
{
    // FIXME: Handle SVGResource cases.
    return false;
}

NaturalDimensions StyleLocalSVGResourceImage::naturalDimensions() const
{
    return NaturalDimensions::none();
}

bool StyleLocalSVGResourceImage::imageHasRelativeWidth() const
{
    return false;
}

bool StyleLocalSVGResourceImage::imageHasRelativeHeight() const
{
    return false;
}

void StyleLocalSVGResourceImage::computeIntrinsicDimensionsForRenderer(const RenderElement* client, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    // In case of an SVG resource, we should return the container size.
    FloatSize size = floorSizeToDevicePixels(m_containerSize, client ? client->document().deviceScaleFactor() : 1);
    intrinsicWidth = Length(size.width(), LengthType::Fixed);
    intrinsicHeight = Length(size.height(), LengthType::Fixed);
    intrinsicRatio = size;
}

bool StyleLocalSVGResourceImage::usesImageContainerSize() const
{
    return false; // FIXME: Needs checking.
}

//void StyleLocalSVGResourceImage::setContainerContextForRenderer(const RenderElement& client, const LayoutSize& containerSize, float containerZoom, const URL&)
//{
//    m_containerSize = containerSize;
//
//    if (containerSize.isEmpty())
//        return;
//
//    ASSERT(containerZoom);
//    auto image = m_cachedImage ? m_cachedImage->rawImage() : nullptr;;
//    if (!image) {
//        m_pendingContainerContextRequests.set(client, ContainerContext { containerSize, containerZoom, imageURL() });
//        return;
//    }
//
//    if (image->drawsSVGImage())
//        m_cachedImage->svgImageCache()->setContainerContextForRenderer(client, containerSize, containerZoom, imageURL());
//    else
//        image->setContainerSize(containerSize);
//}

void StyleLocalSVGResourceImage::addClient(StyleImageClient& client)
{
    m_clients.add(client);
}

void StyleLocalSVGResourceImage::removeClient(StyleImageClient& client)
{
    if (m_clients.remove(client)) {
        for (auto entry : m_clients)
            entry.key.styleImageClientRemoved(*this);
    }
}

bool StyleLocalSVGResourceImage::hasClient(StyleImageClient& client) const
{
    return m_clients.contains(client);
}

bool StyleLocalSVGResourceImage::hasImage() const
{
    return false;
}

Image* StyleLocalSVGResourceImage::image() const
{
    // FIXME: Perhaps wrong?
    return nullptr;
}

float StyleLocalSVGResourceImage::imageScaleFactor() const
{
    return m_scaleFactor;
}

bool StyleLocalSVGResourceImage::usesDataProtocol() const
{
    return false;
}

// MARK: - Internal

LegacyRenderSVGResourceContainer* StyleLocalSVGResourceImage::uncheckedRenderSVGResource(TreeScope& treeScope, const AtomString& fragment) const
{
    auto renderSVGResource = ReferencedSVGResources::referencedRenderResource(treeScope, fragment);
    m_isRenderSVGResource = renderSVGResource != nullptr;
    return renderSVGResource;
}

LegacyRenderSVGResourceContainer* StyleLocalSVGResourceImage::uncheckedRenderSVGResource(const StyleImageSizingContext& context) const
{
    auto fragmentIdentifier = SVGURIReference::fragmentIdentifierFromIRIString(imageURL().string(), context.document());
    return uncheckedRenderSVGResource(context.treeScopeForSVGReferences(), fragmentIdentifier);
}

LegacyRenderSVGResourceContainer* StyleLocalSVGResourceImage::legacyRenderSVGResource(const StyleImageSizingContext& context) const
{
    if (!m_isRenderSVGResource)
        return nullptr;
    return uncheckedRenderSVGResource(client);
}

RenderSVGResourceContainer* StyleLocalSVGResourceImage::renderSVGResource(const StyleImageSizingContext& context) const
{
    if (m_isRenderSVGResource)
        return nullptr;

    if (RefPtr referencedMaskElement = ReferencedSVGResources::referencedMaskElement(context.treeScopeForSVGReferences(), *this)) {
        if (auto* referencedMaskerRenderer = dynamicDowncast<RenderSVGResourceMasker>(referencedMaskElement->renderer()))
            return referencedMaskerRenderer;
    }
    return nullptr;
}

bool StyleLocalSVGResourceImage::isRenderSVGResource(const RenderElement* client) const
{
    return renderSVGResource(client) || legacyRenderSVGResource(client);
}


} // namespace WebCore
