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

#pragma once

namespace WebCore {

class CSSStyleImageValue;
class CanvasBase;
class HTMLCanvasElement;
class HTMLImageElement;
class ImageBitmap;
class SVGImageElement;

#if ENABLE(OFFSCREEN_CANVAS)
class OffscreenCanvas;
#endif
#if ENABLE(VIDEO)
class HTMLVideoElement;
#endif
#if ENABLE(WEB_CODECS)
class WebCodecsVideoFrame;
#endif

using CanvasImageSource = std::variant<
      RefPtr<HTMLImageElement>
    , RefPtr<SVGImageElement>
    , RefPtr<HTMLCanvasElement>
    , RefPtr<ImageBitmap>
    , RefPtr<CSSStyleImageValue>
#if ENABLE(OFFSCREEN_CANVAS)
    , RefPtr<OffscreenCanvas>
#endif
#if ENABLE(VIDEO)
    , RefPtr<HTMLVideoElement>
#endif
#if ENABLE(WEB_CODECS)
    , RefPtr<WebCodecsVideoFrame>
#endif
    >;

// MARK: Image Usability
// https://html.spec.whatwg.org/multipage/canvas.html#check-the-usability-of-the-image-argument

enum class ImageUse : bool {
    Immediate, // State will be used immediately and discarded (i.e. drawImage)
    Persistent // State will be stored for later use (i.e. createPattern, ImageBitmap)
};

template<typename, ImageUse> struct ImageUsabilityGood;

template<> struct ImageUsabilityGood<RefPtr<HTMLImageElement>, ImageUse::Immediate> {
    FloatSize size;
    Ref<Image> source;
};
template<> struct ImageUsabilityGood<RefPtr<HTMLImageElement>, ImageUse::Persistent> {
    FloatSize size;
    Ref<NativeImage> source;
};

template<> struct ImageUsabilityGood<RefPtr<SVGImageElement>, ImageUse::Immediate> {
    FloatSize size;
    Ref<Image> source;
};
template<> struct ImageUsabilityGood<RefPtr<SVGImageElement>, ImageUse::Persistent> {
    FloatSize size;
    Ref<NativeImage> source;
};

template<> struct ImageUsabilityGood<RefPtr<CSSStyleImageValue>, ImageUse::Immediate> {
    FloatSize size;
    Ref<Image> source;
};
template<> struct ImageUsabilityGood<RefPtr<CSSStyleImageValue>, ImageUse::Persistent> {
    FloatSize size;
    Ref<NativeImage> source;
};

template<ImageUse Use> struct ImageUsabilityGood<RefPtr<ImageBitmap>, Use> {
    FloatSize size;
    Ref<ImageBuffer> source;
};

template<> struct ImageUsabilityGood<RefPtr<HTMLCanvasElement>, ImageUse::Immediate> {
    FloatSize size;
    Ref<ImageBuffer> source;
};
template<> struct ImageUsabilityGood<RefPtr<HTMLCanvasElement>, ImageUse::Persistent> {
    FloatSize size;
    Ref<NativeImage> source;
};

#if ENABLE(OFFSCREEN_CANVAS)
template<> struct ImageUsabilityGood<RefPtr<OffscreenCanvas>, ImageUse::Immediate> {
    FloatSize size;
    Ref<ImageBuffer> source;
};
template<> struct ImageUsabilityGood<RefPtr<OffscreenCanvas>, ImageUse::Persistent> {
    FloatSize size;
    Ref<NativeImage> source;
};
#endif

#if ENABLE(VIDEO)
template<> struct ImageUsabilityGood<RefPtr<HTMLVideoElement>, ImageUse::Immediate> {
    FloatSize size;
};
template<> struct ImageUsabilityGood<RefPtr<HTMLVideoElement>, ImageUse::Persistent> {
    FloatSize size;
    SourceImage source;
};
#endif

#if ENABLE(WEB_CODECS)
template<> struct ImageUsabilityGood<RefPtr<WebCodecsVideoFrame, ImageUse::Immediate> {
    FloatSize size;
    Ref<VideoFrame> source;
};

template<> struct ImageUsabilityGood<RefPtr<WebCodecsVideoFrame, ImageUse::Persistent> {
    FloatSize size;
    SourceImage source;
};
#endif

struct ImageUsabilityBad { };

template<typename T, ImageUse Use> using ImageUsability = std::variant<ImageUsabilityBad, ImageUsabilityGood<T, Use>;

ExceptionOr<ImageUsability<RefPtr<HTMLImageElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, HTMLImageElement&);
ExceptionOr<ImageUsability<RefPtr<HTMLImageElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, HTMLImageElement&);
ExceptionOr<ImageUsability<RefPtr<SVGImageElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, SVGImageElement&);
ExceptionOr<ImageUsability<RefPtr<SVGImageElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, SVGImageElement&);
ExceptionOr<ImageUsability<RefPtr<CSSStyleImageValue>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, CSSStyleImageValue&);
ExceptionOr<ImageUsability<RefPtr<CSSStyleImageValue>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, CSSStyleImageValue&);
ExceptionOr<ImageUsability<RefPtr<HTMLCanvasElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, HTMLCanvasElement&);
ExceptionOr<ImageUsability<RefPtr<HTMLCanvasElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, HTMLCanvasElement&);
ExceptionOr<ImageUsability<RefPtr<ImageBitmap>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, ImageBitmap&);
ExceptionOr<ImageUsability<RefPtr<ImageBitmap>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, ImageBitmap&);
#if ENABLE(OFFSCREEN_CANVAS)
ExceptionOr<ImageUsability<RefPtr<OffscreenCanvas>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, OffscreenCanvas&);
ExceptionOr<ImageUsability<RefPtr<OffscreenCanvas>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, OffscreenCanvas&);
#endif
#if ENABLE(VIDEO)
ExceptionOr<ImageUsability<RefPtr<HTMLVideoElement>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, HTMLVideoElement&);
ExceptionOr<ImageUsability<RefPtr<HTMLVideoElement>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, HTMLVideoElement&);
#endif
#if ENABLE(WEB_CODECS)
ExceptionOr<ImageUsability<RefPtr<WebCodecsVideoFrame>, ImageUse::Immediate>> checkUsabilityForImmediateUse(const CanvasBase&, WebCodecsVideoFrame&);
ExceptionOr<ImageUsability<RefPtr<WebCodecsVideoFrame>, ImageUse::Persistent>> checkUsabilityForPersistentUse(const CanvasBase&, WebCodecsVideoFrame&);
#endif

// MARK: Origin Tainting
// https://html.spec.whatwg.org/multipage/canvas.html#the-image-argument-is-not-origin-clean

bool taintsOrigin(const CanvasBase&, const HTMLImageElement&);
bool taintsOrigin(const CanvasBase&, const SVGImageElement&);
bool taintsOrigin(const CanvasBase&, const CSSStyleImageValue&);
bool taintsOrigin(const CanvasBase&, const ImageBitmap&);
bool taintsOrigin(const CanvasBase&, const HTMLCanvasElement&);
#if ENABLE(OFFSCREEN_CANVAS)
bool taintsOrigin(const CanvasBase&, const OffscreenCanvas&);
#endif
#if ENABLE(VIDEO)
bool taintsOrigin(const CanvasBase&, const HTMLVideoElement&);
#endif
#if ENABLE(WEB_CODECS)
bool taintsOrigin(const CanvasBase&, const WebCodecsVideoFrame&);
#endif

} // namespace WebCore
