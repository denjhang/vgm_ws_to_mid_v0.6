#include "VgmReader.h"
#include <fstream>
#include <iostream>
#include <iomanip>

VgmReader::VgmReader(WonderSwanChip& chip) : chip(chip) {}

bool VgmReader::load_and_parse(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    file_data.resize(size);
    if (!file.read(reinterpret_cast<char*>(file_data.data()), size)) {
        std::cerr << "Error reading file: " << filename << std::endl;
        return false;
    }

    return parse();
}

bool VgmReader::parse() {
    if (file_data.size() < 0x40) {
        std::cerr << "Invalid VGM file: header too small." << std::endl;
        return false;
    }

    if (file_data[0] != 'V' || file_data[1] != 'g' || file_data[2] != 'm' || file_data[3] != ' ') {
        std::cerr << "Invalid VGM file: magic number mismatch." << std::endl;
        return false;
    }

    uint32_t data_offset = *reinterpret_cast<uint32_t*>(&file_data[0x34]);
    uint32_t vgm_data_offset = (data_offset == 0) ? 0x40 : (0x34 + data_offset);

    uint32_t current_pos = vgm_data_offset;

    while (current_pos < file_data.size()) {
        uint8_t command_byte = file_data[current_pos];

        switch (command_byte) {
            case 0x61: { // Wait nnnn samples
                if (current_pos + 2 >= file_data.size()) return true;
                uint16_t wait = *reinterpret_cast<uint16_t*>(&file_data[current_pos + 1]);
                chip.advance_time(wait);
                current_pos += 3;
                break;
            }
            case 0x62: // Wait 1/60 second
                chip.advance_time(735); // 44100 / 60
                current_pos += 1;
                break;
            case 0x63: // Wait 1/50 second
                chip.advance_time(882); // 44100 / 50
                current_pos += 1;
                break;
            case 0x66: // End of sound data
                return true;
            
            case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
            case 0x76: case 0x77: case 0x78: case 0x79: case 0x7a: case 0x7b:
            case 0x7c: case 0x7d: case 0x7e: case 0x7f:
                chip.advance_time((command_byte & 0x0F) + 1);
                current_pos += 1;
                break;

            case 0xb3: // WonderSwan I/O write
            case 0xbc: { // WonderSwan Custom I/O write
                if (current_pos + 2 >= file_data.size()) return true;
                uint8_t port = file_data[current_pos + 1];
                uint8_t value = file_data[current_pos + 2];
                chip.write_port(port, value);
                current_pos += 3;
                break;
            }

            // Ignored commands
            case 0x67: { // data block
                if (current_pos + 6 >= file_data.size()) return true;
                current_pos += 6 + (*reinterpret_cast<uint32_t*>(&file_data[current_pos + 2]));
                break;
            }
            case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
            case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b:
            case 0x8c: case 0x8d: case 0x8e: case 0x8f:
                // DAC stream, we can ignore for WS
                current_pos += 3;
                break;

            default:
                // Unknown or unhandled command, just skip
                current_pos++;
                break;
        }
    }

    return true;
}
