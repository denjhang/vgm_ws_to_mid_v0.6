#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_to_dump>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return 1;
    }

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    for (size_t i = 0; i < buffer.size(); ++i) {
        if (i % 16 == 0) {
            std::cout << std::setw(8) << std::setfill('0') << std::hex << i << ": ";
        }

        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)buffer[i] << " ";

        if ((i + 1) % 16 == 0 || i == buffer.size() - 1) {
            std::cout << std::endl;
        }
    }

    return 0;
}
