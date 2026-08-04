#pragma once
#include<iostream>
//#include<windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
static char v6ds[] = {
        0,0,0,0, 0,0,0,0, 0,0, 255,255
};
static bool isUpWSA = 0;

bool initWSA() {
	if (!isUpWSA) {
		WSADATA wsa;
		if (WSAStartup(514, &wsa)) {
			std::cerr << "WSAStartup failed" << std::endl;
			return 0;
		}
		isUpWSA = 1;
	}
	return 1;
}
std::unique_ptr<char[]> siptextV6(const sockaddr_in6& addr6) {
    auto sBuf = std::make_unique<char[]>(INET6_ADDRSTRLEN);

    if (!inet_ntop(AF_INET6, (void*)&addr6.sin6_addr, sBuf.get(), INET6_ADDRSTRLEN)) {
        return nullptr; // автоматический free
    }

    return sBuf;
}
std::unique_ptr<char[]> siptext(const sockaddr_in6& addr6) {
    auto sBuf = std::make_unique<char[]>(60);

    // v6ds — твой префикс для v4-mapped

    if (memcmp(addr6.sin6_addr.u.Byte, v6ds, 12) == 0) {
        in_addr addr4;
        memcpy(&addr4.S_un.S_addr, addr6.sin6_addr.u.Byte + 12, 4);
        if (!inet_ntop(AF_INET, &addr4, sBuf.get(), 40)) return nullptr;
    }
    else {
        if (!inet_ntop(AF_INET6, &addr6.sin6_addr, sBuf.get(), 40)) return nullptr;
    }

    return sBuf;
}
char* iptextV6(const sockaddr_in6& addr6) {
    char* buf = (char*)malloc(INET6_ADDRSTRLEN);
    if (!buf) return 0;

    if (!inet_ntop(AF_INET6, &addr6.sin6_addr, buf, INET6_ADDRSTRLEN)) {
        free(buf);
        return 0;
    }

    return buf;
}
char* iptext(sockaddr_in6& addr6) {
    char* buf = (char*)malloc(60);
    if (!buf) return 0;
    if (memcmp(addr6.sin6_addr.s6_addr, v6ds, 12) == 0) {
        in_addr addr4;
        memcpy(&addr4.s_addr, addr6.sin6_addr.s6_addr + 12, 4);
        return (char*)inet_ntop(AF_INET, &addr4, buf, 40);
    }

    return (char*)inet_ntop(AF_INET6, &addr6.sin6_addr, buf, 40);
}
void set_socket_read_timeout(SOCKET sock, DWORD timeout_ms) {
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
}
void set_socket_write_timeout(SOCKET sock, DWORD timeout_ms) {
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
}

void set_socket_timeout(SOCKET sock, DWORD timeout_ms) {
    set_socket_read_timeout(sock, timeout_ms);
    set_socket_write_timeout(sock, timeout_ms);
}

