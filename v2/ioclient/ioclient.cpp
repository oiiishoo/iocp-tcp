#include <iostream>
#include <thread>
#include <io.h>
#include <fcntl.h>
#include <vector>
#include <list>
#include <algorithm>
#include "tcpcomponent.h"
#include <unordered_map>
#include <mutex>
#include<mswsock.h>
#include"timerthrd.h"
#include"worker.h"
#define tret return *this

static unsigned short lightport = 6934;
struct Qfield;
struct query;
class Light;
struct pktfromacceptertoclient;
struct pktfromlighttoaccepter;

//struct Qfield {
//    char* str;
//    unsigned short len;
//    Qfield(char* string, unsigned short length) : str(string), len(length) {}
//    Qfield(char* stringterminated) :str(stringterminated), len(strlen(stringterminated)) {}
//    Qfield(const Qfield& other) : str(other.str), len(other.len) {}
//
//    bool operator==(const Qfield& other) const {
//        if (len != other.len) return false;
//        return std::memcmp(str, other.str, len) == 0;
//    }
//};
//namespace std {
//    template<>
//    struct hash<Qfield> {
//        size_t operator()(const Qfield& q) const noexcept {
//            size_t h = 14695981039346656037ull; // FNV offset
//            for (unsigned short i = 0; i < q.len; ++i) {
//                h ^= (unsigned char)q.str[i];
//                h *= 1099511628211ull; // FNV prime
//            }
//            return h;
//        }
//    };
//}
//struct query {
//    void(*executable)(char* req, unsigned short reqlen,
//        int& returncode,
//        char*& returnbuffer, unsigned int& retlen);
//
//    unsigned short len;
//    char* string;
//    int retcode;
//    char* defaultbuffer;
//    unsigned int defaultbufferlen;
//
//    // Конструктор
//    query() {}
//    query(char* string, unsigned short len, char* defaultbuffer, unsigned int defaultbufferlen, int returncode, void(*f)(char* req, unsigned short reqlen,
//        int& returncode,
//        char*& returnbuffer, unsigned int& retlen) = 0)
//        : len(len), string(string), retcode(returncode), executable(f), defaultbufferlen(defaultbufferlen), defaultbuffer(defaultbuffer)
//    {
//        if (len > 0 && string == 0) {
//            std::cout << "len > 0 + string == nullptr\n";
//        }
//    }
//    void get(char*& buf, unsigned int& buflen, int& codeout)
//    {
//        if (executable) {
//            executable(string, len, codeout, buf, buflen);
//            return;
//        }
//        codeout = retcode;
//        buflen = defaultbufferlen;
//        memcpy(buf, defaultbuffer, defaultbufferlen);
//
//    }
//    // Оператор сравнения
//    //bool operator==(query& other) {
//    //    if (len != other.len) {
//    //        return 0;
//    //    }
//    //    if (!len) {
//    //        return 1; // обе пустые
//    //    }
//    //    if (!string || !other.string) {
//    //        return 0; // некорректное состояние
//    //    }
//    //    return std::memcmp(string, other.string, len) == 0;
//    //}
//};

/*struct pktfromlighttoaccepter {
    void(**exec_fn)(SOCKET client);
    SOCKET server;
    std::atomic<bool>* work;
    std::mutex* lightmutex;
    std::unordered_map<Qfield, query>* getpool;
    std::unordered_map<Qfield, query>* postpool;
    std::atomic<size_t>* concurrentServers;
    std::atomic<size_t>* totalServers;
    HANDLE iocp;
    LPFN_ACCEPTEX aex;
    timerstruc* tstr;
    pktfromlighttoaccepter(
        void(**execfn)(SOCKET client),
        SOCKET acceptingsock,
        std::atomic<bool>* workbool,
        std::mutex* mutex,
        std::unordered_map<Qfield, query>* getlink,
        std::unordered_map<Qfield, query>* postlink,
        std::atomic<size_t>* ts,
        std::atomic<size_t>* cs,
        HANDLE giocp,
        LPFN_ACCEPTEX aex,
        timerstruc* tszeade) :
        exec_fn(execfn),
        server(acceptingsock),
        work(workbool),
        lightmutex(mutex),
        getpool(getlink),
        postpool(postlink),
        concurrentServers(cs),
        totalServers(ts),
        iocp(giocp),
        aex(aex),
        tstr(tszeade)
    {
    }
    pktfromlighttoaccepter(const pktfromlighttoaccepter& other) :exec_fn(other.exec_fn),
        server(other.server),
        work(other.work),
        lightmutex(other.lightmutex),
        getpool(other.getpool),
        postpool(other.postpool),
        concurrentServers(other.concurrentServers),
        totalServers(other.totalServers),
        iocp(other.iocp),
        aex(other.aex),
        tstr(other.tstr)
    {

    }
    pktfromlighttoaccepter& operator=(const pktfromlighttoaccepter& other) {
        if (this != &other) {
            exec_fn = other.exec_fn;
            server = other.server;
            work = other.work;
            lightmutex = other.lightmutex;
            getpool = other.getpool;
            postpool = other.postpool;
            concurrentServers = other.concurrentServers;
            totalServers = other.totalServers;
            iocp = other.iocp;
            aex = other.aex;
            tstr = other.tstr;
        }
        return *this;
    }
};*/







