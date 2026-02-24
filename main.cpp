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

// --- GLOBAL VARIABLES ---
ma_engine engine;
ma_sound currentSound; // This object represents the currently playing song
bool isSoundInitialized = false; // Flag to track if currentSound is loaded
float globalVolume = 0.5f; // Initial volume (50%)

// --- FILE HANDLING ---

void savePlaylist(const vector<string>& playlist) {
    ofstream outFile("playlist.txt");
    if (outFile.is_open()) {
        for (const string& songPath : playlist) outFile << songPath << endl;
        outFile.close();
    }
}

void loadPlaylist(vector<string>& playlist) {
    ifstream inFile("playlist.txt");
    string line;
    if (inFile.is_open()) {
        playlist.clear();
        while (getline(inFile, line)) {
            if (!line.empty()) playlist.push_back(line);
        }
        inFile.close();
        cout << "[System] Playlist loaded." << endl;
    }
}

// --- AUDIO CONTROL ---

/**
 * Stops the current song and releases its resources.
 */
void stopCurrentSong() {
    if (isSoundInitialized) {
        ma_sound_stop(&currentSound);
        ma_sound_uninit(&currentSound);
        isSoundInitialized = false;
    }
}

/**
 * Loads and plays a new song from the given path.
 */
void playSong(string path) {
    stopCurrentSong(); // Clean up previous song first

    // Initialize the sound object from the file path
    ma_result result = ma_sound_init_from_file(&engine, path.c_str(), 0, NULL, NULL, &currentSound);
    
    if (result == MA_SUCCESS) {
        ma_sound_set_volume(&currentSound, globalVolume); // Apply current volume
        ma_sound_start(&currentSound); // Start playback
        isSoundInitialized = true;
        cout << "Playing: " << path << endl;
    } else {
        cerr << "Error: Could not load file: " << path << endl;
    }
}

string getCommandOutput(string cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), &pclose);
    if (!pipe) return "Unknown_Title";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n') result.pop_back();
    if (!result.empty() && result.back() == '\r') result.pop_back();
    return result;
}

// --- MAIN ---

int main() {
    // Start the engine once at the beginning
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) return -1;

    vector<string> playlist;
    string ytDlp = ".\\knihovny\\yt-dlp.exe";
    int choice = 0;

    loadPlaylist(playlist);

    while (true) {
        cout << "\n--- MUSIC PLAYER ---" << endl;
        cout << "1. Add from YouTube" << endl;
        cout << "2. Playlist & Play" << endl;
        cout << "3. Volume (Current: " << (int)(globalVolume * 100) << "%)" << endl;
        cout << "4. Stop music" << endl;
        cout << "5. Exit" << endl;
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            string url;
            cout << "Link: "; cin >> url;
            cout << "Fetching title..." << endl;
            string title = getCommandOutput(ytDlp + " --get-title --no-warnings \"" + url + "\"");
            for(int i = 0; i < title.length(); i++) if(title[i] == ' ') title[i] = '_';

            string filename = "songs\\" + title + ".mp3";
            cout << "Downloading..." << endl;
            system((ytDlp + " -x --audio-format mp3 --no-warnings -o \"" + filename + "\" \"" + url + "\"").c_str());

            playlist.push_back(filename);
            savePlaylist(playlist);
        } 
        else if (choice == 2) {
            if (playlist.empty()) { cout << "Empty!" << endl; continue; }
            for (int i = 0; i < playlist.size(); i++) cout << i + 1 << ". " << playlist[i] << endl;
            cout << "Song number: ";
            int index; cin >> index;
            if (index > 0 && index <= playlist.size()) {
                playSong(playlist[index - 1]);
            }
        }
        else if (choice == 3) {
            cout << "Enter volume (0-100): ";
            int vol; cin >> vol;
            globalVolume = vol / 100.0f; // Convert 0-100 to 0.0-1.0
            if (isSoundInitialized) {
                ma_sound_set_volume(&currentSound, globalVolume);
            }
            cout << "Volume set to " << vol << "%" << endl;
        }
        else if (choice == 4) {
            stopCurrentSong();
            cout << "Playback stopped." << endl;
        }
        else if (choice == 5) {
            break;
        }
    }

    stopCurrentSong(); // Clean up sound before exiting
    ma_engine_uninit(&engine); // Turn off the engine
    return 0;
}