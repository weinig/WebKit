/*
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

#include "config.h"
#include "StyleMultiImage.h"

#include "CSSCanvasValue.h"
#include "CSSCrossfadeValue.h"
#include "CSSFilterImageValue.h"
#include "CSSGradientValue.h"
#include "CSSImageSetValue.h"
#include "CSSImageValue.h"
#include "CSSNamedImageValue.h"
#include "CSSPaintImageValue.h"
#include "CSSVariableData.h"
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "RenderElement.h"
#include "RenderView.h"
#include "StyleCachedImage.h"
#include "StyleCanvasImage.h"
#include "StyleCrossfadeImage.h"
#include "StyleFilterImage.h"
#include "StyleGradientImage.h"
#include "StyleNamedImage.h"
#include "StylePaintImage.h"

namespace WebCore {

StyleMultiImage::StyleMultiImage(Type type)
    : StyleImage { type }
{
}

StyleMultiImage::~StyleMultiImage() = default;

bool StyleMultiImage::equals(const StyleMultiImage& other) const
{
    return (!m_loadCalled && !other.m_loadCalled && selectedImage() == other.selectedImage());
}

const StyleImage* StyleMultiImage::selectedImage() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) -> const StyleImage* {
            return selectedImage.ptr();
        },
        [](const Pending&) -> const StyleImage* {
            return nullptr;
        }
    );
}

StyleImage* StyleMultiImage::selectedImage()
{
    return WTF::switchOn(m_state,
        [](Ref<StyleImage>& selectedImage) -> StyleImage* {
            return selectedImage.ptr();
        },
        [](Pending&) -> StyleImage* {
            return nullptr;
        }
    );
}

void StyleMultiImage::setSelectedImageAndLoad(Ref<StyleImage>&& selection, CachedResourceLoader& loader, const ResourceLoaderOptions& options)
{
    return WTF::switchOn(m_state,
        [&](Ref<StyleImage>&) {
            ASSERT_NOT_REACHED();
        },
        [&](Pending& pending) {
            // Transfer clients to the selected image.
            for (auto entry : pending.clients) {
                for (unsigned i = 0; i < entry.value; ++i)
                    selection->addClient(entry.key);
            }

            // Transfer clientsWaitingForAsyncDecoding to the selected image.
            for (auto& client : pending.clientsWaitingForAsyncDecoding)
                selection->addClientWaitingForAsyncDecoding(client);

            // Transfer container size requests to the selected image.
            for (auto request : pending.containerContextRequests)
                selection->setContainerContextForRenderer(request.key, request.value.containerSize, request.value.containerZoom, request.value.imageURL);

            if (selection->isPending())
                selection->load(loader, options);

            m_state = WTFMove(selection);
        }
    );
}

void StyleMultiImage::load(CachedResourceLoader& loader, const ResourceLoaderOptions& options)
{
    ASSERT(!m_loadCalled);
    ASSERT(std::holds_alternative<Pending>(m_state));
    ASSERT(loader.document());

    m_loadCalled = true;

    auto bestFitImage = selectBestFitImage(*loader.document());

    ASSERT(is<StyleCachedImage>(bestFitImage.image) || is<StyleGeneratedImage>(bestFitImage.image));

    if (RefPtr styleCachedImage = dynamicDowncast<StyleCachedImage>(bestFitImage.image)) {
        if (styleCachedImage->imageScaleFactor() == bestFitImage.scaleFactor)
            setSelectedImageAndLoad(styleCachedImage.releaseNonNull(), loader, options);
        else
            setSelectedImageAndLoad(StyleCachedImage::copyOverridingScaleFactor(*styleCachedImage, bestFitImage.scaleFactor), loader, options);
    } else
        setSelectedImageAndLoad(*bestFitImage.image, loader, options);
}

CachedImage* StyleMultiImage::cachedImage() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) -> CachedImage* {
            return selectedImage->cachedImage();
        },
        [](const Pending&) -> CachedImage* {
            return nullptr;
        }
    );
}

WrappedImagePtr StyleMultiImage::data() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) -> WrappedImagePtr {
            return selectedImage->data();
        },
        [](const Pending&) -> WrappedImagePtr {
            return nullptr;
        }
    );
}

float StyleMultiImage::imageScaleFactor() const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) -> float {
            return selectedImage->imageScaleFactor();
        },
        [&](const Pending&) -> float {
            return 1;
        }
    );
}

bool StyleMultiImage::canRenderForRenderer(const RenderElement* client, float multiplier) const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) {
            return selectedImage->canRenderForRenderer(client, multiplier);
        },
        [](const Pending&) {
            return false;
        }
    );
}

bool StyleMultiImage::isPending() const
{
    return !m_loadCalled;
}

bool StyleMultiImage::isLoadedForRenderer(const RenderElement* client) const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) {
            return selectedImage->isLoadedForRenderer(client);
        },
        [](const Pending&) {
            return false;
        }
    );
}

bool StyleMultiImage::errorOccurred() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) {
            return selectedImage->errorOccurred();
        },
        [](const Pending&) {
            return false;
        }
    );
}

LayoutSize StyleMultiImage::imageSizeForRenderer(const RenderElement* client, float multiplier, StyleImageSizeType sizeType) const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) -> LayoutSize {
            return selectedImage->imageSizeForRenderer(client, multiplier, sizeType);
        },
        [](const Pending&) -> LayoutSize {
            return { };
        }
    );
}

bool StyleMultiImage::imageHasRelativeWidth() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) {
            return selectedImage->imageHasRelativeWidth();
        },
        [](const Pending&) {
            return false;
        }
    );
}

bool StyleMultiImage::imageHasRelativeHeight() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) {
            return selectedImage->imageHasRelativeHeight();
        },
        [](const Pending&) {
            return false;
        }
    );
}

bool StyleMultiImage::usesImageContainerSize() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) {
            return selectedImage->usesImageContainerSize();
        },
        [](const Pending&) {
            return false;
        }
    );
}

bool StyleMultiImage::hasImage() const
{
    return WTF::switchOn(m_state,
        [](const Ref<StyleImage>& selectedImage) {
            return selectedImage->hasImage();
        },
        [](const Pending&) {
            return false;
        }
    );
}

void StyleMultiImage::computeIntrinsicDimensionsForRenderer(const RenderElement* client, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    return WTF::switchOn(m_state,
        [&](Ref<StyleImage>& selectedImage) {
            selectedImage->computeIntrinsicDimensionsForRenderer(client, intrinsicWidth, intrinsicHeight, intrinsicRatio);
        },
        [](Pending&) { }
    );
}

RefPtr<Image> StyleMultiImage::imageForRenderer(const RenderElement* client, const FloatSize& size, bool isForFirstLine) const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) -> RefPtr<Image> {
            return selectedImage->imageForRenderer(client, size, isForFirstLine);
        },
        [&](const Pending&) -> RefPtr<Image> {
            return nullptr;
        }
    );
}

bool StyleMultiImage::knownToBeOpaque() const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) {
            return selectedImage->knownToBeOpaque();
        },
        [&](const Pending&) {
            return false;
        }
    );
}


void StyleMultiImage::setContainerContextForRenderer(const RenderElement& client, const LayoutSize& containerSize, float containerZoom, const URL& url)
{
    if (containerSize.isEmpty())
        return;

    return WTF::switchOn(m_state,
        [&](Ref<StyleImage>& selectedImage) {
            selectedImage->setContainerContextForRenderer(client, containerSize, containerZoom, url);
        },
        [&](Pending& pending) {
            pending.containerContextRequests.set(client, Pending::ContainerContext { containerSize, containerZoom, url });
        }
    );
}

bool StyleMultiImage::isClientWaitingForAsyncDecoding(const StyleImageClient& client) const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) {
            return selectedImage->isClientWaitingForAsyncDecoding(client);
        },
        [&](const Pending& pending) {
            return pending.forceAllClientsWaitingForAsyncDecoding
                || pending.clientsWaitingForAsyncDecoding.contains(const_cast<StyleImageClient&>(client));
        }
    );
}

void StyleMultiImage::addClientWaitingForAsyncDecoding(StyleImageClient& client)
{
    return WTF::switchOn(m_state,
        [&](Ref<StyleImage>& selectedImage) {
            selectedImage->addClientWaitingForAsyncDecoding(client);
        },
        [&](Pending& pending) {
            if (pending.forceAllClientsWaitingForAsyncDecoding || pending.clientsWaitingForAsyncDecoding.contains(client))
                return;

            if (!pending.clients.contains(client))
                pending.forceAllClientsWaitingForAsyncDecoding = true;
            else
                pending.clientsWaitingForAsyncDecoding.add(client);
        }
    );
}

void StyleMultiImage::removeAllClientsWaitingForAsyncDecoding()
{
    return WTF::switchOn(m_state,
        [](Ref<StyleImage>& selectedImage) {
            return selectedImage->removeAllClientsWaitingForAsyncDecoding();
        },
        [](Pending& pending) {
            pending.clientsWaitingForAsyncDecoding.clear();
            pending.forceAllClientsWaitingForAsyncDecoding = false;
        }
    );
}

void StyleMultiImage::addClient(StyleImageClient& client)
{
    return WTF::switchOn(m_state,
        [&](Ref<StyleImage>& selectedImage) {
            return selectedImage->addClient(client);
        },
        [&](Pending& pending) {
            pending.clients.add(client);
        }
    );
}

void StyleMultiImage::removeClient(StyleImageClient& client)
{
    return WTF::switchOn(m_state,
        [&](Ref<StyleImage>& selectedImage) {
            selectedImage->removeClient(client);
        },
        [&](Pending& pending) {
            if (pending.clients.remove(client)) {
                for (auto entry : pending.clients)
                    entry.key.styleImageClientRemoved(*this);
            }
        }
    );
}

bool StyleMultiImage::hasClient(StyleImageClient& client) const
{
    return WTF::switchOn(m_state,
        [&](const Ref<StyleImage>& selectedImage) {
            return selectedImage->hasClient(client);
        },
        [&](const Pending& pending) {
            return pending.clients.contains(client);
        }
    );
}

} // namespace WebCore
