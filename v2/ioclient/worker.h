#pragma once
#include"tcpcomponent.h"
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include"request.h"
#include<mutex>
#include<list>
#include"hashcode.h"
#include"lights.h"
// ================= IO TYPES =================

enum IO_TYPE {
    IO_CONNECT,
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


struct IoCtx {
    OVERLAPPED ov{};
    IO_TYPE type;
    void* owner;
};

struct ConnectCtx {
    SOCKET server;
    char buffer[(sizeof(sockaddr_in6) + 16) * 2];
    ConnCtx* conn;
};
struct pktfromlighttoaccepter;
struct ConnCtx {
    //forwarding
    pktfromlighttoaccepter* pkt;
    SOCKET s;
    timerstruc* tstr;
    bool ipv4;
    DWORD connect_ms;
    bool connecting;
    OVERLAPPED* ov_connection;
    DWORD connectcounter;
    sockaddr_in6 addr;
    sockaddr_in addr4;
    //user defined

    bool lastIOCPTIMEO;
    IO_TYPE timedouttype;
    OVERLAPPED* ov_recv;
    OVERLAPPED* ov_send;
    bool sending;
    bool reading;
    bool hlreading;
    bool hlsending;
    std::atomic<bool> sendtio;
    std::atomic<bool> recvtio;

    std::atomic<DWORD> sndtimeo;
    std::atomic<DWORD> rcvtimeo;
    std::atomic<DWORD> hlsndtimeo;
    std::atomic<DWORD> hlrcvtimeo;
    std::atomic<DWORD> mdsndtimeo;
    std::atomic<DWORD> mdrcvtimeo;

    std::atomic<DWORD> stimecounter;
    std::atomic<DWORD> rtimecounter;
    std::atomic<DWORD> mdstimecounter;
    std::atomic<DWORD> mdrtimecounter;


    ProtoState ps;

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


