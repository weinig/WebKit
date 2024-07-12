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

#pragma once

#include "CachedImageClient.h"
#include "CachedResourceHandle.h"
#include "StyleImage.h"
#include <wtf/WeakHashCountedSet.h>

namespace WebCore {

class CSSValue;
class CSSImageValue;
class CachedImage;
class Document;
class LegacyRenderSVGResourceContainer;
class RenderSVGResourceContainer;
class TreeScope;

class StyleCachedImage final : public StyleImage, private CachedImageClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static Ref<StyleCachedImage> create(Ref<CSSImageValue>, float scaleFactor = 1);
    static Ref<StyleCachedImage> create(CachedResourceHandle<CachedImage>);
    static Ref<StyleCachedImage> copyOverridingScaleFactor(StyleCachedImage&, float scaleFactor);
    virtual ~StyleCachedImage();

    bool operator==(const StyleImage&) const final;
    bool equals(const StyleCachedImage&) const;

    URL imageURL() const;

    CachedImage* cachedImage() const final;
    bool hasImage() const final;

    RefPtr<Image> imageForRenderer(const RenderElement*, const FloatSize& = { }, bool isForFirstLine = false) const final;
    LayoutSize imageSizeForRenderer(const RenderElement*, float multiplier, StyleImageSizeType) const final;

private:
    StyleCachedImage(Ref<CSSImageValue>&&, float);
    StyleCachedImage(CachedResourceHandle<CachedImage>);

    // StyleImage
    WrappedImagePtr data() const final { return m_cachedImage.get(); }
    Ref<CSSValue> computedStyleValue(const RenderStyle&) const final;

    bool canRenderForRenderer(const RenderElement*, float multiplier) const final;
    void setContainerContextForRenderer(const RenderElement&, const FloatSize&, float, const URL&) final;
    bool knownToBeOpaqueForRenderer(const RenderElement&) const final;
    void computeIntrinsicDimensionsForRenderer(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) final;

    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;
    bool isLoadedForRenderer(const RenderElement*) const final;
    bool errorOccurred() const final;

    bool imageHasRelativeWidth() const final;
    bool imageHasRelativeHeight() const final;
    bool usesImageContainerSize() const final;
    void addClient(StyleImageClient&) final;
    void removeClient(StyleImageClient&) final;
    bool hasClient(StyleImageClient&) const final;
    float imageScaleFactor() const final;
    bool usesDataProtocol() const final;
    bool isClientWaitingForAsyncDecoding(const StyleImageClient&) const final;
    void addClientWaitingForAsyncDecoding(StyleImageClient&) final;
    void removeAllClientsWaitingForAsyncDecoding() final;
    URL reresolvedURL(const Document&) const;

    // CachedImageClient
    void notifyFinished(CachedResource&, const NetworkLoadMetrics&, LoadWillContinueInAnotherProcess) final;
    bool canDestroyDecodedData(CachedImage&) const final;
    bool allowsAnimation(CachedImage&) const final;
    void imageCreated(CachedImage&, const Image&) final;
    void imageChanged(CachedImage&, const IntRect*) final;
    void scheduleRenderingUpdateForImage(CachedImage&) final;
    VisibleInViewportState imageFrameAvailable(CachedImage&, ImageAnimatingState, const IntRect* changeRect, DecodingStatus) final;
    VisibleInViewportState imageVisibleInViewport(CachedImage&, const Document&) const final;

    LegacyRenderSVGResourceContainer* uncheckedRenderSVGResource(TreeScope&, const AtomString& fragment) const;
    LegacyRenderSVGResourceContainer* uncheckedRenderSVGResource(const StyleImageClient*) const;
    LegacyRenderSVGResourceContainer* legacyRenderSVGResource(const StyleImageClient*) const;
    RenderSVGResourceContainer* renderSVGResource(const StyleImageClient*) const;
    bool isRenderSVGResource(const StyleImageClient*) const;

    Image* imageForRenderer(const RenderElement*) const;
    LayoutSize imageSizeForRenderer(const RenderElement*, StyleImageSizeType) const;
    LayoutSize unclampedImageSizeForRenderer(const RenderElement*, float multiplier, StyleImageSizeType = StyleImageSizeType::Used) const;

    struct ContainerContext {
        LayoutSize containerSize;
        float containerZoom;
        URL imageURL;
    };
    Ref<CSSImageValue> m_cssValue;
    bool m_isPending { true };
    mutable float m_scaleFactor { 1 };
    mutable CachedResourceHandle<CachedImage> m_cachedImage;
    mutable std::optional<bool> m_isRenderSVGResource;

    FloatSize m_containerSize;
    SingleThreadWeakHashMap<StyleImageClient, ContainerContext> m_pendingContainerContextRequests;

    SingleThreadWeakHashCountedSet<StyleImageClient> m_clients;
    SingleThreadWeakHashSet<StyleImageClient> m_clientsWaitingForAsyncDecoding;
    bool m_forceAllClientsWaitingForAsyncDecoding { false };

    std::unique_ptr<SVGImageCache> m_svgImageCache;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleCachedImage, isCachedImage)
