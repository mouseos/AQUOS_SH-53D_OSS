/*
 * Copyright 2024 Google LLC
 */

#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// bindgen doesn't create constants for macros that expand to other macros
// https://github.com/Smithay/drm-rs/blob/328742fddc675b3370057b382eb54acbc9b48c79/drm-ffi/drm-sys/build.rs#L110

const unsigned int __BINDGEN_TMP_DRM_MODE_PROP_OBJECT = DRM_MODE_PROP_OBJECT;
#undef DRM_MODE_PROP_OBJECT
const unsigned int DRM_MODE_PROP_OBJECT = __BINDGEN_TMP_DRM_MODE_PROP_OBJECT;
#define DRM_MODE_PROP_OBJECT DRM_MODE_PROP_OBJECT

const unsigned int __BINDGEN_TMP_DRM_MODE_PROP_SIGNED_RANGE =
    DRM_MODE_PROP_SIGNED_RANGE;
#undef DRM_MODE_PROP_SIGNED_RANGE
const unsigned int DRM_MODE_PROP_SIGNED_RANGE =
    __BINDGEN_TMP_DRM_MODE_PROP_SIGNED_RANGE;
#define DRM_MODE_PROP_SIGNED_RANGE DRM_MODE_PROP_SIGNED_RANGE
