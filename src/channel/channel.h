#include "../interface.h"

class Channel{
private:
    int sockfd;
    
    vector<int> scenario;

    sockaddr_in servaddr;
    socklen_t servaddrSize;
    
    sockaddr_in clientaddr;
    socklen_t clientaddrSize;

    int latency;
    int smallCongestionDelay;
    int bigCongestionDelay;
    void WaitandSend(const int latency, const shared_ptr<unsigned char> buffer, const sockaddr_in getAddr);
public:
    Channel();
    ~Channel();
    void StartSimulation();
};