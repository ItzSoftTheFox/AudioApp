/**
 * PROJECT: My C++ Music Player
 * FEATURES: YouTube download, Local file support, Playlist persistence, Volume control.
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <array>
#include <fstream>

using namespace std;

// --- GLOBAL VARIABLES (Audio Engine State) ---
ma_engine engine;
ma_sound currentSound; 
bool isSoundInitialized = false; 
float globalVolume = 0.5f; 

// --- FILE PERSISTENCE (Saving/Loading Playlist) ---

/**
 * Writes the playlist paths to playlist.txt so they persist after closing the app.
 */
void savePlaylist(const vector<string>& playlist) {
    ofstream outFile("playlist.txt");
    if (outFile.is_open()) {
        for (const string& songPath : playlist) {
            outFile << songPath << endl;
        }
        outFile.close();
    }
}

/**
 * Reads playlist.txt and fills the vector with saved song paths.
 */
void loadPlaylist(vector<string>& playlist) {
    ifstream inFile("playlist.txt");
    string line;
    if (inFile.is_open()) {
        playlist.clear();
        while (getline(inFile, line)) {
            if (!line.empty()) playlist.push_back(line);
        }
        inFile.close();
        cout << "[System] Playlist loaded from disk." << endl;
    }
}

// --- AUDIO LOGIC ---

/**
 * Safely stops and uninitializes the current sound object.
 */
void stopCurrentSong() {
    if (isSoundInitialized) {
        ma_sound_stop(&currentSound);
        ma_sound_uninit(&currentSound);
        isSoundInitialized = false;
    }
}

/**
 * Loads a song from a path and starts playback.
 */
void playSong(string path) {
    stopCurrentSong(); // Always stop previous song first

    // Initialize the sound from file (supports MP3, WAV, etc.)
    ma_result result = ma_sound_init_from_file(&engine, path.c_str(), 0, NULL, NULL, &currentSound);
    
    if (result == MA_SUCCESS) {
        ma_sound_set_volume(&currentSound, globalVolume);
        ma_sound_start(&currentSound);
        isSoundInitialized = true;
        cout << "[Playing] " << path << endl;
    } else {
        cerr << "[Error] Failed to load: " << path << endl;
    }
}

// --- SYSTEM UTILITIES ---

/**
 * Runs a terminal command and returns its output as a string.
 */
string getCommandOutput(string cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) return "Unknown_Title";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    // Trim newlines
    if (!result.empty() && result.back() == '\n') result.pop_back();
    if (!result.empty() && result.back() == '\r') result.pop_back();
    return result;
}

// --- MAIN APPLICATION LOOP ---

int main() {
    // Initialize the audio engine once at startup
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        cerr << "Could not init audio engine!" << endl;
        return -1;
    }

    vector<string> playlist;
    string ytDlp = ".\\knihovny\\yt-dlp.exe";
    int choice = 0;

    loadPlaylist(playlist);

    while (true) {
        cout << "\n===============================" << endl;
        cout << "   MY C++ MUSIC PLAYER V1.0   " << endl;
        cout << "===============================" << endl;
        cout << "1. Add from YouTube" << endl;
        cout << "2. Add LOCAL file (path)" << endl;
        cout << "3. Show Playlist & Play" << endl;
        cout << "4. Volume (Current: " << (int)(globalVolume * 100) << "%)" << endl;
        cout << "5. Stop playback" << endl;
        cout << "6. Exit" << endl;
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            string url;
            cout << "Paste URL: "; cin >> url;
            
            cout << "Fetching title..." << endl;
            string title = getCommandOutput(ytDlp + " --get-title --no-warnings \"" + url + "\"");
            
            // Basic sanitization for Windows file paths
            for(char &c : title) if(c == ' ') c = '_';

            string filename = "songs\\" + title + ".mp3";
            cout << "Downloading audio..." << endl;
            string dlCmd = ytDlp + " -x --audio-format mp3 --no-warnings -o \"" + filename + "\" \"" + url + "\"";
            system(dlCmd.c_str());

            playlist.push_back(filename);
            savePlaylist(playlist);
            cout << "[Success] Added YouTube song." << endl;
        } 
        else if (choice == 2) {
            string localPath;
            cout << "Enter full path to .mp3 file: " << endl;
            cin.ignore(); // Clear the newline from previous 'cin >> choice'
            getline(cin, localPath);

            // Verify if the local file exists
            ifstream f(localPath.c_str());
            if (f.good()) {
                playlist.push_back(localPath);
                savePlaylist(playlist);
                cout << "[Success] Local file added." << endl;
            } else {
                cout << "[Error] File not found at that location!" << endl;
            }
        }
        else if (choice == 3) {
            if (playlist.empty()) {
                cout << "Playlist is empty!" << endl;
                continue;
            }
            cout << "\n--- CURRENT PLAYLIST ---" << endl;
            for (int i = 0; i < playlist.size(); i++) {
                cout << i + 1 << ". " << playlist[i] << endl;
            }
            cout << "Pick a song number: ";
            int index; cin >> index;
            if (index > 0 && index <= playlist.size()) {
                playSong(playlist[index - 1]);
            }
        }
        else if (choice == 4) {
            cout << "New volume (0-100): ";
            int vol; cin >> vol;
            globalVolume = vol / 100.0f;
            if (isSoundInitialized) {
                ma_sound_set_volume(&currentSound, globalVolume);
            }
            cout << "Volume adjusted." << endl;
        }
        else if (choice == 5) {
            stopCurrentSong();
            cout << "Music stopped." << endl;
        }
        else if (choice == 6) {
            break;
        }
    }

    // Cleanup before closing
    stopCurrentSong();
    ma_engine_uninit(&engine);
    cout << "Application closed. Happy coding!" << endl;

    return 0;
}