package com.meetingsdk.android

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.meetingsdk.model.Meeting
import com.meetingsdk.model.ProviderMode
import com.meetingsdk.model.SearchResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

data class UiState(
    val meetingIds: List<String> = emptyList(),
    val selectedMeetingId: String? = null,
    val selectedMeeting: Meeting? = null,
    val isRunningDemoMeeting: Boolean = false,
    val recordError: String? = null,
    val isLiveRecording: Boolean = false,
    val liveUnavailable: Boolean = false,
    val livePartialText: String = "",
    val liveUtteranceCount: Int = 0,
    val acousticDiarizationDegraded: Boolean = false,
    val searchQuery: String = "",
    val searchResults: List<SearchResult> = emptyList(),
    val translationMode: ProviderMode = ProviderMode.ON_DEVICE,
    val translateInput: String = "namaste, kaise hain aap",
    val translateOutput: Result<String>? = null,
)

/**
 * Owns the one native repository/search/translation handle for the app's lifetime — the native
 * bindings are not thread-safe, so every call is funneled through Dispatchers.IO one at a time
 * from here rather than from each screen directly. AndroidViewModel (not plain ViewModel) only
 * because live recording needs a Context for LiveSpeechRecorder/SpeechRecognizer.
 */
class MeetingSdkViewModel(application: Application, dbPath: String) : AndroidViewModel(application) {
    private val repository = NativeMeetingRepository(dbPath)
    private val searchService = NativeSearchService(repository)
    private val translationService = NativeTranslationService()

    private var livePipeline: NativeMeetingPipeline? = null
    private var liveRecorder: LiveSpeechRecorder? = null
    private var speakerTagger: AcousticSpeakerTagger? = null

    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState

    init {
        refreshMeetingIds()
    }

    private fun refreshMeetingIds() {
        viewModelScope.launch(Dispatchers.IO) {
            val ids = repository.listMeetingIds()
            _uiState.update { it.copy(meetingIds = ids) }
        }
    }

    fun runDemoMeeting() {
        viewModelScope.launch {
            _uiState.update { it.copy(isRunningDemoMeeting = true, recordError = null) }
            val id = "meeting-${System.currentTimeMillis()}"
            val (ok, meeting) = withContext(Dispatchers.IO) {
                val ok = repository.runDemoMeeting(id)
                ok to (if (ok) repository.getMeeting(id) else null)
            }
            _uiState.update {
                it.copy(
                    isRunningDemoMeeting = false,
                    recordError = if (ok) null else "pipeline failed to complete",
                    selectedMeetingId = if (ok) id else it.selectedMeetingId,
                    selectedMeeting = meeting ?: it.selectedMeeting,
                )
            }
            if (ok) refreshMeetingIds()
        }
    }

    /**
     * Starts a real recording session: Android's on-device SpeechRecognizer captures + transcribes
     * live speech (see LiveSpeechRecorder), each recognized utterance is fed into a fresh
     * MeetingPipeline via ingestUtterance — real language detection and intelligence extraction
     * follow the same path as [runDemoMeeting], just over real transcript text instead of a fixture.
     * Speaker labeling is automatic via [AcousticSpeakerTagger] (real acoustic clustering when a
     * concurrent AudioRecord capture is available on this device, an honest same-speaker-as-last
     * fallback when it isn't — see that class's doc comment). Call sites must already hold
     * RECORD_AUDIO before calling this.
     */
    fun startLiveRecording() {
        if (_uiState.value.isLiveRecording) return
        val id = "meeting-${System.currentTimeMillis()}"
        val pipeline = NativeMeetingPipeline(repository, id)
        livePipeline = pipeline
        val tagger = AcousticSpeakerTagger()
        speakerTagger = tagger
        viewModelScope.launch(Dispatchers.IO) {
            pipeline.start()
            // Deliberately NOT tagger.start(): a second concurrent AudioRecord on the mic
            // starves Android's on-device SpeechRecognizer (SODA hits MIC_END_OF_DATA and
            // cancels every session before delivering onResults). The recognizer owns the mic;
            // AcousticSpeakerTagger runs in its no-capture fallback ("same speaker as last
            // utterance"), which its own doc comment already describes.
        }

        _uiState.update {
            it.copy(
                isLiveRecording = true,
                liveUnavailable = false,
                livePartialText = "",
                liveUtteranceCount = 0,
                acousticDiarizationDegraded = false,
                recordError = null,
                selectedMeetingId = id,
            )
        }

        liveRecorder = LiveSpeechRecorder(
            context = getApplication<Application>(),
            onPartial = { partial -> _uiState.update { it.copy(livePartialText = partial) } },
            onUtterance = { text, startMs, endMs ->
                viewModelScope.launch(Dispatchers.IO) {
                    val speakerId = tagger.assignSpeaker(startMs, endMs)
                    pipeline.ingestUtterance(text, startMs, endMs, speakerId, 0.9F)
                }
                _uiState.update {
                    it.copy(
                        livePartialText = "",
                        liveUtteranceCount = it.liveUtteranceCount + 1,
                        acousticDiarizationDegraded = !tagger.concurrentCaptureReliable,
                    )
                }
            },
            onUnavailable = {
                _uiState.update { it.copy(isLiveRecording = false, liveUnavailable = true) }
                livePipeline = null
                speakerTagger = null
                viewModelScope.launch(Dispatchers.IO) {
                    pipeline.destroy()
                    tagger.stop()
                }
            },
            onError = { message ->
                _uiState.update { it.copy(isLiveRecording = false, recordError = message) }
            },
        ).also { it.start() }
    }

    fun stopLiveRecording() {
        val pipeline = livePipeline ?: return
        val tagger = speakerTagger
        liveRecorder?.stop()
        liveRecorder = null
        livePipeline = null
        speakerTagger = null
        viewModelScope.launch(Dispatchers.IO) {
            val meeting = pipeline.stop()
            pipeline.destroy()
            tagger?.stop()
            val ids = repository.listMeetingIds()
            withContext(Dispatchers.Main) {
                _uiState.update {
                    it.copy(
                        isLiveRecording = false,
                        recordError = if (meeting == null) "live pipeline failed to complete" else null,
                        selectedMeeting = meeting ?: it.selectedMeeting,
                        meetingIds = ids,
                    )
                }
            }
        }
    }

    fun selectMeeting(id: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val meeting = repository.getMeeting(id)
            _uiState.update { it.copy(selectedMeetingId = id, selectedMeeting = meeting) }
        }
    }

    fun updateSearchQuery(query: String) {
        _uiState.update { it.copy(searchQuery = query) }
    }

    fun runSearch() {
        val query = _uiState.value.searchQuery
        viewModelScope.launch(Dispatchers.IO) {
            val results = searchService.search(query)
            _uiState.update { it.copy(searchResults = results) }
        }
    }

    fun setTranslationMode(mode: ProviderMode) {
        _uiState.update { it.copy(translationMode = mode, translateOutput = null) }
    }

    fun updateTranslateInput(text: String) {
        _uiState.update { it.copy(translateInput = text) }
    }

    fun runTranslate() {
        val text = _uiState.value.translateInput
        val mode = _uiState.value.translationMode
        viewModelScope.launch(Dispatchers.IO) {
            val result = translationService.translate(text, "en", mode)
            _uiState.update { it.copy(translateOutput = result) }
        }
    }

    override fun onCleared() {
        liveRecorder?.stop()
        livePipeline?.destroy()
        speakerTagger?.stop()
        repository.close()
    }
}
