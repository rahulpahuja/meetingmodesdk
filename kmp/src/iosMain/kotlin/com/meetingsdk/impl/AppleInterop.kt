package com.meetingsdk.impl

/**
 * Small Obj-C/Swift-facing conveniences. `kotlin.Result` has no representation in the Kotlin/Native
 * Obj-C export, so [NativeTranslationService.translate] is invisible from Swift — this exposes the
 * same on-device translation as a plain nullable String instead.
 */
fun translateOnDeviceOrNull(text: String, targetBcp47: String): String? =
    NativeTranslationService().translate(text, targetBcp47).getOrNull()
