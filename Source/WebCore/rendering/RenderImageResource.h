/*
 * Copyright (C) 1999 Lars Knoll <knoll@kde.org>
 * Copyright (C) 1999 Antti Koivisto <koivisto@kde.org>
 * Copyright (C) 2006 Allan Sandfeld Jensen <kde@carewolf.com>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
#include "CachedResourceHandle.h"
#include "StyleImage.h"
#include <wtf/CheckedPtr.h>
#include <wtf/FastMalloc.h>
#include <wtf/IsoMalloc.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class CachedImage;
class RenderElement;

class RenderImageResource final : public CanMakeCheckedPtr<RenderImageResource> {
    WTF_MAKE_NONCOPYABLE(RenderImageResource);
    WTF_MAKE_ISO_ALLOCATED(RenderImageResource);
    WTF_OVERRIDE_DELETE_FOR_CHECKED_PTR(RenderImageResource);
public:
    explicit RenderImageResource();
    explicit RenderImageResource(RefPtr<StyleImage>);
    ~RenderImageResource();

    void initialize(RenderElement&);
    void shutdown();

    StyleImage* styleImage() const { return m_styleImage.get(); }
    WrappedImagePtr imagePtr() const { return m_styleImage ? m_styleImage->data() : nullptr; }

    void setCachedImage(CachedResourceHandle<CachedImage>&&);
    CachedImage* cachedImage() const { return m_styleImage ? m_styleImage->cachedImage() : nullptr; }

    void resetAnimation();

    RefPtr<Image> image(const StyleImageSizingContext&) const;
    bool errorOccurred() const { return m_styleImage && m_styleImage->errorOccurred(); }

    //    void setContainerContext(const IntSize&, const URL&);
    //    bool imageHasRelativeWidth() const { return m_styleImage && m_styleImage->imageHasRelativeWidth(); }
    //    bool imageHasRelativeHeight() const { return m_styleImage && m_styleImage->imageHasRelativeHeight(); }

    //    LayoutSize imageSize(float multiplier) const { return imageSize(multiplier, StyleImageSizeType::Used); }
    //    LayoutSize intrinsicSize(float multiplier) const { return imageSize(multiplier, StyleImageSizeType::Intrinsic); }

    bool isClientWaitingForAsyncDecoding(const StyleImageClient& client) const { return m_styleImage && m_styleImage->isClientWaitingForAsyncDecoding(client); }
    void addClientWaitingForAsyncDecoding(StyleImageClient& client) { if (!m_styleImage) return; m_styleImage->addClientWaitingForAsyncDecoding(client); }
    void removeAllClientsWaitingForAsyncDecoding() { if (!m_styleImage) return; m_styleImage->removeAllClientsWaitingForAsyncDecoding();}
    
private:
    LayoutSize imageSize(float multiplier, StyleImageSizeType) const;

    SingleThreadWeakPtr<RenderElement> m_renderer;
    RefPtr<StyleImage> m_styleImage;
};

} // namespace WebCore
