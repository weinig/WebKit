/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#pragma once

#include "CachedResourceClient.h"
#include "ImageTypes.h"

namespace WebCore {
class CachedImageClient;
}

namespace WTF {
template<typename T> struct IsDeprecatedWeakRefSmartPointerException;
template<> struct IsDeprecatedWeakRefSmartPointerException<WebCore::CachedImageClient> : std::true_type { };
}

namespace WebCore {

class CachedImage;
class Document;
class Image;
class IntRect;

enum class VisibleInViewportState { Unknown, Yes, No };

class CachedImageClient : public CachedResourceClient {
public:
    virtual ~CachedImageClient() = default;
    static CachedResourceClientType expectedType() { return ImageType; }
    CachedResourceClientType resourceClientType() const override { return expectedType(); }

    // Called when the WebCore::Image object has been created.
    virtual void imageCreated(CachedImage&, const Image&) { }

    // Called whenever a frame of an image changes because we got more data from the network.
    // If not null, the IntRect is the changed rect of the image.
    virtual void imageChanged(CachedImage&, const IntRect* = nullptr) { }

    virtual bool canDestroyDecodedData(CachedImage&) const { return true; }

    // Called when a new decoded frame for a large image is available or when an animated image is ready to advance to the next frame.
    virtual VisibleInViewportState imageFrameAvailable(CachedImage& image, ImageAnimatingState, const IntRect* changeRect, DecodingStatus) { imageChanged(image, changeRect); return VisibleInViewportState::No; }
    virtual VisibleInViewportState imageVisibleInViewport(CachedImage&, const Document&) const { return VisibleInViewportState::No; }

    virtual void didRemoveCachedImageClient(CachedImage&) { }

    virtual void scheduleRenderingUpdateForImage(CachedImage&) { }

    virtual bool allowsAnimation(CachedImage&) const { return true; }
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CACHED_RESOURCE_CLIENT(CachedImageClient, ImageType);
