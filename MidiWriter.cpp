#include "MidiWriter.h"
#include <fstream>
#include <algorithm>
#include <vector>
#include <iostream>

// --- Portable Endian Swap Functions ---
uint32_t swap_endian_32(uint32_t val) {
    return ((val << 24) & 0xff000000) |
           ((val <<  8) & 0x00ff0000) |
           ((val >>  8) & 0x0000ff00) |
           ((val >> 24) & 0x000000ff);
}

uint16_t swap_endian_16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

MidiWriter::MidiWriter(int ppqn) : ppqn(ppqn) {}

void MidiWriter::add_note_on(uint8_t channel, uint8_t note, uint8_t velocity, uint32_t time) {
    events.push_back({time, 0x90, channel, note, velocity});
}

void MidiWriter::add_note_off(uint8_t channel, uint8_t note, uint32_t time) {
    events.push_back({time, 0x80, channel, note, 0});
}

void MidiWriter::add_program_change(uint8_t channel, uint8_t program, uint32_t time) {
    events.push_back({time, 0xC0, channel, program, 0}); // data2 is unused
}

void MidiWriter::add_control_change(uint8_t channel, uint8_t controller, uint8_t value, uint32_t time) {
    events.push_back({time, 0xB0, channel, controller, value});
}

bool MidiWriter::write_to_file(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return false;
    }

    // --- 1. Prepare Track Data ---
    std::vector<uint8_t> track_data;

    // Sort events by time, then by type (Control/Program change before Note On/Off)
    std::sort(events.begin(), events.end(), [](const MidiEvent& a, const MidiEvent& b) {
        if (a.time != b.time) {
            return a.time < b.time;
        }
        // Prioritize non-note events
        uint8_t a_type = a.type & 0xF0;
        uint8_t b_type = b.type & 0xF0;
        if (a_type != b_type) {
             if (a_type == 0x90 || a_type == 0x80) return false;
             if (b_type == 0x90 || b_type == 0x80) return true;
        }
        return a_type < b_type;
    });

    uint32_t last_time = 0;
    for (const auto& event : events) {
        uint32_t delta_time = event.time - last_time;
        write_variable_length(track_data, delta_time);
        
        uint8_t status_byte = event.type | event.channel;
        track_data.push_back(status_byte);
        track_data.push_back(event.data1);

        // Program Change messages only have one data byte
        if ((event.type & 0xF0) != 0xC0) {
            track_data.push_back(event.data2);
        }
        
        last_time = event.time;
    }

    // End of track event
    write_variable_length(track_data, 0);
    track_data.push_back(0xFF);
    track_data.push_back(0x2F);
    track_data.push_back(0x00);

    // --- 2. Write MIDI Header ---
    file.write("MThd", 4);
    uint32_t header_size = swap_endian_32(6);
    file.write(reinterpret_cast<const char*>(&header_size), 4);
    uint16_t format = swap_endian_16(0); // Single track format
    file.write(reinterpret_cast<const char*>(&format), 2);
    uint16_t num_tracks = swap_endian_16(1);
    file.write(reinterpret_cast<const char*>(&num_tracks), 2);
    uint16_t division = swap_endian_16(this->ppqn);
    file.write(reinterpret_cast<const char*>(&division), 2);

    // --- 3. Write Track Chunk ---
    file.write("MTrk", 4);
    uint32_t track_size = swap_endian_32(track_data.size());
    file.write(reinterpret_cast<const char*>(&track_size), 4);
    file.write(reinterpret_cast<const char*>(track_data.data()), track_data.size());
    
    return file.good();
}

void MidiWriter::write_variable_length(std::vector<uint8_t>& buffer, uint32_t value) {
    uint32_t temp = value;
    uint8_t bytes[5];
    int count = 0;

    if (value == 0) {
        buffer.push_back(0x00);
        return;
    }
    
    do {
        bytes[count++] = temp & 0x7F;
        temp >>= 7;
    } while (temp > 0);

    while (count > 0) {
        count--;
        if (count > 0) {
            buffer.push_back(bytes[count] | 0x80);
        } else {
            buffer.push_back(bytes[count]);
        }
    }
}
