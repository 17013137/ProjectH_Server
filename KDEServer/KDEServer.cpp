#include <iostream>
#include <cstdint>
#include <cstring>
#include "KDECoreGlobal.h"
#include "KDEPch.h"

constexpr int BUFSIZE = 1000;

// 메시지 프레이밍: [4바이트 길이(빅엔디안)][payload]
constexpr int HEADER = 4;
constexpr uint32_t MAX_MSG = 1u << 20; // 1MB, 비정상적으로 큰 길이 방어

struct Session {
    SOCKET socket = INVALID_SOCKET;
    vector<char> recvBuf; // 아직 한 메시지를 다 채우지 못한 수신 누적 버퍼
    vector<char> sendBuf; // 아직 다 보내지 못한 송신 버퍼
    size_t sendOffset = 0; // sendBuf에서 이미 보낸 위치
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
    u_long blocking_on = 1;
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

        for (auto& session : sessions) {
            // 읽기는 항상 감시한다.
            FD_SET(session.socket, &reads);
            // 아직 보낼 데이터가 남아 있을 때만 쓰기를 감시한다.
            if (session.sendOffset < session.sendBuf.size())
                FD_SET(session.socket, &writes);
        }

        //recv, accept, send 가능한 소켓이 존재하는지 확인
        if (::select(0, &reads, &writes, nullptr, nullptr) == SOCKET_ERROR)
            continue;

        if (FD_ISSET(listensocket, &reads)) {
            SOCKADDR_IN clientaddr{};
            int clientAddrLen = sizeof(clientaddr);
            SOCKET clientSocket = ::accept(listensocket, (SOCKADDR*)&clientaddr, &clientAddrLen);

            if (clientSocket != INVALID_SOCKET) {
                u_long nonBlocking = 1;
                ::ioctlsocket(clientSocket, FIONBIO, &nonBlocking);

                char ipStr[INET_ADDRSTRLEN]{};
                ::inet_ntop(AF_INET, &clientaddr.sin_addr, ipStr, INET_ADDRSTRLEN);
                cout << "------ Connect!! Client IP : " << ipStr << " ------" << endl;

                Session session;
                session.socket = clientSocket;
                sessions.push_back(session);
            }
        }

        for (size_t i = 0; i < sessions.size(); ++i) {
            Session& session = sessions[i];
            bool remove = false;

            if (FD_ISSET(session.socket, &reads)) {
                char tmp[BUFSIZE];
                int n = ::recv(session.socket, tmp, BUFSIZE, 0);
                if (n <= 0) {
                    remove = true; // 상대가 연결을 끊었거나 에러
                }
                else {
                    // 받은 바이트를 누적한 뒤, 완성된 메시지만 잘라서 처리
                    session.recvBuf.insert(session.recvBuf.end(), tmp, tmp + n);

                    while (session.recvBuf.size() >= HEADER) {
                        uint32_t len;
                        memcpy(&len, session.recvBuf.data(), HEADER);
                        len = ntohl(len);

                        if (len > MAX_MSG) { // 잘못된 길이 → 연결 종료
                            remove = true;
                            break;
                        }
                        if (session.recvBuf.size() < HEADER + static_cast<size_t>(len))
                            break; // 아직 한 메시지가 다 안 옴 → 다음 recv 대기

                        const char* payload = session.recvBuf.data() + HEADER;

                        cout.write(payload, len); // 널 종료에 의존하지 않고 길이만큼만 출력
                        cout << endl;

                        // 에코: [길이][payload]를 송신 버퍼에 그대로 쌓는다.
                        uint32_t netLen = htonl(len);
                        session.sendBuf.insert(session.sendBuf.end(),
                            reinterpret_cast<char*>(&netLen),
                            reinterpret_cast<char*>(&netLen) + HEADER);
                        session.sendBuf.insert(session.sendBuf.end(), payload, payload + len);

                        // 소비한 한 메시지를 누적 버퍼 앞에서 제거
                        session.recvBuf.erase(session.recvBuf.begin(),
                            session.recvBuf.begin() + HEADER + len);
                    }
                }
            }

            if (!remove && FD_ISSET(session.socket, &writes)
                && session.sendOffset < session.sendBuf.size()) {
                int n = ::send(session.socket,
                    session.sendBuf.data() + session.sendOffset,
                    static_cast<int>(session.sendBuf.size() - session.sendOffset), 0);
                if (n == SOCKET_ERROR) {
                    if (::WSAGetLastError() != WSAEWOULDBLOCK)
                        remove = true; // 진짜 에러만 종료, WOULDBLOCK은 다음 기회에 재시도
                }
                else if (n > 0) {
                    session.sendOffset += n;
                    if (session.sendOffset == session.sendBuf.size()) {
                        session.sendBuf.clear();
                        session.sendOffset = 0;
                    }
                }
            }

            if (remove) {
                ::closesocket(session.socket);
                sessions.erase(sessions.begin() + i);
                --i;
            }
        }
    }
}