    char cur_hdr[4];
    char userbuf[128];
    bool keepalive;
    bool streaming;
    Request r;         // сюда наполняется пакет
    char* pointer;
};

struct RecvCtx {
    WSABUF buf;
    ConnCtx* conn;
};

struct SendCtx {
    WSABUF buf;
    ConnCtx* conn;
};


void post_recv(ConnCtx* c, DWORD mindatathreshold = 0);
void post_send(ConnCtx* c, DWORD mindatathreshold = 0);
void handle_connected(ConnCtx* c);
void set_rd_tio(ConnCtx* c, DWORD ms_time, DWORD mindatawait = 0) {
    c->rcvtimeo.store(ms_time, std::memory_order_seq_cst);
    c->hlrcvtimeo.store(ms_time, std::memory_order_seq_cst);
    c->mdrcvtimeo = mindatawait;
}
void set_wr_tio(ConnCtx* c, DWORD ms_time, DWORD mindatawait = 0) {
    c->sndtimeo.store(ms_time, std::memory_order_seq_cst);
    c->hlsndtimeo.store(ms_time, std::memory_order_seq_cst);
    c->mdsndtimeo = mindatawait;
}




// ================= USER PROTOCOL =================
void freerecv(ConnCtx* c) {
    if (c->buf)free(c->buf);
    c->buf = 0;
    c->bufsz = 0;
    c->want = 0;
    c->wantPresent = 0;
}
void freesend(ConnCtx* c) {
    if (c->sendbuf)free(c->sendbuf);
    c->sendbuf = 0;
    c->sendbufsz = 0;
    c->wantsend = 0;
    c->sentOff = 0;
}

void alrecv(ConnCtx* c, DWORD bfsz) {
    if (c->buf)free(c->buf);
    c->bufsz = bfsz;
    c->want = bfsz;
    c->wantPresent = 0;
    if (bfsz)c->buf = (char*)malloc(bfsz);

}
void alsend(ConnCtx* c, DWORD bfsz) {
    if (c->sendbuf)free(c->sendbuf);
    c->sendbufsz = bfsz;
    c->wantsend = bfsz;
    c->sentOff = 0;
    if (bfsz)c->sendbuf = (char*)malloc(bfsz);

}

void freesock(ConnCtx* c, bool rst_on = 0) {

    if (c->pkt) {
        c->pkt->concurrentServers->fetch_sub(1, std::memory_order_seq_cst);
    }
    char*& aux = *(char**)(c->userbuf + 3);
    if (aux) {

        free(aux);
        aux = 0;
    }
    std::unique_lock<std::mutex> lock(c->tstr->mtx);
    auto it = std::find(c->tstr->ctlist.begin(), c->tstr->ctlist.end(), c);
    if (it != c->tstr->ctlist.end())
    {
        c->tstr->ctlist.erase(it);
    }
    lock.unlock();
    //std::cout << "invoked\n";
    if (c->sendbuf)free(c->sendbuf);
    if (c->buf)free(c->buf);
    if (rst_on)rst(c->s); else closesocket(c->s);
    delete c;
}

//enum ProtoState {
//    PS_HDR,        // читаем 4 байта заголовка
//    PS_HEADHEAP,   // читаем headlen
//    PS_ARGVLEN,    // читаем arglenv[]
//    PS_ARGDATA,    // читаем аргументы
//    PS_SENDING,
//    PS_RECEIVING,
//    PS_CLOSED
//};


void handle_packet(ConnCtx* c, char* data, uint32_t len) {
    /*std::cout << "veshestvo " << len << " data:";
    if (data) {
        for (uint32_t i = 0; i < len; i++)
        {
            std::cout << (int)(data[i]) << ' ';
        }
        std::cout << '\n';
    }
    else std::cout << '\n';*/

    /*if (!((( (size_t) c->pointer)) % (100))) {
        std::cout << '\n' << (size_t)c->pointer << '\n';
    }*/
    //_alloca();
    freesock(c);
}


void handle_timeo(ConnCtx* c, IO_TYPE type) {
    std::cout << "IP: " << siptext(c->addr).get() << ' ' << type << "\n";
    if (type == IO_RECV) {
        freesock(c);
        std::cout << "handle_timeo: freesock()\n";
        //post_recv(c);
    }
    else if (type == IO_SEND) {
        freesock(c);
        //post_send(c);
    }
    if (type == IO_SENDTHRES) {
        freesock(c);
        std::cout << "sendthres exep\n";
    }
    else if (type == IO_RECVTHRES) {
        freesock(c);
        std::cout << "recvthres exep\n";
    }
}

void handle_connected(ConnCtx* c) {

    if (c->keepalive) {
        freesend(c);
        freerecv(c);
        c->r.clear();
        char*& aux = *(char**)(c->userbuf + 3);
        if (aux) {
            free(aux);
            aux = 0;
        }
    }
    else {
        int len = sizeof(sockaddr_in6);
        sockaddr_in6 local{};
        getpeername(c->s, (sockaddr*)&local, &len);
        if (local.sin6_family == AF_INET) {
            c->ipv4 = true;
            c->addr4.sin_addr.S_un.S_addr = *(int*)(local.sin6_addr.u.Byte + 12);
            c->addr4.sin_family = AF_INET;
            c->addr4.sin_port = local.sin6_port;
        }
        char comp[] = { 1,0,3,0,'a','n','y' };
        alsend(c, sizeof(comp));
        memcpy(c->sendbuf, comp, sizeof(comp));
        post_send(c);

    }

    //c->streaming = 1;
    //c->ps = PS_HDR;
    /*DWORD task = 5;
    c->want = task;
    alrecv(c, task);
    set_rd_tio(c, 50);
    post_recv(c, task);*/


}

void handle_sent(ConnCtx* c, char* data, DWORD bytes) {
    //_alloca();

    /*std::cout << "veshestvo sent " << bytes << " data:";
    if (data) {
        for (uint32_t i = 0; i < bytes; i++)
        {
            std::cout << (int)(data[i]) << ' ';
        }
        std::cout << '\n';
    }
    else std::cout << '\n';*/
    //freesock(c);
    alrecv(c, 27);
    post_recv(c);
}


// ================= POST ACCEPT =================

void post_connect(const sockaddr_in6& destaddr, HANDLE iocp, DWORD timeoutms, timerstruc* tstr, char* pointer) {
    // --- 1. Создаём сокет ---
    SOCKET sock = 0;
    auto* cc = new ConnectCtx{};
    auto* ctx = new IoCtx{};
    auto* c = new ConnCtx{};
    ctx->type = IO_CONNECT;
    ctx->owner = cc;
    c->connecting = true;
    c->connect_ms = timeoutms;
    c->pointer = pointer;
    cc->conn = c;
    c->tstr = tstr;
    std::unique_lock<std::mutex> lock(tstr->mtx);
    tstr->ctlist.emplace_back(c);
    lock.unlock();
    if (IN6_IS_ADDR_V4MAPPED(&destaddr.sin6_addr)) { // V4
        sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (sock == INVALID_SOCKET) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
        sockaddr_in dest{};
        sockaddr_in local{};
        local.sin_family = AF_INET;
        dest.sin_family = AF_INET;
        dest.sin_port = destaddr.sin6_port;
        dest.sin_addr.S_un.S_addr = *(int*)(destaddr.sin6_addr.u.Byte + 12);

        //pointer переменная начинающаяся с 0 по 254 | /24 от 1 по 255

        //unsigned int ip = (10 << 24) | (0 << 16) | (0 << 8) | ((size_t)pointer+1);
        //local.sin_addr.S_un.S_addr = htonl(ip);

        inet_pton(AF_INET, ("10.0.0." + std::to_string(((size_t)pointer + 1))).c_str(), &local.sin_addr);
        //inet_pton(AF_INET, "127.0.0.1",&local.sin_addr);
        //int opt = 1;
        //setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        cc->server = sock;
        c->s = sock;



        //free memory leak
        if (bind(sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
            //int err = WSAGetLastError();
            //std::cout << "Test bind result: " << err << '\n';
            //std::cout <<"failed 10.0.0." << (size_t)pointer+1 << '\n';
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }

        if (!CreateIoCompletionPort((HANDLE)sock, iocp, (ULONG_PTR)cc, 0)) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }

        //int err = WSAGetLastError();
        //std::cout << "Test bind result: " << err << '\n';

        LPFN_CONNECTEX ConnectEx = nullptr;
        GUID guid = WSAID_CONNECTEX;
        DWORD bytes = 0;
        if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),
            &ConnectEx, sizeof(ConnectEx), &bytes, nullptr, nullptr) != 0 || !ConnectEx) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
        //std::cout << "GLE 4 =" << WSAGetLastError() << '\n';
        // --- 5. Создаём контекст IO ---

        // --- 6. Одномоментный ConnectEx ---
        BOOL ok = ConnectEx(
            sock,
            (sockaddr*)&dest,
            sizeof(dest),
            nullptr,  // данные отправки при connect — не используем
            0,
            nullptr,  // количество реально отправленных байт
            &ctx->ov  // OVERLAPPED
        );
        c->ov_connection = &ctx->ov;

        //std::cout << "GLE 5 =" << WSAGetLastError() << '\n';
        //std::cout << ok << ' ' << ctx->type << ' ' << sock << '\n';
        if (!ok && WSAGetLastError() != ERROR_IO_PENDING) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
    }
    else { // V6
        sock = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (sock == INVALID_SOCKET) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
        sockaddr_in6 local{};
        local.sin6_family = AF_INET6;

        //int opt = 1;
        //setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        cc->server = sock;
        c->s = sock;
        if (bind(sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
        if (!CreateIoCompletionPort((HANDLE)sock, iocp, (ULONG_PTR)cc, 0)) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
        LPFN_CONNECTEX ConnectEx = nullptr;
        GUID guid = WSAID_CONNECTEX;
        DWORD bytes = 0;
        if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),
            &ConnectEx, sizeof(ConnectEx), &bytes, nullptr, nullptr) != 0 || !ConnectEx) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
        //std::cout << "GLE 4 =" << WSAGetLastError() << '\n';
        // --- 5. Создаём контекст IO ---


        // --- 6. Одномоментный ConnectEx ---
        BOOL ok = ConnectEx(
            sock,
            (sockaddr*)&destaddr,
            sizeof(destaddr),
            nullptr,  // данные отправки при connect — не используем
            0,
            nullptr,  // количество реально отправленных байт
            &ctx->ov  // OVERLAPPED
        );
        c->ov_recv = &ctx->ov;
        //std::cout << "GLE 5 =" << WSAGetLastError() << '\n';
        //std::cout << ok << ' ' << ctx->type << ' ' << sock << '\n';
        if (!ok && WSAGetLastError() != ERROR_IO_PENDING) {
            freesock(c);
            delete cc;
            delete ctx;
            return;
        }
    }
}


