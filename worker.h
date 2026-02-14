#pragma once
#include"tcpcomponent.h"
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <atomic>
#include <cstdint>
#include <cstring>


struct IoCtx {
    OVERLAPPED ov{};
    IO_TYPE type;
    void* owner;
};

struct AcceptCtx {
    SOCKET client;
    char buffer[(sizeof(sockaddr_in6) + 16) * 2];
};

struct ConnCtx {
    
    SOCKET s;

    ProtoState ps;

    char* buf;
    DWORD bufsz;
    DWORD want;        
    DWORD wantPresent; 

    char* sendbuf;
    DWORD sendbufsz;
    DWORD wantsend;
    DWORD sentOff;


    char cur_hdr[4];
    char userbuf[128];
    bool keepalive;
    bool streaming;
    Request r;         
};

struct RecvCtx {
    WSABUF buf;
    ConnCtx* conn;
};

struct SendCtx {
    ConnCtx* conn;
    WSABUF buf;
};


void post_recv(ConnCtx* c);
void post_send(ConnCtx* c);


void freerecv(ConnCtx*c) {
    if(c->buf)free(c->buf);
    c->buf = 0;
    c->bufsz = 0;
    c->want = 0;
    c->wantPresent = 0;
    
}
void freesend(ConnCtx*c) {
    if (c->sendbuf)free(c->sendbuf);
    c->sendbuf = 0;
    c->sendbufsz = 0;
    c->wantsend = 0;
    c->sentOff = 0;
}

void freesock(ConnCtx* c) {
    closesocket(c->s);
    delete c;
}

void handle_packet(ConnCtx* c, char* data, uint32_t len){

  //your processing
}

void handle_accepted(ConnCtx* c){

  
}
void handle_sent(ConnCtx* c,char* data, DWORD bytes){

  
}

void post_accept(
    SOCKET listenSock,
    HANDLE iocp,
    LPFN_ACCEPTEX AcceptEx
) {
    auto* ctx = new IoCtx{};
    ctx->type = IO_ACCEPT;

    auto* ac = new AcceptCtx{};
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
        closesocket(ac->client);
        delete ac;
        delete ctx;
    }
}



void post_recv(ConnCtx* c) {
    auto* ctx = new IoCtx{};
    ctx->type = IO_RECV;

    auto* rc = new RecvCtx{};
    rc->conn = c;
    ctx->owner = rc;

    rc->buf.buf = c->buf + c->wantPresent;      
    rc->buf.len = c->want - c->wantPresent; 

    DWORD flags = 0;
    int r = WSARecv(c->s, &rc->buf, 1, nullptr, &flags, &ctx->ov, nullptr);
    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        closesocket(c->s);
        delete rc;
        delete ctx;
    }
}

void post_send(ConnCtx* c) {
    auto* ctx = new IoCtx{};
    ctx->type = IO_SEND;
    
    auto* sc = new SendCtx{};
    sc->conn = c;
    sc->buf.buf = c->sendbuf + c->sentOff;
    sc->buf.len = c->wantsend - c->sentOff;

    ctx->owner = sc;

    int r = WSASend(c->s, &sc->buf, 1, nullptr, 0, &ctx->ov, nullptr);
    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        closesocket(c->s);
        delete[] sc->buf.buf;
        delete sc;
        delete ctx;
    }
}


void handle_event(
    SOCKET listenSock,
    LPFN_ACCEPTEX AcceptEx,
    HANDLE iocp,
    IoCtx* ctx,
    DWORD bytes,
    BOOL ok
) {
    if (!ok) {
        delete ctx;
        return;
    }

    switch (ctx->type) {

        
    case IO_ACCEPT: {
        auto* ac = (AcceptCtx*)ctx->owner;

        setsockopt(
            ac->client,
            SOL_SOCKET,
            SO_UPDATE_ACCEPT_CONTEXT,
            (char*)&listenSock,
            sizeof(listenSock)
        );

        auto* c = new ConnCtx{};
        c->s = ac->client;
        c->keepalive = false;
        c->r.clear();

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
        CreateIoCompletionPort(
            (HANDLE)c->s,
            iocp,
            (ULONG_PTR)c,
            0
        );

        handle_accepted(c);          
        //required order
        post_accept(listenSock, iocp, AcceptEx);

        delete ac;
        delete ctx;
        break;
    }


                  // -------- RECV --------
    case IO_RECV: {
        auto* rc = (RecvCtx*)ctx->owner;
        auto* c = rc->conn;

        if (bytes == 0) {
            closesocket(c->s);
            free(c->buf);
            delete rc;
            delete ctx;
            delete c;
            break;
        }

        DWORD temp = c->wantPresent;
        c->wantPresent += bytes;
        if (c->streaming) { 
            handle_packet(c, c->buf + temp, bytes); 

        }
        else if(c->wantPresent == c->want){
            handle_packet(c, c->buf, c->wantPresent);
        }
        else {
            post_recv(c);
        }

        delete rc;
        delete ctx;
        break;
    }
    case IO_SEND: {
        auto* sc = (SendCtx*)ctx->owner;
        auto* c = sc->conn;
        /* commented help supply
    char* sendbuf;
    DWORD sendbufsz;
    DWORD wantsend;
    DWORD sentOff;
    */
        DWORD temp = c->sentOff;
        c->sentOff += bytes;
        if (c->streaming) {
            handle_sent(c, c->sendbuf + temp,bytes);       
        }
        else if(c->sentOff == c->wantsend){
            handle_sent(c,c->sendbuf, c->sentOff);       
        }
        else {
            post_send(c);
        }
        
        delete sc;
        delete ctx;
        break;
    }

    }
}
