#ifndef WONDERSWAN_CHIP_H
#define WONDERSWAN_CHIP_H

#include "MidiWriter.h"
#include <cstdint>
#include <vector>
#include <fstream>

class WonderSwanChip {
public:
    WonderSwanChip(MidiWriter& midi_writer);
    ~WonderSwanChip();
    void write_port(uint8_t port, uint8_t value);
    void advance_time(uint16_t samples);

private:
    MidiWriter& midi_writer;
    std::vector<uint8_t> io_ram;
    std::vector<int> channel_periods;
    std::vector<int> channel_volumes_left;
    std::vector<int> channel_volumes_right;
    std::vector<bool> channel_enabled;
    std::vector<int> channel_last_note;
    std::vector<int> channel_last_velocity; // For volume dynamics
    uint32_t current_time;
    std::ofstream log_file;

    int period_to_midi_note(int period);
    void check_state_and_update_midi(int channel);
};

#endif // WONDERSWAN_CHIP_H
