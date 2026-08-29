package com.meetingsdk.android.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.AssistChip
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.meetingsdk.android.UiState

@Composable
fun TranscriptScreen(state: UiState, onSelectMeeting: (String) -> Unit, modifier: Modifier = Modifier) {
    Column(modifier = modifier.fillMaxSize().padding(16.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Text("Transcript", style = MaterialTheme.typography.headlineSmall)

        if (state.meetingIds.isEmpty()) {
            Text("No meetings saved yet — run one from the Record tab.")
            return@Column
        }

        LazyRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            items(state.meetingIds) { id ->
                AssistChip(onClick = { onSelectMeeting(id) }, label = { Text(id) })
            }
        }

        val meeting = state.selectedMeeting
        if (meeting == null) {
            Text("Select a meeting above.")
            return@Column
        }

        Text("State: ${meeting.state}", style = MaterialTheme.typography.titleMedium)

        LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            item {
                Text("Segments", style = MaterialTheme.typography.titleSmall)
            }
            items(meeting.transcript) { segment ->
                Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    Column(Modifier.padding(12.dp)) {
                        Text(segment.speakerId.ifBlank { "(unassigned speaker)" }, style = MaterialTheme.typography.labelLarge)
                        Text(segment.text)
                    }
                }
            }

            meeting.summary?.let { summary ->
                item {
                    Text("Summary", style = MaterialTheme.typography.titleSmall)
                    Text(summary.text)
                }
            }
            if (meeting.decisions.isNotEmpty()) {
                item { Text("Decisions", style = MaterialTheme.typography.titleSmall) }
                items(meeting.decisions) { Text("• ${it.text}") }
            }
            if (meeting.actionItems.isNotEmpty()) {
                item { Text("Action items", style = MaterialTheme.typography.titleSmall) }
                items(meeting.actionItems) { Text("• ${it.action}") }
            }
            if (meeting.questions.isNotEmpty()) {
                item { Text("Questions", style = MaterialTheme.typography.titleSmall) }
                items(meeting.questions) { Text("• ${it.text} (resolved: ${it.resolved})") }
            }
            if (meeting.topics.isNotEmpty()) {
                item {
                    Text("Topics", style = MaterialTheme.typography.titleSmall)
                    Text(meeting.topics.joinToString(", ") { it.label })
                }
            }
        }
    }
}