// ================= POST RECV =================

void post_recv(ConnCtx* c, DWORD mindatathreshold) {
    auto* ctx = new IoCtx{};
    ctx->type = IO_RECV;

    auto* rc = new RecvCtx{};
    rc->conn = c;
    ctx->owner = rc;

    rc->buf.buf = c->buf + c->wantPresent;
    rc->buf.len = c->want - c->wantPresent;
    if (!c->buf || !c->want) {
        std::cout << "post_recv buf/len was null\n";
        return;
    }

    if (c->streaming) {
        c->reading = 1;

    }
    else {

        c->hlreading = 1;
    }
    c->recvThreshold = mindatathreshold;

    DWORD flags = 0;
    int r = WSARecv(c->s, &rc->buf, 1, nullptr, &flags, &ctx->ov, nullptr);
    
    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        freesock(c);
        delete rc;
        delete ctx;
        return;
    }
    c->ov_recv = &ctx->ov;
    c->recvtio.store(1, std::memory_order_seq_cst);
}

void post_send(ConnCtx* c, DWORD mindatathreshold) {
    auto* ctx = new IoCtx{};
    ctx->type = IO_SEND;

    auto* sc = new SendCtx{};
    sc->conn = c;
    sc->buf.buf = c->sendbuf + c->sentOff;
    sc->buf.len = c->wantsend - c->sentOff;
    ctx->owner = sc;

    if (!c->sendbuf || !c->wantsend) {
        std::cout << "post_send sendbuf/len was null\n";
        return;
    }

    if (c->streaming) {
        c->sending = 1;
    }
    else {
        c->hlsending = 1;
    }
    c->sendThreshold = mindatathreshold;

    int r = WSASend(c->s, &sc->buf, 1, nullptr, 0, &ctx->ov, nullptr);
    
    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        freesock(c);
        delete sc;
        delete ctx;
        return;
    }
    c->ov_send = &ctx->ov;
    c->sendtio = 1;
}
// ================= HANDLE EVENT =================

