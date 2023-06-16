#pragma once
#include <iostream>
#include <fstream>
#include <cassert>
#include <unistd.h>
#include <string>
#include <cstring>
#include <list>
#include <map>
#include <vector>
#include <chrono>
#include <future>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>


typedef int32_t seq_type;

using namespace std;
using namespace chrono;

#define C_NRML "\033[0m"
#define C_BLCK "\033[30m"
#define C_RED  "\033[31m"
#define C_GREN "\033[32m"
#define C_YLLW "\033[33m"
#define C_BLUE "\033[34m"
#define C_PRPL "\033[35m"
#define C_AQUA "\033[36m"

class Interface{
public:
    Interface();
    uint32_t sender_ip_addr;
    uint8_t sender_port_number;
    uint16_t sender_window_size;
    seq_type sender_init_seq_no;
    chrono::seconds sender_timeout_value;
    string sender_scenario_file;
    uint32_t receiver_ip_addr;
    uint8_t receiver_port_number;
    uint16_t receiver_window_size;
    string receiver_scenario_file;
    uint32_t channel_ip_addr;
    uint8_t channel_port_number;
    string channel_scenario_file;
    int channel_latency;
    int channel_small_congestion_delay;
    int channel_big_congestion_delay;
};

class Segment{
public:
    seq_type seq;
    uint16_t window;
    uint8_t length;
    uint8_t type;

    static const int SEG_SIZE = 8;


    static const uint8_t TYPE_CON = 0xf0;
    static const uint8_t TYPE_DIS = 0xf1;
    static const uint8_t TYPE_REQ = 0xf2;
    static const uint8_t TYPE_ACK = 0xf3;


    Segment(seq_type seq = 0, uint16_t window = 0, uint8_t length = 0, uint8_t type = 0)
        :seq(seq), window(window), length(length), type(type) {}

    Segment(const unsigned char* data){
        seq = 0;
        for(int i =0 ; i < 4; i++){
            seq = seq | (data[i]  << (i * 8));
        }
        window = 0;
        for(int i = 4; i < 6; i++){
            window = window | (data[i] << ((i - 4) * 8));
        }
        length = data[6];
        type = data[7];
    }

    shared_ptr<unsigned char> segmentToData(){
        shared_ptr<unsigned char> data(new unsigned char[SEG_SIZE]);
        for(int i = 0 ;i < 4; i++){
            data.get()[i] = ((seq & (0xff << (i * 8))) >> (i * 8));
        }
        for(int i = 4; i < 6; i++){
            data.get()[i] = ((window & (0xff << ((i - 4) * 8))) >> ((i - 4) * 8));
        }
        data.get()[6] = length;
        data.get()[7] = type;
        return data;
    }
};

