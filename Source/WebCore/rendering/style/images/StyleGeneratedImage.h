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
#include <wtf/HashMap.h>
#include <wtf/WeakHashCountedSet.h>

namespace WebCore {

class CSSValue;
class CachedImage;
class CachedResourceLoader;
class GeneratedImage;
class Image;
class RenderElement;

struct ResourceLoaderOptions;

class StyleGeneratedImage : public StyleImage {
public:
    const SingleThreadWeakHashCountedSet<StyleImageClient>& clients() const { return m_clients; }

protected:
    explicit StyleGeneratedImage(StyleImage::Type, bool fixedSize);
    virtual ~StyleGeneratedImage();

    WrappedImagePtr data() const final { return this; }

    LayoutSize imageSizeForContext(const StyleImageContext&, float multiplier, StyleImageSizeType = StyleImageSizeType::Used) const final;
    // void computeIntrinsicDimensionsForRenderer(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) final;
    // void setContainerContextForRenderer(const RenderElement&, const LayoutSize& containerSize, float, const URL&) final { m_containerSize = containerSize; }

    bool imageHasRelativeWidth() const final { return !m_fixedSize; }
    bool imageHasRelativeHeight() const final { return !m_fixedSize; }
    bool usesImageContainerSize() const final { return !m_fixedSize; }
    bool imageHasNaturalDimensions() const final { return !usesImageContainerSize(); }
    
    void addClient(StyleImageClient&) final;
    void removeClient(StyleImageClient&) final;
    bool hasClient(StyleImageClient&) const final;

    // Allow subclasses to react to clients being added/removed.
    virtual void didAddClient(StyleImageClient&) { };
    virtual void didRemoveClient(StyleImageClient&) { };

    // All generated images must be able to compute their fixed size.
    virtual LayoutSize fixedSizeForContext(const StyleImageContext&) const = 0;

    class CachedGeneratedImage;
    GeneratedImage* cachedImageForSize(FloatSize);
    void saveCachedImageForSize(FloatSize, GeneratedImage&);
    void evictCachedGeneratedImage(FloatSize);

    struct ContainerContext {
        LayoutSize containerSize;
        float containerZoom;
        URL imageURL;
    };
    SingleThreadWeakHashMap<RenderObject, ContainerContext> m_containerContext;
    LayoutSize m_containerSize;
    bool m_fixedSize;
    SingleThreadWeakHashCountedSet<StyleImageClient> m_clients;
    HashMap<FloatSize, std::unique_ptr<CachedGeneratedImage>> m_images;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleGeneratedImage, isGeneratedImage)
