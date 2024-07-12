/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2021 Apple Inc. All rights reserved.
 * Copyright (C) 2020 Noam Rosenthal (noam@webkit.org)
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

#include "StyleImage.h"
#include "StyleInvalidImage.h"

namespace WebCore {

class Document;

struct ImageWithScale {
    Ref<StyleImage> image { StyleInvalidImage::create() };
    float scaleFactor { 1 };
    String mimeType { String() };
};

inline bool operator==(const ImageWithScale& a, const ImageWithScale& b)
{
    return a.image == b.image && a.scaleFactor == b.scaleFactor;
}

class StyleMultiImage : public StyleImage {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~StyleMultiImage();

protected:
    StyleMultiImage(Type);

    bool equals(const StyleMultiImage& other) const;

    virtual ImageWithScale selectBestFitImage(const Document&) = 0;
    CachedImage* cachedImage() const final;

private:
    void setSelectedImage(Ref<StyleImage>&&);

    WrappedImagePtr data() const final;

    const StyleImage* selectedImage() const final;
    StyleImage* selectedImage() final;
    float imageScaleFactor() const final;

    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;
    bool isLoadedForRenderer(const RenderElement*) const final;
    bool errorOccurred() const final;
    bool imageHasRelativeWidth() const final;
    bool imageHasRelativeHeight() const final;
    void computeIntrinsicDimensionsForRenderer(const RenderElement*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) final;
    bool usesImageContainerSize() const final;
    bool hasImage() const final;

    bool canRenderForRenderer(const RenderElement*, float multiplier) const final;
    LayoutSize imageSizeForRenderer(const RenderElement*, float multiplier, StyleImageSizeType) const final;
    RefPtr<Image> imageForRenderer(const RenderElement*, const FloatSize&, bool isForFirstLine) const final;
    void setContainerContextForRenderer(const RenderElement&, const FloatSize&, float, const URL&);
    bool knownToBeOpaqueForRenderer(const RenderElement&) const final;

    void addClient(StyleImageClient&) final;
    void removeClient(StyleImageClient&) final;
    bool hasClient(StyleImageClient&) const final;

    bool isClientWaitingForAsyncDecoding(const StyleImageClient&) final;
    void addClientWaitingForAsyncDecoding(StyleImageClient&) final;
    void removeAllClientsWaitingForAsyncDecoding() final;

    struct Pending {
        struct ContainerContext {
            LayoutSize containerSize;
            float containerZoom;
            URL imageURL;
        };
        SingleThreadWeakHashMap<StyleImageClient, ContainerContext> containerContextRequests;
        SingleThreadWeakHashCountedSet<StyleImageClient> clients;
        SingleThreadWeakHashSet<StyleImageClient> clientsWaitingForAsyncDecoding;
        bool forceAllClientsWaitingForAsyncDecoding { false };
    };

    std::variant<Pending, Ref<StyleImage>> m_state;
    bool m_loadCalled { false };
};

} // namespace WebCore
