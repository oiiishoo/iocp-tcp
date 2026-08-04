#pragma once
#include<Windows.h>
#include<mutex>
#include<list>
#include<atomic>
#include"lights.h"
#define WHLCST 60000
#define ATSIZEA 400'000



// ================= IO TYPES =================
enum IO_TYPE {
    IO_ACCEPT,
    IO_RECV,
    IO_SEND,
    IO_RECVTHRES,
    IO_SENDTHRES,

};

enum requesttype {
    UNDEFINED,
    GET,
    POST,
    //CHANGESLAVORY,

};//0x01 00 03 00 \a\n\y

enum states {
    KEEPALIVE = 1 << 7,
    RETURN = 1 << 6,
    ARGUMENTS = 1 << 5,


};
// ================= CONTEXTS =================

enum ProtoState {
    PS_HDR,        // читаем 4 байта заголовка
    PS_HEADHEAP,   // читаем headlen
    PS_ARGVLEN,    // читаем arglenv[]
    PS_ARGDATA,    // читаем аргументы
    PS_SENDING,
    PS_RECEIVING,
    PS_CLOSED
};

struct IoCtx;
struct ConnCtx;


struct ovsock {
    IoCtx* ioc = nullptr;
    ConnCtx* client = nullptr;
    SOCKET sock = 0;
    OVERLAPPED* ovP;
    std::atomic<bool> hasTouchedByWorker{ false };
};

struct wtimer {
    DWORD wheelpos = 0;
    DWORD index_in_bucket = 0;
    DWORD rounds = 0;
    ovsock os;
};
struct alignas(64) wlst {
    std::vector<DWORD> lst;
    std::mutex lock;
};
struct whstruc {
    //std::vector<ConnCtx*> lst[60000];
    wlst lst[WHLCST];

    wtimer timers[ATSIZEA];
    std::list<DWORD> freeids;
    std::mutex timersmute;

    std::atomic<DWORD> crs;
    timerstruc* tstr;
};


struct IoCtx {
    OVERLAPPED ov{};
    IO_TYPE type;
    void* owner;
    bool timed;
    DWORD tid;
    DWORD startTicks;
    whstruc* timers;
};

struct AcceptCtx {
    SOCKET client;
    char buffer[(sizeof(sockaddr_in6) + 16) * 2];
};

struct ConnCtx {
    bool closed;
    pktfromlighttoaccepter* pkt;
    //forwarding
    SOCKET s;
    timerstruc* tstr;

    sockaddr_in6 addr;
    bool lastIOCPTIMEO;
    IO_TYPE timedouttype;

    // послужит для паралельных io
    OVERLAPPED* ov_recv;
    whstruc* oldtimerrecv;
    whstruc* newtimerrecv;
    DWORD timerrecv;
    DWORD lastreadquantms;
    std::vector<DWORD> hlmsreceivedseries;

    OVERLAPPED* ov_send;
    whstruc* oldtimersend;
    whstruc* newtimersend;
    DWORD timersend;
    DWORD lastsentquantms;
    std::vector<DWORD> hlmssentseries;

    std::atomic<DWORD> refcount;

    // при внедрении паралельных io, данные поля будут убраны
    /*whstruc* lastIOtimer;
    DWORD timerid;
    whstruc* oldtimer;
    whstruc* mytimer;*/

    // пригодится для будущих наблюдений, а сейчас служит наглядным филлером для дебага
    bool sending;
    bool reading;
    bool hlreading;
    bool hlsending;

    // були состояний - применяются в колесе
    //std::atomic<bool> sendtio;
    //std::atomic<bool> recvtio;

    // кванты времени определённых таймаут потоков
    std::atomic<DWORD> sndtimeo;
    std::atomic<DWORD> rcvtimeo;

    // кванты времени определённых таймаут потоков на высоком уровне
    std::atomic<DWORD> hlsndtimeo;
    std::atomic<DWORD> hlrcvtimeo;

    // счётчик высокоуровневых сообщений
    std::atomic<DWORD> hlsndtimecounter;
    std::atomic<DWORD> hlrcvtimecounter;

    // значения в квантах времени для таймаута минимальных сообщений
    std::atomic<DWORD> mdsndtimeo;
    std::atomic<DWORD> mdrcvtimeo;

    // может больше не нужные поля при колёсных таймаутах
    std::atomic<DWORD> stimecounter;
    std::atomic<DWORD> rtimecounter;

    // может больше не нужные поля в связи с колесом
    std::atomic<DWORD> mdstimecounter;
    std::atomic<DWORD> mdrtimecounter;



    // буферные поля, пороги для моментальных исключений
    char* buf;
    DWORD bufsz;
    DWORD want;        // сколько байт хотим прочитать
    DWORD wantPresent; // сколько уже есть
    DWORD recvThreshold;

    char* sendbuf;
    DWORD sendbufsz;
    DWORD wantsend;
    DWORD sentOff;
    DWORD sendThreshold;

    // поля определяющие протокол

    // enum state
    ProtoState ps;

    char cur_hdr[4];
    char userbuf[128];
    bool keepalive;
    bool streaming;
    Request r;         // сюда наполняется запрос
    int lastsendamifree;
    int lastrecvamifree;

};

struct RecvCtx {
    WSABUF buf;
    ConnCtx* conn;
};

struct SendCtx {
    WSABUF buf;
    ConnCtx* conn;
};






