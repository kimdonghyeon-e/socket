#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")
#define BUFSIZE 1024
#define MAXSOCK 5
#define NAMSIZE 50

void ErrorHandling(const char* message);            // 에러 핸들링용 함수
DWORD WINAPI SockThread(LPVOID arg);                // 소켓용 스레드 생성 함수
void MsgSend(char* msg, int len, SOCKET curSock);   // 메시지 전송용 함수
void DelSock(SOCKET delSock);                       // 소켓 정리 함수

CRITICAL_SECTION locker;                            // 크리티컬세션 선언
SOCKET clntSocks[MAXSOCK+1];                        // 클라이언트 소켓 담을 배열 선언
int clntCnt = 0;                                    // 클라이언트 갯수
char clntUName[MAXSOCK][NAMSIZE];                   // 클라이언트 이름 담을 배열

int main(int argc, char* argv[]) {
    ::InitializeCriticalSection(&locker);   // 크리티컬섹션 시작
    WSADATA wsaData;                        // 윈도우 소켓 구조체 선언
    SOCKET servSock, clntSock;              // 서버 및 클라이언트 소켓 선언
    SOCKADDR_IN servAddr, clntAddr;         // 서버 및 클라이언트 주소 선언

    int clntAddrSize;        // 클라이언트 주소 사이즈 변수 선언

    // 소켓 라이브러리 초기화
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        ErrorHandling("WSAStartup() Error");
    }

    // 서버 소켓 생성
    servSock = socket(PF_INET, SOCK_STREAM, 0);
    // 소켓 생성 확인
    if (servSock == INVALID_SOCKET) {
        ErrorHandling("socket() Error!");
    }

    // 서버주소 담을공간 초기화 및 정보담기
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;                  // IPv4
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);   // IP를 빅엔디언(32비트)으로 변경
    servAddr.sin_port = htons(atoi("9898"));        // 포트를 빅엔디안(16비트)으로 변경, 포트는 9898

    // 서버 소켓 바인딩(IP주소와 포트번호 할당)
    if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        ErrorHandling("bind() Error");
    }

    // 리슨 시작
    if (listen(servSock, 5) == SOCKET_ERROR) {
        ErrorHandling("listen() Error");
    }

    // 서버 지속을 위한 무한루프
    while (1) {
        clntAddrSize=sizeof(clntAddr);

        // 클라이언트 연결 수락 및 오류 확인
        clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
        if (clntSock == INVALID_SOCKET) {
            ErrorHandling("accept() Error");
        } else {  // 오류가 없다면 클라이언트 소켓 배열에 클라이언트 소켓 저장 후 해당 소켓용 스레드 생성
            ::EnterCriticalSection(&locker);        // 잠금
            if (clntCnt==MAXSOCK) {     // 클라이언트가 최대 수에 도달시
                send(clntSock, "user is full!", 13, 0);
                std::cout << "client IP: " << inet_ntoa(clntAddr.sin_addr) << ", port: " << ntohs(clntAddr.sin_port) << " connecting fail. user is full" << std::endl;
                closesocket(clntSock);
            } else {
                std::cout << "client accepted, IP: " << inet_ntoa(clntAddr.sin_addr) << ", port: " << ntohs(clntAddr.sin_port) << " sock limit " << clntCnt+1 << "/" << MAXSOCK << std::endl;
                clntSocks[clntCnt] = clntSock;
                HANDLE _clntThread = CreateThread(NULL, 0, SockThread, &clntSock, 0, NULL);
                clntCnt++;
            }
            ::LeaveCriticalSection(&locker);        // 언락
        }
    }

	closesocket(servSock);

	//소켓 라이브러리 및 크리티컬섹션 해제
	WSACleanup();
	system("pause");
    ::DeleteCriticalSection(&locker);
	return 0;
}

void ErrorHandling(const char* message) {     // 에러 나면 출력하고 종료
	fputs(message, stderr);
	fputc('\n', stderr);
	system("pause");
	exit(1);
}

