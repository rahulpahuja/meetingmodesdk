package com.meetingsdk.android.ui

import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.List
import androidx.compose.material.icons.filled.Mic
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import com.meetingsdk.android.MeetingSdkViewModel

private enum class Tab(val label: String) { Record("Record"), Transcript("Transcript"), Search("Search"), Settings("Settings") }

@Composable
fun MeetingSdkApp(viewModel: MeetingSdkViewModel) {
    var tab by remember { mutableStateOf(Tab.Record) }
    val state by viewModel.uiState.collectAsState()

    Scaffold(
        bottomBar = {
            NavigationBar {
                NavigationBarItem(
                    selected = tab == Tab.Record,
                    onClick = { tab = Tab.Record },
                    icon = { Icon(Icons.Filled.Mic, contentDescription = null) },
                    label = { Text(Tab.Record.label) },
                )
                NavigationBarItem(
                    selected = tab == Tab.Transcript,
                    onClick = { tab = Tab.Transcript },
                    icon = { Icon(Icons.Filled.List, contentDescription = null) },
                    label = { Text(Tab.Transcript.label) },
                )
                NavigationBarItem(
                    selected = tab == Tab.Search,
                    onClick = { tab = Tab.Search },
                    icon = { Icon(Icons.Filled.Search, contentDescription = null) },
                    label = { Text(Tab.Search.label) },
                )
                NavigationBarItem(
                    selected = tab == Tab.Settings,
                    onClick = { tab = Tab.Settings },
                    icon = { Icon(Icons.Filled.Settings, contentDescription = null) },
                    label = { Text(Tab.Settings.label) },
                )
            }
        },
    ) { innerPadding ->
        val contentModifier = Modifier.padding(innerPadding)
        when (tab) {
            Tab.Record -> RecordScreen(
                state,
                onRunDemoMeeting = viewModel::runDemoMeeting,
                onStartLiveRecording = viewModel::startLiveRecording,
                onStopLiveRecording = viewModel::stopLiveRecording,
                modifier = contentModifier,
            )
            Tab.Transcript -> TranscriptScreen(state, onSelectMeeting = viewModel::selectMeeting, modifier = contentModifier)
            Tab.Search -> SearchScreen(
                state,
                onQueryChange = viewModel::updateSearchQuery,
                onSearch = viewModel::runSearch,
                modifier = contentModifier,
            )
            Tab.Settings -> SettingsScreen(
                state,
                onTranslationModeChange = viewModel::setTranslationMode,
                onTranslateInputChange = viewModel::updateTranslateInput,
                onRunTranslate = viewModel::runTranslate,
                modifier = contentModifier,
            )
        }
    }
}
