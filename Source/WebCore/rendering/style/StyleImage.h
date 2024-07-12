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
#include "CachedImage.h"
#include "FloatSize.h"
#include "Image.h"
#include "ImageOrientation.h"
#include <optional>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/TypeCasts.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class CachedResourceLoader;
class CSSValue;
class Document;
class RenderStyle;
class StyleImage;
struct ResourceLoaderOptions;

enum class ImageAnimatingState : bool;

typedef const void* WrappedImagePtr;


class WEBCORE_EXPORT StyleImageClient : public CanMakeSingleThreadWeakPtr<StyleImageClient> {
    WTF_MAKE_NONCOPYABLE(StyleImageClient);
public:
    explicit StyleImageClient() = default;
    virtual ~StyleImageClient() = default;

    // Called for all StyleImages when a call to StyleImage::removeClient() fully removes a client.
    virtual void styleImageClientRemoved(StyleImage&) { }

    // Called for all StyleImages when a style image changes.
    virtual void styleImageChanged(StyleImage&, const IntRect* = nullptr) = 0;

    // Called for all StyleCachedImage & StyleMultiImages backed by a StyleCachedImage when the underlying
    // CachedResource load completes.
    virtual void styleImageLoadFinished(StyleImage&, CachedResource&) = 0;

    // Called for all StyleCachedImage & StyleMultiImages backed by a StyleCachedImage when the underlying
    // CachedResource requests a rendering updated.
    virtual void styleImageNeedsScheduledRenderingUpdate(StyleImage&) = 0;

    // Called for all StyleCachedImage & StyleMultiImages backed by a StyleCachedImage when the underlying
    // CachedResource needs to know if can destroy decoded data.
    virtual bool styleImageCanDestroyDecodedData(StyleImage&) const = 0;

    // Called for all StyleCachedImage & StyleMultiImages backed by a StyleCachedImage when the underlying
    // CachedResource needs to know if can allow image animations.
    virtual bool styleImageAnimationAllowed(StyleImage&) const = 0;

    // Called for all StyleCachedImage & StyleMultiImages backed by a StyleCachedImage when the underlying
    // CachedResource has a new frame available.
    virtual VisibleInViewportState styleImageFrameAvailable(StyleImage&, ImageAnimatingState, const IntRect*) = 0;

    // Called for all StyleCachedImage & StyleMultiImages backed by a StyleCachedImage when the underlying
    // CachedResource needs to know if the image is visible in the viewport.
    virtual VisibleInViewportState styleImageVisibleInViewport(StyleImage&, const Document&) const = 0;

    // Called for all StyleImages to determine what orientation to draw the image in.
    virtual ImageOrientation styleImageOrientation(StyleImage&) const { return ImageOrientation::Orientation::FromImage; }

    // Called for all StyleImages to determine the override size from the client.
    virtual std::optional<LayoutSize> styleImageOverrideImageSize(StyleImage&) { return std::nullopt; }
};

enum class StyleImageSizeType : bool {
    Used,
    Intrinsic
};

class StyleImage : public RefCounted<StyleImage>, public CanMakeWeakPtr<StyleImage> {
public:
    virtual ~StyleImage() = default;

    virtual bool operator==(const StyleImage& other) const = 0;

    // Clients
    virtual void addClient(StyleImageClient&) = 0;
    virtual void removeClient(StyleImageClient&) = 0;
    virtual bool hasClient(StyleImageClient&) const = 0;

    // Computed Style representation.
    virtual Ref<CSSValue> computedStyleValue(const RenderStyle&) const = 0;

    // Opaque representation.
    virtual WrappedImagePtr data() const = 0;

    // Underlying representation
    //
    // `cachedImage()` and `hasImage()` are only valid for non-composite images (e.g. a StyleCrossfadeImage
    // will always return nullptr / false, even if `to` or `from` are StyleCachedImages).
    virtual CachedImage* cachedImage() const { return nullptr; }
    virtual bool hasImage() const { return false; }

    // Loading
    virtual bool isPending() const = 0;
    virtual void load(CachedResourceLoader&, const ResourceLoaderOptions&) = 0;
    virtual bool isLoadedForRenderer(const RenderElement*) const { return true; }
    virtual bool errorOccurred() const { return false; }
    virtual bool usesDataProtocol() const { return false; }
    virtual URL reresolvedURL(const Document&) const { return { }; }

    // MultiImage
    virtual StyleImage* selectedImage() { return this; }
    virtual const StyleImage* selectedImage() const { return this; }

    // Size
    virtual bool usesImageContainerSize() const = 0;
    virtual bool imageHasRelativeWidth() const = 0;
    virtual bool imageHasRelativeHeight() const = 0;
    virtual bool imageHasNaturalDimensions() const { return true; }

    // Scale
    virtual float imageScaleFactor() const { return 1; }

    // Rendering
    virtual LayoutSize imageSizeForRenderer(const RenderElement*, float multiplier, StyleImageSizeType = StyleImageSizeType::Used) const = 0;
    virtual RefPtr<Image> imageForRenderer(const RenderElement*, const FloatSize& = { }, bool isForFirstLine = false) const = 0;
    virtual void computeIntrinsicDimensionsForRenderer(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) = 0;
    virtual bool canRenderForRenderer(const RenderElement*, float) const { return true; }
    virtual void setContainerContextForRenderer(const RenderElement&, const FloatSize&, float, const URL& = { }) = 0;
    virtual bool knownToBeOpaqueForRenderer(const RenderElement&) const = 0;

    // Animation
    virtual void stopAnimation() { }
    virtual void resetAnimation() { }

    // Support for optimizing `styleImageFrameAvailable` client callbacks.
    virtual bool isClientWaitingForAsyncDecoding(const StyleImageClient&) const { return false; }
    virtual void addClientWaitingForAsyncDecoding(StyleImageClient&) { }
    virtual void removeAllClientsWaitingForAsyncDecoding() { }

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

    bool hasCachedImage() const { return m_type == Type::CachedImage || selectedImage()->isCachedImage(); }

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
