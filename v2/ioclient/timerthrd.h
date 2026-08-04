#pragma once
#include<chrono>
#include <windows.h>
#include <iostream>
#include <cstdint>
#include"worker.h"
#pragma comment(lib, "winmm.lib")
void CALLBACK timer_callback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    timerstruc* tstr = (timerstruc*)dwUser;

    if (!tstr->work.load(std::memory_order_relaxed)) {
        timeKillEvent(uID);
        timeEndPeriod(1);
        return;
    }

    std::lock_guard<std::mutex> lock(tstr->mtx);
    for (ConnCtx* c : tstr->ctlist) {
        if (c->streaming) {
            if (c->recvtio.load(std::memory_order_seq_cst)) {
                if (c->rcvtimeo.load(std::memory_order_seq_cst)) {
                    c->rtimecounter.fetch_add(1, std::memory_order_seq_cst);
                    if (c->rtimecounter.load(std::memory_order_seq_cst) == c->rcvtimeo.load(std::memory_order_seq_cst)) {
                        c->recvtio.store(0, std::memory_order_seq_cst);
                        c->rtimecounter.store(0, std::memory_order_seq_cst);
                        CancelIoEx((HANDLE)c->s, c->ov_recv);
                    }
                }
            }

            if (c->sendtio.load(std::memory_order_seq_cst)) {
                if (c->sndtimeo.load(std::memory_order_seq_cst)) {
                    c->stimecounter.fetch_add(1, std::memory_order_seq_cst);
                    if (c->stimecounter.load(std::memory_order_seq_cst) == c->sndtimeo.load(std::memory_order_seq_cst)) {
                        c->stimecounter.store(0, std::memory_order_seq_cst);
                        c->sendtio.store(0, std::memory_order_seq_cst);
                        CancelIoEx((HANDLE)c->s, c->ov_send);
                    }
                }
            }
        }
        else {
            if (c->recvtio.load(std::memory_order_seq_cst)) {
                if (c->hlrcvtimeo.load(std::memory_order_seq_cst)) {
                    //std::cout << c->rtimecounter<<' '<<c->recvtio << "\n";
                    c->rtimecounter.fetch_add(1, std::memory_order_seq_cst);
                    if (c->mdrcvtimeo) {
                        c->mdrtimecounter.fetch_add(1, std::memory_order_seq_cst);
                        if (c->mdrtimecounter == c->mdrcvtimeo) {
                            c->recvtio.store(0, std::memory_order_seq_cst);
                            c->rtimecounter.store(0, std::memory_order_seq_cst);
                            c->mdrtimecounter.store(0, std::memory_order_seq_cst);
                            CancelIoEx((HANDLE)c->s, c->ov_recv);
                            goto br;
                        }
                    }
                    if (c->rtimecounter.load(std::memory_order_seq_cst) == c->hlrcvtimeo.load(std::memory_order_seq_cst)) {
                        c->recvtio.store(0, std::memory_order_seq_cst);
                        c->rtimecounter.store(0, std::memory_order_seq_cst);
                        c->mdrtimecounter.store(0, std::memory_order_seq_cst);
                        int e = CancelIoEx((HANDLE)c->s, c->ov_recv);
                        std::cout << e << "e CancelIoEx gle:" << GetLastError() << "\n";
                    }
                }
            }
        br:
            if (c->sendtio.load(std::memory_order_seq_cst)) {
                if (c->hlsndtimeo.load(std::memory_order_seq_cst)) {
                    c->stimecounter.fetch_add(1, std::memory_order_seq_cst);
                    if (c->mdsndtimeo) {
                        c->mdstimecounter.fetch_add(1, std::memory_order_seq_cst);
                        if (c->mdstimecounter == c->mdsndtimeo) {
                            c->sendtio.store(0, std::memory_order_seq_cst);
                            c->stimecounter.store(0, std::memory_order_seq_cst);
                            c->mdstimecounter.store(0, std::memory_order_seq_cst);
                            CancelIoEx((HANDLE)c->s, c->ov_send);
                            continue;
                        }
                    }
                    if (c->stimecounter.load(std::memory_order_seq_cst) == c->hlsndtimeo.load(std::memory_order_seq_cst)) {
                        c->sendtio.store(0, std::memory_order_seq_cst);
                        c->stimecounter.store(0, std::memory_order_seq_cst);
                        c->mdstimecounter.store(0, std::memory_order_seq_cst);
                        CancelIoEx((HANDLE)c->s, c->ov_send);
                    }
                }
            }
        }
    }
}

DWORD WINAPI timer_thread(LPVOID LpParam)
{

    timeBeginPeriod(1);

    UINT timerId = timeSetEvent(
        1,                  // период 1 мс
        1,                  // точность 1 мс (по возможности)
        timer_callback,     // функция callback
        (DWORD_PTR)LpParam,                  // пользовательские данные
        TIME_PERIODIC | TIME_CALLBACK_FUNCTION // периодический таймер
    );

    if (!timerId)
    {
        timeEndPeriod(1);
        std::cout << "Failed to create multimedia timer" << std::endl;
        return 1;
    }

}
