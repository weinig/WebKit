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

#include "FilterOperations.h"
#include "StyleGeneratedImage.h"

namespace WebCore {

class StyleFilterImage final : public StyleGeneratedImage, private StyleImageClient {
public:
    static Ref<StyleFilterImage> create(RefPtr<StyleImage> inputImage, FilterOperations filterOperations)
    {
        return adoptRef(*new StyleFilterImage(WTFMove(inputImage), WTFMove(filterOperations)));
    }
    virtual ~StyleFilterImage();

    bool operator==(const StyleImage&) const final;
    bool equals(const StyleFilterImage&) const;
    bool equalInputImages(const StyleFilterImage&) const;

    RefPtr<StyleImage> inputImage() const { return m_inputImage; }
    const FilterOperations& filterOperations() const { return m_filterOperations; }

private:
    explicit StyleFilterImage(RefPtr<StyleImage>&&, FilterOperations&&);

    Ref<CSSValue> computedStyleValue(const RenderStyle&) const final;
    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;

    NaturalDimensions naturalDimensionsForContext(const StyleImageSizingContext&) const final;
    RefPtr<Image> imageForContext(const StyleImageSizingContext&) const final;
    bool knownToBeOpaque() const final;

    // MARK: - StyleImageClient
    void styleImageChanged(StyleImage&, const IntRect*) final;
    void styleImageFinishedResourceLoad(StyleImage&, CachedResource&) final;
    void styleImageFinishedLoad(StyleImage&) final;
    void styleImageNeedsScheduledRenderingUpdate(StyleImage&) final;
    bool styleImageCanDestroyDecodedData(StyleImage&) const final;
    bool styleImageAnimationAllowed(StyleImage&) const final;
    VisibleInViewportState styleImageFrameAvailable(StyleImage&, ImageAnimatingState, const IntRect*) final;
    VisibleInViewportState styleImageVisibleInViewport(StyleImage&, const Document&) const final;
    HashSet<Element*> styleImageReferencingElements(StyleImage&) const final;

    RefPtr<StyleImage> m_inputImage;
    FilterOperations m_filterOperations;
    bool m_inputImageIsReady;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_STYLE_IMAGE(StyleFilterImage, isFilterImage)
