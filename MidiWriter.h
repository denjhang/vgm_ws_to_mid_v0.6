#ifndef MIDI_WRITER_H
#define MIDI_WRITER_H

#include <string>
#include <vector>
#include <cstdint> // For uint8_t, uint32_t

class MidiWriter {
public:
    MidiWriter(int ppqn = 480);
    void add_note_on(uint8_t channel, uint8_t note, uint8_t velocity, uint32_t time);
    void add_note_off(uint8_t channel, uint8_t note, uint32_t time);
    void add_program_change(uint8_t channel, uint8_t program, uint32_t time);
    void add_control_change(uint8_t channel, uint8_t controller, uint8_t value, uint32_t time);
    bool write_to_file(const std::string& filename);

private:
    struct MidiEvent {
        uint32_t time;
        uint8_t type;
        uint8_t channel;
        uint8_t data1; // Note, Program, or Controller
        uint8_t data2; // Velocity or Value
    };

    int ppqn;
    std::vector<MidiEvent> events;
    void write_variable_length(std::vector<uint8_t>& buffer, uint32_t value);
};

#endif // MIDI_WRITER_H
