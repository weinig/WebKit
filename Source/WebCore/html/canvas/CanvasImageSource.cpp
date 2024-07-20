/*
 * Copyright (C) 2024 Samuel Weinig <sam@webkit.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CanvasImageSource.h"

#include "CSSStyleImageValue.h"
#include "CachedImage.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "ImageBitmap.h"
#include "SVGImageElement.h"
#if ENABLE(OFFSCREEN_CANVAS)
#include "OffscreenCanvas.h"
#endif
#if ENABLE(VIDEO)
#include "HTMLVideoElement.h"
#endif
#if ENABLE(WEB_CODECS)
#include "WebCodecsVideoFrame.h"
#endif

namespace WebCore {

// MARK: Image Usability
// https://html.spec.whatwg.org/multipage/canvas.html#check-the-usability-of-the-image-argument

// Helper for upgrading an immediate check to persistent check
static template<typename T> ExceptionOr<ImageUsability<RefPtr<T>, ImageUse::Persistent>> upgradeToPersistentNativeImageUsingImmediateCheck(const CanvasBase& canvasBase, T& imageElement)
{
    auto immediateUsability = checkUsabilityForImmediateUse(canvasBase, imageElement);
    if (immediateUsability.hasException())
        return immediateUsability.releaseException();

    return WTF::switchOn(immediateUsability.releaseResultValue(),
        [](ImageUsabilityBad&&) -> ExceptionOr<ImageUsability<RefPtr<T>, ImageUse::Persistent>> {
            return ImageUsabilityBad { };
        },
        [](auto&& good) -> ExceptionOr<ImageUsability<RefPtr<T>, ImageUse::Persistent>> {
            RefPtr nativeImage = good.image->nativeImage();
            if (!nativeImage)
                return ImageUsabilityBad { };
            return ImageUsabilityGood<RefPtr<T>, ImageUse::Persistent> { good.size, nativeImage.releaseNonNull() };
        }
    );
}


ExceptionOr<ImageUsability<RefPtr<HTMLImageElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase& canvasBase, HTMLImageElement& imageElement)
{
    // For both HTMLImageElement and SVGImageElement:
    //
    // If image's current request's state is broken, then throw an "InvalidStateError" DOMException.
    // If image is not fully decodable, then return bad.
    // If image has a natural width or natural height (or both) equal to zero, then return bad.

    // FIXME: Expose better interface on HTMLImageElement using these spec terms.

    if (!imageElement.complete())
        return ImageUsabilityBad { };

    auto* cachedImage = imageElement.cachedImage();
    if (!cachedImage)
        return ImageUsabilityBad { };

    if (cachedImage->status() == CachedImage::Status::DecodeError)
        return Exception { ExceptionCode::InvalidStateError, "The HTMLImageElement provided is in the 'broken' state."_s };

    RefPtr image = cachedImage->imageRaw();
    if (!image)
        return ImageUsabilityBad { };

    auto naturalDimensions = image->naturalDimensions();
    if ((naturalDimensions.width && *naturalDimensions.width == 0) || (naturalDimensions.height && *naturalDimensions.height == 0))
        return ImageUsabilityBad { };

    FloatSize size;
    if (naturalDimensions.width && naturalDimensions.height)
        size = { naturalDimensions.width->toFloat(), naturalDimensions.height->toFloat() };
    else {
        auto specifiedSize = ObjectSizeNegotiation::SpecifiedSize { std::nullopt, std::nullopt };
        auto defaultObjectSize = LayoutSize { canvasBase.size() };
        size = ObjectSizeNegotiation::defaultSizingAlgorithm(naturalDimensions, specifiedSize, defaultObjectSize);
    }

    if (RefPtr bitmapImage = dynamicDowncast<BitmapImage>(*image)) {
        // Drawing an animated image to a canvas should draw the first frame (except for a few layout tests)
        if (image->isAnimated() && !imageElement.document().settings().animatedImageDebugCanvasDrawingEnabled()) {
            bitmapImage = BitmapImage::create(image->nativeImage());
            if (!bitmapImage)
                return ImageUsabilityBad { };
            image = bitmapImage.releaseNonNull();
        }
    } else if (RefPtr svgImage = dynamicDowncast<SVGImage>(*image))
        image = SVGImageForContainer::create(svgImage.get(), size, 1, cachedImage->url());

    return ImageUsabilityGood { size, image.releaseNonNull() };
}

ExceptionOr<ImageUsability<RefPtr<HTMLImageElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase& canvasBase, HTMLImageElement& imageElement)
{
    return upgradeToPersistentNativeImageUsingImmediateCheck(canvasBase, imageElement);
}

ExceptionOr<ImageUsability<RefPtr<SVGImageElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase& canvasBase, SVGImageElement& imageElement)
{
    // For both HTMLImageElement and SVGImageElement:
    //
    // If image's current request's state is broken, then throw an "InvalidStateError" DOMException.
    // If image is not fully decodable, then return bad.
    // If image has a natural width or natural height (or both) equal to zero, then return bad.

    // FIXME: Expose better interface on HTMLImageElement using these spec terms.

    // FIXME: Unlike, HTMLImageElement, this one does not check the "complete()" function.

    auto* cachedImage = imageElement.cachedImage();
    if (!cachedImage)
        return ImageUsabilityBad { };

    if (cachedImage->status() == CachedImage::Status::DecodeError)
        return Exception { ExceptionCode::InvalidStateError, "The HTMLImageElement provided is in the 'broken' state."_s };

    RefPtr image = cachedImage->imageRaw();
    if (!image)
        return ImageUsabilityBad { };

    auto naturalDimensions = image->naturalDimensions();
    if ((naturalDimensions.width && *naturalDimensions.width == 0) || (naturalDimensions.height && *naturalDimensions.height == 0))
        return ImageUsabilityBad { };

    FloatSize size;
    if (naturalDimensions.width && naturalDimensions.height)
        size = { naturalDimensions.width->toFloat(), naturalDimensions.height->toFloat() };
    else {
        auto specifiedSize = ObjectSizeNegotiation::SpecifiedSize { std::nullopt, std::nullopt };
        auto defaultObjectSize = LayoutSize { canvasBase.size() };
        size = ObjectSizeNegotiation::defaultSizingAlgorithm(naturalDimensions, specifiedSize, defaultObjectSize);
    }

    if (RefPtr bitmapImage = dynamicDowncast<BitmapImage>(*image)) {
        // Drawing an animated image to a canvas should draw the first frame.
        if (image->isAnimated()) {
            bitmapImage = BitmapImage::create(image->nativeImage());
            if (!bitmapImage)
                return ImageUsabilityBad { };
            image = bitmapImage.releaseNonNull();
        }
    } else if (RefPtr svgImage = dynamicDowncast<SVGImage>(*image))
        image = SVGImageForContainer::create(svgImage.get(), size, 1, cachedImage->url());

    return ImageUsabilityGood { size, image.releaseNonNull() };
}

ExceptionOr<ImageUsability<RefPtr<SVGImageElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase& canvasBase, SVGImageElement& imageElement)
{
    return upgradeToPersistentNativeImageUsingImmediateCheck(canvasBase, imageElement);
}

ExceptionOr<ImageUsability<RefPtr<CSSStyleImageValue>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase& canvasBase, CSSStyleImageValue& styleImageValue)
{
    // https://drafts.css-houdini.org/css-paint-api/#drawing-a-cssimagevalue
    //
    // FIXME: Its not clear what rules to use for this.
    //
    // All the spec currently says is:
    //
    //   "When a CanvasImageSource object represents an CSSImageValue, the result of invoking
    //    the value’s underlying image algorithm must be used as the source image for the
    //    purposes of drawImage."
    //
    // Using rules similar to HTMLImageElement/SVGImageElement for now.

    auto* cachedImage = styleImageValue.image();
    if (!cachedImage)
        return ImageUsabilityBad { };

    if (cachedImage->status() == CachedImage::Status::DecodeError)
        return Exception { ExceptionCode::InvalidStateError };

    RefPtr image = cachedImage->imageRaw();
    if (!image)
        return ImageUsabilityBad { };

    auto naturalDimensions = image->naturalDimensions();
    if ((naturalDimensions.width && *naturalDimensions.width == 0) || (naturalDimensions.height && *naturalDimensions.height == 0))
        return ImageUsabilityBad { };

    FloatSize size;
    if (naturalDimensions.width && naturalDimensions.height)
        size = { naturalDimensions.width->toFloat(), naturalDimensions.height->toFloat() };
    else {
        auto specifiedSize = ObjectSizeNegotiation::SpecifiedSize { std::nullopt, std::nullopt };
        auto defaultObjectSize = LayoutSize { canvasBase.size() };
        size = ObjectSizeNegotiation::defaultSizingAlgorithm(naturalDimensions, specifiedSize, defaultObjectSize);
    }

    if (RefPtr bitmapImage = dynamicDowncast<BitmapImage>(*image)) {
        // Drawing an animated image to a canvas should draw the first frame.
        if (image->isAnimated()) {
            bitmapImage = BitmapImage::create(image->nativeImage());
            if (!bitmapImage)
                return ImageUsabilityBad { };
            image = bitmapImage.releaseNonNull();
        }
    } else if (RefPtr svgImage = dynamicDowncast<SVGImage>(*image))
        image = SVGImageForContainer::create(svgImage.get(), size, 1, cachedImage->url());

    return ImageUsabilityGood { size, image.releaseNonNull() };
}

ExceptionOr<ImageUsability<RefPtr<CSSStyleImageValue>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase& canvasBase, CSSStyleImageValue& imageElement)
{
    return upgradeToPersistentNativeImageUsingImmediateCheck(canvasBase, imageElement);
}

template<ImageUse Use> static ExceptionOr<ImageUsability<RefPtr<ImageBitmap>, Use>> checkUsabilityFor(const CanvasBase&, ImageBitmap& imageBitmap)
{

    // If image's [[Detached]] internal slot value is set to true, then throw an "InvalidStateError" DOMException.

    if (imageBitmap.isDetached())
        return Exception { ExceptionCode::InvalidStateError };

    RefPtr buffer = imageBitmap.buffer();
    if (!buffer)
        return ImageUsabilityBad { };

    auto size = FloatSize { static_cast<float>(imageBitmap.width()), static_cast<float>(imageBitmap.height()) };
    return ImageUsabilityGood { size, buffer.releaseNonNull() };
}

ExceptionOr<ImageUsability<RefPtr<ImageBitmap>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, ImageBitmap& imageBitmap)
{
    return checkUsabilityFor<ImageUse::Immediate>(canvasBase, imageBitmap);
}

ExceptionOr<ImageUsability<RefPtr<ImageBitmap>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, ImageBitmap& imageBitmap)
{
    return checkUsabilityFor<ImageUse::Persistent>(canvasBase, imageBitmap);
}

ExceptionOr<ImageUsability<RefPtr<HTMLCanvasElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, HTMLCanvasElement& canvas)
{
    // If image has either a horizontal dimension or a vertical dimension equal to zero, then throw an "InvalidStateError" DOMException.

    auto size = canvas.size();
    if (size.width() == 0 || size.height() == 0)
        return Exception { ExceptionCode::InvalidStateError };

    RefPtr buffer = canvas.makeRenderingResultsAvailable(ShouldApplyPostProcessingToDirtyRect::No);
    if (!buffer)
        return ImageUsabilityBad { };

    return ImageUsabilityGood { size, buffer.releaseNonNull() };
}

ExceptionOr<ImageUsability<RefPtr<HTMLCanvasElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, HTMLCanvasElement& canvas)
{
    // If image has either a horizontal dimension or a vertical dimension equal to zero, then throw an "InvalidStateError" DOMException.

    auto size = canvas.size();
    if (size.width() == 0 || size.height() == 0)
        return Exception { ExceptionCode::InvalidStateError };

    auto* copiedImage = canvas.copiedImage();
    if (!copiedImage)
        return ImageUsabilityBad { };
    
    auto nativeImage = copiedImage->nativeImage(); // FIXME: Should this be getting a DestinationColorSpace
    if (!nativeImage)
        return ImageUsabilityBad { };

    return ImageUsabilityGood { size, nativeImage.releaseNonNull() };
}

#if ENABLE(OFFSCREEN_CANVAS)
ExceptionOr<ImageUsability<RefPtr<OffscreenCanvas>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, OffscreenCanvas& canvas)
{
    // If image has either a horizontal dimension or a vertical dimension equal to zero, then throw an "InvalidStateError" DOMException.

    auto size = canvas.size();
    if (size.width() == 0 || size.height() == 0)
        return Exception { ExceptionCode::InvalidStateError };

    RefPtr buffer = canvas.makeRenderingResultsAvailable(ShouldApplyPostProcessingToDirtyRect::No);
    if (!buffer)
        return ImageUsabilityBad { };

    return ImageUsabilityGood { size, buffer.releaseNonNull() };
}

ExceptionOr<ImageUsability<RefPtr<OffscreenCanvas>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, OffscreenCanvas& canvas)
{
    // If image has either a horizontal dimension or a vertical dimension equal to zero, then throw an "InvalidStateError" DOMException.

    auto size = canvas.size();
    if (size.width() == 0 || size.height() == 0)
        return Exception { ExceptionCode::InvalidStateError };

    auto* copiedImage = canvas.copiedImage();
    if (!copiedImage)
        return ImageUsabilityBad { };
    
    auto nativeImage = copiedImage->nativeImage(); // FIXME: Should this be getting a DestinationColorSpace
    if (!nativeImage)
        return ImageUsabilityBad { };

    return ImageUsabilityGood { size, nativeImage.releaseNonNull() };
}
#endif

#if ENABLE(VIDEO)
ExceptionOr<ImageUsability<RefPtr<HTMLVideoElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, HTMLVideoElement& videoElement)
{
    // If image's readyState attribute is either HAVE_NOTHING or HAVE_METADATA, then return bad.

    if (videoElement.readyState() == HTMLMediaElement::HAVE_NOTHING || video.readyState() == HTMLMediaElement::HAVE_METADATA)
        return ImageUsabilityBad { };

    // FIXME: Spec doesn't mention it, but it doesn't make sense to draw a video element with zero width or height. Matching HTMLImageElement/SVGImageElement and return 'bad'.
    auto size = videoElement.naturalSize();
    if (size.width() == 0 || size.height() == 0)
        return ImageUsabilityBad { };

    return ImageUsabilityGood { size };
}

ExceptionOr<ImageUsability<RefPtr<HTMLVideoElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase& canvasBase, HTMLVideoElement& videoElement)
{
    // If image's readyState attribute is either HAVE_NOTHING or HAVE_METADATA, then return bad.

    if (videoElement.readyState() == HTMLMediaElement::HAVE_NOTHING || video.readyState() == HTMLMediaElement::HAVE_METADATA)
        return ImageUsabilityBad { };

    // FIXME: Spec doesn't mention it, but it doesn't make sense to draw a video element with zero width or height. Matching HTMLImageElement/SVGImageElement and return 'bad'.
    auto size = videoElement.naturalSize();
    if (size.width() == 0 || size.height() == 0)
        return ImageUsabilityBad { };

#if USE(CG)
    if (auto nativeImage = videoElement.nativeImageForCurrentTime())
        return ImageUsabilityGood { size, nativeImage.releaseNonNull() };
#endif

    // FIXME: Consider passing in context to make this strict.

    auto* canvasBuffer = canvasBase.buffer();
    auto renderingMode = canvasBuffer ? canvasBuffer->context().renderingMode() : RenderingMode::Unaccelerated;
    auto colorSpace = canvasBuffer ? canvasBuffer->context().colorSpace() : DestinationColorSpace::SRGB();
    auto pixelFormat = canvasBuffer ? canvasBuffer->context().pixelFormat() : ImageBufferPixelFormat::BGRA8;

    auto imageBuffer = videoElement.createBufferForPainting(size, renderingMode, colorSpace, pixelFormat);
    if (!imageBuffer)
        return ImageUsabilityBad { };

    videoElement.paintCurrentFrameInContext(imageBuffer->context(), FloatRect(FloatPoint(), size));

    return ImageUsabilityGood { size, imageBuffer.releaseNonNull() };
}
#endif

#if ENABLE(WEB_CODECS)
ExceptionOr<ImageUsability<RefPtr<WebCodecsVideoFrame>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, WebCodecsVideoFrame& videoFrame)
{
    // If image's [[Detached]] internal slot value is set to true, then throw an "InvalidStateError" DOMException.

    if (videoFrame.isDetached())
        return Exception { ExceptionCode::InvalidStateError, "frame is detached"_s };

    RefPtr internalFrame = videoFrame.internalFrame();
    if (!internalFrame)
        return ImageUsabilityBad { };

    auto size = FloatSize { static_cast<float>(videoFrame.displayWidth()), static_cast<float>(videoFrame.displayHeight()) };
    return ImageUsabilityGood { size, internalFrame.releaseNonNull() };
}

ExceptionOr<ImageUsability<RefPtr<WebCodecsVideoFrame>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase& canvasBase, WebCodecsVideoFrame& videoFrame)
{
    // If image's [[Detached]] internal slot value is set to true, then throw an "InvalidStateError" DOMException.

    if (videoFrame.isDetached())
        return Exception { ExceptionCode::InvalidStateError, "frame is detached"_s };

    RefPtr internalFrame = videoFrame.internalFrame();
    if (!internalFrame)
        return ImageUsabilityBad { };

    auto size = FloatSize { static_cast<float>(videoFrame.displayWidth()), static_cast<float>(videoFrame.displayHeight()) };

    // FIXME: Should be possible to use the VideoFrame directly without an intermediate buffer.

    auto* canvasBuffer = canvasBase.buffer();
    auto renderingMode = canvasBuffer ? canvasBuffer->context().renderingMode() : RenderingMode::Unaccelerated;
    auto colorSpace = canvasBuffer ? canvasBuffer->context().colorSpace() : DestinationColorSpace::SRGB();
    auto pixelFormat = canvasBuffer ? canvasBuffer->context().pixelFormat() : ImageBufferPixelFormat::BGRA8;

    auto imageBuffer = ImageBuffer::create(size, RenderingPurpose::MediaPainting, 1, colorSpace, pixelFormat, bufferOptionsForRendingMode(renderingMode));
    if (!imageBuffer)
        return ImageUsabilityBad { };

    return ImageUsabilityGood { size, imageBuffer.releaseNonNull() };
}
#endif

// MARK: Origin Tainting
// https://html.spec.whatwg.org/multipage/canvas.html#the-image-argument-is-not-origin-clean

static bool taintsOrigin(const CanvasBase& canvas, const CachedImage* cachedImage)
{
    if (!cachedImage)
        return false;

    RefPtr image = cachedImage->rawImage();
    if (!image)
        return false;

    if (image->sourceURL().protocolIsData())
        return false;

    if (image->renderingTaintsOrigin())
        return true;

    if (cachedImage->isCORSCrossOrigin())
        return true;

    ASSERT(canvas.securityOrigin());
    ASSERT(cachedImage->origin());
    ASSERT(canvas.securityOrigin()->toString() == cachedImage->origin()->toString());
    return false;
}

bool taintsOrigin(const CanvasBase& canvas, const HTMLImageElement& element)
{
    return taintsOrigin(canvas, element.cachedImage());
}

bool taintsOrigin(const CanvasBase& canvas, const SVGImageElement& element)
{
    return taintsOrigin(canvas, element.cachedImage());
}

bool taintsOrigin(const CanvasBase&, const HTMLCanvasElement& element)
{
    return !element.originClean();
}

bool taintsOrigin(const CanvasBase&, const ImageBitmap& imageBitmap)
{
    return !imageBitmap.originClean();
}

bool taintsOrigin(const CanvasBase&, const CSSStyleImageValue& imageValue)
{
    return taintsOrigin(canvas, imageValue.image());
}

#if ENABLE(OFFSCREEN_CANVAS)
bool taintsOrigin(const CanvasBase&, const OffscreenCanvas& offscreenCanvas)
{
    return !offscreenCanvas.originClean();
}
#endif

#if ENABLE(VIDEO)
bool taintsOrigin(const CanvasBase& canvas, const HTMLVideoElement& video)
{
    return video.taintsOrigin(*canvas.securityOrigin());
}
#endif

#if ENABLE(WEB_CODECS)
bool taintsOrigin(const CanvasBase&, const WebCodecsVideoFrame&)
{
    // FIXME: This is currently undefined in the standard, but it does not appear a WebCodecsVideoFrame can ever be constructed in a way that is not origin clean.
    //        See https://github.com/whatwg/html/issues/10489
    return false;
}
#endif

} // namespace WebCore
