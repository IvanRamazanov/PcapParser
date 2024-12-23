#include "moex_schemes.h"

using namespace std;

int64_t INT64_NULL = 9223372036854775807;
double DECIMAL_5_NULL = 9223372036854775807 / 10'000.0;

uint16_t parse_moex_message(FileReader* file_stream, ofstream *out_stream, const uint16_t template_id, const uint16_t data_length, const uint16_t skip_size){
    bool big_endian = false;

    *out_stream << "{\"TemplateID\":" << template_id;
    switch(template_id){
        case 15: // Order Update
        {
            uint16_t scheme_size = 50;
            if (data_length < scheme_size){
                throw runtime_error("Not enough data for Order Update message.");
            }

            *out_stream << ", \"MDEntryID\":" << (int64_t)file_stream->get_uint64(big_endian); // order ID
            double price = file_stream->get_decimal5(big_endian);
            *out_stream << ", \"MDEntryPx\":" << fixed << setprecision(5) << price; // order price
            *out_stream << ", \"MDEntrySize\":" << (int64_t)file_stream->get_uint64(big_endian); // order volume

            uint64_t flags1 = file_stream->get_uint64(); // MDFlagsSet
            *out_stream << ", \"MDFlags\":" << flags1;

            uint64_t flags2 = file_stream->get_uint64(); // MDFlags2Set. Always 0 (?)
            int32_t security_id = (int32_t)file_stream->get_uint32(big_endian);
            uint32_t rpt_seq_num = file_stream->get_uint32(big_endian);

            uint8_t action = file_stream->get_uint8(); //MDUpdateAction (0 - new; 1 - change; 2 - delete)
            *out_stream << ", \"MDUpdateAction\":" << +action;
            char entry_type = file_stream->get_char(); //MDEntryType ('0' - Bid; '1' - Offer; 'J' - Empty Book)
            *out_stream << ", \"MDEntryType\":\"" <<  entry_type << "\"";
            if (entry_type != '0' && entry_type != '1' && entry_type != 'J'){
                throw runtime_error("Corrupted entry type!");
            }

            *out_stream << "},";
            return scheme_size;
        }
        case 16: // Order Execution
        {
            uint16_t scheme_size = 74;
            if (data_length < scheme_size){
                throw runtime_error("Not enough data for Order Update message.");
            }

            *out_stream << ", \"MDEntryID\":" << (int64_t)file_stream->get_uint64(big_endian); // order ID

            // order price
            double price = file_stream->get_decimal5(big_endian);
            if (price != DECIMAL_5_NULL){
                *out_stream << ", \"MDEntryPx\":" << fixed << setprecision(5) << price;
            }

            // Remaining quantity
            int64_t rem_quantity = (int64_t)file_stream->get_uint64(big_endian);
            if (rem_quantity != INT64_NULL){
                *out_stream << ", \"MDEntrySize\":" << rem_quantity;
            }

            double trade_price = file_stream->get_decimal5(big_endian);
            *out_stream << ", \"LastPx\":" << fixed << setprecision(5) << trade_price;
            *out_stream << ", \"LastQty\":" << (int64_t)file_stream->get_uint64(big_endian); // trade volume
            *out_stream << ", \"TradeID\":" << (int64_t)file_stream->get_uint64(big_endian); // trade ID

            uint64_t flags1 = file_stream->get_uint64(); // MDFlagsSet
            *out_stream << ", \"MDFlags\":" << flags1;

            uint64_t flags2 = file_stream->get_uint64(); // MDFlags2Set. Always 0 (?)
            int32_t security_id = (int32_t)file_stream->get_uint32(big_endian);
            uint32_t rpt_seq_num = file_stream->get_uint32(big_endian);

            uint8_t action = file_stream->get_uint8(); //MDUpdateAction (0 - new; 1 - change; 2 - delete)
            *out_stream << ", \"MDUpdateAction\":" << +action;
            char entry_type = file_stream->get_char(); //MDEntryType ('0' - Bid; '1' - Offer; 'J' - Empty Book)
            *out_stream << ", \"MDEntryType\":\"" <<  entry_type << "\"";
            if (entry_type != '0' && entry_type != '1' && entry_type != 'J'){
                throw runtime_error("Corrupted entry type!");
            }

            *out_stream << "},";
            return scheme_size;
        }
        case 17: // Order Book Snapshot
        {
            uint16_t scheme_size = 19; // size of the 'header'
            if (data_length < scheme_size){
                throw runtime_error("Not enough data for Order Update message.");
            }

            int32_t security_id = (int32_t)file_stream->get_uint32(big_endian); // ??
            int32_t last_msg_seq_num_processed = file_stream->get_uint32(big_endian); // The 'MsgSeqNum' of the last message sent into incremental feed at the time of the current snapshot generation.
            uint32_t rpt_seq_num = file_stream->get_uint32(big_endian);
            uint32_t session_id = file_stream->get_uint32(big_endian);
            *out_stream << ", \"ExchangeTradingSessionID\":" << session_id;

            // groupSize (Number of 'MDEntry' records in the current message)
            // block length (uint16) + numInGroup (uint8)
            uint16_t gs_block_len = file_stream->get_uint16(big_endian);
            uint16_t gs_num_in_grp = file_stream->get_uint8();

            // repeat
            uint16_t repeat_available = data_length - scheme_size;
            uint16_t block_len = 57; // Note: should be == gs_block_len, but to be safe - hardcode it

            // loop
            *out_stream << ", \"MDEntries\":[";
            for (uint16_t i = 0; i < gs_num_in_grp; ++i){
                if(repeat_available < block_len){
                    throw runtime_error("Not enough data for Order Update message.");
                }

                *out_stream << "{";

                int64_t order_id = (int64_t)file_stream->get_uint64(big_endian);
                if (order_id != INT64_NULL){
                    *out_stream << "\"MDEntryID\": " <<  order_id << ", ";
                }
                
                *out_stream << "\"TransactTime\":\"" << ts2str((file_stream)->get_uint64(big_endian)) << "\", "; // transaction timestamp

                double price = file_stream->get_decimal5(big_endian);
                if (price != DECIMAL_5_NULL){
                    // order price
                    *out_stream << "\"MDEntryPx\":" << fixed << setprecision(5) << price << ", ";
                }

                order_id = (int64_t)file_stream->get_uint64(big_endian);
                if (order_id != INT64_NULL){
                    // order volume
                    *out_stream << "\"MDEntrySize\":" << order_id << ", ";
                }

                order_id = (int64_t)file_stream->get_uint64(big_endian);
                if (order_id != INT64_NULL){
                    // trade ID
                    *out_stream << "\"TradeID\":" << order_id << ", ";
                }

                uint64_t flags1 = file_stream->get_uint64(); // MDFlagsSet
                *out_stream << "\"MDFlags\":" << flags1 << ", ";
                uint64_t flags2 = file_stream->get_uint64(); // MDFlags2Set. Always 0 (?)

                char entry_type = file_stream->get_char(); //MDEntryType ('0' - Bid; '1' - Offer; 'J' - Empty Book)
                *out_stream << "\"MDEntryType\":\"" <<  entry_type << "\"},";

                if (entry_type != '0' && entry_type != '1' && entry_type != 'J'){
                    throw runtime_error("Corrupted entry type!");
                }

                repeat_available -= block_len;
                scheme_size += block_len;
            }
            *out_stream << "]},";
            return scheme_size;
        }
        case 14: // Best Prices
        {
            uint16_t gs_block_len = file_stream->get_uint16(big_endian);
            uint16_t gs_num_in_grp = file_stream->get_uint8();

            if (data_length < 3 + gs_num_in_grp * gs_block_len){
                throw runtime_error("Not enough data for Best Prices message.");
            }
            // just skip
            file_stream->skip(gs_num_in_grp * gs_block_len);
            *out_stream << "},";
            return gs_num_in_grp * gs_block_len + 3;
        }
        case 19: // Security Mass Status
        {
            uint16_t gs_block_len = file_stream->get_uint16(big_endian);
            uint16_t gs_num_in_grp = file_stream->get_uint16(big_endian);
            if (data_length < 4 + gs_num_in_grp * gs_block_len){
                throw runtime_error("Not enough data for Best Prices message.");
            }
            // just skip
            file_stream->skip(gs_num_in_grp * gs_block_len);
            *out_stream << "},";
            return gs_num_in_grp * gs_block_len + 4;
        }
        default:
        {
            //cout << "Unsupported Msg Template: " << template_id << endl;
            file_stream->skip(skip_size);
            *out_stream << "},";
            return skip_size;
        }
    }
}
