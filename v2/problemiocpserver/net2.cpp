
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
//#include"timerthrd.h"
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
//    Qfield(char* stringterminated):str(stringterminated), len(strlen(stringterminated)){}
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
//    query(char*string,unsigned short len, char* defaultbuffer, unsigned int defaultbufferlen, int returncode, void(*f)(char* req, unsigned short reqlen,
//        int& returncode,
//        char*& returnbuffer, unsigned int& retlen)  = 0)
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
//struct pktfromlighttoaccepter {
//    void(**exec_fn)(SOCKET client);
//    SOCKET server;
//    std::atomic<bool>* work;
//    std::mutex* lightmutex;
//    std::unordered_map<Qfield, query>* getpool;
//    std::unordered_map<Qfield, query>* postpool;
//    std::atomic<size_t>* concurrentClients;
//    std::atomic<size_t>* totalClients;
//    HANDLE iocp;
//    LPFN_ACCEPTEX aex;
//    timerstruc* tstr;
//    pktfromlighttoaccepter(
//        void(**execfn)(SOCKET client),
//        SOCKET acceptingsock,
//        std::atomic<bool>* workbool,
//        std::mutex* mutex,
//        std::unordered_map<Qfield, query>* getlink,
//        std::unordered_map<Qfield, query>* postlink,
//        std::atomic<size_t>* tc,
//        std::atomic<size_t>* cc,
//        HANDLE giocp,
//        LPFN_ACCEPTEX aex,
//        timerstruc* tszeade) :
//        exec_fn(execfn),
//        server(acceptingsock),
//        work(workbool),
//        lightmutex(mutex),
//        getpool(getlink),
//        postpool(postlink),
//        concurrentClients(cc), 
//        totalClients(tc),
//        iocp(giocp),
//        aex(aex),
//        tstr(tszeade)
//    {
//    }
//    pktfromlighttoaccepter(const pktfromlighttoaccepter& other) :exec_fn(other.exec_fn),
//        server(other.server),
//        work(other.work),
//        lightmutex(other.lightmutex),
//        getpool(other.getpool),
//        postpool(other.postpool),
//        concurrentClients(other.concurrentClients),
//        totalClients(other.totalClients),
//        iocp(other.iocp),
//        aex(other.aex),
//        tstr(other.tstr)
//    {
//
//    }
//    pktfromlighttoaccepter& operator=(const pktfromlighttoaccepter& other) {
//        if (this != &other) {
//            exec_fn = other.exec_fn;
//            server = other.server;
//            work = other.work;
//            lightmutex = other.lightmutex;
//            getpool = other.getpool;
//            postpool = other.postpool;
//            concurrentClients = other.concurrentClients;
//            totalClients = other.totalClients;
//            iocp = other.iocp;
//            aex = other.aex;
//            tstr = other.tstr;
//        }
//        return *this;
//    }
//};

