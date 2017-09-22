#include <string>
using namespace std;

#define maxQueue 1024


#pragma once
class WorkQueue
{
public:
	WorkQueue();
	~WorkQueue();
	// Help from Robert Sedgewick, pg 30.
	bool addItem(string payload) {
		this->queue[this->tail++] = payload;
		if (this->tail > maxQueue) {
			this->tail = 0;
		}
		return true;
	};

	string getItem() {
		string payload = this->queue[this->head++];
		if (this->head > maxQueue) {
			this->head = 0;
		}
		return payload;
	};

private:
	static string queue[maxQueue + 1];
	int head = 0;
	int tail = 0;
};

