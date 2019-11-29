// Copyright 2019 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_CANVAS_TEST_H_
#define TESTING_CANVAS_TEST_H_

#include "flutter/fml/macros.h"
#include "flutter/testing/mock_canvas.h"
#include "third_party/skia/include/core/SkRect.h"

namespace flutter {
namespace testing {

static constexpr SkRect kEmptyRect = SkRect::MakeEmpty();

// This fixture allows creating tests that make use of a mock |SkCanvas|.
template <typename BaseT>
class CanvasTest : public BaseT {
 public:
  CanvasTest() = default;

  MockCanvas& mock_canvas() { return canvas_; }

 private:
  MockCanvas canvas_;

  FML_DISALLOW_COPY_AND_ASSIGN(CanvasTest);
};

}  // namespace testing
}  // namespace flutter

#endif  // TESTING_CANVAS_TEST_H_