//struct pktfromacceptertoclient {
//    void(**exec_fn)(SOCKET client);
//    SOCKET client;
//    std::atomic<bool>* work;
//    std::mutex* lightmutex;
//    std::unordered_map<Qfield, query>* getlink;
//    std::unordered_map<Qfield, query>* postlink;
//    std::atomic<size_t>* concurrentClients;
//    std::atomic<size_t>* totalClients;
//    pktfromacceptertoclient(
//        void(**exec_fn)(SOCKET client),
//        SOCKET client,
//        std::atomic<bool>* work,
//        std::mutex* lightmutex,
//        std::unordered_map<Qfield, query>* getlink,
//        std::unordered_map<Qfield, query>* postlink,
//        std::atomic<size_t>* concurrentClients,
//        std::atomic<size_t>* totalClients
//    ) :exec_fn(exec_fn), client(client), work(work), lightmutex(lightmutex), getlink(getlink), postlink(postlink), totalClients(totalClients), concurrentClients(concurrentClients) {
//
//    }
//    pktfromacceptertoclient(pktfromacceptertoclient& pkt) : 
//        exec_fn(pkt.exec_fn),
//        client(pkt.client),
//        work(pkt.work),
//        lightmutex(pkt.lightmutex),
//        getlink(pkt.getlink),
//        postlink(pkt.postlink),
//        concurrentClients(pkt.concurrentClients),
//        totalClients(pkt.totalClients) {
//
//    }
//    pktfromacceptertoclient& operator=(const pktfromacceptertoclient& other) {
//        if (this != &other) {
//            exec_fn = other.exec_fn;
//            client = other.client;
//            work = other.work;
//            lightmutex = other.lightmutex;
//            getlink = other.getlink;
//            postlink = other.postlink;
//            concurrentClients = other.concurrentClients;
//            totalClients = other.totalClients;
//        }
//        return *this;
//    }
//};
//DWORD WINAPI handler(LPVOID lpParam) {
//    int clientlen = sizeof(sockaddr_in6);
//
//    SOCKET client = *(SOCKET*)lpParam;
//    sockaddr_in6 addr = *(sockaddr_in6*)((char*)lpParam + sizeof(SOCKET));
//    bool* work = ((bool*)lpParam + sizeof(SOCKET) + clientlen);
//    std::unordered_map<std::string,query> getpool;
//    /*std::list<query>* getpool = *(std::list<query>**)((int*)lpParam + sizeof(SOCKET) + clientlen + sizeof(bool*));
//    std::list<query>* postpool = *(std::list<query>**)((int*)lpParam + sizeof(SOCKET) + clientlen + sizeof(bool*) + sizeof(std::list<query>**));*/
//    delete lpParam;
//    
//    set_socket_timeout(client, 1000);
//    while (*work) {
//        char head[4] = {};
//        int errc = recv(client, head, sizeof(head), 0);
//        if (errc == SOCKET_ERROR) {
//            int err = WSAGetLastError();
//            if (err == WSAETIMEDOUT) {
//                // Таймаут — просто закрываем соединение
//                closesocket(client);
//                return 1;
//            }
//            else {
//                std::cout << "SOCKET_ERROR at handler recv " << wsa_error_string(err) << '\n';
//                rst(client);
//                return 1;
//            }
//        }
//        if (errc != sizeof(head)) {
//            rst(client);
//            return 1;
//        }
//
//        // Читаем длину буфера правильно (network byte order → host)
//        unsigned short buflen = *(unsigned short*)(head + 2);
//        if (buflen == 0) {
//            rst(client);
//            return 1;
//        }
//
//        // Выделяем память под тело
//        char* buf = new char[buflen];
//        if (!buf) {
//            rst(client);
//            return 1;
//        }
//
//        errc = recv(client, buf, buflen, 0);
//        if (errc == SOCKET_ERROR) {
//            int err = WSAGetLastError();
//            delete[] buf;
//            if (err == WSAETIMEDOUT) {
//                closesocket(client);
//                return 1;
//            }
//            else {
//                std::cout << "SOCKET_ERROR at handler recv body " << wsa_error_string(err) << '\n';
//                closesocket(client);
//                return 1;
//            }
//        }
//        if (errc != buflen) {
//            delete[] buf;
//            closesocket(client);
//            return 1;
//        }
//
//
//
//        int retcode;
//        if (*head==GET) {
//            query q(buflen,buf);
//            
//            auto it = std::find(getpool->begin(), getpool->end(), q);
//            if (it != getpool->end()) {
//                //found
//            }
//            else {
//                delete[] buf;
//                //return error code in send
//                closesocket(client);
//                return 0;
//            }
//            delete[]buf;
//            int dbl= (*it).defaultbufferlen;
//            if (!(dbl && dbl > 0)) {
//                closesocket(client);
//            }
//            char* db = new char[10+dbl];
//            if (!db) {
//                rst(client);
//                return 1;
//            }
//            retcode = (*it).retcode;
//            memcpy(db+10, (*it).defaultbuffer, dbl);
//            // клиент ждёт 10 байт
//            // первый байт ответа на запрос
//            // второй байт, тут пометка о том что это возврат
//            // длина данных uint
//            // код int
//            // данные
//            db[1] = RETURN;
//            *(int*)(db+2) = dbl;
//            *(int*)(db + 6) = retcode;
//            errc = send(client, db, 10, 0);
//            if (!(errc==10)) {
//                delete[]db;
//                rst(client);
//                return 1;
//            }
//            
//            int speed=1'000'000, current = errc, sent = 0;
//            int remaining = dbl,tries=5; // dbl = размер всего буфера
//
//            while (remaining > 0) {
//                // берём либо "speed", либо меньше, если осталось мало
//                unsigned int chunk = (remaining > speed) ? speed : remaining;
//
//                sent = send(client, db + current, chunk, 0);
//                if (sent == SOCKET_ERROR) {
//                    int err = WSAGetLastError();
//                    // Таймаут или разрыв соединения
//                    // WSAETIMEDOUT = истёк таймаут
//                    // WSAECONNRESET = клиент закрыл соединение
//                    // WSAENOTCONN / WSAEPIPE и др. = соединение разорвано
//                    if (err == WSAETIMEDOUT) {
//                        //closesocket(client); // просто закрываем
//                        if (!tries) {
//                            closesocket(client);
//                            return 1;
//                        }
//                        tries--;
//                        continue;
//                    }
//                    else if (err == WSAECONNRESET) {
//                         // соединение уже сброшено другой стороной
//                    }
//                    else if (err == WSAENOTCONN) {
//                         // сокет не подключён, закрываем
//                    }
//                    delete[] db;
//                    closesocket(client);
//                    return 1;
//                }
//
//                // может случиться, что отправилось меньше чем "chunk"
//                if (sent == 0) {
//                    tries--;
//                    if (tries)continue;
//                    closesocket(client);
//                    return 1;
//                }
//                tries = 5;
//                current += sent;    // сдвигаем указатель
//                remaining -= sent;    // уменьшаем остаток
//            }
//
//            //errc = send(client, db, dbl + 10, 0);
//            
//        }else if(*head==POST){
//            //todo
//
//            //errc = send(client,);
//            
//            
//
//        }
//        if (head[1] & KEEPALIVE)continue;
//
//        closesocket(client);
//        break;
//    }
//    closesocket(client);
//    return 0;
//}

