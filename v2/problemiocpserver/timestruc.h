#pragma once
#include<Windows.h>
#include<mutex>
#include<list>
#include<vector>



struct ConnCtx;
struct GlobalTrack {
    std::mutex mtx;
    std::vector<ConnCtx*> conns;
    void addMe(ConnCtx* c) {
        mtx.lock();
        conns.push_back(c);
        mtx.unlock();
    }
    void removeMe(ConnCtx* c) {
        mtx.lock();

        auto it = std::find(
            conns.begin(),
            conns.end(),
            c);

        if (it != conns.end())
            conns.erase(it);

        mtx.unlock();
    }
    
};
struct timerstruc {
    HANDLE iocp;
    std::atomic<bool> work;
    std::mutex mtx;
    std::vector<ConnCtx*>ctlist;
    GlobalTrack gt;
};
