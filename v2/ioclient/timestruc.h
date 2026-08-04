#pragma once
#include<Windows.h>
#include<mutex>
#include<list>
struct ConnCtx;
struct timerstruc {
    HANDLE iocp;
    std::atomic<bool> work;
    std::mutex mtx;
    std::list<ConnCtx*>ctlist;
};