//enum requesttype {
//    GET,
//    POST,
//    UNDEFINED,
//    //CHANGESLAVORY,
//
//};
//enum states {
//    KEEPALIVE = 1 << 7,
//    RETURN = 1 << 6,
//    ARGUMENTS = 1 << 5,
//
//
//};


//struct Request {
//    char rt;        // тип запроса
//    char state;     // состояние
//
//    char* hdr;
//    unsigned short hdrlen;
//
//    char** argv;
//    unsigned char* argvlen;
//    unsigned char argc;
//
//    unsigned int totallen;
//
//    // ---------------- CONSTRUCTORS ----------------
//
//    Request() {
//        rt = 0;
//        state = 0;
//        hdr = nullptr;
//        hdrlen = 0;
//        argv = nullptr;
//        argvlen = nullptr;
//        argc = 0;
//        totallen = 0;
//    }
//
//    Request(char rtype, char st, unsigned char argcount) : Request() {
//        rt = rtype;
//        state = st;
//        allocArgs(argcount);
//    }
//
//    Request(char* packment, unsigned int len) : Request() {
//        unpack(packment, len);
//    }
//
//    // ---------------- HEADER ----------------
//
//    void setHeader(const char* s) {
//        if (!s) return;
//
//        if (hdr) free(hdr);
//
//        hdrlen = (unsigned short)strnlen(s, 65535);
//        hdr = (char*)malloc(hdrlen + 1);
//        memcpy(hdr, s, hdrlen);
//        hdr[hdrlen] = 0;
//
//        updateTotal();
//    }
//
//    // ---------------- ARGS ----------------
//
//    void allocArgs(unsigned char count) {
//        clearArgs();
//
//        argc = count;
//        if (!argc) return;
//
//        argv = (char**)calloc(argc, sizeof(char*));
//        argvlen = (unsigned char*)calloc(argc, sizeof(unsigned char));
//
//        updateTotal();
//    }
//
//    void setarg(unsigned char index, char* s) {
//        if (!s || index >= argc) return;
//        setarg(index, s, (unsigned char)strnlen(s, 255));
//    }
//
//    void setarg(unsigned char index, char* data, unsigned char len) {
//        if (!data || index >= argc) return;
//
//        if (argv[index]) free(argv[index]);
//
//        argv[index] = (char*)malloc(len + 1);
//        memcpy(argv[index], data, len);
//        argv[index][len] = 0; // не часть бинарного протокола
//
//        argvlen[index] = len;
//        updateTotal();
//    }
//
//    // ---------------- PACK ----------------
//
//    char* pack(unsigned int* outLen) const {
//        if (!outLen) return nullptr;
//
//        char* buf = (char*)malloc(totallen);
//        unsigned int pos = 0;
//
//        buf[pos++] = rt;
//        buf[pos++] = state;
//
//        buf[pos++] = (char)((hdrlen >> 8) & 0xFF);
//        buf[pos++] = (char)(hdrlen & 0xFF);
//
//        if (hdrlen) {
//            memcpy(buf + pos, hdr, hdrlen);
//            pos += hdrlen;
//        }
//
//        buf[pos++] = (char)argc;
//
//        for (unsigned char i = 0; i < argc; i++) {
//            buf[pos++] = (char)argvlen[i];
//            if (argvlen[i]) {
//                memcpy(buf + pos, argv[i], argvlen[i]);
//                pos += argvlen[i];
//            }
//        }
//
//        *outLen = totallen;
//        return buf;
//    }
//
//    // ---------------- UNPACK ----------------
//
//    void unpack(char* packment, unsigned int len) {
//        if (!packment || len < 5) return;
//
//        clear();
//
//        unsigned int pos = 0;
//
//        rt = packment[pos++];
//        state = packment[pos++];
//
//        hdrlen = ((unsigned short)(unsigned char)packment[pos] << 8) |
//            (unsigned short)(unsigned char)packment[pos + 1];
//        pos += 2;
//
//        if (hdrlen > len - pos) return;
//
//        if (hdrlen) {
//            hdr = (char*)malloc(hdrlen + 1);
//            memcpy(hdr, packment + pos, hdrlen);
//            hdr[hdrlen] = 0;
//            pos += hdrlen;
//        }
//
//        if (pos >= len) return;
//        argc = (unsigned char)packment[pos++];
//
//        if (argc) {
//            argv = (char**)calloc(argc, sizeof(char*));
//            argvlen = (unsigned char*)calloc(argc, sizeof(unsigned char));
//        }
//
//        for (unsigned char i = 0; i < argc; i++) {
//            if (pos >= len) return;
//
//            unsigned char alen = (unsigned char)packment[pos++];
//            if (alen > len - pos) return;
//
//            argv[i] = (char*)malloc(alen + 1);
//            memcpy(argv[i], packment + pos, alen);
//            argv[i][alen] = 0;
//
//            argvlen[i] = alen;
//            pos += alen;
//        }
//
//        updateTotal();
//    }
//
//    // ---------------- HELPERS ----------------
//
//    void updateTotal() {
//        totallen = 0;
//
//        totallen += 1; // rt
//        totallen += 1; // state
//        totallen += 2; // hdrlen
//        totallen += hdrlen;
//        totallen += 1; // argc
//
//        for (unsigned char i = 0; i < argc; i++) {
//            totallen += 1;          // len byte
//            totallen += argvlen[i]; // data
//        }
//    }
//
//    void clearArgs() {
//        if (!argv) return;
//
//        for (unsigned char i = 0; i < argc; i++) {
//            if (argv[i]) free(argv[i]);
//        }
//
//        free(argv);
//        free(argvlen);
//
//        argv = nullptr;
//        argvlen = nullptr;
//        argc = 0;
//    }
//
//    void clear() {
//        if (hdr) {
//            free(hdr);
//            hdr = nullptr;
//            hdrlen = 0;
//        }
//
//        clearArgs();
//        totallen = 0;
//    }
//
//    ~Request() {
//        clear();
//    }
//};





