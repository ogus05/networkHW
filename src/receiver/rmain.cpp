#include "receiver.h"

int main(){
    Receiver receiver;
    thread pollingReceiveThread = thread(&Receiver::pollingReceive, &receiver);
    thread sendAppDataThread = thread(&Receiver::sendAppData, &receiver);

    pollingReceiveThread.join();
    sendAppDataThread.join();
}