DWORD WINAPI SockThread(LPVOID arg) {   // 접속한 클라이언트 처리
    SOCKET curSock = *((SOCKET*)arg);   // 소켓 포인터 매개변수로 받아서 소켓 그대로 복사
    char msg[BUFSIZE] = { 0, };         // 메시지 전달받을 공간
    int msgLen = 0;                     // 메시지 사이즈
    char userName[NAMSIZE] = { 0, };    // 유저네임 전달받을 공간
    int uNamLen = 0;                    // 유저네임 사이즈
    int clntNum = 0;                    // 현재 clnt위치
    char tmpMsg[BUFSIZE] = { 0, };
    char tmpUName[NAMSIZE];

    // 웰컴메시지 보내기
    char welcome[BUFSIZE] = "Welcome!\n";
    send(curSock, welcome, strlen(welcome), 0);

    // // 사용자들 이름 보내기
    // ::EnterCriticalSection(&locker);
    // for (int i = 0; i < clntCnt; i++) {
    //     strcpy(tmpUName, clntUName[i]);
    //     sprintf(tmpMsg, "%s %s ", tmpMsg, tmpUName);
    // } 
    // sprintf(tmpMsg, "%s is in.", tmpMsg);
    // ::DeleteCriticalSection(&locker);
    // send(curSock, tmpMsg, strlen(tmpMsg), 0);

    // 처음 유저네임 받고 알리고 배열에 저장
    uNamLen = recv(curSock, userName, NAMSIZE, 0);
    if (uNamLen == SOCKET_ERROR) {
        DelSock(curSock);
        return 0;
    }
    char *ptr = strtok(userName, ": ");
    std::cout << ptr << " is connected" << std::endl;
    ::EnterCriticalSection(&locker);         // 잠금
    for (int i = 0; i < clntCnt; i++) {      // 카운트만큼 루프톨려 내위치 찾기
        if ( clntSocks[i] == curSock) {      // 만약 i가 지금 소켓이면
            clntNum = i;                     // i 저장
        }
    }
    strcpy(clntUName[clntNum], ptr);
    ::LeaveCriticalSection(&locker);         // 언락
    sprintf(welcome, "%s is connected\n", ptr);
    MsgSend(welcome, strlen(welcome), curSock);

    // 소켓 종료될때까지 혹은 0바이트 수신 때까지 recv()해서 메시지 받기
    while ((msgLen = recv(curSock, msg, BUFSIZE, 0)) != 0) { 
        if (msgLen == SOCKET_ERROR) {   // 소켓 에러면 끝내기
            break;
        }
        MsgSend(msg, msgLen, curSock);       // MsgSend 함수 통해 메시지 전달
    }

    // 소켓 종료(while루프 탈출) 판단시 정리 진행
    DelSock(curSock);
    return 0;
}

void MsgSend(char* msg, int len, SOCKET curSock) {  // 메시지 전송
    int toggle = 0;
    ::EnterCriticalSection(&locker);         // 잠금
    for (int i = 0; i < clntCnt; i++) {      // 카운트만큼 루프톨려 메시지 전송
        if ( clntSocks[i] == curSock) {      // 만약 i가 지금 소켓이면
            continue;           // 그냥 다음으로 넘어가기
        } else {
            if (send(clntSocks[i], msg, len, 0) == SOCKET_ERROR) {
                std::cout << "Can't send clntSock " << i << ". this will closed.";
                DelSock(clntSocks[i]);
            }
        }
    }
    ::LeaveCriticalSection(&locker);         // 언락
}

void DelSock(SOCKET delSock) {              // 소켓 삭제용
    char delUName[NAMSIZE];
    char delBuf[BUFSIZE];
    ::EnterCriticalSection(&locker);        // 잠금
    for (int i = 0; i < clntCnt; i++) {     // clntSocks에서 현재 소켓 찾아 제거
        if (delSock == clntSocks[i]) {
            strcpy(delUName, clntUName[i]);
            while (i < clntCnt) {
                clntSocks[i] = clntSocks[i + 1];
                strcpy(clntUName[i], clntUName[i+1]);
                i++;
            }
            break;
        }
    }
    clntCnt--;
    ::LeaveCriticalSection(&locker);        // 언락
    sprintf(delBuf, "%s is disconnected\n", delUName);
    MsgSend(delBuf, strlen(delBuf), delSock);
    closesocket(delSock);       // 클라이언트 소켓 종료
    std::cout << delUName << "'s socket closed. sock limit " << clntCnt << "/" << MAXSOCK << std::endl;
    
}