#include "../interface.h"



class Receiver{
private:
    int sockfd;
    seq_type RcvBase;
    seq_type LastByteRead;
    seq_type LastByteRcvd;

    uint16_t windowSize;

    map<seq_type, shared_ptr<Segment>> window;
    map<seq_type, shared_ptr<Segment>>::iterator nextEraseWindow;
    vector<int> scenario;

    sockaddr_in getAddr;
    socklen_t getAddrSize;
    
    mutex M_Print;

    bool connection_finished;
    bool connection_started;
    system_clock::time_point startTime;

    void printWindow(string str);
public:
    Receiver();
    ~Receiver();
    void pollingReceive();
    void sendAppData();
};