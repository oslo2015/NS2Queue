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
	fidRecord = NULL;
	delayCheck = false;

	defaultPrior = 0;
}

DtPrQueue::~DtPrQueue() {
	delete[] qList;
	delete fidRecord;
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

	defaultPrior = 0;
	if (qNum > 0) {
//		defaultPrior = (qNum >> 1) + 1;
		defaultPrior = 1;
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
	int p_fid = iph->flowid();
	int jobId = (p_fid > 999) ? p_fid / 1000 : p_fid;

	if (delayCheck && !isFidPrior(jobId)) {
		jobId = defaultPrior;
	}

	/// 队满
	/// 如果qNum=0，丢弃
	/// 否则，如果有比该包低优先级的队列，则把其中最低的队列的队尾包丢弃，将该包加入到相应队列
	/// 否则，丢弃
	if (isQueueFull()) {
		if (qNum > 0) {
			if (0 == jobId) {
				drop(p);
				return;
			}
			// jobId > 0
			int i;
			bool flag = false;
			for (i = 0; (0 == i) || (i > jobId); --i) {
				if (qList[i].length() > 0) {
					Packet * toRemove = qList[i].tail();
					qList[i].remove(toRemove);
					drop(toRemove);
					qList[jobId].enque(p);
					flag = true;
					break;
				}
				if (0 == i)
					i = qNum + 1;
			} // for
			if (!flag)
				drop(p);
		} else {
			// 0 == qNum
			drop(p);
		}
	} else {
		// 队列不满
		if (qNum > 0) {
			if (jobId <= qNum && jobId > 0) {
				qList[jobId].enque(p);
			} else {
				//printf("$$$$$$$---%d\n", jobId);
				qList[0].enque(p);
			}
		} else {
			qList[0].enque(p);
		}
	}
}

// 0 优先级是最低的。
Packet * DtPrQueue::deque() {
	Packet * p;
	int i = 0;
	for (i = 1; i <= qNum; ++i) {
		p = qList[i].deque();
		if (0 != p)
			return p;
	}
	p = qList[0].deque();
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
			//printf("test success.\n");
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

		if (strcmp(argv[1], "enableDelayCheck") == 0) {
			enableDelayCheck();
			return (TCL_OK);
		}

		if (strcmp(argv[1], "clearQueue") == 0) {
			clearQueue();
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

		if (strcmp(argv[1], "addFidPrior") == 0) {
			int key = atoi(argv[2]);
			addFidPrior(key);
			//printf("%d\n", key);
			return (TCL_OK);
		}

		if (strcmp(argv[1], "removeFidPrior") == 0) {
			int key = atoi(argv[2]);
			removeFidPrior(key);
			//printf("%d\n", key);
			return (TCL_OK);
		}

	}
	return Queue::command(argc, argv);
}

int DtPrQueue::getDelayCheck() {
	return delayCheck ? 1 : 0;
}

void DtPrQueue::enableDelayCheck() {
	delayCheck = true;
	delete fidRecord;
	fidRecord = new INTLIST;
}

bool DtPrQueue::isFidPrior(int jobId) {
	if (!delayCheck)
		return true;
	if (NULL == fidRecord || 0 == fidRecord->size())
		return false;

	return findInList(*fidRecord, jobId);
}

void DtPrQueue::addFidPrior(int jobId) {
	if (!delayCheck)
		return;
	if (NULL != fidRecord && !findInList(*fidRecord, jobId)) {
		fidRecord->push_back(jobId);
		fidRecord->sort();

		// 将默认队列中，jobId的包移动到相应队列中， 注意顺序。
		if (qNum > 0) {
			PacketQueue * defaultQueue = &qList[defaultPrior];
			defaultQueue->resetIterator();
			Packet * tmpPkt = NULL;
			PacketQueue tmpQueue;
			// 遍历默认队列， 找到jobId的包，加入到临时队列中。
			while (NULL != (tmpPkt = defaultQueue->getNext())) {
				hdr_ip* iph = hdr_ip::access(tmpPkt);
				int p_fid = iph->flowid();
				int tmpJobId = (p_fid > 999) ? p_fid / 1000 : p_fid;
				if (jobId == tmpJobId) {
					defaultQueue->remove(tmpPkt);
					tmpQueue.enqueHead(tmpPkt);
				}
			}
			tmpQueue.resetIterator();
			while (NULL != (tmpPkt = tmpQueue.getNext())) {
				qList[jobId].enqueHead(tmpPkt);
			}
			// 清空临时队列
			while (NULL != tmpQueue.deque()) {
				;
			}
		}
	}
}

void DtPrQueue::removeFidPrior(int jobId) {
	if (!delayCheck)
		return;
	if (NULL != fidRecord) {
		fidRecord->remove(jobId);
	}
}

// 清空该队列
void DtPrQueue::clearQueue() {
	int i;
	for (i = 0; i <= qNum; ++i) {
		PacketQueue * tmpQueue = &qList[i];
		Packet * tmpPkt = NULL;
		while (NULL != (tmpQueue = tmpQueue->deque())) {
			drop(tmpPkt);
		}
	}
}
