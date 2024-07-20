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

#include "CanvasObserver.h"
#include "StyleGeneratedImage.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class Document;
class HTMLCanvasElement;

class StyleCanvasImage final : public StyleGeneratedImage, public CanvasObserver {
public:
    static Ref<StyleCanvasImage> create(const Document* document, String name)
    {
        return adoptRef(*new StyleCanvasImage(document, WTFMove(name)));
    }
    virtual ~StyleCanvasImage();

    bool operator==(const StyleImage&) const final;
    bool equals(const StyleCanvasImage&) const;

    const Document* document() const { return m_document.get(); }

    static constexpr bool isFixedSize = true;

private:
    explicit StyleCanvasImage(const Document* document, String&&);

    Ref<CSSValue> computedStyleValue(const RenderStyle&) const final;
    bool isPending() const final;
    void load(CachedResourceLoader&, const ResourceLoaderOptions&) final;
    RefPtr<Image> imageForRenderer(const RenderElement*, const FloatSize&, bool isForFirstLine) const final;
    bool knownToBeOpaque() const final;

    LayoutSize fixedSizeForRenderer(const RenderElement&) const final;
    void didAddClient(StyleImageClient&) final;
    void didRemoveClient(StyleImageClient&) final;

    // CanvasObserver.
    bool isStyleCanvasImage() const final { return true; }
    void canvasChanged(CanvasBase&, const FloatRect&) final;
    void canvasResized(CanvasBase&) final;
    void canvasDestroyed(CanvasBase&) final;
    HashSet<Element*> canvasReferencingElements(CanvasBase&) final;

    HTMLCanvasElement* element() const;

    String m_name;
    WeakPtr<const Document, WeakPtrImplWithEventTargetData> m_document;
    mutable HTMLCanvasElement* m_element;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::StyleCanvasImage)
    static bool isType(const WebCore::StyleImage& image) { return image.isCanvasImage(); }
    static bool isType(const WebCore::CanvasObserver& canvasObserver) { return canvasObserver.isStyleCanvasImage(); }
SPECIALIZE_TYPE_TRAITS_END()

