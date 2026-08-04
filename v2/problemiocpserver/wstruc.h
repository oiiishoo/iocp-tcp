#pragma once
#include<Windows.h>
//#include"worker.h"
#include<mutex>
#include<list>
#include<atomic>
#include<vector>
#include"typs.h"







DWORD maketimer(whstruc* wheel, IoCtx* ioc, DWORD timeout, ConnCtx* c) {

	DWORD aux = wheel->crs.load(std::memory_order_seq_cst);
	DWORD wheelnum = (aux + timeout) % WHLCST;
	DWORD myid = -1;
	std::mutex& tmute = wheel->timersmute;
	std::mutex& wmute = wheel->lst[wheelnum].lock;
	if (timeout > 0) {

		tmute.lock();

		
		if (!wheel->freeids.size()) {
			tmute.unlock();

			std::cout << "expected error, maketimer cant allocate timer\n";
			return -1;
		}

		myid = wheel->freeids.back();
		if (myid >= ATSIZEA) {
			std::cout << "maketimer overflown\n";
			tmute.unlock();
			return -1;
		}
		wheel->freeids.pop_back();

		if (ioc->type==IO_SEND) {
			std::cout << "IO_SEND & c == " << c << "\n";

		}


		// nullify timerstruct
		memset(&wheel->timers[myid], 0, sizeof(wtimer));

		wtimer& t = wheel->timers[myid];
		t.os.sock = c->s;
		t.os.ioc = ioc;
		t.os.client = c;
		t.wheelpos = wheelnum;
		ioc->timed = 1;
		ioc->tid = myid;
		ioc->timers = wheel;
		//c->lastIOtimer = wheel;
		t.os.ovP = &ioc->ov;


		if ((ioc->type == IO_RECV)||(ioc->type == IO_SEND)) {

			if (ioc->type == IO_RECV) {
				c->ov_recv = t.os.ovP;

			}
			else if (ioc->type == IO_SEND) {
				c->ov_send = t.os.ovP;

			}

		}
		
		t.rounds = (aux + timeout) / WHLCST;
		tmute.unlock();
		// где то тут чёта продолжай

		//wheel
		wmute.lock();
		tmute.lock();
		t.index_in_bucket = (DWORD)wheel->lst[wheelnum].lst.size();
		tmute.unlock();
		wheel->lst[wheelnum].lst.push_back(myid);
		wmute.unlock();
	}
	else { //timeout zero
		return -1;
	}
	return myid;
}

bool removetimerid(whstruc* wheel, DWORD id, ConnCtx* c) {
	if ((id == -1) || (id == 0)) return false;
	if (!wheel) return false;

	if (id >= ATSIZEA) {
		std::cout << "removetimerid overflown\n";
		return false;
	}

	if (!wheel->timers[id].os.ioc)return false;



	DWORD aux = wheel->crs.load(std::memory_order_seq_cst);
	std::mutex& tmute = wheel->timersmute;
	wtimer& timer = wheel->timers[id];
	DWORD pos = wheel->timers[id].wheelpos;
	std::mutex& wmute = wheel->lst[pos].lock;
	IoCtx* ioc = wheel->timers[id].os.ioc;



	if ((ioc->type == IO_RECV) || (ioc->type == IO_SEND)) {

		if (ioc->type == IO_RECV) {
			c->ov_recv = 0;

		}
		else if (ioc->type == IO_SEND) {
			c->ov_send = 0;

		}

	}


	if (pos == aux) {

		//tmute.lock();
		////timer.os.client->lastIOtimer = 0;
		//timer.os.ioc->timed = 0;
		//timer.os.hasTouchedByWorker.store(true, std::memory_order_seq_cst);
		//tmute.unlock();

		wmute.lock();
		tmute.lock();
		timer.os.ioc->timed = 0;
		timer.os.hasTouchedByWorker.store(true, std::memory_order_seq_cst);

		pos = wheel->timers[id].wheelpos;
		if (!wheel->lst[pos].lst.size()) {
			tmute.unlock();
			wmute.unlock();
			return false;
		}
		
		wheel->lst[pos].lst;

		tmute.unlock();
		wmute.unlock();
		return false;
	}
	else {
		
		//отчисление затем добавление в список свободных
		
		// WHEEL 
		

		wmute.lock();
		tmute.lock();
		if (wheel->lst[pos].lst.size()) {
			DWORD idx = timer.index_in_bucket;
			DWORD last = wheel->lst[pos].lst.back();
			wheel->lst[pos].lst[idx] = last;
			wheel->lst[pos].lst.pop_back();
			wheel->timers[last].index_in_bucket = idx;
		}

		wheel->freeids.push_front(id);
		timer.os.ioc->timed = 0;

		tmute.unlock();
		wmute.unlock();
		// WHEEL END
		
		// TIMER
		/*tmute.lock();
		wheel->freeids.push_front(id);
		timer.os.ioc->timed = 0;
		tmute.unlock();*/
		// TIMER END

		return true;
	}
	
}
