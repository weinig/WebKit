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
#include <wtf/HashCountedSet.h>
#include <wtf/WeakHashCountedSet.h>

namespace WebCore {

class CSSValue;
class CSSImageValue;
class CachedImage;
class Document;
class LegacyRenderSVGResourceContainer;
class RenderElement;
class RenderSVGResourceContainer;
class TreeScope;

class StyleCachedImage final : public StyleImage, public CachedImageClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static Ref<StyleCachedImage> create(Ref<CSSImageValue>, float scaleFactor = 1);
    static Ref<StyleCachedImage> copyOverridingScaleFactor(StyleCachedImage&, float scaleFactor);
    virtual ~StyleCachedImage();

    bool operator==(const StyleImage&) const final;
    bool equals(const StyleCachedImage&) const;

    URL imageURL() const;
    float imageScaleFactor() const final;
    CachedImage* cachedImage() const;

private:
    StyleCachedImage(Ref<CSSImageValue>&&, float);

    // StyleImage overrides
    void addStyleImageClient(StyleImageClient&) final;
    void removeStyleImageClient(StyleImageClient&) final;
    bool hasStyleImageClient(StyleImageClient&) const final;
    WrappedImagePtr data() const final { return m_cachedImage.get(); }
    Ref<CSSValue> computedStyleValue(const RenderStyle&) const final;
    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;
    bool isLoaded(const RenderElement*) const final;
    bool errorOccurred() const final;
    bool usesDataProtocol() const final;
    bool hasImage() const final;
    URL reresolvedURL(const Document&) const final;
    FloatSize imageSize(const RenderElement*, float multiplier) const final;
    bool usesImageContainerSize() const final;
    void computeIntrinsicDimensions(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) final;
    bool imageHasRelativeWidth() const final;
    bool imageHasRelativeHeight() const final;
    RefPtr<Image> image(const RenderElement*, const FloatSize&, bool isForFirstLine) const final;
    RefPtr<Image> image() const final;
    bool canRender(const RenderElement*, float multiplier) const final;
    void setContainerContextForRenderer(const RenderElement&, const FloatSize&, float) final;
    bool knownToBeOpaque(const RenderElement&) const final;

    bool isClientWaitingForAsyncDecoding(RenderElement&) const final;
    void addClientWaitingForAsyncDecoding(RenderElement&) final;
    void removeAllClientsWaitingForAsyncDecoding() final;

    // CachedImageClient overrides
    void imageChanged(CachedImage*, const IntRect* = nullptr) final;

    LegacyRenderSVGResourceContainer* uncheckedRenderSVGResource(TreeScope&, const AtomString& fragment) const;
    LegacyRenderSVGResourceContainer* uncheckedRenderSVGResource(const RenderElement*) const;
    LegacyRenderSVGResourceContainer* legacyRenderSVGResource(const RenderElement*) const;
    RenderSVGResourceContainer* renderSVGResource(const RenderElement*) const;
    bool isRenderSVGResource(const RenderElement*) const;

    Ref<CSSImageValue> m_cssValue;
    bool m_isPending { true };
    mutable float m_scaleFactor { 1 };
    mutable CachedResourceHandle<CachedImage> m_cachedImage;
    mutable std::optional<bool> m_isRenderSVGResource;
    FloatSize m_containerSize;

    HashCountedSet<StyleImageClient*> m_clients; // FIXME: This should be a SingleThreadWeakHashCountedSet<StyleImageClient> m_clients;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleCachedImage, isCachedImage)
