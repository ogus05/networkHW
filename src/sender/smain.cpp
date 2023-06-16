#include "sender.h"


using namespace std;

int main() {
	Sender sender;
	sender.connect();
	thread getAppDataThread = thread(&Sender::getAppData, &sender);
	thread pollingWindowThread = thread(&Sender::pollingWindow, &sender);
	thread receiveDataThread = thread(&Sender::receiveData, &sender);

	getAppDataThread.join();
	pollingWindowThread.join();
	receiveDataThread.join();
}