const char* wsa_error_string(int err) {
    switch (err) {
    case WSAEINTR: return "Interrupted function call";
    case WSAEBADF: return "Bad file descriptor";
    case WSAEACCES: return "Permission denied";
    case WSAEFAULT: return "Bad address";
    case WSAEINVAL: return "Invalid argument";
    case WSAEMFILE: return "Too many open sockets";

    case WSAEWOULDBLOCK: return "Resource temporarily unavailable (would block)";
    case WSAEINPROGRESS: return "Operation now in progress";
    case WSAEALREADY: return "Operation already in progress";
    case WSAENOTSOCK: return "Not a socket";
    case WSAEDESTADDRREQ: return "Destination address required";
    case WSAEMSGSIZE: return "Message too long";
    case WSAEPROTOTYPE: return "Protocol wrong type for socket";
    case WSAENOPROTOOPT: return "Bad protocol option";
    case WSAEPROTONOSUPPORT: return "Protocol not supported";
    case WSAESOCKTNOSUPPORT: return "Socket type not supported";
    case WSAEOPNOTSUPP: return "Operation not supported";
    case WSAEPFNOSUPPORT: return "Protocol family not supported";
    case WSAEAFNOSUPPORT: return "Address family not supported";
    case WSAEADDRINUSE: return "Address already in use";
    case WSAEADDRNOTAVAIL: return "Cannot assign requested address";

    case WSAENETDOWN: return "Network is down";
    case WSAENETUNREACH: return "Network is unreachable";
    case WSAENETRESET: return "Network dropped connection on reset";
    case WSAECONNABORTED: return "Software caused connection abort";
    case WSAECONNRESET: return "Connection reset by peer";
    case WSAENOBUFS: return "No buffer space available";
    case WSAEISCONN: return "Socket is already connected";
    case WSAENOTCONN: return "Socket is not connected";
    case WSAESHUTDOWN: return "Cannot send after socket shutdown";
    case WSAETIMEDOUT: return "Connection timed out";
    case WSAECONNREFUSED: return "Connection refused";
    case WSAEHOSTDOWN: return "Host is down";
    case WSAEHOSTUNREACH: return "No route to host";

    case WSASYSNOTREADY: return "Network subsystem is unavailable";
    case WSAVERNOTSUPPORTED: return "Winsock.dll version out of range";
    case WSAEPROCLIM: return "Too many processes";
    case WSANOTINITIALISED: return "WSAStartup not called";

    default: return "Unknown WSA error";
    }
}
/// Создать клиентское подключение.
/// host, port — куда подключаться.
/// localPort — опционально, какой локальный порт использовать.
SOCKET newConnection(const char* host, unsigned short port, unsigned short localPort = 0,bool noDelay=0) {
    if (!initWSA()) return INVALID_SOCKET;

    // --- 1. Резолв хоста через getaddrinfo ---
    addrinfo hints{}, * res = nullptr;
    hints.ai_family = AF_UNSPEC;           // IPv4 или IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char portStr[6];
    sprintf_s(portStr, "%u", port);

    int err = getaddrinfo(host, portStr, &hints, &res);
    if (err != 0) {
        //std::cerr << "getaddrinfo failed: " << err << '\n';
        return INVALID_SOCKET;
    }

    SOCKET sock = INVALID_SOCKET;

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        // --- 2. Создаём legacy-сокет через sockaddr_in6 ---
        sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) continue;

        // Включаем dual-stack для IPv4-mapped адресов
        DWORD off = 0;
        setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off));

        // --- 3. Биндим локальный порт, если указан ---
        if (localPort) {
            sockaddr_in6 localAddr{};
            localAddr.sin6_family = AF_INET6;
            localAddr.sin6_port = htons(localPort);
            localAddr.sin6_addr = in6addr_any;

            if (bind(sock, (sockaddr*)&localAddr, sizeof(localAddr)) != 0) {
                //std::cerr << "bind(localPort) failed: " << WSAGetLastError() << '\n';
                closesocket(sock);
                sock = INVALID_SOCKET;
                continue;
            }
        }

        // --- 4. Приводим найденный адрес к sockaddr_in6 ---
        sockaddr_in6 destAddr{};
        if (p->ai_family == AF_INET) {
            // IPv4 → IPv4-mapped IPv6
            sockaddr_in* v4 = (sockaddr_in*)p->ai_addr;
            destAddr.sin6_family = AF_INET6;
            destAddr.sin6_port = v4->sin_port;
            destAddr.sin6_addr = in6addr_any;
            memcpy(&destAddr.sin6_addr.u.Byte[12], &v4->sin_addr, 4); // последние 4 байта
            destAddr.sin6_addr.u.Word[5] = 0xffff; // ::ffff:IPv4
        }
        else if (p->ai_family == AF_INET6) {
            memcpy(&destAddr, p->ai_addr, sizeof(sockaddr_in6));
        }
        else {
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }

        // --- 5. Подключаемся ---
        if (noDelay) {
            int flag = 1;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
        }
        if (connect(sock, (sockaddr*)&destAddr, sizeof(destAddr)) == 0) {
            break; // успешное подключение
        }

        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    freeaddrinfo(res);

    if (sock == INVALID_SOCKET) {
        //std::cerr << "❌ Connection failed\n";
    }

    return sock;
}


