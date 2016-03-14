#ifndef MYQ_H_INCLUDED
#define MYQ_H_INCLUDED

//
// Author:    Oslo Wong
// File:      dtpr-queue.h
// Written:   01/07/14 (for ns-2.35)

#include <string.h>
#include "queue.h"
#include "address.h"

#include <iostream>
#include <list>
#include <algorithm>

using namespace std;

typedef list<int> INTLIST;
bool findInList(INTLIST l, int key);

class DtPrQueue: public Queue {
public:
	DtPrQueue();
	~DtPrQueue();
	int getQnum();

protected:
	void enque(Packet*);
	void setQNum(int num);
	bool isQueueFull();
	Packet* deque();
	int command(int argc, const char* const * argv);
	void print();
	void setMaxPriority(int num);

	PacketQueue *qList;
	int qNum;
	// add
	int maxPriority;

	// 20160311
	INTLIST *fidRecord;
	bool delayCheck;

	int getDelayCheck();
	void enableDelayCheck();

	bool isFidPrior(int jobId);
	void addFidPrior(int jobId);
	void removeFidPrior(int jobId);

private:
	int defaultPrior;    // 用来设置默认优先级。用于scheduleDelay情景下，没有对应优先级时设置的默认优先级。

};

#endif // MYQ_H_INCLUDED

