plugins {
    kotlin("multiplatform") version "2.1.20"
    kotlin("plugin.serialization") version "2.1.20"
}

repositories {
    mavenCentral()
}

kotlin {
    // `expect object NativeBridge` is the one intentional expect/actual *class* in this module;
    // the Beta-warning it triggers is just noise here.
    compilerOptions {
        freeCompilerArgs.add("-Xexpect-actual-classes")
    }

    // One shared module, two consumers: Android/desktop via JVM+JNI (bindings/kotlin/jni),
    // Apple via Kotlin/Native cinterop straight onto the same stable C ABI (bindings/c). All
    // logic lives in commonMain; each platform supplies only the thin `actual` NativeBridge.
    jvm()

    listOf(iosArm64(), iosSimulatorArm64(), iosX64()).forEach { target ->
        target.compilations.getByName("main").cinterops.create("meeting_sdk_c") {
            definitionFile.set(project.file("src/nativeInterop/cinterop/meeting_sdk_c.def"))
            includeDirs(project.file("../bindings/c/include"))
        }
        // The meeting_sdk_c static library is linked by the consuming Xcode project (the same way
        // the Android app links the JNI .so), so no staticLibrary is wired in here.
    }

    sourceSets {
        commonMain.dependencies {
            implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.7.3")
        }
        commonTest.dependencies {
            implementation(kotlin("test"))
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
