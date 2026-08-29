package com.meetingsdk.android.ui

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.meetingsdk.android.UiState

@Composable
fun RecordScreen(
    state: UiState,
    onRunDemoMeeting: () -> Unit,
    onStartLiveRecording: () -> Unit,
    onStopLiveRecording: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val context = LocalContext.current
    var hasRecordAudioPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(context, Manifest.permission.RECORD_AUDIO) ==
                PackageManager.PERMISSION_GRANTED,
        )
    }
    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission(),
    ) { granted -> hasRecordAudioPermission = granted }

    Column(
        modifier = modifier.fillMaxSize().padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("Record", style = MaterialTheme.typography.headlineSmall)

        Text("Live speech", style = MaterialTheme.typography.titleMedium)
        Text(
            "Real microphone capture + real transcription via Android's on-device SpeechRecognizer, " +
                "fed into the real orchestration::MeetingPipeline. Requires the device to have an " +
                "on-device speech model installed (API 31+); no fallback to cloud recognition. " +
                "Speakers are labeled automatically (\"speaker-1\", \"speaker-2\", ...) using a real, " +
                "non-neural acoustic feature vector (pitch, energy, zero-crossing rate) clustered by " +
                "the SDK's speaker::SpeakerClusterer — no neural voice-embedding model ships in this " +
                "SDK, so this is a crude but genuine voiceprint, not a fabricated one.",
            style = MaterialTheme.typography.bodyMedium,
        )
        if (!hasRecordAudioPermission) {
            Button(onClick = { permissionLauncher.launch(Manifest.permission.RECORD_AUDIO) }) {
                Text("Grant microphone permission")
            }
        } else if (state.isLiveRecording) {
            Button(onClick = onStopLiveRecording) { Text("Stop recording") }
            Text("Listening… ${state.liveUtteranceCount} utterance(s) captured")
            if (state.livePartialText.isNotBlank()) {
                Text("Hearing: \"${state.livePartialText}\"", style = MaterialTheme.typography.bodyMedium)
            }
            if (state.acousticDiarizationDegraded) {
                Text(
                    "No reliable microphone signal alongside SpeechRecognizer on this device — " +
                        "speaker labels are falling back to \"same speaker as last utterance\" " +
                        "instead of real acoustic clustering.",
                    color = MaterialTheme.colorScheme.error,
                )
            }
        } else {
            Button(onClick = onStartLiveRecording) { Text("Start recording") }
        }
        if (state.liveUnavailable) {
            Text(
                "On-device speech recognition isn't available on this device — no fallback to " +
                    "cloud recognition was attempted.",
                color = MaterialTheme.colorScheme.error,
            )
        }

        HorizontalDivider()

        Text("Synthetic demo", style = MaterialTheme.typography.titleMedium)
        Text(
            "There is no real diarizer model in meeting-sdk yet (Milestone 9-11 providers work). " +
                "This runs the real orchestration::MeetingPipeline end-to-end over a short synthetic " +
                "utterance, using the same hand-written STT/diarization fakes the SDK's own pipeline " +
                "tests use. Language detection and meeting-intelligence extraction (summary, action " +
                "items, decisions, topics, questions) are the SDK's real heuristic engines.",
            style = MaterialTheme.typography.bodyMedium,
        )
        Button(onClick = onRunDemoMeeting, enabled = !state.isRunningDemoMeeting) {
            Text(if (state.isRunningDemoMeeting) "Running pipeline..." else "Run demo meeting")
        }
        if (state.isRunningDemoMeeting) {
            CircularProgressIndicator()
        }

        state.recordError?.let { Text("Error: $it", color = MaterialTheme.colorScheme.error) }
        state.selectedMeeting?.let { meeting ->
            Text("Last run: ${meeting.id} — state ${meeting.state}", style = MaterialTheme.typography.titleMedium)
            Text("Open the Transcript tab to see the full result.")
        }
    }
}