void handle_event(
    HANDLE iocp,
    IoCtx* ctx,
    DWORD bytes,
    BOOL ok,
    timerstruc* tstr,
    pktfromlighttoaccepter* pkt
) {
    //std::cout << "handleevent:" << ok << " ticked\n";
    if (!ok) {

        //std::cout << "!ok:" << ok << " ticked\n";
        //std::cout <<"hevt GLE " << GetLastError()<<'\n';
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            if (!(ctx->type == IO_RECV || ctx->type == IO_SEND || ctx->type == IO_CONNECT)) {

                return;
            }




            //c->lastIOCPTIMEO = 1;
            //c->timedouttype = ctx->type;
            if (ctx->type == IO_CONNECT) {
                ConnCtx* c = ((ConnectCtx*)(ctx->owner))->conn;
                if (c->ipv4) {
                    sockaddr_in6 local = {};
                    local.sin6_addr.u.Byte[9] = 255;
                    local.sin6_addr.u.Byte[10] = 255;
                    memcpy(local.sin6_addr.u.Byte + 11, &c->addr4.sin_addr.S_un.S_addr, 4);
                    std::cout << "connection failed to " << siptext(local).get();
                }
                else {
                    std::cout << "connection failed to " << siptextV6(c->addr).get();
                }
                freesock(c);
                delete ctx->owner;
                delete ctx;
                return;
            }
            else if (ctx->type == IO_RECV) {
                ConnCtx* c = ((RecvCtx*)(ctx->owner))->conn;
                c->recvtio.store(0, std::memory_order_seq_cst);
                c->rtimecounter.store(0, std::memory_order_seq_cst);
                handle_timeo(c, ctx->type);
                delete ctx->owner;
                delete ctx;
                return;
            }
            else if (ctx->type == IO_SEND) {
                ConnCtx* c = ((SendCtx*)(ctx->owner))->conn;
                c->sendtio.store(0, std::memory_order_seq_cst);
                c->stimecounter.store(0, std::memory_order_seq_cst);
                handle_timeo(c, ctx->type);
                delete ctx->owner;
                delete ctx;
                return;
            }
            delete ctx;
            delete ctx->owner;
            return;
        }
        else {
            //auto* ac = (ConnectCtx*)ctx->owner;
            //int soerr = 0;
            //int len = sizeof(soerr);
            //getsockopt(ac->server, SOL_SOCKET, SO_ERROR, (char*)&soerr, &len);
            //std::cout << "experience\n";
            //std::cout <<"that err "<< soerr <<'\n';
            if (ctx->type == IO_CONNECT) {
                ConnCtx* c = ((ConnectCtx*)(ctx->owner))->conn;
                //std::cout << "IO_CONNECT\n";
                //std::cout << "experience100\n";

                freesock(c);
                delete ctx->owner;
                delete ctx;
                //std::cout << "GLE WSAGLE "<<GetLastError() <<' '<< WSAGetLastError()<<'\n';
                return;
            }
            ConnCtx* c = 0;
            if (ctx->type == IO_SEND) {
                c = ((SendCtx*)(ctx->owner))->conn;
            }
            else if (ctx->type == IO_RECV) {
                c = ((RecvCtx*)(ctx->owner))->conn;
            }
            freesock(c);
            delete ctx->owner;
            delete ctx;
            return;
        }
        return;
    }

    switch (ctx->type) {

        // -------- ACCEPT --------
    case IO_CONNECT: {
        auto* cc = (ConnectCtx*)ctx->owner;
        auto* c = cc->conn;
        setsockopt(cc->server, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
        c->pkt = pkt;
        pkt->totalServers->fetch_add(1, std::memory_order_seq_cst);
        pkt->concurrentServers->fetch_add(1, std::memory_order_seq_cst);
        c->keepalive = false;
        c->r.clear();

        //c->ps = PS_HDR;

        c->buf = nullptr;
        c->bufsz = 0;
        c->want = 0;
        c->wantPresent = 0;

        c->wantsend = 0;
        c->sentOff = 0;
        c->sendbuf = 0;
        c->sendbufsz = 0;
        c->streaming = 0;
        c->lastIOCPTIMEO = 0;


        //c->tstr = tstr;
        handle_connected(c);          // ← ВЫЗОВ ХЕНДЛЕРА

        //post_accept(listenSock, iocp, AcceptEx);

        delete cc;
        delete ctx;
        break;
    }


                   // -------- RECV --------
    case IO_RECV: {
        auto* rc = (RecvCtx*)ctx->owner;
        auto* c = rc->conn;

        DWORD temp = c->wantPresent;
        c->wantPresent += bytes;
        //std::cout << "EVT:IO_RECV " << bytes << " " << c->recvThreshold << "\n";
        if (bytes < c->recvThreshold) {
            handle_timeo(c, IO_RECVTHRES);
            delete rc;
            delete ctx;
            return;
        }
        // твой пользовательский обработчик
        else if (c->streaming) {
            c->reading = 0;
            c->recvtio.store(0, std::memory_order_seq_cst);
            c->rtimecounter.store(0, std::memory_order_seq_cst);
            handle_packet(c, c->buf + temp, bytes);
            delete rc;
            delete ctx;
            return;
        }
        else if (c->wantPresent == c->want) {
            c->hlreading = 0;
            c->recvtio.store(0, std::memory_order_seq_cst);
            c->rtimecounter.store(0, std::memory_order_seq_cst);
            c->mdrtimecounter.store(0, std::memory_order_seq_cst);
            handle_packet(c, c->buf, c->wantPresent);
            delete rc;
            delete ctx;
            return;
        }
        else {
            post_recv(c);
            delete rc;
            delete ctx;
            return;
        }


        // после обработки — начинаем читать снова
        //post_recv(c);
        freesock(c);
        delete rc;
        delete ctx;
        break;
    }
    case IO_SEND: {
        auto* sc = (SendCtx*)ctx->owner;
        auto* c = sc->conn;
        /*
    char* sendbuf;
    DWORD sendbufsz;
    DWORD wantsend;
    DWORD sentOff;
    */
        DWORD temp = c->sentOff;
        c->sentOff += bytes;
        if (bytes < c->sendThreshold) {
            handle_timeo(c, IO_SENDTHRES);
            delete sc;
            delete ctx;
            return;
        }
        else if (c->streaming) {
            c->sending = 0;
            c->sendtio.store(0, std::memory_order_seq_cst);
            c->stimecounter.store(0, std::memory_order_seq_cst);
            handle_sent(c, c->sendbuf + temp, bytes);       // ← ВЫЗОВ ХЕНДЛЕРА
            delete sc;
            delete ctx;
            return;
        }
        else if (c->sentOff == c->wantsend) {
            c->hlsending = 0;
            c->sendtio.store(0, std::memory_order_seq_cst);
            c->stimecounter.store(0, std::memory_order_seq_cst);
            c->mdstimecounter.store(0, std::memory_order_seq_cst);
            handle_sent(c, c->sendbuf, c->sentOff);       // ← ВЫЗОВ ХЕНДЛЕРА
            delete sc;
            delete ctx;
            return;
        }
        else {
            post_send(c);
            delete sc;
            delete ctx;
            return;
        }
        freesock(c);
        delete sc;
        delete ctx;
        break;
    }

    }
}





