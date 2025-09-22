#include "WonderSwanChip.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>

// Conversion factor from VGM samples (at 44100 Hz) to MIDI ticks (at 480 PPQN, 120 BPM)
const double SAMPLES_TO_TICKS = (480.0 * 120.0) / (44100.0 * 60.0);

WonderSwanChip::WonderSwanChip(MidiWriter& midi_writer)
    : midi_writer(midi_writer),
      io_ram(256, 0),
      channel_periods(4, 0),
      channel_volumes_left(4, 0),
      channel_volumes_right(4, 0),
      channel_enabled(4, false),
      channel_last_note(4, 0),
      channel_last_velocity(4, -1), // Initialize with -1 to force initial CC message
      current_time(0) {
    log_file.open("vgm_ws_to_mid/debug_output.txt", std::ios::out | std::ios::trunc);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open vgm_ws_to_mid/debug_output.txt for writing." << std::endl;
    }

    // Set default instrument to Square Wave (GM 81) for all channels
    for (int i = 0; i < 4; ++i) {
        midi_writer.add_program_change(i, 80, 0); // GM uses 0-indexed programs, so 80 is Square Wave
    }
}

WonderSwanChip::~WonderSwanChip() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

void WonderSwanChip::advance_time(uint16_t samples) {
    current_time += samples;
}

void WonderSwanChip::check_state_and_update_midi(int channel) {
    bool is_on = channel_enabled[channel] && (channel_volumes_left[channel] > 0 || channel_volumes_right[channel] > 0);
    int current_note_pitch = period_to_midi_note(channel_periods[channel]);
    
    // Apply a non-linear curve to map volume for better dynamics and audibility.
    int vgm_vol = std::max(channel_volumes_left[channel], channel_volumes_right[channel]);
    double normalized_vol = vgm_vol / 15.0;
    // A power of ~0.3 provides a more aggressive boost to lower volumes.
    double curved_vol = pow(normalized_vol, 0.3); 
    int velocity = static_cast<int>(curved_vol * 127.0);
    if (velocity > 127) velocity = 127;

    int last_note = channel_last_note[channel];
    bool was_on = last_note > 0;

    uint32_t midi_time = static_cast<uint32_t>(current_time * SAMPLES_TO_TICKS);

    if (is_on && !was_on) {
        midi_writer.add_note_on(channel, current_note_pitch, velocity, midi_time);
        channel_last_note[channel] = current_note_pitch;
        channel_last_velocity[channel] = velocity;
    } else if (!is_on && was_on) {
        midi_writer.add_note_off(channel, last_note, midi_time);
        channel_last_note[channel] = 0;
        channel_last_velocity[channel] = -1;
    } else if (is_on && was_on) {
        // Note is currently on, check for changes
        if (current_note_pitch != last_note) {
            // Pitch change (legato)
            midi_writer.add_note_off(channel, last_note, midi_time);
            midi_writer.add_note_on(channel, current_note_pitch, velocity, midi_time);
            channel_last_note[channel] = current_note_pitch;
            channel_last_velocity[channel] = velocity;
        } else if (velocity != channel_last_velocity[channel]) {
            // Volume change (software envelope)
            // Use CC#11 (Expression) for dynamic volume changes, which is more standard than CC#7.
            midi_writer.add_control_change(channel, 11, velocity, midi_time); // CC 11 is Expression
            channel_last_velocity[channel] = velocity;
        }
    }
}

void WonderSwanChip::write_port(uint8_t port, uint8_t value) {
    uint8_t addr = port + 0x80;
    io_ram[addr] = value;

    switch (addr) {
        case 0x80: case 0x81:
            channel_periods[0] = ((io_ram[0x81] & 0x07) << 8) | io_ram[0x80];
            check_state_and_update_midi(0);
            break;
        case 0x82: case 0x83:
            channel_periods[1] = ((io_ram[0x83] & 0x07) << 8) | io_ram[0x82];
            check_state_and_update_midi(1);
            break;
        case 0x84: case 0x85:
            channel_periods[2] = ((io_ram[0x85] & 0x07) << 8) | io_ram[0x84];
            check_state_and_update_midi(2);
            break;
        case 0x86: case 0x87:
            channel_periods[3] = ((io_ram[0x87] & 0x07) << 8) | io_ram[0x86];
            check_state_and_update_midi(3);
            break;
        case 0x88:
            channel_volumes_left[0] = (io_ram[0x88] >> 4) & 0x0F;
            channel_volumes_right[0] = io_ram[0x88] & 0x0F;
            check_state_and_update_midi(0);
            break;
        case 0x89:
            channel_volumes_left[1] = (io_ram[0x89] >> 4) & 0x0F;
            channel_volumes_right[1] = io_ram[0x89] & 0x0F;
            check_state_and_update_midi(1);
            break;
        case 0x8A:
            channel_volumes_left[2] = (io_ram[0x8A] >> 4) & 0x0F;
            channel_volumes_right[2] = io_ram[0x8A] & 0x0F;
            check_state_and_update_midi(2);
            break;
        case 0x8B:
            channel_volumes_left[3] = (io_ram[0x8B] >> 4) & 0x0F;
            channel_volumes_right[3] = io_ram[0x8B] & 0x0F;
            check_state_and_update_midi(3);
            break;
        case 0x90:
            channel_enabled[0] = (io_ram[0x90] & 0x01) != 0;
            channel_enabled[1] = (io_ram[0x90] & 0x02) != 0;
            channel_enabled[2] = (io_ram[0x90] & 0x04) != 0;
            channel_enabled[3] = (io_ram[0x90] & 0x08) != 0;
            for (int i = 0; i < 4; ++i) {
                check_state_and_update_midi(i);
            }
            break;
        case 0x91:
            for (int i = 0; i < 4; ++i) {
                check_state_and_update_midi(i);
            }
            break;
    }
}

int WonderSwanChip::period_to_midi_note(int period) {
    if (period >= 2048) return 0;

    double freq = (3072000.0 / (2048.0 - period)) / 32.0;
    if (freq <= 0) return 0;
    
    // The pitch is now at its original calculated octave.
    int note = static_cast<int>(round(69 + 12 * log2(freq / 440.0)));
    
    return note;
}
