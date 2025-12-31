#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <array>
#include <fstream>
#include <algorithm> // NEW: For 'std::find' to check if a song is already in favorites

using namespace std;

// --- GLOBAL VARIABLES ---
ma_engine engine;
ma_sound currentSound; 
bool isSoundInitialized = false; 
float globalVolume = 0.5f; 
string currentSoundPath = ""; // NEW: Stores the path of the currently playing track

// --- STORAGE LOGIC (Playlist & Favorites) ---

void saveList(const vector<string>& list, string filename) {
    ofstream outFile(filename);
    if (outFile.is_open()) {
        for (const string& path : list) outFile << path << endl;
        outFile.close();
    }
}

void loadList(vector<string>& list, string filename) {
    ifstream inFile(filename);
    string line;
    if (inFile.is_open()) {
        list.clear();
        while (getline(inFile, line)) {
            if (!line.empty()) list.push_back(line);
        }
        inFile.close();
    }
}

// --- AUDIO LOGIC ---

void stopCurrentSong() {
    if (isSoundInitialized) {
        ma_sound_stop(&currentSound);
        ma_sound_uninit(&currentSound);
        isSoundInitialized = false;
    }
}

void playSong(string path) {
    stopCurrentSong();
    ma_result result = ma_sound_init_from_file(&engine, path.c_str(), 0, NULL, NULL, &currentSound);
    
    if (result == MA_SUCCESS) {
        ma_sound_set_volume(&currentSound, globalVolume);
        ma_sound_start(&currentSound);
        isSoundInitialized = true;
        currentSoundPath = path; // Remember what is playing right now
        cout << "[Playing] " << path << endl;
    }
}

// --- UTILITIES ---

string getCommandOutput(string cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) return "Unknown_Title";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) result += buffer.data();
    if (!result.empty() && result.back() == '\n') result.pop_back();
    if (!result.empty() && result.back() == '\r') result.pop_back();
    return result;
}

// --- MAIN ---

int main() {
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) return -1;

    vector<string> playlist;
    vector<string> favorites; // NEW: Our favorites list
    string ytDlp = ".\\knihovny\\yt-dlp.exe";
    int choice = 0;

    // Load both lists on startup
    loadList(playlist, "playlist.txt");
    loadList(favorites, "favorites.txt");

    while (true) {
        cout << "\n--- MUSIC PLAYER V2.0 (Favorites Edition) ---" << endl;
        cout << "1. Add from YouTube" << endl;
        cout << "2. Add LOCAL file" << endl;
        cout << "3. SHOW PLAYLIST" << endl;
        cout << "4. SHOW FAVORITES" << endl; // NEW
        cout << "5. ADD CURRENT TO FAVORITES" << endl; // NEW
        cout << "6. Volume (" << (int)(globalVolume * 100) << "%)" << endl;
        cout << "7. Stop & Exit" << endl;
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            string url; cout << "URL: "; cin >> url;
            string title = getCommandOutput(ytDlp + " --get-title --no-warnings \"" + url + "\"");
            for(char &c : title) if(c == ' ') c = '_';
            string filename = "songs\\" + title + ".mp3";
            system((ytDlp + " -x --audio-format mp3 --no-warnings -o \"" + filename + "\" \"" + url + "\"").c_str());
            playlist.push_back(filename);
            saveList(playlist, "playlist.txt");
        } 
        else if (choice == 2) {
            string path; cout << "Path: "; cin.ignore(); getline(cin, path);
            ifstream f(path.c_str());
            if (f.good()) {
                playlist.push_back(path);
                saveList(playlist, "playlist.txt");
            }
        }
        else if (choice == 3 || choice == 4) {
            // Logic to handle both Playlist and Favorites display
            vector<string>& targetList = (choice == 3) ? playlist : favorites;
            if (targetList.empty()) { cout << "List is empty!" << endl; continue; }
            
            for (int i = 0; i < targetList.size(); i++) cout << i + 1 << ". " << targetList[i] << endl;
            cout << "Pick number (0 to cancel): ";
            int idx; cin >> idx;
            if (idx > 0 && idx <= targetList.size()) playSong(targetList[idx - 1]);
        }
        else if (choice == 5) {
            // Check if something is actually playing
            if (currentSoundPath == "") {
                cout << "[!] Nothing is playing right now." << endl;
            } else {
                // Check if it's already in favorites to avoid duplicates
                auto it = find(favorites.begin(), favorites.end(), currentSoundPath);
                if (it == favorites.end()) {
                    favorites.push_back(currentSoundPath);
                    saveList(favorites, "favorites.txt");
                    cout << "[<3] Added to favorites!" << endl;
                } else {
                    cout << "[!] Already in favorites." << endl;
                }
            }
        }
        else if (choice == 6) {
            cout << "Vol (0-100): "; int v; cin >> v;
            globalVolume = v / 100.0f;
            if (isSoundInitialized) ma_sound_set_volume(&currentSound, globalVolume);
        }
        else if (choice == 7) break;
    }

    stopCurrentSong();
    ma_engine_uninit(&engine);
    return 0;
}