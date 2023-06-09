// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the thirdpartylicense.txt file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "talk/owt/sdk/base/cameravideocapturer.h"

#include <algorithm>

#include "api/scoped_refptr.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"

// This file is borrowed from webrtc project
namespace owt {
namespace base {
CameraVideoCapturer::CameraVideoCapturer() = default;
CameraVideoCapturer::~CameraVideoCapturer() = default;

void CameraVideoCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  int cropped_width = 0;
  int cropped_height = 0;
  int out_width = 0;
  int out_height = 0;

  if (!video_adapter_.AdaptFrameResolution(
          frame.width(), frame.height(), frame.timestamp_us() * 1000,
          &cropped_width, &cropped_height, &out_width, &out_height)) {
    // Drop frame in order to respect frame rate constraint.
    return;
  }

  if (out_height != frame.height() || out_width != frame.width()) {
    // Video adapter has requested a down-scale. Allocate a new buffer and
    // return scaled version.
    rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
        webrtc::I420Buffer::Create(out_width, out_height);
    scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());
    broadcaster_.OnFrame(webrtc::VideoFrame::Builder()
                             .set_video_frame_buffer(scaled_buffer)
                             .set_rotation(webrtc::kVideoRotation_0)
                             .set_timestamp_us(frame.timestamp_us())
                             .set_id(frame.id())
                             .build());
  } else {
    // No adaptations needed, just return the frame as is.
    broadcaster_.OnFrame(frame);
  }
}

rtc::VideoSinkWants CameraVideoCapturer::GetSinkWants() {
  return broadcaster_.wants();
}

void CameraVideoCapturer::AddOrUpdateSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
  broadcaster_.AddOrUpdateSink(sink, wants);
  UpdateVideoAdapter();
}

void CameraVideoCapturer::RemoveSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
  broadcaster_.RemoveSink(sink);
  UpdateVideoAdapter();
}

void CameraVideoCapturer::UpdateVideoAdapter() {
  rtc::VideoSinkWants wants = broadcaster_.wants();
  video_adapter_.OnResolutionFramerateRequest(
      wants.target_pixel_count, wants.max_pixel_count, wants.max_framerate_fps);
}

}  // namespace test
}  // namespace webrtc
