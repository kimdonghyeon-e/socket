#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
void error_handling(char* message);

int main(int argc, char* argv[])
{
	int clnt_sock;
	struct sockaddr_in serv_addr;
	char message[1024] = {0x00, };

	//TCP연결지향형이고 ipv4 도메인을 위한 소켓을 생성
	clnt_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(clnt_sock == -1)
		error_handling("socket error");

	//인자로 받은 서버 주소 정보를 저장
	memset(&serv_addr, 0, sizeof(serv_addr));
	//서버주소체계는 IPv4이다
	serv_addr.sin_family = AF_INET;                  
	//서버주소 IP저장해주기(인자로 받은거 넘겨주기)
	// serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//서버주소 포트번호 인자로 받은거 저장해주기
	// serv_addr.sin_port = htons(atoi(argv[2]));
    serv_addr.sin_port = htons(atoi("3555"));

	//클라이언트 소켓부분에 서버를 연결!
	if(connect(clnt_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect error");

	//연결이 성공적으로 되었으면 데이터 받기
	if(read(clnt_sock, message, sizeof(message)-1) == -1)
		error_handling("read error");
	printf("Message from server :%s\n", message);
    
	//통신 후 소켓 클로우즈
	close(clnt_sock);
	return 0;
}
void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}