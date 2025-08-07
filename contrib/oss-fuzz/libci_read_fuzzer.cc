// libci_read_fuzzer.cc
// Copyright 2017-2018 Glenn Randers-Pehrson
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that may
// be found in the LICENSE file https://cs.chromium.org/chromium/src/LICENSE

// The modifications in 2017 by Glenn Randers-Pehrson include
// 1. addition of a CI_CLEANUP macro,
// 2. setting the option to ignore ADLER32 checksums,
// 3. adding "#include <string.h>" which is needed on some platforms
//    to provide memcpy().
// 4. adding read_end_info() and creating an end_info structure.
// 5. adding calls to ci_set_*() transforms commonly used by browsers.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#define CI_INTERNAL
#include "ci.h"

#define CI_CLEANUP \
  if(ci_handler.ci_ptr) \
  { \
    if (ci_handler.row_ptr) \
      ci_free(ci_handler.ci_ptr, ci_handler.row_ptr); \
    if (ci_handler.end_info_ptr) \
      ci_destroy_read_struct(&ci_handler.ci_ptr, &ci_handler.info_ptr,\
        &ci_handler.end_info_ptr); \
    else if (ci_handler.info_ptr) \
      ci_destroy_read_struct(&ci_handler.ci_ptr, &ci_handler.info_ptr,\
        nullptr); \
    else \
      ci_destroy_read_struct(&ci_handler.ci_ptr, nullptr, nullptr); \
    ci_handler.ci_ptr = nullptr; \
    ci_handler.row_ptr = nullptr; \
    ci_handler.info_ptr = nullptr; \
    ci_handler.end_info_ptr = nullptr; \
  }

struct BufState {
  const uint8_t* data;
  size_t bytes_left;
};

struct CiObjectHandler {
  ci_infop info_ptr = nullptr;
  ci_structp ci_ptr = nullptr;
  ci_infop end_info_ptr = nullptr;
  ci_voidp row_ptr = nullptr;
  BufState* buf_state = nullptr;

  ~CiObjectHandler() {
    if (row_ptr)
      ci_free(ci_ptr, row_ptr);
    if (end_info_ptr)
      ci_destroy_read_struct(&ci_ptr, &info_ptr, &end_info_ptr);
    else if (info_ptr)
      ci_destroy_read_struct(&ci_ptr, &info_ptr, nullptr);
    else
      ci_destroy_read_struct(&ci_ptr, nullptr, nullptr);
    delete buf_state;
  }
};

void user_read_data(ci_structp ci_ptr, ci_bytep data, size_t length) {
  BufState* buf_state = static_cast<BufState*>(ci_get_io_ptr(ci_ptr));
  if (length > buf_state->bytes_left) {
    ci_error(ci_ptr, "read error");
  }
  memcpy(data, buf_state->data, length);
  buf_state->bytes_left -= length;
  buf_state->data += length;
}

void* limited_malloc(ci_structp, ci_alloc_size_t size) {
  // libci may allocate large amounts of memory that the fuzzer reports as
  // an error. In order to silence these errors, make libci fail when trying
  // to allocate a large amount. This allocator used to be in the Chromium
  // version of this fuzzer.
  // This number is chosen to match the default ci_user_chunk_malloc_max.
  if (size > 8000000)
    return nullptr;

  return malloc(size);
}

void default_free(ci_structp, ci_voidp ptr) {
  return free(ptr);
}

static const int kCiHeaderSize = 8;

