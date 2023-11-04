#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <WinSock2.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <Ws2tcpip.h>
#include <errno.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")


#define PORT 6678   // 임의로 할당
#define BUF_SIZE 1024

#define IP "192.168.123.104"


void error_handling(char *message);
void *send_msg(void *arg);
void *recv_msg(void *arg);

struct sockaddr_in serv_addr;
SOCKET sock;


int main(int argc, char *argv[])
{
	SetConsoleTitleA("Client");

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 0;
	}

	char message[BUF_SIZE];
	int str_len, recv_len, recv_cnt;

	//socket(int domain, int type, int protocol)
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		error_handling((char*)"socket() error!");

	//bind()
	//구조체 초기화 (gabage값 비우기)
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(IP);
	serv_addr.sin_port = htons(PORT);

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling((char*)"connect() error!");
	else
		puts("Connected...........");



	//thread_create() send와 recv용 둘다만들어줌
	std::thread sender(send_msg, (void *)&sock);
	std::thread receiver(recv_msg, (void *)&sock);

	sender.join();
	receiver.join();

	_close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void *send_msg(void *arg)
{
	int str_len;
	char message[BUF_SIZE];
	int sock = *((int*)arg);

	int serv_size = sizeof(serv_addr);

	while (1)
	{
		fgets(message, BUF_SIZE, stdin);
		send(sock, message, BUF_SIZE, 0);
		str_len = send(sock, message, strlen(message), 0);
	}
}

void *recv_msg(void *arg)
{
	int sock = *((int*)arg);
	int recv_cnt;
	char message[BUF_SIZE];

	while (1)
	{
		recv_cnt = recv(sock, message, BUF_SIZE - 1, 0);

		if (recv_cnt == -1) {
			perror("ERROR:");
			// 디버깅용
			std::cout << "message: " << message << std::endl;
			std::cout << "recv_cnt: " << recv_cnt << std::endl;
			std::cout << "WSAGetLastError:" << WSAGetLastError() << std::endl;
			// WSAECONNRESET	10054	연결이 원격 호스트에 의해 재설정되었음.

			error_handling((char*)"read() error!");
		}

		message[recv_cnt] = 0;
		if (!strcmp(message, "fail"))   // 게임 인원이 다찼거나 게임 진행 중 새로운 클라이언트가 접속 시도 시 종료 (server에서 fail메세지전송)
			error_handling((char*)"Connect Fail-game already start");

		printf("%s", message);
	}
}