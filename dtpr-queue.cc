//
// Author:    Oslo Wong
// File:      dtpr-queue.cc
// Written:   01/07/15 (for ns-2.35)

#include "dtpr-queue.h"

static class DtPrQueueClass: public TclClass {
public:
	DtPrQueueClass() :
			TclClass("Queue/DTPR") {
	}
	TclObject* create(int, const char* const *) {
		return (new DtPrQueue);
	}
} class_dropt_tail_prior;

DtPrQueue::DtPrQueue() {
	qList = new PacketQueue[1];
	qNum = 0;
	maxPriority = 0;
}

DtPrQueue::~DtPrQueue() {
	delete[] qList;
}

void DtPrQueue::setQNum(int num) {
	if (num < 0) {
		printf("error num in setQNum().\n");
		exit(0);
	} else {
		reset();
		delete[] qList;
		qList = new PacketQueue[num + 1];
		qNum = num;
		if (qNum > 0)
			maxPriority = 1;
	}
}

void DtPrQueue::setMaxPriority(int num) {
	if (qNum > 0 && num > 1) {
		maxPriority = num;
		//printf("maxPriority = %d\n", maxPriority);
	}
}

bool DtPrQueue::isQueueFull() {
	int total = 0;
	int i;
	for (i = 0; i <= qNum; ++i) {
		total += qList[i].length();
	}
	return (total + 1) >= qlim_;
}

void DtPrQueue::enque(Packet* p) {
	hdr_ip* iph = hdr_ip::access(p);
	/// 队满，则drop
	/// 否则，加入到相对应的队列中
	if (isQueueFull()) {
		drop(p);
	} else {
		int p_fid = iph->flowid();
		p_fid = (p_fid > 999) ? p_fid / 1000 : p_fid;
		if (qNum > 0) {
			if (p_fid <= qNum && p_fid > 0) {
				if (maxPriority != p_fid) {
					//printf("######---%d:%d\n", p_fid, qlim_ / 2);
					int i, total = 0;
					for (i = 1; i <= qNum; ++i) {
						if (i != maxPriority) {
							total += qList[i].length();
						}
					}
					if (qList[maxPriority].length() > 0
							&& total >= qlim_ * 0.5) {
						//printf("######---%d:%d\n", p_fid, qlim_ / 2);
						drop(p);
						return;
					}
				}
				qList[p_fid].enque(p);

			} else {
				printf("$$$$$$$---%d\n", p_fid);
				qList[0].enque(p);
			}
		} else {
			qList[0].enque(p);
		}
	}
}

Packet* DtPrQueue::deque() {
	Packet *p;
	int i = 0;
	//printf("%d\n", qNum);
	for (i = 1; i <= qNum; ++i) {
		p = qList[i].deque();
		if (0 != p)
			return p;
	}
	p = qList[0].deque();
	//printf("%d\n", i);
	return p;
}

void DtPrQueue::print() {
	printf("######\n");
	int i;
	for (i = 0; i <= qNum; ++i) {
		printf("%d  ---  %d\n", i, qList[i].length());
	}
	printf("######\n");
}

int DtPrQueue::getQnum() {
	return qNum;
}

int DtPrQueue::command(int argc, const char* const * argv) {
	if (argc == 2) {
		if (strcmp(argv[1], "queue-test") == 0) {
			printf("test success.\n");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "print") == 0) {
			print();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "get-qnum") == 0) {
			int key = getQnum();
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%d", key);
			return (TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "queue-num") == 0) {
			int key = atoi(argv[2]);
			setQNum(key);
			//printf("%d\n", key);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "setMaxPriority") == 0) {
			int key = atoi(argv[2]);
			setMaxPriority(key);
			//printf("%d\n", key);
			return (TCL_OK);
		}
	}
	return Queue::command(argc, argv);
}

