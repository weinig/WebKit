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

#pragma once

#include "CSSParserContext.h"
#include "CachedResourceHandle.h"
#include "StyleImage.h"

namespace WebCore {

class CachedImage;
class Document;

class StyleCachedImage final : public StyleImage {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static Ref<StyleCachedImage> create(ResolvedURL, LoadedFromOpaqueSource, AtomString initiatorName = { }, float scaleFactor = 1);
    static Ref<StyleCachedImage> create(URL, LoadedFromOpaqueSource, AtomString initiatorName = { }, float scaleFactor = 1);
    static Ref<StyleCachedImage> create(StyleCachedImage&, float scaleFactor);
    virtual ~StyleCachedImage();

    bool operator==(const StyleImage& other) const final;

    CachedImage* cachedImage() const final;

    WrappedImagePtr data() const final { return m_cachedImage.get(); }

    Ref<CSSValue> computedStyleValue(const RenderStyle&) const final;

    bool canRender(const RenderElement*, float multiplier) const final;
    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;
    bool isLoaded() const final;
    bool errorOccurred() const final;
    FloatSize imageSize(const RenderElement*, float multiplier) const final;
    bool imageHasRelativeWidth() const final;
    bool imageHasRelativeHeight() const final;
    void computeIntrinsicDimensions(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) final;
    bool usesImageContainerSize() const final;
    void setContainerContextForRenderer(const RenderElement&, const FloatSize&, float) final;
    void addClient(RenderElement&) final;
    void removeClient(RenderElement&) final;
    bool hasClient(RenderElement&) const final;
    bool hasImage() const final;
    RefPtr<Image> image(const RenderElement*, const FloatSize&) const final;
    float imageScaleFactor() const final;
    bool knownToBeOpaque(const RenderElement&) const final;
    bool usesDataProtocol() const final;

    URL reresolvedURL(const Document&) const;

    // Take care when using this, and read https://drafts.csswg.org/css-values/#relative-urls
    URL imageURL() const { return m_url.resolvedURL; }

private:
    StyleCachedImage(ResolvedURL&&, LoadedFromOpaqueSource, AtomString&&, float);
    StyleCachedImage(StyleCachedImage&, float);

    ResolvedURL m_url;
    LoadedFromOpaqueSource m_loadedFromOpaqueSource;
    AtomString m_initiatorName;
    bool m_isPending;
    mutable float m_scaleFactor { 1 };
    mutable CachedResourceHandle<CachedImage> m_cachedImage;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleCachedImage, isCachedImage)
