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

#include "CachedImage.h"
#include "FloatSize.h"
#include "Image.h"
#include "ImageOrientation.h"
#include "StyleImageObjectSizeNegotiation.h"
#include <optional>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class Element;
class Document;
class StyleImage;

enum class ImageAnimatingState : bool;
enum class VisibleInViewportState : uint8_t;

class WEBCORE_EXPORT StyleImageClient : public CanMakeSingleThreadWeakPtr<StyleImageClient> {
    WTF_MAKE_NONCOPYABLE(StyleImageClient);
public:
    explicit StyleImageClient() = default;
    virtual ~StyleImageClient() = default;

    // Called when a client has been fully removed from the client set.
    virtual void styleImageClientRemoved(StyleImage&) { }

    // Called when a style image changes.
    virtual void styleImageChanged(StyleImage&, const IntRect* = nullptr) = 0;

    // Called when when an underlying CachedResource load completes. May be called multiple
    // times if there are multiple underlying CachedResources (such as with StyleCrossfadeImage).
    virtual void styleImageFinishedResourceLoad(StyleImage&, CachedResource&) = 0;

    // Called when when ALL underlying CachedResource loads have completed.
    virtual void styleImageFinishedLoad(StyleImage&) = 0;

    // Called to request a rendering updated.
    virtual void styleImageNeedsScheduledRenderingUpdate(StyleImage&) = 0;

    // Called to determine if it is profitable to destroy decoded data.
    virtual bool styleImageCanDestroyDecodedData(StyleImage&) const = 0;

    // Called to determine if animations are allowed.
    virtual bool styleImageAnimationAllowed(StyleImage&) const = 0;

    // Called when an underlying CachedImage has a new frame available.
    virtual VisibleInViewportState styleImageFrameAvailable(StyleImage&, ImageAnimatingState, const IntRect*) = 0;

    // Called to determine if the image is visible in the viewport.
    virtual VisibleInViewportState styleImageVisibleInViewport(StyleImage&, const Document&) const = 0;

    // Called to determine the set of Elements referencing this StyleImage.
    virtual HashSet<Element*> styleImageReferencingElements(StyleImage&) const = 0;

    // Called to determine what orientation to draw the image in.
    virtual ImageOrientation styleImageOrientation(StyleImage&) const { return ImageOrientation::Orientation::FromImage; }

    // Called to determine an override size from the client.
    virtual std::optional<LayoutSize> styleImageOverrideImageSize(StyleImage&) const { return std::nullopt; }
};

} // namespace WebCore
