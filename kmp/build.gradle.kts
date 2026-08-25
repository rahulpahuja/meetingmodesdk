plugins {
    kotlin("multiplatform") version "2.1.20"
    kotlin("plugin.serialization") version "2.1.20"
}

repositories {
    mavenCentral()
}

kotlin {
    // Only the JVM target is built/verified in Milestone 8. androidTarget() and iosArm64()/
    // iosSimulatorArm64() are added in Milestones 9 and 10 respectively, alongside the
    // platform-specific capture/permissions/lifecycle work those milestones own — commonMain
    // below does not need to change when they are.
    jvm()

    sourceSets {
        commonTest.dependencies {
            implementation(kotlin("test"))
        }
        jvmMain.dependencies {
            implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.7.3")
        }
        jvmTest.dependencies {
            implementation(kotlin("test-junit5"))
        }
    }
}

tasks.withType<Test> {
    useJUnitPlatform()
    // Native artifacts are built separately via CMake (see ../bindings/kotlin/jni and
    // ../bindings/c) — this points the JVM at the already-built shared library so
    // System.loadLibrary("meeting_sdk_jni") in NativeBridge can find it.
    systemProperty("java.library.path", file("$rootDir/../bindings/kotlin/jni/build").absolutePath)
}
