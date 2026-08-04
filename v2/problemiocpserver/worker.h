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
#include"wstruc.h"




void post_recv(ConnCtx* c, DWORD mindatathreshold = 0);
void post_send(ConnCtx* c, DWORD mindatathreshold = 0);
void handle_accepted(ConnCtx* c);
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
    c->hlmsreceivedseries.clear();
    if (c->buf)free(c->buf);
    c->bufsz = bfsz;
    c->want = bfsz;
    c->wantPresent = 0;
    if (bfsz)c->buf = (char*)malloc(bfsz);

}
void alsend(ConnCtx* c, DWORD bfsz) {
    c->hlmssentseries.clear();
    if (c->sendbuf)free(c->sendbuf);
    c->sendbufsz = bfsz;
    c->wantsend = bfsz;
    c->sentOff = 0;
    if (bfsz)c->sendbuf = (char*)malloc(bfsz);

}

void freesock(ConnCtx* c, bool rst_on = 0) {
    if (!c || !c->pkt || !c->tstr) {
        std::cout << "freesock ConnCtx is null\n";
        return;
    }
    //std::cout << "codepoint freesock at " << c << "\n";
    //std::cout << "c->refcount " << c->refcount.load(std::memory_order_relaxed) << "\n";
    if (!c->refcount.load(std::memory_order_seq_cst)) {
        c->tstr->gt.removeMe(c);
        if (c->closed) {
            std::cout << "double freesock\n";
        }
        else {
            c->closed = 1;
        }
        if (c->pkt) {
            c->pkt->concurrentClients->fetch_sub(1, std::memory_order_seq_cst);
        }
        char*& aux = *(char**)(c->userbuf + 3);
        if (aux) {
            free(aux);
            aux = 0;
        }
        

        //std::cout << "invoked\n";

        if (rst_on)rst(c->s); else closesocket(c->s);
        if (c->sendbuf)free(c->sendbuf);
        if (c->buf)free(c->buf);
        c->r.clear();

        delete c;
    }

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
            std::cout << (int)(data[i])<<' ';
        }
        std::cout << '\n';
    }
    else std::cout << '\n';*/

    if (c->ps == PS_HDR) {

        memcpy(c->cur_hdr, data, 4);
        DWORD task = *(unsigned short*)(c->cur_hdr + 2);

        if (c->r.state & ARGUMENTS) {
            if (!data[4]) {
                freesock(c);
                return;
            }
            else {
                c->userbuf[0] = data[4];
                c->r.allocArgs(c->userbuf[0]);
            }

        }
        else {
            c->userbuf[12] = data[4];
            task--;
            c->userbuf[11] = true;
        }
        alrecv(c, task);


        c->r.state = *(char*)(c->cur_hdr + 1);
        c->r.rt = *c->cur_hdr;
        c->ps = PS_HEADHEAP;
        post_recv(c);
        return;
    }

    if (c->ps == PS_HEADHEAP) {
        if (c->userbuf[11]) {
            c->r.hdr = (char*)malloc(c->bufsz + 1);
            c->r.hdrlen = c->bufsz + 1;
            c->r.hdr[0] = c->userbuf[12];
            memcpy(c->r.hdr + 1, c->buf, c->bufsz);

        }
        else {
            c->r.setHeader(c->buf, c->bufsz);
        }

        //std::cout << siptext(c->addr) << ':' << ntohs(c->addr.sin6_port) << " post heap processing + hdrname + hdrlen " << c->r.hdr << ' ' << c->r.hdrlen << '\n';
        if (c->r.state & ARGUMENTS) {
            c->ps = PS_ARGVLEN;
            alrecv(c, c->userbuf[0]);
            post_recv(c);
        }
        else if (c->r.rt == GET) {
            c->ps = PS_SENDING;
            goto sen;
        }
        else if (c->r.rt == POST) {
            c->ps = PS_RECEIVING;
            goto rec;
        }

        return;
    }

    if (c->ps == PS_ARGVLEN) {
        unsigned short sum = 0;
        unsigned char auxL = ((unsigned char*)c->userbuf)[0];
        *(char**)(c->userbuf + 3) = (char*)malloc(auxL);
        for (size_t i = 0; i < auxL; i++)
        {
            sum += ((unsigned char*)c->buf)[i];
            (*(char**)(c->userbuf + 3))[i] = c->buf[i];
        }
        *(short*)(c->userbuf + 1) = sum;
        // +4 INT DATA SIZE IN RECV OR SENDING
        DWORD aux = 0;
        if (c->r.rt == GET) {
            aux = 0;
        }
        else if (c->r.rt == POST) {
            aux = 4 + 4 + 4 + 4 + 4;
        }
        alrecv(c, sum + aux);
        c->ps = PS_ARGDATA;
        post_recv(c);
        return;
    }

    if (c->ps == PS_ARGDATA) {
        c->r.allocArgs(c->userbuf[0]);
        unsigned short cursor = 0;
        for (size_t i = 0; i < c->userbuf[0]; i++)
        {
            unsigned char carg = (*(unsigned char**)(c->userbuf + 3))[i];
            c->r.setarg(i, c->buf + cursor, carg);
            cursor += carg;
        }

        if (c->r.rt == GET) {
            c->ps = PS_SENDING;
            goto sen;
        }
        else if (c->r.rt == POST) {
            alrecv(c, *(DWORD*)((c->buf + cursor)));
            c->ps = PS_RECEIVING;
            post_recv(c);
        }
        return;
    }

    if (c->ps == PS_SENDING) {
    sen:
        static char streng[] = "get cab";
        DWORD retrievedsize = strlen(streng);

        int erc = 1;
        DWORD msglen = 4 + 4 + 4 + 4 + 4 + retrievedsize;
        alsend(c, msglen);
        *(char*)(c->sendbuf) = c->r.rt;
        *(char*)(c->sendbuf + 1) = RETURN;
        *(short*)(c->sendbuf + 2) = c->r.hdrlen;
        DWORD h = hashcode(c->r.hdr, c->r.hdrlen);
        DWORD datahash = hashcode(streng, retrievedsize);

        *(int*)(c->sendbuf + 4) = h;
        *(int*)(c->sendbuf + 8) = erc;
        *(DWORD*)(c->sendbuf + 12) = retrievedsize;
        *(int*)(c->sendbuf + 16) = datahash;
        memcpy(c->sendbuf + 20, streng, retrievedsize);
        c->ps = PS_CLOSED;
        post_send(c);
        return;
    }

    if (c->ps == PS_RECEIVING) {
    rec:
        static char streng[] = "post abc";
        DWORD retrievedsize = strlen(streng);

        int erc = 1;
        DWORD msglen = 4 + 4 + 4 + 4 + 4 + retrievedsize;

        alsend(c, msglen);
        //учесть возврат так что бы получателю вернулся хедер, код, длина ответа если это данные
        *(char*)(c->sendbuf) = c->r.rt;
        *(char*)(c->sendbuf + 1) = RETURN;
        *(short*)(c->sendbuf + 2) = c->r.hdrlen;
        DWORD h = hashcode(c->r.hdr, c->r.hdrlen);
        DWORD datahash = hashcode(streng, retrievedsize);

        *(int*)(c->sendbuf + 4) = h;
        *(int*)(c->sendbuf + 8) = erc;
        *(DWORD*)(c->sendbuf + 12) = retrievedsize;
        *(int*)(c->sendbuf + 16) = datahash;
        memcpy(c->sendbuf + 20, streng, retrievedsize);
        if (c->keepalive) {
            handle_accepted(c);
            post_send(c);
        }
        else freesock(c);
        return;
    }

}


