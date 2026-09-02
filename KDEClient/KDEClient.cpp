// KDEClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <string>
#include "KDECoreGlobal.h"
#include "KDEPch.h"

constexpr int BUFSIZE = 1000;

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

    char recvBuffer[BUFSIZE]{};

    while (true) {
        cout << "Input: ";
        string input;
        if (!getline(cin, input))
            break;

        if (input == "quit")
            break;

        if (input.empty())
            continue;

        int sendLen = ::send(clientSocket, input.c_str(), static_cast<int>(input.size()), 0);
        if (sendLen == SOCKET_ERROR) {
            cout << "Send ERROR !!" << endl;
            break;
        }

        int recvLen = ::recv(clientSocket, recvBuffer, BUFSIZE - 1, 0);
        if (recvLen <= 0) {
            cout << "Recv ERROR or Server disconnected !!" << endl;
            break;
        }

        recvBuffer[recvLen] = '\0';
        cout << "Recv: " << recvBuffer << endl;
    }

    ::closesocket(clientSocket);
    ::WSACleanup();
    return 0;
}
