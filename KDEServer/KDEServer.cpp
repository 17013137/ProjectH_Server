#include <iostream>
#include "KDECoreGlobal.h"
#include "KDEPch.h"

constexpr int BUFSIZE = 1000;

struct Session {
    SOCKET socket = INVALID_SOCKET;
    char recvBuffer[BUFSIZE]{};
    int recvBytes = 0;
    int sendBytes = 0;
};

int main()
{
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 0;

    SOCKET listensocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listensocket == INVALID_SOCKET)
        return 0;

    // 0 = blocking , 1 = non-blocking
    u_long blocking_on = 0;
    if (ioctlsocket(listensocket, FIONBIO, &blocking_on) == INVALID_SOCKET) {
        cout << "You Can't Controll your Socket!!!" << endl;
        return 0;
    }

    SOCKADDR_IN sockaddr{};
    sockaddr.sin_family = AF_INET;

    //ipv4, ipv6 두개다 호환시키는 함수로 inet_pton을 사용
    //presentation to network : 문자열 -> 바이너리
    //n to p : 반대
    ::inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    //--> Host To Network Short의 약자로, 호스트 바이트 순서를 네트워크 바이트 순서로 변환하는 함수다.
    //바이트 순서
    //호스트 바이트 순서    리틀 앤디안   낮은 주소에서 낮은 바이트 저장 ex)0x1234 -> 메모리에 34 12 순서로 저장
    //네트워크 바이트 순서   빅 앤디안     낮은 주소에 높은 바이트 저장 ex)0x1234 -> 메모리에 12 34 순서로 저장
    sockaddr.sin_port = ::htons(7777);

    if (::bind(listensocket, (SOCKADDR*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
        return 0;

    if (::listen(listensocket, SOMAXCONN) == SOCKET_ERROR)
        return 0;
    cout << "----------- Server -----------" << endl;

    cout << "Accept" << endl;

    vector<Session> sessions;
    sessions.reserve(100);

    //fd_set은 여러 소켓을 동시에 감시하는 역할
    //FD_ZERO   fd_set 초기화
    //FD_SET    fd_set에 소켓 추가
    //FD_CLR    fd_set에서 소켓 제거
    //FD_ISSET  fd_set에 소켓이 포함되어 있는지 확인

    fd_set reads;
    fd_set writes;

    while (true) {
        FD_ZERO(&reads);
        FD_ZERO(&writes);

        FD_SET(listensocket, &reads);

        SOCKADDR_IN clientaddr{};
        int clientDataLength = 0;
        SOCKET clientSocket = ::accept(listensocket, (SOCKADDR*)&clientaddr, &clientDataLength);
        //if (clientSocket == SOCKET_ERROR)
        //    cout << "Accept error client socket!!!" << endl;
        //else if (clientSocket == INVALID_SOCKET)
        //    cout << "Invalid client socket!!!" << endl;
        if(clientSocket != INVALID_SOCKET){
            char ipStr[INET_ADDRSTRLEN]{};
            ::inet_ntop(AF_INET, &clientaddr.sin_addr, ipStr, INET_ADDRSTRLEN);
            cout << "Client IP : " << ipStr << endl;
        }


    }
}