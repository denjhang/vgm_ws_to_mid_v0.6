#include <iostream>
#include "MidiWriter.h"
#include "WonderSwanChip.h"
#include "VgmReader.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.vgm> <output.mid>" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    std::cout << "VGM to MIDI conversion process started." << std::endl;

    MidiWriter midi_writer;
    WonderSwanChip chip(midi_writer);
    VgmReader reader(chip);

    if (!reader.load_and_parse(input_filename)) {
        std::cerr << "Failed to load or parse VGM file." << std::endl;
        return 1;
    }

    midi_writer.write_to_file(output_filename);

    std::cout << "VGM to MIDI conversion completed successfully." << std::endl;

    return 0;
}
