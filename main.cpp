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
//abstract server creation, if in search of tcpcomponent.h > look in librare

int main(){

SOCKET _sock = newServer(port, 0, backlog);
if (_sock == INVALID_SOCKET) return  0;
localfd = _sock;

HANDLE iocp = CreateIoCompletionPort(
    INVALID_HANDLE_VALUE,
    nullptr,
    0,
    0
);

if (!iocp) {
    std::cout << "CreateIoCompletionPort failed\n";
    return  0;
}

CreateIoCompletionPort(
    (HANDLE)_sock,
    iocp,
    0,
    0
);
LPFN_ACCEPTEX aex;
GUID guid = WSAID_ACCEPTEX;
DWORD bytes = 0;

int r = WSAIoctl(
    localfd,
    SIO_GET_EXTENSION_FUNCTION_POINTER,
    &guid,
    sizeof(guid),
    &aex,
    sizeof(aex),
    &bytes,
    nullptr,
    nullptr
);
if(r != 0 || !aex)return  0;

if (localfd != SOCKET_ERROR) {
    //work.store(1, std::memory_order_seq_cst);
    for (size_t i = 0; i < 4; i++)
    {
        post_accept(localfd, iocp, aex);
        HANDLE h = CreateThread(
            0,      
            0,      
            worker,
            abc,      
            0,
            0
        );
        if (!h) {
            //work.store(0, std::memory_order_seq_cst);
            std::cout << "thread creating error serve() failed\n";
            return  0;
        }
    }
    return  0;
}
  
}