/// Создать TCP сервер.
/// port — локальный порт для прослушивания.
/// reuseAddr — включить SO_REUSEADDR.
/// backlog — размер очереди (по умолчанию 1024).
SOCKET newServer(unsigned short port, bool reuseAddr = false, int backlog = 10240) {
    if (!initWSA()) return INVALID_SOCKET;

    // --- 1. getaddrinfo для резолва локального адреса ---
    addrinfo hints{}, * res = nullptr;
    hints.ai_family = AF_INET6;          // IPv6 (dual-stack)
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;         // для bind

    char portStr[6];
    sprintf_s(portStr, "%u", port);


    int err = getaddrinfo(0, portStr, &hints, &res);
    if (err != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerrorA(err) << '\n';
        return INVALID_SOCKET;
    }

    SOCKET sock = INVALID_SOCKET;

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        // --- 2. Создаём legacy AF_INET6 сокет ---
        sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) continue;

        // --- 3. Опции сокета ---
        if (reuseAddr) {
            int opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        }

        // Включаем dual-stack
        DWORD off = 0;
        setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off));

        // --- 4. Биндим через sockaddr_in6 ---
        sockaddr_in6 bindAddr{};
        bindAddr.sin6_family = AF_INET6;
        bindAddr.sin6_port = htons(port);
        bindAddr.sin6_addr = in6addr_any;  // слушаем на всех интерфейсах
        memset(&bindAddr.sin6_addr, 0, sizeof(bindAddr.sin6_addr));

        bindAddr.sin6_addr.s6_addr[10] = 0xff;
        bindAddr.sin6_addr.s6_addr[11] = 0xff;
        inet_pton(AF_INET, "10.0.0.1", &bindAddr.sin6_addr.s6_addr[12]);

        if (bind(sock, (sockaddr*)&bindAddr, sizeof(bindAddr)) != 0) {
            std::cerr << "bind failed: " << WSAGetLastError() << '\n';
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }

        // --- 5. Ставим сокет в listen ---
        if (listen(sock, backlog) != 0) {
            std::cerr << "listen failed: " << WSAGetLastError() << '\n';
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }

        break; // готовый сервер
    }

    freeaddrinfo(res);

    if (sock == INVALID_SOCKET) {
        std::cerr << "❌ Failed to create server on port " << port << '\n';
    }
    return sock;
}

bool rst(SOCKET sockfd) {
    if (sockfd == INVALID_SOCKET) return 1;

    // Установим "abortive close" через linger = 0
    linger lg;
    lg.l_onoff = 1;   // включить linger
    lg.l_linger = 0;  // 0 секунд => RST при close
    if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char*)&lg, sizeof(lg)) == SOCKET_ERROR) {
        int e = WSAGetLastError();
        std::cerr << "setsockopt(SO_LINGER) failed: " << wsa_error_string(e) << '\n';
        // тем не менее попробуем закрыть socket
    }

    // Закрываем сокет (будет отправлен RST из-за linger)
    if (closesocket(sockfd) == SOCKET_ERROR) {
        int e = WSAGetLastError();
        std::cerr << "closesocket failed: " << wsa_error_string(e) << '\n';
        return 0;
    }
    
    return 1;
}
void fin(SOCKET sock) {
    shutdown(sock, SD_SEND);  
    closesocket(sock);
}


int recvall(SOCKET sock, char* buf, int buflen, int bytethreshold=0) {
    int total = 0;

    while (total < buflen) {
        int n = recv(sock, buf + total, buflen - total, 0);
        //std::cout << "recvall n " << n << '\n';
        if (n == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) {
                break;
            }
            //closesocket(sock);
            return -1;
        }

        if (n == 0) {
            break;
        }

        total += n;

        if (n < bytethreshold) {
            if (total == buflen) {
                break;
            }
            //closesocket(sock);
            return -1;
        }
    }

    if (total == buflen) {
        return total;
    }

    //closesocket(sock);
    return -1;
}