void handle_timeo(ConnCtx* c, IO_TYPE type) {
    //std::cout <<"IP: " << siptext(c->addr).get() <<' '<<type<<"\n";
    if (type == IO_RECV) {
        freesock(c);
        //std::cout << "handle_timeo: freesock()\n";
        //post_recv(c);
    }
    else if (type == IO_SEND) {
        freesock(c);
        //post_send(c);
    }
    else if (type == IO_SENDTHRES) {
        freesock(c);
        //std::cout << "sendthres exep\n";
    }
    else if (type == IO_RECVTHRES) {
        freesock(c);
        //std::cout << "recvthres exep\n";
    }
    std::cout << "timeo type: " << type << '\n';
}

void handle_accepted(ConnCtx* c) {
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
        int len = sizeof(c->addr);
        getpeername(c->s, (sockaddr*)&c->addr, &len);
    }

    //c->streaming = 1;
    c->ps = PS_HDR;
    DWORD task = 5;
    c->want = task;
    alrecv(c, task);
    set_rd_tio(c, 50);
    set_wr_tio(c, 50);
    post_recv(c, task);


}

void handle_sent(ConnCtx* c, char* data, DWORD bytes) {
    // пусто
    //std::cout << "real sent " << bytes<<'\n';
    if (c->ps == PS_CLOSED) {
        char* ms = c->sendbuf + 20;
        DWORD mslen = *(DWORD*)(c->sendbuf + 12);
        if ((*c->sendbuf) == GET) {
            //std::cout << "GET: server sent message to client ms contains: " << ms << " mslen: " << mslen << '\n';

        }
        else if ((*c->sendbuf) == POST) {
            //DWORD aux = *((DWORD*)(c->buf));
            //std::cout << "POST: client sent message to server ms contains: " << c->buf + 4 << " mslen: " << aux << '\n';

        }
        if (c->keepalive)handle_accepted(c); else freesock(c);
        return;
    }
    if (c->ps == PS_SENDING) {
        static char sndleak[] = "sndleak";

        alsend(c, strlen(sndleak));
        memcpy(c->sendbuf, sndleak, strlen(sndleak));
        //repeat accept logic
        if (c->keepalive)handle_accepted(c); else {
            freesock(c);
        }
        return;
    }

}


