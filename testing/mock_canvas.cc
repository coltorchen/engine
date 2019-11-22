// Copyright 2019 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/testing/mock_canvas.h"

#include "flutter/fml/logging.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/core/SkTextBlob.h"

namespace flutter {
namespace testing {

constexpr SkISize kSize = SkISize::Make(64, 64);

MockCanvas::MockCanvas()
    : SkCanvasVirtualEnforcer<SkCanvas>(kSize.fWidth, kSize.fHeight),
      internal_canvas_(imageInfo().width(), imageInfo().height()),
      current_layer_(0) {
  internal_canvas_.addCanvas(this);
}

MockCanvas::~MockCanvas() = default;

void MockCanvas::ExpectDrawCalls(std::vector<DrawCall> expected_calls) {
  EXPECT_EQ(expected_calls, draw_calls_);
  FML_DCHECK(current_layer_ == 0);
}

void MockCanvas::willSave() {
  draw_calls_.emplace_back(
      DrawCall{current_layer_, SaveData{current_layer_ + 1}});
  current_layer_++;  // Must go here; func params order of eval is undefined
}

SkCanvas::SaveLayerStrategy MockCanvas::getSaveLayerStrategy(
    const SaveLayerRec& rec) {
  // saveLayer calls this prior to running, so we use it to track saveLayer
  // calls
  draw_calls_.emplace_back(DrawCall{
      current_layer_,
      SaveLayerData{rec.fBounds ? *rec.fBounds : SkRect(),
                    rec.fPaint ? *rec.fPaint : SkPaint(),
                    rec.fBackdrop ? sk_ref_sp<SkImageFilter>(rec.fBackdrop)
                                  : sk_sp<SkImageFilter>(),
                    current_layer_ + 1}});
  current_layer_++;  // Must go here; func params order of eval is undefined
  return kNoLayer_SaveLayerStrategy;
}

void MockCanvas::willRestore() {
  FML_DCHECK(current_layer_ > 0);

  draw_calls_.emplace_back(
      DrawCall{current_layer_, RestoreData{current_layer_ - 1}});
  current_layer_--;  // Must go here; func params order of eval is undefined
}

void MockCanvas::didConcat(const SkMatrix& matrix) {
  draw_calls_.emplace_back(DrawCall{current_layer_, ConcatMatrixData{matrix}});
}

void MockCanvas::didSetMatrix(const SkMatrix& matrix) {
  draw_calls_.emplace_back(DrawCall{current_layer_, SetMatrixData{matrix}});
}

void MockCanvas::onDrawTextBlob(const SkTextBlob* text,
                                SkScalar x,
                                SkScalar y,
                                const SkPaint& paint) {
  // This duplicates existing logic in SkCanvas::onDrawPicture
  // that should probably be split out so it doesn't need to be here as well.
  SkRect storage;
  const SkRect* bounds = nullptr;
  if (paint.canComputeFastBounds()) {
    storage = text->bounds().makeOffset(x, y);
    SkRect tmp;
    if (this->quickReject(paint.computeFastBounds(storage, &tmp))) {
      return;
    }
    bounds = &storage;
  }

  draw_calls_.emplace_back(DrawCall{
      current_layer_, DrawTextData{text ? text->serialize(SkSerialProcs{})
                                        : SkData::MakeUninitialized(0),
                                   paint, SkPoint::Make(x, y)}});
}

void MockCanvas::onDrawRect(const SkRect& rect, const SkPaint& paint) {
  draw_calls_.emplace_back(DrawCall{current_layer_, DrawRectData{rect, paint}});
}

void MockCanvas::onDrawPath(const SkPath& path, const SkPaint& paint) {
  draw_calls_.emplace_back(DrawCall{current_layer_, DrawPathData{path, paint}});
}

void MockCanvas::onDrawShadowRec(const SkPath& path,
                                 const SkDrawShadowRec& rec) {
  (void)rec;  // Can't use b/c Skia keeps this type anonymous.
  draw_calls_.emplace_back(DrawCall{current_layer_, DrawShadowData{path}});
}

void MockCanvas::onDrawPicture(const SkPicture* picture,
                               const SkMatrix* matrix,
                               const SkPaint* paint) {
  // This duplicates existing logic in SkCanvas::onDrawPicture
  // that should probably be split out so it doesn't need to be here as well.
  if (!paint || paint->canComputeFastBounds()) {
    SkRect bounds = picture->cullRect();
    if (paint) {
      paint->computeFastBounds(bounds, &bounds);
    }
    if (matrix) {
      matrix->mapRect(&bounds);
    }
    if (this->quickReject(bounds)) {
      return;
    }
  }

  draw_calls_.emplace_back(DrawCall{
      current_layer_,
      DrawPictureData{
          picture ? picture->serialize() : SkData::MakeUninitialized(0),
          paint ? *paint : SkPaint(), matrix ? *matrix : SkMatrix()}});
}

void MockCanvas::onClipRect(const SkRect& rect,
                            SkClipOp op,
                            ClipEdgeStyle style) {
  draw_calls_.emplace_back(
      DrawCall{current_layer_, ClipRectData{rect, op, style}});
}

void MockCanvas::onClipRRect(const SkRRect& rrect,
                             SkClipOp op,
                             ClipEdgeStyle style) {
  draw_calls_.emplace_back(
      DrawCall{current_layer_, ClipRRectData{rrect, op, style}});
}

void MockCanvas::onClipPath(const SkPath& path,
                            SkClipOp op,
                            ClipEdgeStyle style) {
  draw_calls_.emplace_back(
      DrawCall{current_layer_, ClipPathData{path, op, style}});
}

bool MockCanvas::onDoSaveBehind(const SkRect*) {
  FML_DCHECK(false);
  return false;
}

void MockCanvas::onDrawAnnotation(const SkRect&, const char[], SkData*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawDrawable(SkDrawable*, const SkMatrix*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawPatch(const SkPoint[12],
                             const SkColor[4],
                             const SkPoint[4],
                             SkBlendMode,
                             const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawPaint(const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawBehind(const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawPoints(PointMode,
                              size_t,
                              const SkPoint[],
                              const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawRegion(const SkRegion&, const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawOval(const SkRect&, const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawArc(const SkRect&,
                           SkScalar,
                           SkScalar,
                           bool,
                           const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawRRect(const SkRRect&, const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawBitmap(const SkBitmap&,
                              SkScalar,
                              SkScalar,
                              const SkPaint*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawImage(const SkImage*,
                             SkScalar,
                             SkScalar,
                             const SkPaint*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawBitmapRect(const SkBitmap&,
                                  const SkRect*,
                                  const SkRect&,
                                  const SkPaint*,
                                  SrcRectConstraint) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawImageRect(const SkImage*,
                                 const SkRect*,
                                 const SkRect&,
                                 const SkPaint*,
                                 SrcRectConstraint) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawImageNine(const SkImage*,
                                 const SkIRect&,
                                 const SkRect&,
                                 const SkPaint*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawBitmapNine(const SkBitmap&,
                                  const SkIRect&,
                                  const SkRect&,
                                  const SkPaint*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawImageLattice(const SkImage*,
                                    const Lattice&,
                                    const SkRect&,
                                    const SkPaint*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawBitmapLattice(const SkBitmap&,
                                     const Lattice&,
                                     const SkRect&,
                                     const SkPaint*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawVerticesObject(const SkVertices*,
                                      const SkVertices::Bone[],
                                      int,
                                      SkBlendMode,
                                      const SkPaint&) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawAtlas(const SkImage*,
                             const SkRSXform[],
                             const SkRect[],
                             const SkColor[],
                             int,
                             SkBlendMode,
                             const SkRect*,
                             const SkPaint*) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawEdgeAAQuad(const SkRect&,
                                  const SkPoint[4],
                                  QuadAAFlags,
                                  const SkColor4f&,
                                  SkBlendMode) {
  FML_DCHECK(false);
}

void MockCanvas::onDrawEdgeAAImageSet(const ImageSetEntry[],
                                      int,
                                      const SkPoint[],
                                      const SkMatrix[],
                                      const SkPaint*,
                                      SrcRectConstraint) {
  FML_DCHECK(false);
}

void MockCanvas::onClipRegion(const SkRegion&, SkClipOp) {
  FML_DCHECK(false);
}

}  // namespace testing
}  // namespace flutter
