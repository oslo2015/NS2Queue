#ifndef MYQ_H_INCLUDED
#define MYQ_H_INCLUDED

//
// Author:    Oslo Wong
// File:      dtpr-queue.h
// Written:   01/07/14 (for ns-2.35)

#include <string.h>
#include "queue.h"
#include "address.h"

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
};

#endif // MYQ_H_INCLUDED

