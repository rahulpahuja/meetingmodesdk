plugins {
    // AGP 9.x has built-in Kotlin support and rejects org.jetbrains.kotlin.android as redundant
    // (see https://kotl.in/gradle/agp-built-in-kotlin) — only the Kotlin compiler plugins
    // (compose, serialization) are applied explicitly.
    id("com.android.application") version "9.2.1" apply false
    id("org.jetbrains.kotlin.plugin.compose") version "2.4.10" apply false
    id("org.jetbrains.kotlin.plugin.serialization") version "2.4.10" apply false
}
