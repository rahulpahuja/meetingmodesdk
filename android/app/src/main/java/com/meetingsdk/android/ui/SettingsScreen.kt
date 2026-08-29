package com.meetingsdk.android.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.FilterChip
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.meetingsdk.android.UiState
import com.meetingsdk.model.ProviderMode

// core::AIProviderConfig has five independently-configurable slots (stt/llm/translation/
// embeddings/diarization) so that enabling cloud for one capability never implicitly enables it
// for another. This demo app only lets you flip the one slot the C API actually gates
// (translation, via GatedTranslator) — the rest are shown as fixed/unavailable rather than as
// working-looking toggles that don't actually change anything native.
private data class FixedCapability(val name: String, val mode: String)

private val fixedCapabilities = listOf(
    FixedCapability("Speech-to-text", "On-device (demo fixture — no real STT model ships yet)"),
    FixedCapability("Diarization", "On-device (demo fixture — no real diarizer model ships yet)"),
    FixedCapability("Meeting intelligence", "On-device (HeuristicLlmEngine)"),
    FixedCapability("Embeddings", "Not implemented (no multilingual embedding model available)"),
)

@Composable
fun SettingsScreen(
    state: UiState,
    onTranslationModeChange: (ProviderMode) -> Unit,
    onTranslateInputChange: (String) -> Unit,
    onRunTranslate: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(modifier = modifier.fillMaxSize().padding(16.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Text("Settings", style = MaterialTheme.typography.headlineSmall)
        Text("core::AIProviderConfig, one row per capability.", style = MaterialTheme.typography.bodySmall)

        fixedCapabilities.forEach { capability ->
            Column {
                Text(capability.name, style = MaterialTheme.typography.titleSmall)
                Text(capability.mode, style = MaterialTheme.typography.bodySmall)
            }
        }

        HorizontalDivider()

        Text("Translation", style = MaterialTheme.typography.titleSmall)
        Text(
            "The only capability this demo actually gates: translation::GatedTranslator rejects " +
                "every call when Disabled, before it ever reaches the on-device dictionary " +
                "translator. No cloud translator ships in this SDK, so Cloud isn't offered here.",
            style = MaterialTheme.typography.bodySmall,
        )
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            FilterChip(
                selected = state.translationMode == ProviderMode.ON_DEVICE,
                onClick = { onTranslationModeChange(ProviderMode.ON_DEVICE) },
                label = { Text("On-device") },
            )
            FilterChip(
                selected = state.translationMode == ProviderMode.DISABLED,
                onClick = { onTranslationModeChange(ProviderMode.DISABLED) },
                label = { Text("Disabled") },
            )
        }

        OutlinedTextField(
            value = state.translateInput,
            onValueChange = onTranslateInputChange,
            label = { Text("Text to translate to English") },
            modifier = Modifier.fillMaxWidth(),
        )
        Button(onClick = onRunTranslate) { Text("Translate") }

        state.translateOutput?.let { result ->
            result.fold(
                onSuccess = { Text("Result: $it") },
                onFailure = { Text("Rejected: ${it.message}", color = MaterialTheme.colorScheme.error) },
            )
        }
    }
}
