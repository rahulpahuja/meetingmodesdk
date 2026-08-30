import org.jetbrains.kotlin.gradle.plugin.mpp.apple.XCFramework

plugins {
    kotlin("multiplatform") version "2.1.20"
    kotlin("plugin.serialization") version "2.1.20"
    `maven-publish`
}

group = "com.meetingsdk"
version = "0.1.0"

repositories {
    mavenCentral()
}

// `gradle publishToMavenLocal` makes the shared module resolvable as
// com.meetingsdk:kmp:0.1.0 (all published targets: jvm, iosArm64, iosSimulatorArm64) — the
// kotlin("multiplatform") plugin wires the per-target publications automatically.
publishing {
    repositories {
        maven {
            name = "GitHubPackages"
            url = uri("https://maven.pkg.github.com/rahulpahuja/meetingmodesdk")
            credentials {
                username = providers.gradleProperty("gpr.user").orElse(providers.environmentVariable("GITHUB_ACTOR")).orNull
                password = providers.gradleProperty("gpr.key").orElse(providers.environmentVariable("GITHUB_TOKEN")).orNull
            }
        }
    }
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

    // MeetingSdkKit.xcframework — the finished artifact an Xcode app imports. Each iOS target's
    // framework binary statically links ../ios/native/<sdk>/libMeetingSdkNative.a (the whole C++
    // core + C ABI + sqlite3 + libsodium, cross-compiled by ios/scripts/build-native.sh), so the
    // framework is self-contained and the app needs no CMake step of its own.
    val xcf = XCFramework("MeetingSdkKit")
    listOf(iosArm64(), iosSimulatorArm64()).forEach { target ->
        val nativeSdk = if (target.name == "iosArm64") "iphoneos" else "iphonesimulator"
        target.compilations.getByName("main").cinterops.create("meeting_sdk_c") {
            definitionFile.set(project.file("src/nativeInterop/cinterop/meeting_sdk_c.def"))
            includeDirs(project.file("../bindings/c/include"))
        }
        target.binaries.framework {
            baseName = "MeetingSdkKit"
            isStatic = false
            linkerOpts(
                "-L${project.file("../ios/native/$nativeSdk").absolutePath}",
                "-lMeetingSdkNative",
                "-lc++",
            )
            xcf.add(this)
        }
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
