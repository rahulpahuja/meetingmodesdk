package com.meetingsdk.jvm

import com.meetingsdk.api.TranslationService

class JvmTranslationService : TranslationService {
    override fun translate(text: String, targetBcp47: String): Result<String> {
        val translated = NativeBridge.translate(text, targetBcp47)
        return if (translated != null) {
            Result.success(translated)
        } else {
            Result.failure(IllegalArgumentException("translation failed for target '$targetBcp47'"))
        }
    }
}
