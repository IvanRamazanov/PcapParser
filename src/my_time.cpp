#include "my_time.h"
#include <iostream>

using namespace std;

string ts2str(uint32_t ts, const uint32_t extra, const bool extra_in_nanosec){
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const uint8_t days[] = {   31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31};

    // get year
    uint32_t year = 1970;
    uint32_t full_year;
    while(true){
        full_year = 365;
        if (year % 4 == 0){
            if (year % 100 == 0){
                if (year % 400 == 0){
                    // leap year
                    ++full_year;
                }
            }else{
                // leap year
                ++full_year;
            }
        }

        uint32_t year_secs = full_year * 24 * 60 * 60;
        if (year_secs > ts){
            // less seconds than in full year
            break;
        }else{
            // reduce by full year
            ts -= year_secs;
            ++year;
        }
    }

    uint32_t day = ts / (24 * 60 * 60);
    ts %= 24 * 60 * 60;
    uint32_t hour = ts / (60 * 60);
    ts %= 60 * 60;
    uint32_t minute = ts / 60;
    uint32_t secs = ts % 60;

    int month_idx = -1;
    for (int i=0; i < 12; ++i){
        if (day < days[i]){
            month_idx = i;
            day++; // add a day, since they are 0 based
            break;
        }
        day -= days[i];
    }
    
    ostringstream out;
    out << months[month_idx] << "-" << day << "-" << year << " " << \
    setfill('0') << setw(2) << hour << ":" << \
    setfill('0') << setw(2) << minute << ":" << \
    setfill('0') << setw(2) << secs << \
    "+" << extra << (extra_in_nanosec ? "ns" : "ms");
    return out.str();
}

string ts2str(uint64_t ts){
    return ts2str(ts / 1'000'000'000, ts % 1'000'000'000, true);
}