// ================= POST ACCEPT =================

void post_accept(
    SOCKET listenSock,
    HANDLE iocp,
    LPFN_ACCEPTEX AcceptEx
) {
    IoCtx* ctx = new IoCtx{};
    ctx->type = IO_ACCEPT;

    AcceptCtx* ac = new AcceptCtx{};
    ctx->owner = ac;

    ac->client = WSASocket(
        AF_INET6,
        SOCK_STREAM,
        IPPROTO_TCP,
        nullptr,
        0,
        WSA_FLAG_OVERLAPPED
    );
    DWORD off = 0;
    setsockopt(
        listenSock,
        IPPROTO_IPV6,
        IPV6_V6ONLY,
        (char*)&off,
        sizeof(off)
    );

    DWORD bytes = 0;
    BOOL ok = AcceptEx(
        listenSock,
        ac->client,
        ac->buffer,
        0,
        sizeof(sockaddr_in6) + 16,
        sizeof(sockaddr_in6) + 16,
        &bytes,
        &ctx->ov
    );

    if (!ok && WSAGetLastError() != ERROR_IO_PENDING) {
        //std::cout << "WSAGLE ACCEPTED " << WSAGetLastError()<<'\n';
        closesocket(ac->client);
        delete ac;
        delete ctx;
    }
}

// ================= POST RECV =================

void post_recv(ConnCtx* c, DWORD mindatathreshold) {
    IoCtx* ctx = new IoCtx{};
    ctx->type = IO_RECV;

    RecvCtx* rc = new RecvCtx{};
    rc->conn = c;
    ctx->owner = rc;

    rc->buf.buf = c->buf + c->wantPresent;
    rc->buf.len = c->want - c->wantPresent;
    if (!c->buf || !c->want) {
        std::cout << "post_recv buf/len was null\n";
        freesock(c);
        delete rc;
        delete ctx;
        return;
    }

    if (c->streaming) {
        c->reading = 1;

    }
    else {

        c->hlreading = 1;
    }
    c->recvThreshold = mindatathreshold;


    c->ov_recv = &ctx->ov;
    //c->recvtio.store(1, std::memory_order_seq_cst);
    DWORD aux = c->newtimerrecv->crs.load(std::memory_order_seq_cst);
    DWORD whenms;
    if (c->streaming) {
        whenms = c->rcvtimeo;
    }
    else {
        whenms = c->hlrcvtimeo;
    }
    c->lastrecvamifree = 2;
    c->timerrecv = maketimer(c->newtimerrecv, ctx, whenms, c);
    ctx->startTicks = GetTickCount();




    DWORD flags = 0;
    c->refcount.fetch_add(1, std::memory_order_seq_cst);
    int r = WSARecv(c->s, &rc->buf, 1, nullptr, &flags, &ctx->ov, nullptr);

    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        //std::cout << "WSAGLE RECV "<< WSAGetLastError() << '\n';
        removetimerid(c->oldtimerrecv, c->timerrecv, c);
        c->refcount.fetch_sub(1, std::memory_order_seq_cst);
        freesock(c);
        delete rc;
        delete ctx;
        return;
    }
    else {
        c->refcount.fetch_sub(1, std::memory_order_seq_cst);
    }
    

}

