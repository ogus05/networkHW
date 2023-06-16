#include "sender.h"

Sender::Sender()
{

    Interface interface;
    ifstream ifs(interface.sender_scenario_file);
    if(!ifs){
        cout << "sender scenario file open error" << endl;
        exit(1);
    }

    if(scenario.size() % 2 != 0){
        cout << "sender scenario file count error" << endl;
        exit(1);
    }
    string ss;

    while(ifs >> ss){
        if(stoi(ss) > 255){
            cerr << "A segment length must be smaller than 256bit" << endl;
            exit(1);
        }
        scenario.push_back(stoi(ss));
    }
    if(interface.sender_init_seq_no < 1){
        cerr << "A segment init sequence must be larger than 1" << endl;
        exit(1);
    }

    LastByteWritten = interface.sender_init_seq_no - 1;
    AdvWindow = 0;
    NextSeqNum = interface.sender_init_seq_no;
    LastByteSent = interface.sender_init_seq_no - 1;
    SendBase = interface.sender_init_seq_no;
    LastByteAcked = interface.sender_init_seq_no - 1;
    WindowSize = interface.sender_window_size;

    timeout = interface.sender_timeout_value;

    timer = nullptr;

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(interface.receiver_port_number);
    servaddr.sin_addr.s_addr = htonl(interface.receiver_ip_addr);
    servaddrSize = sizeof(servaddr);

    chanaddr.sin_family = AF_INET;
    chanaddr.sin_port = htons(interface.channel_port_number);
    chanaddr.sin_addr.s_addr = htonl(interface.channel_ip_addr);
    chanaddrSize = sizeof(chanaddr);


    scenario_finished = connection_finished = false;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        exit(EXIT_FAILURE);
    }

    sockaddr_in clientaddr;
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(interface.sender_port_number);
    clientaddr.sin_addr.s_addr = htonl(interface.sender_ip_addr);

    if ( bind(sockfd, (const struct sockaddr *)&clientaddr, 
            sizeof(clientaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
        close(sockfd);
    }
    startTime = system_clock::now();
}

Sender::~Sender(){
    close(sockfd);
}

void Sender::connect()
{
    Segment seg(SendBase, 0, 0, Segment::TYPE_CON);
    sendto(sockfd, seg.segmentToData().get(), Segment::SEG_SIZE, MSG_CONFIRM, (const struct sockaddr *)&servaddr, servaddrSize);
    shared_ptr<unsigned char> data(new unsigned char[Segment::SEG_SIZE]);
    recvfrom(sockfd, data.get(), Segment::SEG_SIZE,
                    MSG_WAITALL, (struct sockaddr *) &servaddr,
                    &servaddrSize);
    Segment ack(data.get());
    assert(ack.type == Segment::TYPE_CON);
    AdvWindow = ack.window;
}

void Sender::disconnect()
{
    Segment seg(0, 0, 0, Segment::TYPE_DIS);
    sendto(sockfd, seg.segmentToData().get(), Segment::SEG_SIZE, MSG_CONFIRM, (const struct sockaddr*)&servaddr, servaddrSize);
}

void Sender::printWindow(const string str)
{
    M_Print.lock();
    M_Info.lock();
    auto printLine = []() ->void {
        string str("");
        for(int i = 0; i < 60; i++) str.append("-");
        printf("%s %s\n", C_NRML, str.c_str());
    };
    duration<double> dur = system_clock::now() - startTime;
    cout << str << "\t\ttime: " << (int)(duration_cast<seconds>(dur).count()) << endl;
    printf("Free Window Size  = %u\n", WindowSize - (LastByteWritten - LastByteAcked));
    printLine();


    for(auto& e : window){
        seq_type beginSeq = e.second->startSeq;
        seq_type endSeq = e.second->startSeq + e.second->length - 1;
        string color;
        string specialSeq("\t<-");
        if(beginSeq < LastByteAcked){
            color = C_GREN;
        } else if(beginSeq < LastByteSent){
            color = C_YLLW;
        } else if(beginSeq < LastByteWritten){
            color = C_BLUE;
        }

        if(endSeq == LastByteWritten){
            specialSeq.append("(LastByteWritten)");
        }
        if(endSeq == LastByteSent){
            specialSeq.append("(LastByteSent)");
        }
        if(endSeq == LastByteAcked){
            specialSeq.append("(LastByteAcked)");
        }
        if(specialSeq == "\t<-") specialSeq = "";

        printf("%s %u\t\t%u%s\n",color.c_str(), beginSeq, endSeq, specialSeq.c_str());
        printLine();
    }
    cout << "\n\n";
    M_Info.unlock();
    M_Print.unlock();
}

void Sender::printSendData(const shared_ptr<WindowComponent> wc, const bool resend)
{
    M_Print.lock();
    auto printLine = []() ->void {
        string str("");
        for(int i = 0; i < 60; i++) str.append("-");
        printf("%s %s\n", C_NRML, str.c_str());
    };
    printLine();
    duration<double> d(wc->sentTime - startTime);
    if(resend){
        printf("%s resend data -> time: %d, length: %u\n", C_PRPL, static_cast<int>(d.count()), wc->length);
    }else{
        printf("%s send data -> time: %d, length: %u\n", C_RED, static_cast<int>(d.count()), wc->length);
    }
    printLine();
    cout << "\n\n";
    M_Print.unlock();
}

void Sender::getAppData()
{
	while(scenario.size() > 1){
        if(LastByteWritten - SendBase + scenario.front() > WindowSize){
            sleep(1);
        }else{
            window.insert(make_pair(LastByteWritten + 1, new WindowComponent(scenario.front(), system_clock::now(), LastByteWritten + 1)));
            M_Info.lock();
            LastByteWritten += scenario.front();
            M_Info.unlock();
            scenario.pop_front();
            printWindow("getAppData");
            sleep(1 * scenario.front());
            scenario.pop_front();
        }
    }
    scenario_finished = true;
}

void Sender::pollingWindow()
{
    while(1){
        sleep(1);
        if(connection_finished) break;
        auto wc = window.find(LastByteSent + 1);
        if(wc != window.end()) {
            thread thr = thread(&Sender::sendData, this, (*wc).second, false);
            thr.detach();
        }
    }
}

void Sender::sendData(const shared_ptr<WindowComponent> wc, const bool resend)
{
    if(!resend){
        if(AdvWindow >= wc->length && LastByteSent < LastByteWritten){
            Segment seg(wc->startSeq, 0, wc->length, Segment::TYPE_REQ);
            sendto(sockfd, seg.segmentToData().get(), Segment::SEG_SIZE, MSG_CONFIRM, (const struct sockaddr*)&chanaddr, chanaddrSize);
            M_Info.lock();
            NextSeqNum =  wc->startSeq + wc->length;
            AdvWindow -= wc->length;
            LastByteSent = wc->startSeq + wc->length - 1;
            M_Info.unlock();
            wc->sentTime = system_clock::now();
            printSendData(wc, resend);
            printWindow("sendData");
            if(wc->startSeq == SendBase) {
                startTimer(timeout);
            }
        }
    }else{
        Segment seg(wc->startSeq, 0, wc->length, Segment::TYPE_REQ);
        sendto(sockfd, seg.segmentToData().get(), Segment::SEG_SIZE, MSG_CONFIRM, (const struct sockaddr*)&chanaddr, chanaddrSize);
        wc->sentTime = system_clock::now();
        printSendData(wc, resend);
        printWindow("resendData");
        if(wc->startSeq == SendBase) {
            startTimer(timeout);
        }
    }
    
}

void Sender::startTimer(const seconds time){
    timer.reset(new promise<bool>);
    future<bool> f;
    f = timer->get_future();
    future_status fs = f.wait_for(time);
    if(fs == future_status::timeout){
        auto timeoutWindow = window.find(SendBase);
        sendData((*timeoutWindow).second, true);
    }
}

void Sender::stopTimer()
{
    if(LastByteAcked < LastByteSent){
        try{
            timer->set_value(true);
        } catch(const std::future_error& e){
            
        }
        auto nextWindow = window.find(SendBase);
        if(nextWindow != window.end()){
            duration<float> dur = (system_clock::now() - ((*nextWindow).second->sentTime));
            startTimer(timeout - duration_cast<seconds>(dur));

        }
    }
}

void Sender::receiveData(){
    shared_ptr<unsigned char> data(new unsigned char[Segment::SEG_SIZE]);
    while(1){
        data.reset(new unsigned char[Segment::SEG_SIZE]);
        recvfrom(sockfd, data.get(), Segment::SEG_SIZE, MSG_WAITALL, (struct sockaddr*)&chanaddr, &chanaddrSize);
        Segment ack(data.get());
        if(ack.type == Segment::TYPE_DIS && scenario_finished){
            connection_finished = true;
            break;
        }else if(ack.type == Segment::TYPE_ACK){
            M_Info.lock();
            AdvWindow = ack.window;
            if(ack.seq > SendBase){
                auto eraseWindow = window.begin();
                while((eraseWindow->second->startSeq + eraseWindow->second->length) != ack.seq){
                    eraseWindow = window.erase(eraseWindow);
                }
                SendBase = ack.seq;
                thread t = thread(&Sender::stopTimer, this);
                t.detach();
                LastByteAcked = ack.seq - 1;
            }
            M_Info.unlock();
            printWindow("receiveData");
            if(scenario_finished && LastByteAcked == LastByteWritten){
                disconnect();
            }
        } else{
            cerr << "receive wrong data type : " << (int)ack.type << endl;
            exit(1);
        }
    }
}