/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004-2021 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "CSSParserContext.h"
#include "CSSValue.h"
#include "ResourceLoaderOptions.h"

namespace WebCore {

class CSSStyleDeclaration;
class DeprecatedCSSOMValue;
class StyleImage;

namespace Style {
class BuilderState;
}

// FIXME: Rename to something along the lines of CSSImageURLValue or CSSImageSrcValue to make it clear this
// is just one of a few different values that CSS's <image> production supports.
class CSSImageValue final : public CSSValue {
public:
    static Ref<CSSImageValue> create(ResolvedURL, LoadedFromOpaqueSource, AtomString initiatorName = { });
    static Ref<CSSImageValue> create(URL, LoadedFromOpaqueSource, AtomString initiatorName = { });
    ~CSSImageValue();

    bool equals(const CSSImageValue&) const;
    String customCSSText() const;
    Ref<DeprecatedCSSOMValue> createDeprecatedCSSOMWrapper(CSSStyleDeclaration&) const;

    // Take care when using this, and read https://drafts.csswg.org/css-values/#relative-urls
    URL imageURL() const { return m_url.resolvedURL; }

    RefPtr<StyleImage> createStyleImage(Style::BuilderState&) const;

private:
    explicit CSSImageValue(ResolvedURL&&, LoadedFromOpaqueSource, AtomString&&);

    ResolvedURL m_url;
    LoadedFromOpaqueSource m_loadedFromOpaqueSource;
    AtomString m_initiatorName;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CSS_VALUE(CSSImageValue, isImageValue())