//hd
//
// requesttype
// states
// 
// 
// hd
//DWORD WINAPI handler(LPVOID lpParam) {
//    pktfromacceptertoclient abc = *(pktfromacceptertoclient*)lpParam;
//    delete lpParam;
//    abc.concurrentClients->fetch_add(1, std::memory_order_seq_cst);
//    if (*abc.exec_fn) {
//        (*abc.exec_fn)(abc.client);
//    }
//    ///
//    SOCKET client = abc.client;
//    int clientlen = sizeof(sockaddr_in6);
//
//    /*while (abc.work->load(std::memory_order_seq_cst)) {
//
//    }*/
//    sockaddr_in6 addr = {};
//    //getpeername(client, (sockaddr*)&addr, &clientlen);
//    //auto sit = siptext(addr);
//    bool previousLogicwaskeep = 0;
//    set_socket_timeout(client, 1000);
//    //recv(client, buf, len, MSG_WAITALL);
//    
//loop:
//    //recv len + if len?
//    //break;
//    Request r;
//    int e = 0;
//    unsigned char argc = 0;
//    char** argv = 0;
//    unsigned char* arglenv = 0;
//    unsigned short headlen = 0;
//    char* headheap = 0;
//
//    previousLogicwaskeep = 0;
//    //std::cout << sit.get() << " was in do\n";
//    int hlen = 4;
//    char* buf = (char*)malloc(hlen);
//    if (!buf)goto br;
//
//    if ((e = recv(client, buf, hlen, 0)) != hlen) {
//        //std::cout << "connection struck at header " << e << ' ' << sit.get() << '\n';
//        previousLogicwaskeep = 0;
//        goto br;
//    }
//    previousLogicwaskeep = buf[1] & KEEPALIVE;
//    headlen = *(unsigned short*)(buf + 2);
//
//    headheap = 0;
//    if (headlen) {
//        headheap = (char*)malloc(headlen);
//        if (!headheap) {
//            //std::cout << "connection struck at HEADHEAPmalloc " << sit.get() << '\n';
//            previousLogicwaskeep = 0;
//            goto br;
//        }
//        if (recvall(client, headheap, headlen, 1000) != headlen) {
//            //std::cout << "connection struck at HEADHEAP " << sit.get() << '\n';
//            previousLogicwaskeep = 0;
//            goto br;
//        }
//    }
//    else { // root header
//
//
//    }
//
//    
//    if (buf[1] & ARGUMENTS) {
//
//        if (recv(client, (char*)&argc, 1, 0) != 1) {
//            //std::cout << "connection struck at ARGUMENTS " << sit.get() << '\n';
//            previousLogicwaskeep = 0;
//            goto br;
//        }
//
//        argv = (char**)malloc(sizeof(char*) * argc);
//        if (!argv) goto br;
//        arglenv = (unsigned char*)malloc(argc);
//        if (!arglenv) {
//            std::cout << "arglenv appeared null\n";
//            previousLogicwaskeep = 0;
//            goto br;
//        }
//        if (recv(client, (char*)arglenv, argc, 0) != argc) {
//            //std::cout << "connection struck at ARGUMENTS(len) " << sit.get() << '\n';
//            previousLogicwaskeep = 0;
//            goto br;
//        }
//        int argtlen=0;
//        for (unsigned char i = 0; i < argc; i++) {
//
//            argtlen += arglenv[i];
//
//            if (!arglenv[i]) {
//                previousLogicwaskeep = 0;
//                goto br;
//            }
//        }
//        char* argdata = (char*)malloc(argtlen);
//        if (recvall(client, argdata, argtlen, 1000) != argtlen) {
//            //std::cout << "connection struck at recv ARGUMENTS(data) argtlen " << argtlen << sit.get() << '\n';
//            previousLogicwaskeep = 0;
//            free(argdata);
//            goto br;
//        }
//        int cur = 0;
//        for (unsigned char i = 0; i < argc; i++)
//        {
//            argv[i]=(char*)malloc(arglenv[i]);
//            memcpy(argv[i], argdata + cur, arglenv[i]);
//            cur += arglenv[i];
//        }
//        free(argdata);
//    }
//
//
//
//
//br:
//    if (buf)free(buf);
//    if (argv) {
//        for (size_t i = 0; i < argc; i++)
//        {
//            if(argv[i])free(argv[i]);
//        }
//        free(argv);
//    }
//    if (headheap)free(headheap);
//    if (arglenv)free(arglenv);
//    //break;
//    if(previousLogicwaskeep)goto loop;
//
//    ///
//    //std::cout << "rst should havebeen\n";
//    rst(client);
//    abc.concurrentClients->fetch_sub(1, std::memory_order_seq_cst);
//    return 0;
//}