DWORD WINAPI worker(LPVOID lpParam) {
    auto pkt = (pktfromlighttoaccepter*)lpParam;
    std::cout << "start\n";
    while (pkt->work->load(std::memory_order_relaxed)) {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        OVERLAPPED* ov = nullptr;
        //std::cout << "wait\n";
        BOOL ok = GetQueuedCompletionStatus(
            pkt->iocp,
            &bytes,
            &key,
            &ov,
            INFINITE
        );
        //std::cout << "waited\n";
        //DWORD err = ok ? 0 : GetLastError();
        //std::cout << err<<'\n';
        if (!ov)
            continue;

        auto* ctx = (IoCtx*)ov;
        handle_event(
            pkt->iocp,
            ctx,
            bytes,
            ok,
            pkt->tstr,
            pkt
        );
    }

    return 0;
}

class IOCL {



    //
    std::atomic<bool> work;
    std::unordered_map<Qfield, query> getpool;
    std::unordered_map<Qfield, query> postpool;
    std::mutex mapmutex;
    //
    HANDLE iocp;
    pktfromlighttoaccepter* abc;
    timerstruc* timestr;

public:
    std::atomic<size_t> concurrentServers;
    std::atomic<size_t> totalServers;
    void clear() {
        if (work)close();
        getpool.clear();
        postpool.clear();
        work.store(0, std::memory_order_seq_cst);
        concurrentServers.store(0, std::memory_order_seq_cst);
        totalServers.store(0, std::memory_order_seq_cst);
    }
    IOCL& close() {
        work.store(0, std::memory_order_seq_cst);

        tret;
    }

    IOCL& serve() {
        std::lock_guard<std::mutex> lock(mapmutex);
        clear();

        iocp = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            nullptr,
            0,
            0
        );

        if (!iocp) {
            std::cout << "CreateIoCompletionPort failed\n";
            tret;
        }



        timestr = new timerstruc;
        if (!timestr) {
            std::cout << "timer alloc failed\n";
            tret;
        }

        timestr->iocp = iocp;
        timestr->work = &work;

        abc = new pktfromlighttoaccepter(
            0,
            INVALID_SOCKET,
            &work,
            &mapmutex,
            &getpool,
            &postpool,
            &totalServers,
            &concurrentServers,
            iocp,
            0,
            timestr
        );

        work.store(1, std::memory_order_seq_cst);

        // worker threads
        for (size_t i = 0; i < 6; i++) {

            HANDLE h = CreateThread(
                0,
                0,
                worker,
                abc,
                0,
                0
            );

            if (!h) {
                work.store(0, std::memory_order_seq_cst);
                std::cout << "thread creating error\n";
                tret;
            }
        }

        // запускаем подключения


        HANDLE h = CreateThread(
            0,
            0,
            timer_thread,
            timestr,
            0,
            0
        );

        tret;
    }
    void connect(char* where, short port, char* data, DWORD timeoutMs) {
        sockaddr_in6 local;
        resolveHost(where, port, local);
        post_connect(local, iocp, timeoutMs, timestr, data);
    }
};

void caller(IOCL* clients, int b) {
    for (int i = 0; i < 1; i++) {
        clients->connect((char*)"10.0.0.1", lightport, (char*)b + 1, 1000);
    }
}

int main()
{
    initWSA();
    IOCL clients;
    clients.serve();


    int second = 0;
    while (1) {
        for (size_t i = 0; i < 5; i++)
        {
            //if (second > 100)break;
            std::thread t(caller, &clients, i);
            t.detach();
        }
        Sleep(1000);
        std::cout << ++second <<
            ' ' <<
            clients.concurrentServers.load(std::memory_order_seq_cst) <<
            ' ' <<
            clients.totalServers.load(std::memory_order_seq_cst) <<
            '\n';
    }
}
