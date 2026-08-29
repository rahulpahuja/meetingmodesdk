package com.meetingsdk.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewmodel.CreationExtras
import com.meetingsdk.android.ui.MeetingSdkApp

class MainActivity : ComponentActivity() {
    private val viewModel: MeetingSdkViewModel by viewModels {
        object : ViewModelProvider.Factory {
            @Suppress("UNCHECKED_CAST")
            override fun <T : ViewModel> create(modelClass: Class<T>, extras: CreationExtras): T {
                val dbFile = getDatabasePath("meetings.db")
                dbFile.parentFile?.mkdirs()
                return MeetingSdkViewModel(application, dbFile.absolutePath) as T
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                Surface(modifier = Modifier) {
                    MeetingSdkApp(viewModel)
                }
            }
        }
    }
}