//DWORD WINAPI accepter(LPVOID lpParam) {
//
//    pktfromlighttoaccepter abc = *(pktfromlighttoaccepter*)lpParam;
//    delete lpParam;
//    int clientlen = sizeof(sockaddr_in6);
//    SOCKET sock = abc.server;
//    while (abc.work->load(std::memory_order_seq_cst)) {
//        sockaddr_in6 addr = {};
//        SOCKET client = accept(sock, (sockaddr*)&addr, &clientlen);
//        if (client == SOCKET_ERROR) {
//            std::cout << "SOCKET_ERROR accepter() failed "<<wsa_error_string(WSAGetLastError())<<'\n';
//            continue;
//        }
//        abc.totalClients->fetch_add(1, std::memory_order_seq_cst);
//        pktfromacceptertoclient* mem = new pktfromacceptertoclient(abc.exec_fn, client, abc.work, abc.lightmutex, abc.getpool, abc.getpool, abc.concurrentClients, abc.totalClients);
//        if (!mem) {
//            std::cout << "mem is null at accepter malloc\n";
//            Sleep(100); //10cps
//            continue;
//        }
//
//        HANDLE h = CreateThread(
//            0,      // атрибуты безопасности
//            16 * 4096,      // размер стека 64 KiB
//            handler,
//            mem,      // аргумент
//            0,
//            0
//        );
//        if (!h) {
//            std::cout << "thread creating error accepter() failed\n";
//        }
//
//        /*char* addraccepted = iptext(addr);
//        std::cout << "accepted " << addraccepted<<':'<< ntohs(addr.sin6_port)<<'\n';
//        free(addraccepted);*/
//    }
//    
//    return 0;
//}