void post_send(ConnCtx* c, DWORD mindatathreshold) {
    IoCtx* ctx = new IoCtx{};
    if (!ctx) {
        std::cout << "postsend ctx null appeared\n";
    }
    ctx->type = IO_SEND;

    SendCtx* sc = new SendCtx{};
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

    c->ov_send = &ctx->ov;
    //c->sendtio = 1;

    DWORD whenms;
    if (c->streaming) {
        whenms = c->sndtimeo;
    }
    else {
        whenms = c->hlsndtimeo;
    }
    c->lastsendamifree = 2;
    c->timersend = maketimer(c->newtimersend, ctx, whenms, c);
    ctx->startTicks = GetTickCount();

    c->sendThreshold = mindatathreshold;
    c->refcount.fetch_add(1, std::memory_order_seq_cst);
    int r = WSASend(c->s, &sc->buf, 1, nullptr, 0, &ctx->ov, nullptr);

    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        //std::cout << "WSAGLE RECV " << WSAGetLastError() << '\n';
        removetimerid(c->oldtimersend, c->timersend, c);
        c->refcount.fetch_sub(1, std::memory_order_seq_cst);
        freesock(c);
        delete sc;
        delete ctx;
        return;
    }
    else {
        c->refcount.fetch_sub(1, std::memory_order_seq_cst);
    }
    
    
}
// ================= HANDLE EVENT =================

