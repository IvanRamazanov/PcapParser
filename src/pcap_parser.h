#include <cstdint>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <iostream>

using namespace std;

#ifndef PCAP_FILE_PARSER_H
#define PCAP_FILE_PARSER_H
class FileReader{
    const uint32_t buff_size = 1 << 30;
    ifstream f;
    uint32_t current_byte, buff_len;
    vector<unsigned char> buff; // 1GB buffer
    bool reached_end = false;

    unsigned char* get_byte();
    ~FileReader();

    public:
        FileReader(char* path);

        uint64_t get_uint64(bool=true);
        double get_decimal5(bool=true);
        uint32_t get_uint32(bool=true);
        uint16_t get_uint16(bool=true);
        uint8_t get_uint8();
        char get_char();

        void skip(const int);
        bool eof();
        bool last_byte();
};
#endif
