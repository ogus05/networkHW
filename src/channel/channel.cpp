#include "channel.h"

Channel::Channel()
{
    Interface interface;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        exit(EXIT_FAILURE);
    }
    sockaddr_in chanaddr;
    socklen_t chanaddrSize;
    chanaddr.sin_family = AF_INET;
    chanaddr.sin_port = htons(interface.channel_port_number);
    chanaddr.sin_addr.s_addr = htonl(interface.channel_ip_addr);
    chanaddrSize = sizeof(chanaddr);

    
    if ( bind(sockfd, (const struct sockaddr *)&chanaddr, 
            sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(interface.receiver_port_number);
    servaddr.sin_addr.s_addr = htonl(interface.receiver_ip_addr);
    servaddrSize = sizeof(servaddr);
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(interface.sender_port_number);
    clientaddr.sin_addr.s_addr = htonl(interface.sender_ip_addr);
    clientaddrSize = sizeof(clientaddr);
    latency = interface.channel_latency;
    smallCongestionDelay = interface.channel_small_congestion_delay;
    bigCongestionDelay = interface.channel_big_congestion_delay;

    ifstream ifs(interface.channel_scenario_file);
    if(!ifs){
        cout << "channel file open error" << endl;
        exit(1);
    }

    string scene;
    ifs >> scene;
    if(scene.size() % 2 == 1){
        cout << "channel scenario file count error" << endl;
        exit(1);
    }
    for(int i = 0; i < scene.size(); i += 2){
        int curLatency;
        switch(scene[i]){
            case 'L':{
                curLatency = 0;
            }break;
            case 'N':{
                curLatency = latency;
            }break;
            case 'C':{
                curLatency = latency + bigCongestionDelay;
            }break;
            case 'c':{
                curLatency = latency + smallCongestionDelay;
            } break;
            default:{
                cout << "channel scenario file type error" << endl;
                exit(1);
            }
        }
        if(scene[i + 1] == '*'){
            scenario.push_back(curLatency);
            scenario.push_back(-1);
        }else{
            for(int j = 0; j < scene[i + 1] - '0'; j++){
                scenario.push_back(curLatency);
            }
        }
    }
}

Channel::~Channel(){
    close(sockfd);
}

void Channel::StartSimulation()
{
    cout << "start simulation...." << endl;
    int i = 0;
    while(1){
        sockaddr_in	 getaddr;
        socklen_t getaddrSize = sizeof(getaddr);
        shared_ptr<unsigned char> buffer(new unsigned char[Segment::SEG_SIZE]);
        recvfrom(sockfd, buffer.get(), Segment::SEG_SIZE, MSG_WAITALL, (struct sockaddr*)&getaddr, &getaddrSize);
        if(scenario[i] != 0){
            thread t = thread(&Channel::WaitandSend, this, scenario[i], buffer, getaddr);
            t.detach();
        }
        if(scenario[i + 1] != -1) i++;
        if(scenario.size() < i) i = 0;
    }
    cout << "end simulation...." << endl;
}

void Channel::WaitandSend(const int latency, const shared_ptr<unsigned char> buffer, const sockaddr_in getAddr)
{
    Segment seg(buffer.get());
    auto PrintInfo = [&](bool isServer)->void {
        cout << "send data to" << (isServer ? "server" : "client") << "\tlatency: " << latency << "\tsequence: " << (int)seg.seq << endl;

    };
    if(getAddr.sin_addr.s_addr == servaddr.sin_addr.s_addr && getAddr.sin_port == servaddr.sin_port){
        PrintInfo(false);
        sleep(latency);
        sendto(sockfd, buffer.get(), Segment::SEG_SIZE, MSG_CONFIRM, (const struct sockaddr*)&clientaddr, sizeof(clientaddr));
    }
    else if(getAddr.sin_addr.s_addr == clientaddr.sin_addr.s_addr && getAddr.sin_port == clientaddr.sin_port){
        PrintInfo(true);
        sleep(latency);
        sendto(sockfd, buffer.get(), Segment::SEG_SIZE, MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr));
    }
}