#include"mwheel.h"
DWORD WINAPI worker(LPVOID lpParam) {
    auto pkt = (pktfromlighttoaccepter*)lpParam;
    std::cout << "start\n";
    whstruc* whs = new whstruc{};

    for (auto& t : whs->timers) {
        t.wheelpos = 0;
        t.index_in_bucket = 0;
        t.os.ioc = nullptr;
        t.os.client = nullptr;
        t.os.sock = 0;
        t.os.hasTouchedByWorker.store(false, std::memory_order_relaxed);
    }

    for (size_t i = 1; i < ATSIZEA; i++)
    {
        whs->freeids.emplace_back(i);
    }

    whs->tstr = pkt->tstr;
    whs->crs.store(WHLCST - 1);

    UINT timerId = timeSetEvent(
        1,                  // период 1 мс
        1,                  // точность 1 мс (по возможности)
        wheel_callback,     // функция callback
        (DWORD_PTR)whs,                  // пользовательские данные
        TIME_PERIODIC | TIME_CALLBACK_FUNCTION // периодический таймер
    );

    if (!timerId)
    {
        timeEndPeriod(1);
        std::cout << "Failed to create multimedia timer" << std::endl;
        return 1;
    }

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
            pkt->server,
            pkt->aex,
            pkt->iocp,
            ctx,
            bytes,
            ok,
            pkt->tstr,
            pkt,
            whs
        );
    }
    // stage wheel clear
    for (size_t i = 0; i < WHLCST; i++)
    {
        whs->lst[i].lock.lock();
        whs->lst[i].lst.clear();
        whs->lst[i].lock.unlock();
    }

    // stage timers cancel 
    for (size_t i = 0; i < ATSIZEA; i++)
    {
        whs->timersmute.lock();
        if (whs->timers[i].os.ioc) {
            CancelIoEx((HANDLE)whs->timers[i].os.sock, whs->timers[i].os.ovP);
        }
        whs->timersmute.unlock();
    }
    delete whs;

    return 0;
}





class Light {
    std::atomic<size_t> concurrentClients;
    std::atomic<size_t> totalClients;

