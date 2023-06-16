#include "../interface.h"


struct WindowComponent{
    uint8_t length;
    system_clock::time_point sentTime;
    seq_type startSeq;
    WindowComponent(uint8_t length, system_clock::time_point sentTime, seq_type seq)
        :length(length), sentTime(sentTime), startSeq(seq){};
};


class Sender{
private:
    int sockfd;
    sockaddr_in	 servaddr;
    socklen_t servaddrSize;
    sockaddr_in	 chanaddr;
    socklen_t chanaddrSize;

    seq_type LastByteWritten;
    seq_type NextSeqNum;
    seq_type LastByteSent;
    seq_type SendBase;

    uint16_t AdvWindow;
    uint32_t LastByteAcked;
    uint16_t WindowSize;
    seconds timeout;
    
    list<int> scenario;

    map<seq_type, shared_ptr<WindowComponent>> window;
    system_clock::time_point startTime;


    unique_ptr<promise<bool>> timer;
    
    mutex M_Info;
    mutex M_Print;

    bool scenario_finished;
    bool connection_finished;
    
    void sendData(const shared_ptr<WindowComponent> wc, const bool resend);
    void startTimer(const seconds time);
    void stopTimer();
    void disconnect();
    void printWindow(const string str);
    void printSendData(const shared_ptr<WindowComponent> wc, const bool resend);
public:
    Sender();
    ~Sender();
    void connect();
    void pollingWindow();
    void getAppData();
    void receiveData();
};