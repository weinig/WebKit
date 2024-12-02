/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "ShadowData.h"

#include "StylePrimitiveNumericTypes+Logging.h"
#include <wtf/PointerComparison.h>
#include <wtf/TZoneMallocInlines.h>
#include <wtf/text/TextStream.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(ShadowData);

ShadowData::ShadowData(const ShadowData& other)
    : m_shadow(other.m_shadow)
    , m_next(other.m_next ? makeUnique<ShadowData>(*other.m_next) : nullptr)
{
}

void ShadowData::deleteNextLinkedListWithoutRecursion()
{
    // Avoid recursion errors when the linked list is too long.
    for (auto next = std::exchange(m_next, nullptr); (next = std::exchange(next->m_next, nullptr));) { }
}

std::optional<ShadowData> ShadowData::clone(const ShadowData* data)
{
    if (!data)
        return std::nullopt;
    return *data;
}

bool ShadowData::operator==(const ShadowData& other) const
{
    if (m_shadow != other.m_shadow)
        return false;

    // Avoid relying on recursion in case the linked list is very long.
    auto* next = m_next.get();
    auto* otherNext = other.m_next.get();
    while (next || otherNext) {
        if (!next || !otherNext || next->m_shadow != otherNext->m_shadow)
            return false;
        next = next->m_next.get();
        otherNext = otherNext->m_next.get();
    }

    return true;
}

LayoutBoxExtent ShadowData::shadowOutsetExtent() const
{
    LayoutUnit top;
    LayoutUnit right;
    LayoutUnit bottom;
    LayoutUnit left;

    for (auto* shadow = this; shadow; shadow = shadow->next()) {
        auto extentAndSpread = shadow->paintingExtent() + LayoutUnit(shadow->spread().value);
        if (shadow->style() == ShadowStyle::Inset)
            continue;

        left = std::min(LayoutUnit(shadow->x().value) - extentAndSpread, left);
        right = std::max(LayoutUnit(shadow->x().value) + extentAndSpread, right);
        top = std::min(LayoutUnit(shadow->y().value) - extentAndSpread, top);
        bottom = std::max(LayoutUnit(shadow->y().value) + extentAndSpread, bottom);
    }

    return { top, right, bottom, left };
}

LayoutBoxExtent ShadowData::shadowInsetExtent() const
{
    LayoutUnit top;
    LayoutUnit right;
    LayoutUnit bottom;
    LayoutUnit left;

    for (auto* shadow = this; shadow; shadow = shadow->next()) {
        if (shadow->style() == ShadowStyle::Normal)
            continue;

        auto extentAndSpread = shadow->paintingExtent() + LayoutUnit(shadow->spread().value);
        top = std::max<LayoutUnit>(top, LayoutUnit(shadow->y().value) + extentAndSpread);
        right = std::min<LayoutUnit>(right, LayoutUnit(shadow->x().value) - extentAndSpread);
        bottom = std::min<LayoutUnit>(bottom, LayoutUnit(shadow->y().value) - extentAndSpread);
        left = std::max<LayoutUnit>(left, LayoutUnit(shadow->x().value) + extentAndSpread);
    }

    return { top, right, bottom, left };
}

void ShadowData::adjustRectForShadow(LayoutRect& rect) const
{
    auto shadowExtent = shadowOutsetExtent();

    rect.move(shadowExtent.left(), shadowExtent.top());
    rect.setWidth(rect.width() - shadowExtent.left() + shadowExtent.right());
    rect.setHeight(rect.height() - shadowExtent.top() + shadowExtent.bottom());
}

void ShadowData::adjustRectForShadow(FloatRect& rect) const
{
    auto shadowExtent = shadowOutsetExtent();

    rect.move(shadowExtent.left(), shadowExtent.top());
    rect.setWidth(rect.width() - shadowExtent.left() + shadowExtent.right());
    rect.setHeight(rect.height() - shadowExtent.top() + shadowExtent.bottom());
}

TextStream& operator<<(TextStream& ts, const ShadowData& data)
{
    ts.dumpProperty("location", data.location());
    ts.dumpProperty("radius", data.radius());
    ts.dumpProperty("spread", data.spread());
    ts.dumpProperty("color", data.color());

    return ts;
}

} // namespace WebCore
