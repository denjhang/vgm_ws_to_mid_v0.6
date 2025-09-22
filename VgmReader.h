#ifndef VGM_READER_H
#define VGM_READER_H

#include <string>
#include <vector>
#include <cstdint>
#include "WonderSwanChip.h"

class VgmReader {
public:
    VgmReader(WonderSwanChip& chip);
    bool load_and_parse(const std::string& filename);

private:
    WonderSwanChip& chip;
    std::vector<uint8_t> file_data;
    bool parse();
};

#endif // VGM_READER_H