    //
    SOCKET localfd;
    std::atomic<bool> work;
    std::unordered_map<Qfield, query> getpool;
    std::unordered_map<Qfield, query> postpool;
    std::mutex mapmutex;
    //
    HANDLE iocp;
    pktfromlighttoaccepter* abc;
    timerstruc* timestr;
public:
    void(*exec_fn)(SOCKET client);
    size_t getConcurrentClients() {
        return concurrentClients.load(std::memory_order_seq_cst);
    }
    size_t getTotalClients() {
        return totalClients.load(std::memory_order_seq_cst);
    }
    timerstruc* getTstr() {
        return timestr;
    }
    Light() {
        std::lock_guard<std::mutex> lock(mapmutex);
        clear();


    }
    Light& registerString(
        requesttype rt,
        Qfield& header,
        char* defaultbuffer,
        unsigned int defaultbufferlen,
        int returncode,
        void(*exec_fn)(
            char* req,
            unsigned short reqlen,
            int& returncode,
            char*& returnbuffer,
            unsigned int& retlen
            ) = 0) {
        std::lock_guard<std::mutex> lock(mapmutex);
        query qr(header.str, header.len, defaultbuffer, defaultbufferlen, returncode, exec_fn);
        Qfield qf(header.str, header.len);
        if (rt == GET) {
            getpool[qf] = qr;
        }
        if (rt == POST) {
            postpool[qf] = qr;
        }

        tret;
    }
    //mutex
    Light& unregister(requesttype rt, Qfield qf) {
        std::lock_guard<std::mutex> lock(mapmutex);
        if (rt == GET) {
            getpool.erase(qf);

        }
        if (rt == POST) {
            postpool.erase(qf);
        }
        tret;
    }
    void clear() {
        if (work)close();
        getpool.clear();
        postpool.clear();
        work.store(0, std::memory_order_seq_cst);
        concurrentClients.store(0, std::memory_order_seq_cst);
        totalClients.store(0, std::memory_order_seq_cst);
        exec_fn = 0;
        CloseHandle(iocp);
        closesocket(localfd);
        Sleep(500);
    }
    //mutex
    //dispatcher thread
    Light& serve(unsigned short port = lightport, int backlog = 512) {
        std::lock_guard<std::mutex> lock(mapmutex);
        clear();

        SOCKET _sock = newServer(port, 0, backlog);
        if (_sock == INVALID_SOCKET) tret;
        localfd = _sock;

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
        if (r != 0 || !aex)tret;



        timestr = new timerstruc;
        if (!timestr) {
            tret;
        }
        timestr->iocp = iocp;
        timestr->work = &work;
        abc = new pktfromlighttoaccepter(&exec_fn, _sock, &work, &mapmutex, &getpool, &postpool, &totalClients, &concurrentClients, iocp, aex, timestr);
        if (localfd != SOCKET_ERROR) {
            work.store(1, std::memory_order_seq_cst);
            timeBeginPeriod(1);
            for (size_t i = 0; i < 6; i++)
            {
                post_accept(localfd, iocp, aex);
                HANDLE h = CreateThread(
                    0,      // атрибуты безопасности
                    0,      // размер стека 16 KiB 4 * 4096
                    worker,
                    abc,      // аргумент
                    0,
                    0
                );
                if (!h) {
                    work.store(0, std::memory_order_seq_cst);
                    std::cout << "thread creating error serve() failed\n";
                    tret;
                }
            }

        }
        //HANDLE h = CreateThread(
        //    0,      // атрибуты безопасности
        //    0,      // размер стека 16 KiB 4 * 4096
        //    timer_thread,
        //    timestr,      // аргумент
        //    0,
        //    0
        //);


        tret;
    }
    Light& close() {
        this->clear();
        timeEndPeriod(1);
        tret;
    }
};






