#include "receiver.h"

Receiver::Receiver()
{
    Interface interface;
    ifstream ifs(interface.receiver_scenario_file);
    if(!ifs){
        cout << "receiver scenario file open error" << endl;
        exit(1);
    }

    string ss;

    while(ifs >> ss){
        scenario.push_back(stoi(ss));
    }
    if(scenario.size() % 2 != 0){
        cout << "receiver scenario file count error" << endl;
        exit(1);
    }
    ifs.close();
    
    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(interface.receiver_port_number);
    servaddr.sin_addr.s_addr = htonl(interface.receiver_ip_addr);
    windowSize = interface.receiver_window_size;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        exit(EXIT_FAILURE);
    }
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, 
            sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    getAddrSize = sizeof(getAddr);
    RcvBase = 0;
    LastByteRcvd = 0;
    LastByteRead = 0;
    nextEraseWindow = window.end();

    connection_finished = connection_started = false;
}

Receiver::~Receiver(){
    close(sockfd);
}

void Receiver::printWindow(string str)
{
    M_Print.lock();
    duration<double> dur = system_clock::now() - startTime;
    cout << str << "\t\ttime: " << (int)(duration_cast<seconds>(dur).count()) << endl;
    auto printLine = []() ->void {
        string str("");
        for(int i = 0; i < 60; i++) str.append("-");
        printf("%s %s\n", C_NRML, str.c_str());
    };
    printf("Free Window Size  = %u\n", windowSize - (LastByteRcvd - LastByteRead));
    printLine();
    if(nextEraseWindow == window.end()){
        string specialSeq("\t<-");
        specialSeq.append("(LastByteRead)");
        if(LastByteRcvd == LastByteRead){
            specialSeq.append("(LastByteRcvd)");
        }
        printf("%s %s\t\t%s%s\n", C_BLCK, "dummy", "dummy", specialSeq.c_str());
        printLine();
    }    


    for(auto& e : window){
        seq_type beginSeq = e.second.get()->seq;
        seq_type endSeq = e.second.get()->seq + e.second.get()->length - 1;
        string color;
        string specialSeq("\t<-");
        if(beginSeq < LastByteRead){
            color = C_GREN;
        } else if(beginSeq < LastByteRcvd){
            color = C_YLLW;
        }

        if(endSeq == LastByteRead){
            specialSeq.append("(LastByteRead)");
        }
        if(endSeq == LastByteRcvd){
            specialSeq.append("(LastByteRcvd)");
        }
        if(specialSeq == "\t<-") specialSeq = "";

        printf("%s %u\t\t%u%s\n",color.c_str(), beginSeq, endSeq, specialSeq.c_str());
        printLine();
    }
    cout << "\n\n";
    M_Print.unlock();
}

void Receiver::pollingReceive()
{
    shared_ptr<unsigned char> data(new unsigned char[Segment::SEG_SIZE]);
    while(1){
        data.reset(new unsigned char[Segment::SEG_SIZE]);
        recvfrom(sockfd, data.get(), Segment::SEG_SIZE, MSG_WAITALL, (sockaddr *)&getAddr, &getAddrSize);
        shared_ptr<Segment> seg(new Segment(data.get()));
        switch(seg->type){
            case(Segment::TYPE_CON):{
                cout << "receive connect" << endl;
                RcvBase = seg->seq;
                LastByteRcvd = seg->seq - 1;
                LastByteRead = seg->seq - 1;
                Segment ack(RcvBase, windowSize, 0, Segment::TYPE_CON);
                sendto(sockfd, ack.segmentToData().get(), Segment::SEG_SIZE, MSG_CONFIRM, (sockaddr *)&getAddr, getAddrSize);
                connection_started = true;
                startTime = system_clock::now();
            }break;
            case(Segment::TYPE_DIS):{
                cout << "receive disconnect" << endl;
                if(connection_started){
                    connection_finished = true;
                    Segment ack(0, 0, 0, Segment::TYPE_DIS);
                    sendto(sockfd, ack.segmentToData().get(), Segment::SEG_SIZE, MSG_CONFIRM, (sockaddr *)&getAddr, getAddrSize);
                    return;
                }
            }break;
            case(Segment::TYPE_REQ):{
                if(connection_started){
                    if(seg->seq == RcvBase){
                        window.insert(pair<seq_type, shared_ptr<Segment>>(seg->seq, seg));
                        LastByteRcvd = seg->seq + seg->length - 1;
                        RcvBase = seg->seq + seg->length;
                        Segment ack(RcvBase, windowSize - (LastByteRcvd - LastByteRead), 0, Segment::TYPE_ACK);
                        sendto(sockfd, ack.segmentToData().get(), Segment::SEG_SIZE, MSG_CONFIRM, (sockaddr*)&getAddr, getAddrSize);
                        printWindow("receive request");;
                    }else{
                        cout << "receive but not a needed sequence." << endl;
                    }
                }
            }break;
            default:{
                cerr << "receive wrong data type : " << (int)seg->type << endl;
                exit(1);
            }
        }
    }
}


void Receiver::sendAppData(){
    while(!connection_started) {}
    auto nextScenario = scenario.begin();
    while(!connection_finished){
        bool needPrintWindow = false;
        sleep((*nextScenario));
        nextScenario++;
        nextScenario++;
        if(nextScenario == scenario.end()) nextScenario = scenario.begin();
        while(1){
            auto windowComponentToRead = window.find(LastByteRead + 1);
            if(windowComponentToRead == window.end()) break;
            needPrintWindow = true;
            LastByteRead = windowComponentToRead->second->seq + windowComponentToRead->second->length - 1;
            if(nextEraseWindow != window.end()){
                window.erase(nextEraseWindow);
            }
            nextEraseWindow = windowComponentToRead;
        }
        if(needPrintWindow) {
            printWindow("sendAppData");
        }
    }
}