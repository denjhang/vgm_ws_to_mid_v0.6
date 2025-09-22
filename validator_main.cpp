#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

using namespace std;

fstream MIDI;
fstream fout;
uint8_t MThd[4] = { 'M','T','h','d' };
uint8_t MTrk[4] = { 'M','T','r','k' };
uint8_t buf[256];
int len;

void outHex();
void outAscii();

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <midi_file>" << endl;
        return 1;
    }

    fout.open("vgm_ws_to_mid/validation_result.txt", ios::out);
    MIDI.open(argv[1], ios::in | ios::binary);

    if (!MIDI.is_open()) {
        cerr << "Error opening MIDI file: " << argv[1] << endl;
        fout << "Error opening MIDI file: " << argv[1] << endl;
        return 1;
    }

    // Header
    memset(buf, '\0', sizeof(buf));
    MIDI.read((char*)&buf, len = 4);
    if (buf[0] == MThd[0] && buf[1] == MThd[1] && buf[2] == MThd[2] && buf[3] == MThd[3]) {
        cout << "MIDI file confirmed." << endl;
        fout << "MIDI file confirmed." << endl;
    }
    else {
        cout << "Not a MIDI file." << endl;
        fout << "Not a MIDI file." << endl;
        return 1;
    }
    outHex();

    MIDI.read((char*)&buf, len = 4); // Length
    uint32_t header_length = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    cout << "Header length: " << header_length << endl;
    fout << "Header length: " << header_length << endl;
    outHex();

    MIDI.read((char*)&buf, len = header_length); // Content
    uint16_t format = (buf[0] << 8) | buf[1];
    uint16_t tracks = (buf[2] << 8) | buf[3];
    uint16_t TickUnit = (buf[4] << 8) | buf[5];
    cout << "Format: " << format << ", Tracks: " << tracks << ", Ticks/QN: " << TickUnit << endl;
    fout << "Format: " << format << ", Tracks: " << tracks << ", Ticks/QN: " << TickUnit << endl;
    outHex();

    for (int t = 0; t < tracks; t++)
    {
        cout << "\n--- Track " << t + 1 << " ---" << endl;
        fout << "\n--- Track " << t + 1 << " ---" << endl;

        MIDI.read((char*)&buf, len = 4); // Track chunk ID
        if (buf[0] != MTrk[0] || buf[1] != MTrk[1] || buf[2] != MTrk[2] || buf[3] != MTrk[3]) {
            cout << "Error: MTrk chunk not found for track " << t + 1 << endl;
            fout << "Error: MTrk chunk not found for track " << t + 1 << endl;
            outHex();
            break;
        }
        cout << "MTrk chunk found." << endl;
        fout << "MTrk chunk found." << endl;
        outHex();

        MIDI.read((char*)&buf, len = 4); // Track length
        uint32_t track_length = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
        cout << "Track length: " << track_length << endl;
        fout << "Track length: " << track_length << endl;
        outHex();

        // Just skip the track data for validation purposes
        MIDI.seekg(track_length, ios::cur);
    }

    cout << "\nValidation finished." << endl;
    fout << "\nValidation finished." << endl;

    MIDI.close();
    fout.close();

    return 0;
}

void outHex() {
    fout.setf(ios_base::hex, ios_base::basefield);
    fout.setf(ios::uppercase);
    cout.setf(ios_base::hex, ios_base::basefield);
    cout.setf(ios::uppercase);
    for (int i = 0; i < len; i++) {
        cout << setfill('0') << setw(2) << (int)buf[i] << " ";
        fout << setfill('0') << setw(2) << (int)buf[i] << " ";
    }
    memset(buf, '\0', sizeof(buf));
    cout << endl;
    cout << std::defaultfloat;
    fout << std::defaultfloat;
}

void outAscii() {
    for (int i = 0; i < len; i++) {
        cout << buf[i];
        fout << buf[i];
    }
    memset(buf, '\0', sizeof(buf));
    cout << endl;
    fout << endl;
}
