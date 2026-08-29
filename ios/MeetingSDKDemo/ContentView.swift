import SwiftUI
import MeetingSdkKit

/// Thin bridge over the shared KMP module (MeetingSdkKit.xcframework). Every call here crosses
/// Swift -> Kotlin/Native -> the same C ABI (bindings/c) -> the C++ core that the Android app
/// drives over JNI. No business logic lives on the Swift side.
@MainActor
final class MeetingStore: ObservableObject {
    private let repository: NativeMeetingRepository
    private let search: NativeSearchService

    @Published var meeting: Meeting?
    @Published var meetingIds: [String] = []
    @Published var searchResults: [SearchResult] = []
    @Published var translationOutput: String = ""
    @Published var status: String = ""

    init() {
        let dir = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
        let dbPath = (dir as NSString).appendingPathComponent("meetings.db")
        repository = NativeMeetingRepository(dbPath: dbPath)
        search = NativeSearchService(repository: repository)
        refreshIds()
    }

    func refreshIds() {
        meetingIds = repository.listMeetingIds()
    }

    func runDemoMeeting() {
        let id = "meeting-\(Int(Date().timeIntervalSince1970 * 1000))"
        let ok = repository.runDemoMeeting(id: id)
        status = ok ? "pipeline completed" : "pipeline failed"
        if ok {
            meeting = repository.getMeeting(id: id)
            refreshIds()
        }
    }

    func runSearch(_ query: String) {
        searchResults = query.isEmpty ? [] : search.search(queryText: query)
    }

    func translate(_ text: String) {
        translationOutput = AppleInteropKt.translateOnDeviceOrNull(text: text, targetBcp47: "en")
            ?? "rejected / no translation"
    }
}

struct ContentView: View {
    @StateObject private var store = MeetingStore()
    @State private var searchQuery = "launch"
    @State private var translateInput = "namaste, kaise hain aap"

    var body: some View {
        NavigationStack {
            List {
                Section("Synthetic demo") {
                    Text("Runs the real orchestration::MeetingPipeline end-to-end over a synthetic "
                        + "utterance. Language detection and meeting-intelligence extraction are the "
                        + "SDK's real heuristic engines.")
                        .font(.footnote).foregroundStyle(.secondary)
                    Button("Run demo meeting") { store.runDemoMeeting() }
                    if !store.status.isEmpty { Text(store.status).font(.footnote) }
                }

                if let meeting = store.meeting {
                    Section("Result — \(meeting.id)") {
                        Text("State: \(meeting.state.name)")
                        ForEach(meeting.transcript, id: \.id) { seg in
                            Text("\(seg.speakerId): \(seg.text)").font(.callout)
                        }
                        if let summary = meeting.summary {
                            Text("Summary: \(summary.text)").font(.callout)
                        }
                        ForEach(Array(meeting.decisions.enumerated()), id: \.offset) { _, d in
                            Text("Decision: \(d.text)").font(.callout)
                        }
                        ForEach(Array(meeting.actionItems.enumerated()), id: \.offset) { _, a in
                            Text("Action: \(a.action)").font(.callout)
                        }
                    }
                }

                Section("Search") {
                    TextField("query", text: $searchQuery)
                    Button("Search") { store.runSearch(searchQuery) }
                    ForEach(Array(store.searchResults.enumerated()), id: \.offset) { _, r in
                        Text("\(r.meetingId) / \(r.segmentId) — score \(r.score)").font(.callout)
                    }
                }

                Section("Translate (on-device)") {
                    TextField("text", text: $translateInput)
                    Button("Translate") { store.translate(translateInput) }
                    if !store.translationOutput.isEmpty {
                        Text(store.translationOutput).font(.callout)
                    }
                }

                Section("Saved meetings") {
                    ForEach(store.meetingIds, id: \.self) { Text($0).font(.caption) }
                }
            }
            .navigationTitle("Meeting SDK")
            .task {
                // `-autorun` exercises the whole SDK surface on launch, no taps — used to smoke-test
                // the framework against a simulator.
                if ProcessInfo.processInfo.arguments.contains("-autorun") {
                    store.runDemoMeeting()
                    store.runSearch(searchQuery)
                    store.translate(translateInput)
                }
            }
        }
    }
}