// Entry point for LibFuzzer.
// Roughly follows the libci book example:
// http://www.libci.org/pub/ci/book/chapter13.html
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < kCiHeaderSize) {
    return 0;
  }

  std::vector<unsigned char> v(data, data + size);
  if (ci_sig_cmp(v.data(), 0, kCiHeaderSize)) {
    // not a CI.
    return 0;
  }

  CiObjectHandler ci_handler;
  ci_handler.ci_ptr = nullptr;
  ci_handler.row_ptr = nullptr;
  ci_handler.info_ptr = nullptr;
  ci_handler.end_info_ptr = nullptr;

  ci_handler.ci_ptr = ci_create_read_struct
    (CI_LIBCI_VER_STRING, nullptr, nullptr, nullptr);
  if (!ci_handler.ci_ptr) {
    return 0;
  }

  ci_handler.info_ptr = ci_create_info_struct(ci_handler.ci_ptr);
  if (!ci_handler.info_ptr) {
    CI_CLEANUP
    return 0;
  }

  ci_handler.end_info_ptr = ci_create_info_struct(ci_handler.ci_ptr);
  if (!ci_handler.end_info_ptr) {
    CI_CLEANUP
    return 0;
  }

  // Use a custom allocator that fails for large allocations to avoid OOM.
  ci_set_mem_fn(ci_handler.ci_ptr, nullptr, limited_malloc, default_free);

  ci_set_crc_action(ci_handler.ci_ptr, CI_CRC_QUIET_USE, CI_CRC_QUIET_USE);
#ifdef CI_IGNORE_ADLER32
  ci_set_option(ci_handler.ci_ptr, CI_IGNORE_ADLER32, CI_OPTION_ON);
#endif

  // Setting up reading from buffer.
  ci_handler.buf_state = new BufState();
  ci_handler.buf_state->data = data + kCiHeaderSize;
  ci_handler.buf_state->bytes_left = size - kCiHeaderSize;
  ci_set_read_fn(ci_handler.ci_ptr, ci_handler.buf_state, user_read_data);
  ci_set_sig_bytes(ci_handler.ci_ptr, kCiHeaderSize);

  if (setjmp(ci_jmpbuf(ci_handler.ci_ptr))) {
    CI_CLEANUP
    return 0;
  }

  // Reading.
  ci_read_info(ci_handler.ci_ptr, ci_handler.info_ptr);

  // reset error handler to put ci_deleter into scope.
  if (setjmp(ci_jmpbuf(ci_handler.ci_ptr))) {
    CI_CLEANUP
    return 0;
  }

  ci_uint_32 width, height;
  int bit_depth, color_type, interlace_type, compression_type;
  int filter_type;

  if (!ci_get_IHDR(ci_handler.ci_ptr, ci_handler.info_ptr, &width,
                    &height, &bit_depth, &color_type, &interlace_type,
                    &compression_type, &filter_type)) {
    CI_CLEANUP
    return 0;
  }

  // This is going to be too slow.
  if (width && height > 100000000 / width) {
    CI_CLEANUP
    return 0;
  }

  // Set several transforms that browsers typically use:
  ci_set_gray_to_rgb(ci_handler.ci_ptr);
  ci_set_expand(ci_handler.ci_ptr);
  ci_set_packing(ci_handler.ci_ptr);
  ci_set_scale_16(ci_handler.ci_ptr);
  ci_set_tRNS_to_alpha(ci_handler.ci_ptr);

  int passes = ci_set_interlace_handling(ci_handler.ci_ptr);

  ci_read_update_info(ci_handler.ci_ptr, ci_handler.info_ptr);

  ci_handler.row_ptr = ci_malloc(
      ci_handler.ci_ptr, ci_get_rowbytes(ci_handler.ci_ptr,
                                            ci_handler.info_ptr));

  for (int pass = 0; pass < passes; ++pass) {
    for (ci_uint_32 y = 0; y < height; ++y) {
      ci_read_row(ci_handler.ci_ptr,
                   static_cast<ci_bytep>(ci_handler.row_ptr), nullptr);
    }
  }

  ci_read_end(ci_handler.ci_ptr, ci_handler.end_info_ptr);

  CI_CLEANUP

#ifdef CI_SIMPLIFIED_READ_SUPPORTED
  // Simplified READ API
  ci_image image;
  memset(&image, 0, (sizeof image));
  image.version = CI_IMAGE_VERSION;

  if (!ci_image_begin_read_from_memory(&image, data, size)) {
    return 0;
  }

  image.format = CI_FORMAT_RGBA;
  std::vector<ci_byte> buffer(CI_IMAGE_SIZE(image));
  ci_image_finish_read(&image, NULL, buffer.data(), 0, NULL);
#endif

  return 0;
}