void dumpConnCtx(ConnCtx* c)
{
    std::cout << "========== CONNCTX " << c << " ==========\n";

    std::cout << "closed            = " << c->closed << "\n";
    std::cout << "pkt               = " << c->pkt << "\n";

    std::cout << "socket            = " << (uint64_t)c->s << "\n";
    std::cout << "tstr              = " << c->tstr << "\n";

    std::cout << "lastIOCPTIMEO     = " << c->lastIOCPTIMEO << "\n";
    std::cout << "timedouttype      = " << (int)c->timedouttype << "\n";

    std::cout << "ov_recv           = " << c->ov_recv << "\n";
    std::cout << "oldtimerrecv      = " << c->oldtimerrecv << "\n";
    std::cout << "newtimerrecv      = " << c->newtimerrecv << "\n";

    std::cout << "timerrecv         = " << c->timerrecv << "\n";
    std::cout << "lastreadquantms   = " << c->lastreadquantms << "\n";
    std::cout << "hlmsreceivedseries.size = "
        << c->hlmsreceivedseries.size() << "\n";

    std::cout << "ov_send           = " << c->ov_send << "\n";
    std::cout << "oldtimersend      = " << c->oldtimersend << "\n";
    std::cout << "newtimersend      = " << c->newtimersend << "\n";

    std::cout << "timersend         = " << c->timersend << "\n";
    std::cout << "lastsentquantms   = " << c->lastsentquantms << "\n";
    std::cout << "hlmssentseries.size = "
        << c->hlmssentseries.size() << "\n";

    std::cout << "refcount          = " << c->refcount.load() << "\n";

    std::cout << "sending           = " << c->sending << "\n";
    std::cout << "reading           = " << c->reading << "\n";
    std::cout << "hlreading         = " << c->hlreading << "\n";
    std::cout << "hlsending         = " << c->hlsending << "\n";

    std::cout << "sndtimeo          = " << c->sndtimeo.load() << "\n";
    std::cout << "rcvtimeo          = " << c->rcvtimeo.load() << "\n";

    std::cout << "hlsndtimeo        = " << c->hlsndtimeo.load() << "\n";
    std::cout << "hlrcvtimeo        = " << c->hlrcvtimeo.load() << "\n";

    std::cout << "hlsndcounter      = "
        << c->hlsndtimecounter.load() << "\n";
    std::cout << "hlrcvcounter      = "
        << c->hlrcvtimecounter.load() << "\n";

    std::cout << "mdsndtimeo        = " << c->mdsndtimeo.load() << "\n";
    std::cout << "mdrcvtimeo        = " << c->mdrcvtimeo.load() << "\n";

    std::cout << "stimecounter      = " << c->stimecounter.load() << "\n";
    std::cout << "rtimecounter      = " << c->rtimecounter.load() << "\n";

    std::cout << "mdstimecounter    = "
        << c->mdstimecounter.load() << "\n";
    std::cout << "mdrtimecounter    = "
        << c->mdrtimecounter.load() << "\n";

    std::cout << "buf               = " << (void*)c->buf << "\n";
    std::cout << "bufsz             = " << c->bufsz << "\n";
    std::cout << "want              = " << c->want << "\n";
    std::cout << "wantPresent       = " << c->wantPresent << "\n";
    std::cout << "recvThreshold     = " << c->recvThreshold << "\n";

    std::cout << "sendbuf           = " << (void*)c->sendbuf << "\n";
    std::cout << "sendbufsz         = " << c->sendbufsz << "\n";
    std::cout << "wantsend          = " << c->wantsend << "\n";
    std::cout << "sentOff           = " << c->sentOff << "\n";
    std::cout << "sendThreshold     = " << c->sendThreshold << "\n";

    std::cout << "ps                = " << (int)c->ps << "\n";

    std::cout << "cur_hdr           = "
        << (int)(unsigned char)c->cur_hdr[0] << " "
        << (int)(unsigned char)c->cur_hdr[1] << " "
        << (int)(unsigned char)c->cur_hdr[2] << " "
        << (int)(unsigned char)c->cur_hdr[3] << "\n";

    std::cout << "keepalive         = " << c->keepalive << "\n";
    std::cout << "streaming         = " << c->streaming << "\n";

    std::cout << "Request:\n";

    std::cout << "  rt              = " << (int)c->r.rt << "\n";
    std::cout << "  state           = " << (int)c->r.state << "\n";
    std::cout << "  hdr             = " << (void*)c->r.hdr << "\n";
    std::cout << "  hdrlen          = " << c->r.hdrlen << "\n";
    std::cout << "  argv            = " << (void*)c->r.argv << "\n";
    std::cout << "  argvlen         = " << (void*)c->r.argvlen << "\n";
    std::cout << "  argc            = " << (int)c->r.argc << "\n";
    std::cout << "  totallen        = " << c->r.totallen << "\n";
    std::cout << "  lastsendamifree        = " << c->lastsendamifree << "\n";
    std::cout << "  lastrecvamifree        = " << c->lastrecvamifree << "\n";

    std::cout << "==========================================\n";
}













// =======================================================
// main
// =======================================================


#include <immintrin.h>
int main() {
    initWSA();
    Light tcp;
    tcp.serve(6934);

    size_t prevConcurrent = 0;
    size_t prevTotal = 0;
    // статистика
    while (true) {
        Sleep(1000);
        size_t currentConcurrent = tcp.getConcurrentClients();
        size_t currentTotal = tcp.getTotalClients();

        size_t deltaConcurrent = 0;
        size_t deltaTotal = 0;

        if (currentConcurrent >= prevConcurrent)
            deltaConcurrent = currentConcurrent - prevConcurrent;

        if (currentTotal >= prevTotal)
            deltaTotal = currentTotal - prevTotal;

        // текущее
        std::cout << "current " << currentConcurrent << ' ' << currentTotal << "\n";

        // дельта (без минусов)
        std::cout << "delta " << deltaConcurrent << ' ' << deltaTotal << "\n";

        prevConcurrent = currentConcurrent;
        prevTotal = currentTotal;
        std::cout << "connctx dump\n";

        tcp.getTstr()->gt.mtx.lock();
        for (size_t i = 0; i < tcp.getTstr()->gt.conns.size(); i++)
        {
            std::cout << "GlobalTrack printAll list dump start###\n";
            /*for (size_t j = 0; j < 496; j++)
            {
                printf("%x", 0xff & ((unsigned  char*)(tcp.getTstr()->gt.conns[i]))[j]);
            }*/
            dumpConnCtx(tcp.getTstr()->gt.conns[i]);
            std::cout << "###GlobalTrack printAll list dump end\n";
        }
        tcp.getTstr()->gt.mtx.unlock();

        std::cout << "connctx dump end\n";
        std::cout << sizeof(ConnCtx) << "\n";
    }



}

