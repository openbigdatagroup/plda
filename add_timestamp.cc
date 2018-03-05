//
// Created by Donguk Lim on 2018. 2. 20..
//

#include "add_timestamp.h"


std::string AddTimeStamp::get_time(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf1[40];
    char buf2[40];
    tstruct = *localtime(&now);
    //format: day DD-MM-YYYY
    strftime(buf1, sizeof(buf1), "%Y/%m/%d", &tstruct);
    std::string date_str(buf1);
    strftime(buf2, sizeof(buf2), "%X", &tstruct);
    std::string time_str(buf2);

    return "[" + date_str + " " + time_str + "] ";
}