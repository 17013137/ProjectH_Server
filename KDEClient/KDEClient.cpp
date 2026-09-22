// KDEClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <string>
#include <cstdint>
#include "KDECoreGlobal.h"
#include "KDEPch.h"

constexpr int HEADER = 4; // [4바이트 길이(빅엔디안)][payload]

// 요청한 바이트를 전부 보낼 때까지 반복 (부분 전송 대비)
static bool sendAll(SOCKET s, const char* p, int len)
{
    int sent = 0;
    while (sent < len) {
        int n = ::send(s, p + sent, len - sent, 0);
        if (n <= 0)
            return false;
        sent += n;
    }
    return true;
}

// 요청한 바이트를 전부 받을 때까지 반복 (부분 수신 대비)
static bool recvAll(SOCKET s, char* p, int len)
{
    int got = 0;
    while (got < len) {
        int n = ::recv(s, p + got, len - got, 0);
        if (n <= 0)
            return false;
        got += n;
    }
    return true;
}

int main()
{
    cout << "----------- Client -----------" << endl;

    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup Error !!" << endl;
        return 0;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "clientSocket ERROR !!" << endl;
        ::WSACleanup();
        return 0;
    }

    SOCKADDR_IN serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = ::htons(7777);
    ::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    int TryCount = 0;

    while(::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Loading... " << TryCount << endl;
        TryCount++;
        if (TryCount > 500) {
            cout << "Failed Loading!!" << endl;
            return 0;
        }
    }

    cout << "Connected to Server !" << endl;

    while (true) {
        cout << "Input: ";
        string input;
        if (!getline(cin, input))
            break;

        if (input == "quit")
            break;

        if (input.empty())
            continue;

        // 보내기: [길이][payload]
        uint32_t netLen = htonl(static_cast<uint32_t>(input.size()));
        if (!sendAll(clientSocket, reinterpret_cast<char*>(&netLen), HEADER)
            || !sendAll(clientSocket, input.c_str(), static_cast<int>(input.size()))) {
            cout << "Send ERROR !!" << endl;
            break;
        }

        // 받기: 먼저 4바이트 길이 → 그 길이만큼 payload
        uint32_t len;
        if (!recvAll(clientSocket, reinterpret_cast<char*>(&len), HEADER)) {
            cout << "Recv ERROR or Server disconnected !!" << endl;
            break;
        }
        len = ntohl(len);

        string msg(len, '\0');
        if (len > 0 && !recvAll(clientSocket, &msg[0], static_cast<int>(len))) {
            cout << "Recv ERROR or Server disconnected !!" << endl;
            break;
        }

        cout << "Recv: " << msg << endl;
    }

    ::closesocket(clientSocket);
    ::WSACleanup();
    return 0;
}
