/*
 * Copyright (C) 2022 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include "CachedImageClient.h"
#include "CachedResourceHandle.h"
#include "FilterOperations.h"
#include "StyleGeneratedImage.h"

namespace WebCore {

class StyleFilterImage final : public StyleGeneratedImage, private StyleImageClient {
public:
    static Ref<StyleFilterImage> create(RefPtr<StyleImage> image, FilterOperations filterOperations)
    {
        return adoptRef(*new StyleFilterImage(WTFMove(image), WTFMove(filterOperations)));
    }
    virtual ~StyleFilterImage();

    bool operator==(const StyleImage& other) const final;
    bool equals(const StyleFilterImage&) const;
    bool equalInputImages(const StyleFilterImage&) const;

    RefPtr<StyleImage> inputImage() const { return m_image; }
    const FilterOperations& filterOperations() const { return m_filterOperations; }

    static constexpr bool isFixedSize = true;

private:
    explicit StyleFilterImage(RefPtr<StyleImage>&&, FilterOperations&&);

    // StyleImage overrides
    Ref<CSSValue> computedStyleValue(const RenderStyle&) const final;
    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;
    bool isLoaded(const RenderElement* renderer) const final;
    bool errorOccurred() const final;
    RefPtr<Image> image(const RenderElement*, const FloatSize&, bool isForFirstLine) const final;
    bool canRender(const RenderElement* renderer, float multiplier) const final;
    void setContainerContextForRenderer(const RenderElement&, const FloatSize&, float) final;
    bool knownToBeOpaque(const RenderElement&) const final;

    // StyleGeneratedImage overrides
    void didAddClient(StyleImageClient&) final { }
    void didRemoveClient(StyleImageClient&) final { }
    FloatSize fixedSize(const RenderElement&) const final;
    bool isClientWaitingForAsyncDecoding(RenderElement&) const final;
    void addClientWaitingForAsyncDecoding(RenderElement&) final;
    void removeAllClientsWaitingForAsyncDecoding() final;

    // StyleImageClient overrides
    void styleImageChanged(StyleImage&, const IntRect*) final;

    // StyleFilterOperationsClient overrides
    // FIXME: Finish or remove.
    void styleFilterOperationsChanged(FilterOperations&); // final;

    RefPtr<StyleImage> m_image;
    // FIXME: FilterOperations need some client interface to let us know if a reference filter has loaded or failed to load.
    FilterOperations m_filterOperations;
    bool m_inputImageIsReady;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleFilterImage, isFilterImage)
