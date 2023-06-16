#include "interface.h"

Interface::Interface()
{
    ifstream file("../../RDTP.conf");
    string key;
    string value;
    while(file >> key){
        file >> value >> value;
        if(!key.compare("sender_ip_addr")){
            vector<int> addr;
            int pos = 0;
            int cur_pos = 0;
            while(cur_pos = value.find(".", pos)){
                addr.push_back(stoi(value.substr(pos, cur_pos - pos)));
                pos = cur_pos + 1;
                if(cur_pos == string::npos) break;
            }
            sender_ip_addr = (addr[0] << 24 | addr[1] << 16 | addr[2] << 8 | addr[3]);
        }else if(!key.compare("sender_port_number")){
            sender_port_number = stoi(value);
        }else if(!key.compare("sender_window_size")){
            sender_window_size = stoi(value);
        }else if(!key.compare("sender_init_seq_no")){
            sender_init_seq_no = stoi(value);
        }else if(!key.compare("sender_timeout_value")){
            sender_timeout_value = chrono::seconds(stoi(value));
        }else if(!key.compare("sender_scenario_file")){
            sender_scenario_file = value;
        }else if(!key.compare("receiver_ip_addr")){
            vector<int> addr;
            int pos = 0;
            int cur_pos = 0;
            while(cur_pos = value.find(".", pos)){
                addr.push_back(stoi(value.substr(pos, cur_pos - pos)));
                pos = cur_pos + 1;
                if(cur_pos == string::npos) break;
            }
            receiver_ip_addr = (addr[0] << 24 | addr[1] << 16 | addr[2] << 8 | addr[3]);
        }else if(!key.compare("receiver_port_number")){
            receiver_port_number = stoi(value);
        }else if(!key.compare("receiver_window_size")){
            receiver_window_size = stoi(value);
        }else if(!key.compare("receiver_scenario_file")){
            receiver_scenario_file = value;
        }else if(!key.compare("channel_ip_addr")){
            vector<int> addr;
            int pos = 0;
            int cur_pos = 0;
            while(cur_pos = value.find(".", pos)){
                addr.push_back(stoi(value.substr(pos, cur_pos - pos)));
                pos = cur_pos + 1;
                if(cur_pos == string::npos) break;
            }
            channel_ip_addr = (addr[0] << 24 | addr[1] << 16 | addr[2] << 8 | addr[3]);
        }else if(!key.compare("channel_port_number")){
            channel_port_number = stoi(value);
        }else if(!key.compare("channel_scenario_file")){
            channel_scenario_file = value;
        }else if(!key.compare("channel_latency")){
            channel_latency = stoi(value);
        } else if(!key.compare("channel_small_congestion_delay")){
            channel_small_congestion_delay = stoi(value);
        }else if(!key.compare("channel_big_congestion_delay")){
            channel_big_congestion_delay = stoi(value);
        } else{
            cout << key << endl;
            cerr << "Wrong Input in Configuration File" << endl;
        }
    }
}