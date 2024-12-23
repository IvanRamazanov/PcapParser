#include "pcap_parser.h"

#define PARSE_INT(T,range,big_endian) \
    T value = 0;\
    for (int i=0; i<range; ++i){\
        if (big_endian){\
            value += static_cast<T>(*get_byte()) << ((range - 1 - i) * 8);\
        }else{\
            value += static_cast<T>(*get_byte()) << (i * 8);\
        }\
    }

FileReader::FileReader(char* path){
    f.open(path, ios_base::binary | ios_base::in);
    if (!f.is_open()){
        throw runtime_error(string("File not found: ") + path);
    }
    
    buff_len = 0;
    current_byte = -1;
    buff.reserve(buff_size);

    // fill the buffer
    buff_len = f.read((char*)&buff[0], buff_size).gcount();
    reached_end = f.eof();

    if (buff_len == 0){
        throw runtime_error(path + string(": File is empty"));
    }
}

FileReader::~FileReader(){
    f.close();
}

uint64_t FileReader::get_uint64(bool big_endian){
    PARSE_INT(uint64_t, 8, big_endian);
    return value;
}

uint32_t FileReader::get_uint32(bool big_endian){
    PARSE_INT(uint32_t, 4, big_endian);
    return value;
}

uint16_t FileReader::get_uint16(bool big_endian){
    PARSE_INT(uint16_t, 2, big_endian);
    return value;
}

uint8_t FileReader::get_uint8(){
    PARSE_INT(uint8_t, 1, true);
    return value;
}

char FileReader::get_char(){
    PARSE_INT(uint8_t, 1, true);
    return (char)value;
}

double FileReader::get_decimal5(bool big_endian){
    PARSE_INT(uint64_t, 8, big_endian);
    return ((int64_t)value) / 100'000.0;
}

void FileReader::skip(const int num_bytes){
    for (int i = num_bytes; i > 0; --i){
        get_byte();
    }
}

bool FileReader::eof(){
    return reached_end && current_byte >= buff_len; 
}

bool FileReader::last_byte(){
    return reached_end && current_byte == buff_len - 1;
}


unsigned char* FileReader::get_byte(){
    ++current_byte;
    if (current_byte >= buff_len){
        // overflow, fetch new buffer
        if (buff_len == buff_size && !reached_end){
            buff_len = f.read((char*)&buff[0], buff_size).gcount();
            reached_end = f.eof();
            current_byte = 0;
        }else{
            // no more data
            throw out_of_range("EOF reached. Attempt to read outside of file.");
        }
    }

    return &buff[current_byte];
}