void handle_event(
    SOCKET listenSock,
    LPFN_ACCEPTEX AcceptEx,
    HANDLE iocp,
    IoCtx* ctx,
    DWORD bytes,
    BOOL ok,
    timerstruc* tstr,
    pktfromlighttoaccepter* pkt,// durka
    whstruc* mywheel
) {
    
    bool amifree = 0;

    if (ctx->type == IO_RECV || ctx->type == IO_SEND) {
        ConnCtx* c = 0;
        SendCtx* ssc = (SendCtx*)(ctx->owner);
        RecvCtx* rrc = (RecvCtx*)(ctx->owner);

        DWORD difference = GetTickCount() - ctx->startTicks;
        if (ctx->type == IO_RECV) {
            c = rrc->conn;

            whstruc* ot = c->newtimerrecv;
            c->oldtimerrecv = c->newtimerrecv;
            c->newtimerrecv = mywheel;
            if (ctx->timed) {
                ot->timersmute.lock();
                ctx->timers->timers[ctx->tid].os.hasTouchedByWorker.store(1, std::memory_order_seq_cst);
                ot->timersmute.unlock();

                removetimerid(ot, ctx->tid, c);

                
                
                if (c->hlreading) {
                    c->hlmsreceivedseries.push_back(difference);
                    c->hlrcvtimecounter.fetch_add(difference, std::memory_order_seq_cst);

                    /*if ((c->wantPresent + bytes) == c->want) {
                        std::cout << "final hlread #" << c->hlmsreceivedseries.size() << ": ms taken " << difference << "\n";
                        std::cout << "series of final hlread ";
                        for (DWORD iter : c->hlmsreceivedseries)
                        {
                            std::cout << iter;
                        }
                        std::cout << "\n";
                    }
                    else {
                        std::cout << "hlread #" << c->hlmsreceivedseries.size() << ": ms taken " << difference << "\n";
                    }*/
                }
                else {
                    c->lastreadquantms = difference;
                    std::cout << "streaming read taken bytes " << rrc->buf.len <<" difference " << difference << "\n";
                }
            }
            c->timerrecv = 0;
            amifree = 1;
        }
        else {
            c = ssc->conn;

            whstruc* ot = c->newtimersend;
            c->oldtimersend = c->newtimersend;
            c->newtimersend = mywheel;
            if (ctx->timed) {
                ot->timersmute.lock();
                ctx->timers->timers[ctx->tid].os.hasTouchedByWorker.store(1, std::memory_order_seq_cst);
                ot->timersmute.unlock();

                removetimerid(ot, ctx->tid, c); 
                    

                if (c->hlsending) {
                    c->hlmssentseries.push_back(difference);
                    c->hlsndtimecounter.fetch_add(difference, std::memory_order_seq_cst);
                }
                else {
                    c->lastsentquantms = difference;
                    std::cout << "streaming send taken "<< difference <<"\n";
                }
            }
            c->timersend = 0;
            amifree = 1;
        }
    }

    //std::cout <<"handleevent:" << ok << " ticked\n";
    if (!ok) {
        //std::cout << "st !ok "<<ctx<<'\n';
        if (!ctx) {
            std::cout << "!ok !ctx\n";
            return;
        }
        //std::cout << "!ok:" << ok << " ticked\n";
        ConnCtx* c = 0;

        AcceptCtx* ac = (AcceptCtx*)(ctx->owner);
        SendCtx* sc = (SendCtx*)(ctx->owner);
        RecvCtx* rc = (RecvCtx*)(ctx->owner);
        if (!ctx->owner) {
            std::cout << "if (!ok) !ctx->owner " << ctx->owner << ' ' << ctx->type << '\n';
        }



        if (ctx->type == IO_ACCEPT) {

            //std::cout << "experience100 "<<ctx->owner<<'\n';

            closesocket(ac->client);
            delete ctx->owner;
            delete ctx;
            post_accept(listenSock, iocp, AcceptEx);
            //std::cout << "GLE WSAGLE "<<GetLastError() <<' '<< WSAGetLastError()<<'\n';
            return;
        }
        else if (ctx->type == IO_SEND) {
            c = ((SendCtx*)(ctx->owner))->conn;

        }
        else if (ctx->type == IO_RECV) {
            c = ((RecvCtx*)(ctx->owner))->conn;

        }

        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            if (!(ctx->type == IO_RECV || ctx->type == IO_SEND || ctx->type == IO_ACCEPT)) {
                return;
            }
            //c->lastIOCPTIMEO = 1;
            //c->timedouttype = ctx->type;
            if (ctx->type == IO_RECV) {
                //c->recvtio.store(0, std::memory_order_seq_cst);
                //c->rtimecounter.store(0, std::memory_order_seq_cst);
                handle_timeo(c, ctx->type);
                if (amifree){
                    delete rc;
                    delete ctx;
                }
                return;
            }
            else if (ctx->type == IO_SEND) {
                //c->sendtio.store(0, std::memory_order_seq_cst);
                //c->stimecounter.store(0, std::memory_order_seq_cst);
                handle_timeo(c, ctx->type);
                if (amifree) {
                    delete sc;
                    delete ctx;
                }
                return;
            }
            return;
        }
        else {
            //std::cout << "end !ok "<<ctx<<' '<<ctx->owner << ' '<<c<<' ' << ctx->type << '\n';
            if (ctx->type == IO_ACCEPT) {
                //std::cout << "end !ok IO_ACCEPT " << ctx << ' ' << ctx->owner << ' ' << c << ' ' << ctx->type << '\n';
                post_accept(listenSock, iocp, AcceptEx);
                delete ctx->owner;
                delete ctx;
                return;
            }
            freesock(c);
            if (amifree) {
                delete ctx->owner;
                delete ctx;
            }
            else {
                std::cout << "exotic1 context dump bytes start###\n";
                for (size_t i = 0; i < sizeof(ConnCtx); i++)
                {
                    printf("%x ", ((char*)(c))[i]);
                }
                std::cout << "###exotic1 context dump bytes end\n";
            }
            std::cout << "exotic2 context dump bytes start###\n";
            for (size_t i = 0; i < sizeof(ConnCtx); i++)
            {
                printf("%x ", ((char*)(c))[i]);
            }
            std::cout << "###exotic2 context dump bytes end\n";
            //std::cout << "end !ok freed"  << '\n';
            return;
        }

        std::cout << "exotic3 context dump bytes start###\n";
        for (size_t i = 0; i < sizeof(ConnCtx); i++)
        {
            printf("%x ", ((char*)(c))[i]);
        }
        std::cout << "###exotic3 context dump bytes end\n";
    }
    if (!ctx) {
        std::cout << "ok !ctx\n";
        return;
    }
    if (!ctx->owner) {
        std::cout << "ok !ctx->owner " << ctx->owner << '\n';
        return;
    }
    switch (ctx->type) {

        // -------- ACCEPT --------
    case IO_ACCEPT: {
        AcceptCtx* ac = (AcceptCtx*)ctx->owner;
        if (!ac) {
            std::cout << "ac !ac\n";
        }
        setsockopt(
            ac->client,
            SOL_SOCKET,
            SO_UPDATE_ACCEPT_CONTEXT,
            (char*)&listenSock,
            sizeof(listenSock)
        );

        ConnCtx* c = new ConnCtx{};
        //memset(c, 0, sizeof(ConnCtx));

        c->s = ac->client;
        c->keepalive = false;
        c->r.clear();
        c->pkt = pkt;
        pkt->concurrentClients->fetch_add(1, std::memory_order_seq_cst);
        pkt->totalClients->fetch_add(1, std::memory_order_seq_cst);
        c->ps = PS_HDR;

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


        CreateIoCompletionPort(
            (HANDLE)c->s,
            iocp,
            (ULONG_PTR)c,
            0
        );
        /*std::unique_lock<std::mutex> lock(tstr->mtx);
        tstr->ctlist.emplace_back(c);
        lock.unlock();*/
        c->tstr = tstr;
        c->tstr->gt.addMe(c);

        c->newtimerrecv = mywheel;
        c->newtimersend = mywheel;
        post_accept(listenSock, iocp, AcceptEx);

        handle_accepted(c);          // ← ВЫЗОВ ХЕНДЛЕРА

        delete ac;
        delete ctx;
        break;
    }


                  // -------- RECV --------
    case IO_RECV: {
        RecvCtx* rc = (RecvCtx*)ctx->owner;

        ConnCtx* c = rc->conn;


        DWORD temp = c->wantPresent;
        c->wantPresent += bytes;
        //std::cout << "EVT:IO_RECV " <<bytes<< " " <<c->recvThreshold<< "\n";
        if (bytes < c->recvThreshold) {
            handle_timeo(c, IO_RECVTHRES);
            if (amifree) {
                delete rc;
                delete ctx;
            }
            return;
        }
        // твой пользовательский обработчик
        else if (c->streaming) {
            c->reading = 0;
            //c->recvtio.store(0, std::memory_order_seq_cst);
            //c->rtimecounter.store(0, std::memory_order_seq_cst);
            handle_packet(c, c->buf + temp, bytes);
            if (amifree) {
                delete rc;
                delete ctx;
            }
            return;
        }
        else if (c->wantPresent == c->want) {
            c->hlreading = 0;
            //c->recvtio.store(0, std::memory_order_seq_cst);
            //c->rtimecounter.store(0, std::memory_order_seq_cst);
            //c->mdrtimecounter.store(0, std::memory_order_seq_cst);
            handle_packet(c, c->buf, c->wantPresent);
            c->lastrecvamifree = amifree;
            if (amifree) {
                delete rc;
                delete ctx;
            }
            return;
        }
        else {
            post_recv(c);
            if (amifree) {
                delete rc;
                delete ctx;
            }
            return;
        }


        // после обработки — начинаем читать снова
        //post_recv(c);
        std::cout << "recv fallen down\n";
        freesock(c);
        if (amifree) {
            delete rc;
            delete ctx;
        }
        break;
    }
    case IO_SEND: {
        SendCtx* sc = (SendCtx*)ctx->owner;

        ConnCtx* c = sc->conn;

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
            if (amifree) {
                delete sc;
                delete ctx;
            }
            return;
        }
        else if (c->streaming) {
            c->sending = 0;
            //c->sendtio.store(0, std::memory_order_seq_cst);
            c->stimecounter.store(0, std::memory_order_seq_cst);
            handle_sent(c, c->sendbuf + temp, bytes);       // ← ВЫЗОВ ХЕНДЛЕРА
            if (amifree) {
                delete sc;
                delete ctx;
            }
            return;
        }
        else if (c->sentOff == c->wantsend) {
            c->hlsending = 0;
            //c->sendtio.store(0, std::memory_order_seq_cst);
            c->stimecounter.store(0, std::memory_order_seq_cst);
            c->mdstimecounter.store(0, std::memory_order_seq_cst);
            handle_sent(c, c->sendbuf, c->sentOff);       // ← ВЫЗОВ ХЕНДЛЕРА
            c->lastsendamifree = amifree;
            if (amifree) {
                delete sc;
                delete ctx;
            }
            return;
        }
        else {
            post_send(c);
            if (amifree) {
                delete sc;
                delete ctx;
            }
            return;
        }
        std::cout << "send fallen down\n";
        //delete[] sc->buf.buf;
        freesock(c);
        if (amifree) {
            delete sc;
            delete ctx;
        }
        break;
    }

    }
}





