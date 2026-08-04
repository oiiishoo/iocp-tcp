#pragma once
#include<atomic>
#include<unordered_map>
#include<mutex>
#include<Windows.h>
#include<list>
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include<iostream>
#include"timestruc.h"
#include"request.h"
struct Qfield {
    char* str;
    unsigned short len;
    Qfield(char* string, unsigned short length) : str(string), len(length) {}
    Qfield(char* stringterminated) :str(stringterminated), len(strlen(stringterminated)) {}
    Qfield(const Qfield& other) : str(other.str), len(other.len) {}

    bool operator==(const Qfield& other) const {
        if (len != other.len) return false;
        return std::memcmp(str, other.str, len) == 0;
    }
};
namespace std {
    template<>
    struct hash<Qfield> {
        size_t operator()(const Qfield& q) const noexcept {
            size_t h = 14695981039346656037ull; // FNV offset
            for (unsigned short i = 0; i < q.len; ++i) {
                h ^= (unsigned char)q.str[i];
                h *= 1099511628211ull; // FNV prime
            }
            return h;
        }
    };
}
struct query {
    void(*executable)(char* req, unsigned short reqlen,
        int& returncode,
        char*& returnbuffer, unsigned int& retlen);

    unsigned short len;
    char* string;
    int retcode;
    char* defaultbuffer;
    unsigned int defaultbufferlen;

    // Конструктор
    query() {}
    query(char* string, unsigned short len, char* defaultbuffer, unsigned int defaultbufferlen, int returncode, void(*f)(char* req, unsigned short reqlen,
        int& returncode,
        char*& returnbuffer, unsigned int& retlen) = 0)
        : len(len), string(string), retcode(returncode), executable(f), defaultbufferlen(defaultbufferlen), defaultbuffer(defaultbuffer)
    {
        if (len > 0 && string == 0) {
            std::cout << "len > 0 + string == nullptr\n";
        }
    }
    void get(char*& buf, unsigned int& buflen, int& codeout)
    {
        if (executable) {
            executable(string, len, codeout, buf, buflen);
            return;
        }
        codeout = retcode;
        buflen = defaultbufferlen;
        memcpy(buf, defaultbuffer, defaultbufferlen);

    }
    // Оператор сравнения
    //bool operator==(query& other) {
    //    if (len != other.len) {
    //        return 0;
    //    }
    //    if (!len) {
    //        return 1; // обе пустые
    //    }
    //    if (!string || !other.string) {
    //        return 0; // некорректное состояние
    //    }
    //    return std::memcmp(string, other.string, len) == 0;
    //}
};



struct pktfromlighttoaccepter {
    void(**exec_fn)(SOCKET client);
    SOCKET server;
    std::atomic<bool>* work;
    std::mutex* lightmutex;
    std::unordered_map<Qfield, query>* getpool;
    std::unordered_map<Qfield, query>* postpool;
    std::atomic<size_t>* concurrentClients;
    std::atomic<size_t>* totalClients;
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
        std::atomic<size_t>* tc,
        std::atomic<size_t>* cc,
        HANDLE giocp,
        LPFN_ACCEPTEX aex,
        timerstruc* tszeade) :
        exec_fn(execfn),
        server(acceptingsock),
        work(workbool),
        lightmutex(mutex),
        getpool(getlink),
        postpool(postlink),
        concurrentClients(cc),
        totalClients(tc),
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
        concurrentClients(other.concurrentClients),
        totalClients(other.totalClients),
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
            concurrentClients = other.concurrentClients;
            totalClients = other.totalClients;
            iocp = other.iocp;
            aex = other.aex;
            tstr = other.tstr;
        }
        return *this;
    }
};
