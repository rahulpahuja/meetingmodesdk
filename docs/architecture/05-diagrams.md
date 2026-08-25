# Architecture Diagrams

Ten diagrams covering the Meeting Mode SDK's structure, pipeline, platform integrations, data
model, and security boundaries. This is a companion reference for Milestone 0 — each diagram is
architecture-only (no implementation code), with brief prose for orientation only.

## 1. System architecture

The SDK is layered: platform apps talk to a Kotlin Multiplatform shared layer, which crosses into
native code through a stable C API, into the C++ core, which in turn drives pluggable providers.

```mermaid
flowchart TB
    subgraph AND["Android App"]
        AUI[App UI / ViewModels]
    end
    subgraph IOS["iOS App"]
        IUI[App UI / SwiftUI-VM]
    end
    subgraph KMP["KMP Shared Layer"]
        KAPI[Common Kotlin API]
    end
    CAPI[["C API Boundary\n(stable ABI)"]]
    subgraph CORE["C++ Core SDK"]
        ENGINE[Meeting Engine\ndomain + orchestration]
    end
    subgraph PROV["Providers"]
        OD[On-Device Providers]
        CLOUD[Cloud Providers\nopt-in]
    end

    AUI --> KAPI
    IUI --> KAPI
    KAPI --> CAPI
    CAPI --> ENGINE
    ENGINE --> OD
    ENGINE --> CLOUD
```

## 2. C++ module dependency graph

Modules form a DAG: every module depends on `core/domain`, feature modules never depend on each
other, and `orchestration` is the only module allowed to see across features. `platform/android`
and `platform/ios` are deliberately **not** part of this tree — per `01-requirements-and-architecture.md` §3,
keeping them inside the C++ SDK would violate the requirement that the core build independently of
Android/iOS. Instead, the platform apps implement `core/interfaces::IAudioSource` and inject it in;
`bindings/kotlin,swift` marshal across the JNI/Obj-C++ boundary and depend on `orchestration`'s
facade, but nothing in this graph depends on them.

```mermaid
flowchart LR
    CORE["core/domain (incl. IAudioSource)"]
    AUDIO[audio] --> CORE
    SPEECH[speech] --> CORE
    SPEAKER[speaker] --> CORE
    TRANSLATION[translation] --> CORE
    INTEL[intelligence] --> CORE
    SEARCH[search] --> CORE
    STORAGE[storage] --> CORE
    PROV_OD["providers/on_device"] --> CORE
    PROV_CLOUD["providers/cloud"] --> CORE
    ORCH[orchestration] --> AUDIO
    ORCH --> SPEECH
    ORCH --> SPEAKER
    ORCH --> TRANSLATION
    ORCH --> INTEL
    ORCH --> SEARCH
    ORCH --> STORAGE
    ORCH --> CORE
    PROV_OD -. implements .-> SPEECH
    PROV_OD -. implements .-> SPEAKER
    PROV_CLOUD -. implements .-> TRANSLATION
    PROV_CLOUD -. implements .-> INTEL

    subgraph EXT["Outside the C++ SDK tree"]
        PLATFORM["Android app / iOS app\n(implements IAudioSource;\nowns permissions, foreground\nservice, AVAudioEngine, lifecycle)"]
        BINDINGS["bindings/kotlin,swift\n(JNI / Obj-C++, marshal only)"]
    end
    PLATFORM -. implements .-> CORE
    PLATFORM --> BINDINGS
    BINDINGS --> ORCH
```

## 3. Meeting processing pipeline

The full pipeline from raw audio to searchable transcript. At each pluggable stage — STT here as
the representative example — the Strategy-pattern `AIProvider` decides on-device vs. cloud at
runtime; the same branch shape repeats for diarization, translation, embeddings, and intelligence.

```mermaid
flowchart TD
    A[Audio Capture] --> B[Audio Buffer]
    B --> C[Noise Reduction]
    C --> D[Voice Activity Detection]
    D --> E[Speech Segmentation]
    E --> F[Language Detection]
    F --> G{Provider Strategy:\nOnDevice or Cloud?}
    G -->|OnDevice| G1[On-Device STT]
    G -->|Cloud, consent required| G2[Cloud STT]
    G1 --> H[Speaker Diarization]
    G2 --> H
    H --> I[Speaker Assignment]
    I --> J[Transcript Assembly]
    J --> K{Translation enabled?}
    K -->|yes| L[Translation]
    K -->|no| M[Embeddings]
    L --> M
    M --> N[Meeting Intelligence\nSummary / Action Items /\nDecisions / Topics / Questions]
    N --> O[Encrypted Storage]
    O --> P[Semantic Search Index]
```

## 4. Audio pipeline (thread boundaries)

The capture thread only ever writes into a lock-free ring buffer and never blocks; a separate
worker thread drains it for VAD and segmentation, keeping real-time audio capture isolated from
variable-latency processing.

```mermaid
flowchart LR
    subgraph RT["Capture Thread (real-time, non-blocking)"]
        CAP[Platform Capture Callback]
    end
    RING[["Lock-free Ring Buffer"]]
    subgraph WORK["VAD Worker Thread"]
        VAD[Voice Activity Detector]
        SEG[Segmenter\nlookahead + hangover]
        VAD --> SEG
    end
    CAP -->|push, non-blocking| RING
    RING -->|drain, non-blocking pop| VAD
    SEG -->|SpeechSegment event| PIPE[Rest of Pipeline]
```

## 5. AI provider architecture

Every pluggable capability is a `AIProvider` interface with independently swappable
implementations. Cloud implementations are never constructed by default; they activate only after
an explicit per-capability consent grant.

