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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "CSSImageValue.h"

#include "CSSMarkup.h"
#include "CSSPrimitiveValue.h"
#include "CSSValueKeywords.h"
#include "DeprecatedCSSOMPrimitiveValue.h"
#include "StyleBuilderState.h"
#include "StyleCachedImage.h"

namespace WebCore {

Ref<CSSImageValue> CSSImageValue::create(ResolvedURL url, LoadedFromOpaqueSource loadedFromOpaqueSource, AtomString initiatorName)
{
    return adoptRef(*new CSSImageValue(WTFMove(url), loadedFromOpaqueSource, WTFMove(initiatorName)));
}

Ref<CSSImageValue> CSSImageValue::create(URL url, LoadedFromOpaqueSource loadedFromOpaqueSource, AtomString initiatorName)
{
    return adoptRef(*new CSSImageValue(makeResolvedURL(WTFMove(url)), loadedFromOpaqueSource, WTFMove(initiatorName)));
}

CSSImageValue::CSSImageValue(ResolvedURL&& url, LoadedFromOpaqueSource loadedFromOpaqueSource, AtomString&& initiatorName)
    : CSSValue { ImageClass }
    , m_url { WTFMove(url) }
    , m_loadedFromOpaqueSource { loadedFromOpaqueSource }
    , m_initiatorName { WTFMove(initiatorName) }
{
}

CSSImageValue::~CSSImageValue() = default;

bool CSSImageValue::equals(const CSSImageValue& other) const
{
    return m_url == other.m_url;
}

String CSSImageValue::customCSSText() const
{
    return serializeURL(m_url.specifiedURLString);
}

Ref<DeprecatedCSSOMValue> CSSImageValue::createDeprecatedCSSOMWrapper(CSSStyleDeclaration& styleDeclaration) const
{
    // NOTE: We expose CSSImageValues as URI primitive values in CSSOM to maintain old behavior.
    return DeprecatedCSSOMPrimitiveValue::create(CSSPrimitiveValue::create(m_url.resolvedURL.string(), CSSUnitType::CSS_URI), styleDeclaration);
}

RefPtr<StyleImage> CSSImageValue::createStyleImage(Style::BuilderState&) const
{
    // FIXME: Resolve URL here?
    return StyleCachedImage::create(m_url, m_loadedFromOpaqueSource, m_initiatorName);
}

} // namespace WebCore
