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

#include "FloatSize.h"
#include "FloatSizeHash.h"
#include "StyleImage.h"
#include <wtf/HashCountedSet.h>
#include <wtf/HashMap.h>
#include <wtf/WeakHashCountedSet.h>

namespace WebCore {

class CSSValue;
class CachedResourceLoader;
class GeneratedImage;
class Image;
class RenderElement;

struct ResourceLoaderOptions;

class StyleGeneratedImage : public StyleImage {
public:
    //     const HashCountedSet<StyleImageClient*>& clients() const { return m_clients; } // const SingleThreadWeakHashCountedSet<StyleImageClient>& clients() const { return m_clients; }

protected:
    explicit StyleGeneratedImage(StyleImage::Type, bool fixedSize);
    virtual ~StyleGeneratedImage();

    // StyleImage overrides
    void addStyleImageClient(StyleImageClient&) final;
    void removeStyleImageClient(StyleImageClient&) final;
    bool hasStyleImageClient(StyleImageClient&) const final;
    WrappedImagePtr data() const final { return this; }
    FloatSize imageSize(const RenderElement*, float multiplier) const final;
    void computeIntrinsicDimensions(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) final;
    bool imageHasRelativeWidth() const final { return !m_fixedSize; }
    bool imageHasRelativeHeight() const final { return !m_fixedSize; }
    bool usesImageContainerSize() const final { return !m_fixedSize; }
    void setContainerContextForRenderer(const RenderElement&, const FloatSize& containerSize, float) override { m_containerSize = containerSize; }
    bool imageHasNaturalDimensions() const final { return !usesImageContainerSize(); }

    // Additional virtual methods for subclasses of `StyleGeneratedImage`.

    // Allow subclasses to react to clients being added/removed.
    virtual void didAddClient(StyleImageClient&) = 0;
    virtual void didRemoveClient(StyleImageClient&) = 0;

    // All generated images must be able to compute their fixed size.
    virtual FloatSize fixedSize(const RenderElement&) const = 0;

    // Caching
    class CachedGeneratedImage;
    GeneratedImage* cachedImageForSize(FloatSize);
    void saveCachedImageForSize(FloatSize, GeneratedImage&);
    void evictCachedGeneratedImage(FloatSize);

    void notifyClientsOfChange();

    // FIXME: Consider using a per-client (or perhaps per-RenderElement) cache of container sizes like CachedImage.
    //    struct ContainerContext {
    //        LayoutSize containerSize;
    //        float containerZoom;
    //        URL imageURL;
    //    };
    //
    //    using ContainerContextRequests = HashMap<SingleThreadWeakRef<const CachedImageClient>, ContainerContext>;
    //    ContainerContextRequests m_pendingContainerContextRequests;
    FloatSize m_containerSize;

    bool m_fixedSize;
    HashCountedSet<StyleImageClient*> m_clients; // SingleThreadWeakHashCountedSet<StyleImageClient> m_clients;
    HashMap<FloatSize, std::unique_ptr<CachedGeneratedImage>> m_images;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleGeneratedImage, isGeneratedImage)
