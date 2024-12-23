#include <cstdint>
#include <iomanip>
#include "pcap_parser.h"
#include "my_time.h"

/**
 * Read a message (specified by template ID) from byte stream (FileReader).
 * 
 * @return number of bytes consumed
 */
uint16_t parse_moex_message(FileReader*, ofstream*, const uint16_t, const uint16_t, const uint16_t);

#ifndef DECIMAL_5_NULL_H
#define DECIMAL_5_NULL_H
extern double DECIMAL_5_NULL;
#endif

#ifndef INT64_NULL_H
#define INT64_NULL_H
extern int64_t INT64_NULL;
#endif
