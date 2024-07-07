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

#include "CSSValue.h"
#include "FloatSize.h"
#include "Image.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/TypeCasts.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class CSSValue;
class CachedImage;
class CachedResourceLoader;
class Document;
class IntRect;
class RenderElement;
class RenderObject;
class RenderStyle;
class StyleImage;
struct ResourceLoaderOptions;

typedef const void* WrappedImagePtr;

class StyleImageClient { // FIXME: This really needs to be a WeakPtr } : public CanMakeWeakPtr<StyleImageClient> {
public:
    virtual ~StyleImageClient() = default;

    virtual void styleImageChanged(StyleImage&, const IntRect*) = 0;
};

class StyleImage : public RefCounted<StyleImage>, public CanMakeWeakPtr<StyleImage> {
public:
    virtual ~StyleImage() = default;

    virtual bool operator==(const StyleImage& other) const = 0;

    // Clients.
    virtual void addStyleImageClient(StyleImageClient&) = 0;
    virtual void removeStyleImageClient(StyleImageClient&) = 0;
    virtual bool hasStyleImageClient(StyleImageClient&) const = 0;

    // Computed Style representation.
    virtual Ref<CSSValue> computedStyleValue(const RenderStyle&) const = 0;

    // Opaque representation.
    virtual WrappedImagePtr data() const = 0;

    // Loading.
    virtual bool isPending() const = 0;
    virtual void load(CachedResourceLoader&, const ResourceLoaderOptions&) = 0;
    virtual bool isLoaded(const RenderElement*) const { return true; }
    virtual bool errorOccurred() const { return false; }
    virtual bool usesDataProtocol() const { return false; }
    virtual bool hasImage() const { return false; }
    virtual URL reresolvedURL(const Document&) const { return { }; }
    virtual bool isOriginClean(Document&) const { return true; } // FIXME: Implement.

    // Size / scale.
    virtual FloatSize imageSize(const RenderElement*, float multiplier) const = 0;
    virtual bool usesImageContainerSize() const = 0;
    virtual void computeIntrinsicDimensions(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) = 0;
    virtual bool imageHasRelativeWidth() const = 0;
    virtual bool imageHasRelativeHeight() const = 0;
    virtual float imageScaleFactor() const { return 1; }
    virtual bool imageHasNaturalDimensions() const { return true; }

    // Image.
    virtual RefPtr<Image> image(const RenderElement*, const FloatSize&, bool isForFirstLine = false) const = 0;
    virtual StyleImage* selectedImage() { return this; }
    virtual const StyleImage* selectedImage() const { return this; }

    // CachedImage forwarding
    // virtual CachedImage* cachedImage() const { return nullptr; }
    // bool hasCachedImage() const { return m_type == Type::CachedImage || selectedImage()->isCachedImage(); }
    virtual RefPtr<Image> image() const { return nullptr; }
    virtual bool isClientWaitingForAsyncDecoding(RenderElement&) const { return false; }
    virtual void addClientWaitingForAsyncDecoding(RenderElement&) { }
    virtual void removeAllClientsWaitingForAsyncDecoding() { }
    virtual bool hasValidImage() const { return true; }
        // True if generated, True if cachedImage() && cachedImage()->hasImage()
        //
        //    if (m_image->hasCachedImage()) {
        //        auto* cachedImage = m_image->cachedImage();
        //        return cachedImage && cachedImage->hasImage();
        //    }
        //    return m_image->isGeneratedImage();

    virtual bool canDirectlyCompositeBackgroundBackgroundImage() const { return false; }
        // False if generated, True if selected image `isBitmapImage`.
        //
        //    if (!styleImage->hasCachedImage())
        //        return false;
        //
        //    auto* image = styleImage->cachedImage()->image();
        //    if (!image->isBitmapImage())
        //        return false;


    // Rendering.
    virtual bool canRender(const RenderElement*, float /*multiplier*/) const { return true; }
    virtual void setContainerContextForRenderer(const RenderElement&, const FloatSize&, float) = 0;
    virtual bool knownToBeOpaque(const RenderElement&) const = 0;

    // Derived type.
    ALWAYS_INLINE bool isCachedImage() const { return m_type == Type::CachedImage; }
    ALWAYS_INLINE bool isCursorImage() const { return m_type == Type::CursorImage; }
    ALWAYS_INLINE bool isImageSet() const { return m_type == Type::ImageSet; }
    ALWAYS_INLINE bool isGeneratedImage() const { return isFilterImage() || isCanvasImage() || isCrossfadeImage() || isGradientImage() || isNamedImage() || isPaintImage() || isInvalidImage(); }
    ALWAYS_INLINE bool isFilterImage() const { return m_type == Type::FilterImage; }
    ALWAYS_INLINE bool isCanvasImage() const { return m_type == Type::CanvasImage; }
    ALWAYS_INLINE bool isCrossfadeImage() const { return m_type == Type::CrossfadeImage; }
    ALWAYS_INLINE bool isGradientImage() const { return m_type == Type::GradientImage; }
    ALWAYS_INLINE bool isNamedImage() const { return m_type == Type::NamedImage; }
    ALWAYS_INLINE bool isPaintImage() const { return m_type == Type::PaintImage; }
    ALWAYS_INLINE bool isInvalidImage() const { return m_type == Type::InvalidImage; }

protected:
    enum class Type : uint8_t {
        CachedImage,
        CursorImage,
        ImageSet,
        FilterImage,
        CanvasImage,
        CrossfadeImage,
        GradientImage,
        NamedImage,
        InvalidImage,
        PaintImage,
    };

    StyleImage(Type type)
        : m_type { type }
    {
    }

    Type m_type;
};

} // namespace WebCore

#define SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(ToClassName, predicate) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ToClassName) \
    static bool isType(const WebCore::StyleImage& image) { return image.predicate(); } \
SPECIALIZE_TYPE_TRAITS_END()
