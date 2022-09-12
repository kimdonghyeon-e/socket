#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
#define BUFSIZE 512
#define NAMSIZE 50

void ErrorHandling(const char* message);    // 에러 핸들링
DWORD WINAPI MsgSend(LPVOID arg);           // 수신용
DWORD WINAPI MsgRecv(LPVOID arg);           // 발신용

char name[NAMSIZE] = "[DEFAULT]";           // 사용자 이름
char buf[BUFSIZE] = { 0, };                 // 버퍼

int main(int argc, char* argv[]) {
    WSADATA wsaData;        // 윈도우 소켓 구조체 선언
    SOCKET sock;            // 클라이언트 소켓 선언
    SOCKADDR_IN servAddr;   // 클라이언트 구조체 선언

    std::cout << "Enter your name: "; std::cin >> name;
    fflush(stdin);

    // 소켓 라이브러리 초기화
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        ErrorHandling("WSAStartup() Error");
    }

    // 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    // 소켓 생성 확인
    if (sock == INVALID_SOCKET) {
        ErrorHandling("socket() Error!");
    }

    // 서버주소 담을공간 초기화 및 정보담기
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;                       // IPv4
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");   // IP를 빅엔디언(32비트)으로 변경, 주소는 루프백
    servAddr.sin_port = htons(atoi("9898"));             // 포트를 빅엔디안(16비트)으로 변경, 포트는 9898

    // 서버 접속
    if (connect(sock, (SOCKADDR*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR) {
        ErrorHandling("connect() Error");
    }

    // 메시지 송수신 스레드 실행
    HANDLE _sendThread = CreateThread(NULL, 0, MsgSend, &sock, 0, NULL);
    HANDLE _recvThread = CreateThread(NULL, 0, MsgRecv, &sock, 0, NULL);

    // 메시지 송수신 스레드 종료 대기
    WaitForSingleObject(_sendThread, INFINITE);
    WaitForSingleObject(_recvThread, INFINITE);

    // 소켓종료 및 정리
    closesocket(sock);
    WSACleanup();
    return 0;
}

void ErrorHandling(const char* message) {     // 에러 나면 출력하고 종료
	fputs(message, stderr);
	fputc('\n', stderr);
	system("pause");
	exit(1);
}

DWORD WINAPI MsgSend(LPVOID arg) {          // 수신용
    SOCKET sock = *((SOCKET*)arg);          // 소켓 받고
    char nameMsg[NAMSIZE+BUFSIZE] = { 0, }; // 이름이랑 메시지 합칠 문자열 담을거

    while (1) {     // 계속 돌면서 보내기
        fgets(buf,BUFSIZE,stdin);   // 입력받고
        if (!strcmp(buf, "q\n") || !strcmp(buf, "Q\n")) {   // q치는지 검사, 치면 종료
            std::cout << "socket closed. program will stop" << std::endl;
            closesocket(sock);
            exit(0);
        }

        sprintf(nameMsg, "%s: %s", name, buf);      // 문자열 합치고
        send(sock, nameMsg, strlen(nameMsg), 0);    // 보내기
    }

    return 0;
}

DWORD WINAPI MsgRecv(LPVOID arg) {      // 발신용
    SOCKET sock = *((SOCKET*)arg);      // 소켓 받고
    char nameMsg[NAMSIZE+BUFSIZE];      // 이름이랑 메시지 합친 문자열 담을거
    int strLen;                         // 문자열 길이 담을거

    while (1) {     // 계속 돌면서 받기
        strLen = recv(sock, nameMsg, NAMSIZE+BUFSIZE-1, 0);     // 리시브하고
        if (strLen == SOCKET_ERROR) {       // 에러 체크
            return -1;      // 에러면 끝내기
        }

        nameMsg[strLen] = 0;        // 문자열 마지막 표시
        fputs(nameMsg, stdout);     // 화면에 표시
    }

    return 0;
}