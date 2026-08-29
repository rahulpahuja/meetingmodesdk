package com.meetingsdk.android.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.meetingsdk.android.UiState

@Composable
fun SearchScreen(
    state: UiState,
    onQueryChange: (String) -> Unit,
    onSearch: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(modifier = modifier.fillMaxSize().padding(16.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Text("Search", style = MaterialTheme.typography.headlineSmall)
        Text(
            "Keyword search (search::InvertedIndexSearch) over every saved meeting's transcript. " +
                "Real ranked-by-term-match-count search, not semantic — this SDK has no " +
                "embedding model to do that with yet.",
            style = MaterialTheme.typography.bodySmall,
        )
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            OutlinedTextField(
                value = state.searchQuery,
                onValueChange = onQueryChange,
                label = { Text("Query") },
                modifier = Modifier.fillMaxWidth(0.7f),
            )
            Button(onClick = onSearch) { Text("Search") }
        }
        LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            items(state.searchResults) { result ->
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(12.dp)) {
                        Text("Meeting ${result.meetingId}", style = MaterialTheme.typography.titleSmall)
                        Text("Segment ${result.segmentId} — score ${result.score}")
                    }
                }
            }
        }
        if (state.searchQuery.isNotBlank() && state.searchResults.isEmpty()) {
            Text("No matches.")
        }
    }
}
