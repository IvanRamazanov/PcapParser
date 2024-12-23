#include "parser.h"

using namespace std;

void parse_simba_packet(FileReader *file_parser, ofstream *output_stream){
    // Market Data header
    uint32_t msg_seq_num = file_parser->get_uint32(false);
    uint16_t msg_size = file_parser->get_uint16(false);
    uint16_t tmp = file_parser->get_uint16(false);
    bool msg_flag_last_fragment = tmp & 0x1;
    bool msg_flag_start_of_snapshot = tmp & 0x2;
    bool msg_flag_end_of_snapshot = tmp & 0x4;
    bool msg_flag_is_incremental = tmp & 0x8; // false - 'Snapshot' packet; true - 'Incremental' packet
    bool msg_flag_poss_dup = tmp & 0x10; //  false - flag of broadcasting online updates, true - flag of broadcasting full order-books in the form of Incremental packages
    uint64_t msg_ts = file_parser->get_uint64(false); // nanosecs

    *output_stream << "{\"timestamp\":\"" << ts2str(msg_ts) << "\", \"messages\":[";

    // heade is included into packet size, subtract
    msg_size -= 16;
    
    // data
    if (msg_flag_is_incremental){
        // incremental header + multiple SBE messages (SBE header + root block)
        uint64_t incr_p_ts = file_parser->get_uint64(false);
        uint32_t incr_p_session_id = file_parser->get_uint32(false);
        msg_size -= 12;

        // SBE
        // header
        while(msg_size > 0){
            uint16_t sbe_data_length = file_parser->get_uint16(false); // in bytes; without SBE header and without 'NoMDEntries' field groups
            uint16_t sbe_template_id = file_parser->get_uint16(false);
            uint16_t sbe_scheme_id = file_parser->get_uint16(false);
            uint16_t sbe_version = file_parser->get_uint16(false);
            msg_size -= 8;
            // Root block (FIX message)
            // Repeat > SBE header

            // root block
            uint16_t bytes_read = parse_moex_message(file_parser, output_stream, sbe_template_id, msg_size, sbe_data_length);
            msg_size -= bytes_read;
        }
    }else{
        // snapshots: one SBE message (header + root block), repeating section header + 1..N sections

        // SBE
        // header
        uint16_t sbe_data_length = file_parser->get_uint16(false); // in bytes; without SBE header and without 'NoMDEntries' field groups
        uint16_t sbe_template_id = file_parser->get_uint16(false);
        uint16_t sbe_scheme_id = file_parser->get_uint16(false);
        uint16_t sbe_version = file_parser->get_uint16(false);
        msg_size -= 8;

        // Root block (FIX message)
        msg_size -= parse_moex_message(file_parser, output_stream, sbe_template_id, msg_size, msg_size);
    }

    *output_stream << "]}," << endl;
}

void parse_ip_packet(FileReader *file_parser, ofstream* output_stream){
    // packet header
    uint32_t ts = file_parser->get_uint32(false);
    uint32_t precision_ts = file_parser->get_uint32(false);
    uint32_t packet_length = file_parser->get_uint32(false);
    uint32_t orig_packet_length = file_parser->get_uint32(false); // if orig > packet_len -> packet was truncated!

    // packet data ... (packet_length bytes)
    // Ethernet header
    // MAC
    uint64_t eth_dst = (file_parser->get_uint32() << 16) + file_parser->get_uint16();
    uint64_t eth_src = (file_parser->get_uint32() << 16) + file_parser->get_uint16();
    uint16_t eth_type = file_parser->get_uint16();
    if (eth_type != 0x0800){
        throw runtime_error("Unsupported ethernet payload. Expected IPv4, got: " + eth_type);
    }

    // Eth Data
    // IP header
    uint16_t tmp = file_parser->get_uint16();
    unsigned char ip_ver = (0xF000 & tmp) >> 12;
    if (ip_ver != 4){
        throw runtime_error("Only IPv4 is supported");
    }

    unsigned char ip_head_len = (tmp & 0x0F00) >> 8;
    // DSCP (8:13)
    // ECN  (14:15)
    uint16_t ip_total_len = file_parser->get_uint16();
    uint16_t ipv4_id = file_parser->get_uint16();
    tmp = file_parser->get_uint16();
    bool ip_no_fragments = tmp & 0x4000;
    bool ip_more_fragments = tmp & 0x2000;
    uint16_t ip_frag_offset = tmp & 0x1FFF;
    tmp = file_parser->get_uint16();
    uint8_t ip_ttl = (tmp & 0xFF00) >> 8;
    uint8_t ip_transport_protocol = tmp & 0x00FF; // 17 for UDP or 6 for TCP
    // TODO: verify?
    uint16_t ip_crc = file_parser->get_uint16();
    uint32_t ip_src = file_parser->get_uint32();
    uint32_t ip_dst = file_parser->get_uint32();
    // IPv4 options, just skip (?)
    file_parser->skip((ip_head_len-5)*4);

    // IP Data
    if (ip_transport_protocol == 17){
        //UDP
        uint16_t udp_src = file_parser->get_uint16();
        uint16_t udp_dst = file_parser->get_uint16();
        uint16_t udp_len = file_parser->get_uint16();
        uint16_t udp_crc = file_parser->get_uint16();

        // UDP data
        parse_simba_packet(file_parser, output_stream);
    }else{
        char err_msg[64];
        sprintf(err_msg, "Unsupported transport protocol: %d\n", ip_transport_protocol);
        throw runtime_error(err_msg);
    }
}


int main(int argc, char ** argv){
    if(argc == 1){
        throw invalid_argument("Argument FILE was expected. None received.");
    }

    FileReader* fs = new FileReader(argv[1]);

    // output file arg
    string out_path = "output.json";
    for (int i = 0; i < argc; ++i){
        if (string(argv[i]) == "-o"){
            // flag found, check if has following arg
            if(i + 1 == argc){
                throw runtime_error("Output file argument missing. Expected input: parser FILE -o OUTPUT_FILE");
            }
            out_path = string(argv[i+1]);
            break;
        }
    }
    ofstream* out_stream = new ofstream(out_path, ios_base::out);

    // pcap header
    uint32_t magic_number = fs->get_uint32(false);

    bool format_nanosec = false;
    if (magic_number == 0xA1B2C3D4){
        // microsec
        format_nanosec = false;
    }else if (magic_number == 0xA1B23C4D)
    {
        format_nanosec = true;
    }else{
        throw runtime_error("PCAP Magic number doesn't match; the file might be corrupted: " + string(argv[1]));
    }

    uint16_t major_ver = fs->get_uint16(false);
    uint16_t minor_ver = fs->get_uint16(false);

    // reserved fields
    fs->skip(8);

    uint32_t snap_len = fs->get_uint32(false);

    uint16_t temp = fs->get_uint16(false);
    bool is_fcs_set = temp & 0x1000;
    int fcs_words_num = (temp & 0xE0000) >> 13;

    uint16_t link_type = fs->get_uint16(false);
    if (link_type != 0){
        throw runtime_error("Unsupported LinkType: " + link_type);
    }

    *out_stream << "{\"packets\":[\n";
    // packet
    while(!(fs->eof() || fs->last_byte())){
        parse_ip_packet(fs, out_stream);
    }
    *out_stream << "\n]}";
    out_stream->close();
    cout << "success!";
}