```mermaid
flowchart TB
    subgraph SLOTS["AIProvider Slots (per capability)"]
        STTP[STT]
        LLMP[LLM]
        TRP[Translation]
        EMBP[Embeddings]
        DIARP[Diarization]
    end
    STTP --> STT_OD[OnDevice] & STT_CL[Cloud] & STT_CU[Custom]
    LLMP --> LLM_OD[OnDevice] & LLM_CL[Cloud] & LLM_CU[Custom]
    TRP --> TR_OD[OnDevice] & TR_CL[Cloud] & TR_CU[Custom]
    EMBP --> EMB_OD[OnDevice] & EMB_CL[Cloud] & EMB_CU[Custom]
    DIARP --> DIAR_OD[OnDevice] & DIAR_CL[Cloud] & DIAR_CU[Custom]

    STT_CL -. requires .-> CONSENT{{"Explicit per-capability\nconsent gate"}}
    LLM_CL -. requires .-> CONSENT
    TR_CL -. requires .-> CONSENT
    EMB_CL -. requires .-> CONSENT
    DIAR_CL -. requires .-> CONSENT
```

## 6. Android integration

`AudioRecord` runs inside a foreground service so capture survives backgrounding; a thin JNI
bridge marshals data only, with no business logic, into the C++ core.

```mermaid
flowchart TD
    PERM[RECORD_AUDIO Permission]
    AR[AudioRecord]
    SVC[Foreground Service\nnotification + lifecycle]
    JNI[Thin JNI Bridge\nmarshal only, no logic]
    ENGINE[C++ Core]

    PERM --> AR --> SVC --> JNI --> ENGINE
    ENGINE -->|callbacks: partial transcript, state| JNI --> SVC -->|LiveData/Flow| UI[Compose UI]
```

## 7. iOS integration

`AVAudioEngine` taps the input node under a configured audio session; a thin Objective-C++/C-API
bridge marshals buffers and callbacks into the C++ core without embedding logic.

```mermaid
flowchart TD
    ENG[AVAudioEngine Tap]
    SESS[Audio Session\nconfiguration + interruption handling]
    BRIDGE[Thin Objective-C++ / C-API Bridge\nmarshal only, no logic]
    CORE[C++ Core]

    ENG --> SESS --> BRIDGE --> CORE
    CORE -->|callbacks: partial transcript, state| BRIDGE --> UI[SwiftUI]
```

## 8. KMP architecture

Kotlin app code calls a platform-neutral common API; `expect`/`actual` resolves to the native
bridge per platform, where domain models are marshalled across the JNI or Objective-C++ boundary.

```mermaid
flowchart TB
    APP[Kotlin App Code]
    subgraph COMMON["commonMain"]
        API[MeetingEngineApi]
        MODELS[Platform-neutral models:\nMeeting, TranscriptSegment,\nSpeaker, ActionItem, Decision, Summary]
    end
    APP --> API
    API -.expect/actual.-> AACT[androidMain actual]
    API -.expect/actual.-> IACT[iosMain actual]
    AACT -->|marshal models| JNI[JNI Bridge]
    IACT -->|marshal models| OBJCXX[Objective-C++ Bridge]
    JNI --> CORE1[C++ Core]
    OBJCXX --> CORE2[C++ Core]
```

## 9. Data model (ER)

Core entities and their cardinalities: a meeting owns its transcript, speakers, and derived
intelligence; transcript segments carry many-to-many language spans for code-switched speech.

```mermaid
erDiagram
    MEETING ||--o{ TRANSCRIPT_SEGMENT : contains
    MEETING ||--o{ SPEAKER : has
    MEETING ||--|| SUMMARY : produces
    MEETING ||--o{ ACTION_ITEM : produces
    MEETING ||--o{ DECISION : produces
    MEETING ||--o{ TOPIC : produces
    MEETING ||--o{ EMBEDDING : produces
    TRANSCRIPT_SEGMENT }o--o{ LANGUAGE_SPAN : "code-switch spans"
```

## 10. Security / data-flow boundaries

Trust boundaries from microphone hardware down to storage, with cloud as a separate, off-by-default
boundary. The offline path never leaves boundary 3; the cloud path exists only past an explicit
consent gate and carries the minimum data slice for the one consented capability.

```mermaid
flowchart TB
    MIC[["Mic Hardware"]]
    subgraph B1["Boundary: Capture (OS-mediated)"]
        CAP[Audio Capture Buffer]
    end
    subgraph B2["Boundary: C++ Core (in-process)"]
        PIPE[Pipeline: VAD / STT / Diarization]
        subgraph B3["Boundary: On-Device Model Runtime"]
            MODELS[(Bundled models,\nintegrity-checked)]
        end
        PIPE --> MODELS
        DOMAIN[Domain Objects:\nTranscript / Summary / Embeddings]
        PIPE --> DOMAIN
    end
    subgraph B4["Boundary: Encrypted Storage"]
        DB[(Encrypted DB + audio segments)]
    end
    subgraph B5["Boundary: Cloud Adapter (off by default)"]
        ADAPTER[Cloud Provider Adapter]
        CLOUDEP[Cloud Provider Endpoint]
        ADAPTER --> CLOUDEP
    end

    MIC --> CAP --> PIPE
    DOMAIN -->|encrypt with DEK| DB

    DOMAIN -.->|offline path: default, always available| DB
    DOMAIN -.->|cloud path: minimum data slice only| CONSENT{Explicit\nCloud Consent?}
    CONSENT -->|yes| ADAPTER
    CONSENT -->|no: default| BLOCK[No network egress]
```
