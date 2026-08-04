#pragma once
#include <windows.h>
#include <iostream>
#include <cstdint>
#include"wstruc.h"
#include <atomic>
#pragma comment(lib, "winmm.lib")



void CALLBACK wheel_callback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    whstruc& wstr = *(whstruc*)dwUser;


    if (!wstr.tstr->work.load(std::memory_order_relaxed)) { 

        timeKillEvent(uID);
        return;
    }
    DWORD cur = wstr.crs.load(std::memory_order_seq_cst);

    if (cur == (WHLCST - 1)) {
        wstr.crs.store(0, std::memory_order_seq_cst);
    }
    else {
        wstr.crs.fetch_add(1, std::memory_order_seq_cst);
    }

    std::mutex& wlock = wstr.lst[cur].lock;

    wlock.lock();

    std::vector<DWORD>& nextvec = wstr.lst[cur].lst;
    std::vector<DWORD> curvec;
    curvec.swap(nextvec);

    for (DWORD dw : curvec) {
        if ((!dw) || (dw == -1)) {
            continue;
        }
        wstr.timersmute.lock();
        wtimer& t = wstr.timers[dw];
        if (wstr.timers[dw].os.hasTouchedByWorker.load(std::memory_order_seq_cst)) {

        }
        else if(!t.rounds){ //callback
            bool err = CancelIoEx((HANDLE)t.os.sock, t.os.ovP);
            std::cout << "mwheel.h " << (err ? "" : "!") << "CancelIoEx " << GetLastError() << "\n";


        }
        else { // times
            size_t aux = nextvec.size();
            nextvec.push_back(dw);
            
            t.index_in_bucket = aux;
            t.rounds--;
        }
        wstr.timersmute.unlock();

    }

    wlock.unlock();
    //curvec.clear();

}
