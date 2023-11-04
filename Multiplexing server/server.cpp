#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <winsock2.h> // 소켓을 사용하기 위한 헤더
#include <WS2tcpip.h>
#include <thread>
#include <windows.h>


#pragma comment(lib,"ws2_32.lib") // 라이브러리 링크

#define BUF_SIZE 1024
#define HINT 3
#define PORT 6678


void error_handling(char *message);
const char* drawHangman(int num);

SOCKET sock;

// user information
typedef struct U {
	int socket;
	int flag;
}USER;

int main(int argc, char *argv[])
{
	SetConsoleTitleA("Server");

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 0;
	}

	int serv_sock, clnt_sock;
	//SOCKET serv_sock, clnt_sock;

	char message[BUF_SIZE];
	int str_len;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	fd_set reads, copy_reads;
	struct timeval timeout;
	int fd_max, fd_num, i;

	int user_cnt = 0;        //user count
	SOCKET user[2] = { 0 }; // //user정보=[소켓]
	int game_flag = 0;    //게임진행여부


	USER challenger = { 0 };  //도전자 [0]=소켓 [1]=플래그
	USER examiner = { 0 }; //출제자 [0]=소켓 [1]=플래그

	char word[BUF_SIZE];    //출제답안
	char question[BUF_SIZE];    //맞추고있는단어
	int hint_cnt, hang_cnt, j, iscorrect, iscontinue;


	//socket(int domain, int type, int protocol)
	serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (serv_sock == -1)
		error_handling((char*)"socket() error!");


	//구조체 초기화 (gabage값 비우기)
	memset(&serv_addr, 0, sizeof(serv_addr));
	//구조체 serv_addr 값채우기 : struct sockaddr_in - 소켓의 주소를 담는 기본 구조체
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	const char *error = "bind() error!";
	if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling((char*)"bind() error!");

	if (listen(serv_sock, 5) == -1)
		error_handling((char*)"listen() error!");

	clnt_addr_size = sizeof(clnt_addr);

	// fd_set 
	FD_ZERO(&reads);    // fd_set 초기화
	FD_SET(serv_sock, &reads);  // serv_sock을 관리 대상으로 지정
	fd_max = serv_sock;  // 최대 파일 디스크립터 값

	// Multiplexing 서버
	// select 방식
	while (1)
	{
		copy_reads = reads;    //원본훼손 방지, copy해서넘김 - 원본 fd_set 복사 (select에 사용할 fd_set은 항상 원본은 두고 사본을 만들어 사용하는 방식으로 구현해야 함)
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;    // 타임아웃 설정 (microsec)
		fd_num = select(fd_max + 1, &copy_reads, 0, 0, &timeout); 
		// n : 감시할 파일디스크립터(소켓)의 최대 갯수
		//readfds : 데이터 수신을 감시할 소켓의 목록
	    // writefds : 데이터 송신을 감시할 소켓의 목록
	    // exceptfds : 예외 발생을 감시할 소켓의 목록
		// timeout : select() 함수가 감시하고 있을 시간
		// 아직 서버 소켓만 있으므로 connect 연결 요청 시 서버소켓에 데이터가 들어오게 됨

		if (fd_num == -1) break;    //오류시 종료
		if (fd_num == 0) continue;   // timeout 시간동안 변화된게 없으면 0리턴, continue

		// fd_set의 인덱스를 하나씩 보면서 변화가 있는 인덱스를 찾아냄
		// 그 인덱스가 서버 소켓이라면 connect요청이므로 새로운 소켓을 생성하여 fd_set에 등록
		for (i = 0; i < fd_max + 1; i++)
		{
			if (FD_ISSET(i, &copy_reads))   // i번 FD 발생 - 데이터 정보 변했는지 확인
			{
				if (i == serv_sock)    // listening 소켓인지확인
				{    
					// 변화가 일어난 소켓이 서버 소켓이라면 connect요청이 들어온 것임. - client 소켓 등록
					clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
					if (clnt_sock == -1)
						error_handling((char*)"accept() error!");
					// 생성한 소켓을 원본 셋에 등록한다
					FD_SET(clnt_sock, &reads);
					if (clnt_sock > fd_max)
						fd_max = clnt_sock;
					printf("connected client: %d\n", clnt_sock);
					user_cnt++;
					if (user_cnt <= 2) {
						user[user_cnt - 1] = clnt_sock;
						strcpy(message, "행맨게임에 오신 것을 환영합니다. -Player");
						if (user_cnt == 1) {
							strcat(message, "1\n상대방이 접속하면 게임이 시작됩니다.\n");
						}
						else if (FD_ISSET(user[0], &reads) && user_cnt == 2) {
							strcat(message, "2\n잠시 후 게임이 시작됩니다.\n");
							send(user[0], "잠시 후 게임이 시작됩니다.\n", strlen("잠시 후 게임이 시작됩니다.\n"), 0);
							game_flag = 1;
						}
						send(clnt_sock, message, strlen(message), 0);
					}
					else {   // 2명 정원이 다차면
						send(clnt_sock, "fail", strlen("fail"), 0);
					}

					// user1,2 둘다 connect 됐다면 게임시작
					if (FD_ISSET(user[0], &reads) && FD_ISSET(user[1], &reads) && game_flag == 1 && user_cnt == 2) {  
						hint_cnt = HINT; hang_cnt = 0;
						strcpy(message, "이번 게임의 출제자는~~?\n");
						send(user[0], message, strlen(message), 0);
						//난수생성 
						// Client 2개 중 랜덤으로 출제자 client와 맞추는 client를 정한다
						srand(time(NULL));
						examiner.socket = user[rand() % 2];
						if (examiner.socket == user[0])
							challenger.socket = user[1];
						else
							challenger.socket = user[0];
						strcpy(message, "상대방입니다! (참고로 저는 사형집행인입니다 ^-^)\n");
						send(challenger.socket, message, strlen(message), 0);
						strcpy(message, "출제자가 문제를 낼 동안 잠시만 기다려주세요!\n");
						send(challenger.socket, message, strlen(message), 0);
						strcpy(message, "당신입니다! (참고로 저는 사형집행인입니다 ^-^)\n");
						send(examiner.socket, message, strlen(message), 0);
						strcpy(message, "출제할 영어단어를 소문자 알파벳으로 입력하세요 : ");
						send(examiner.socket, message, strlen(message), 0);
						examiner.flag = 1;
					}
				}
				else
				{    
					//client소켓이므로 receive
					str_len = recv(i, message, BUF_SIZE, 0);

					//client가 종료한경우
					if (str_len == 0)
					{   
						// 원본 셋중 해당 fd의 비트를 0으로 변경. 지워준다
						FD_CLR(i, &reads);
						_close(i);
						printf("closed socket: %d\n", i);
						user_cnt--;
						if (user_cnt == 1 && game_flag == 1) { // Client 하나만 남게되면 game_flag변경. 게임이 종료되었음을 알림 
							strcpy(message, "\n상대방이 게임을 종료하여 게임이 중지되었습니다.\n");
							challenger.flag = 0;
							examiner.flag = 0;
							if (user[0] == i) {
								strcat(message, "Player1이 되었습니다.\n");
								strcat(message, "상대방이 접속하면 게임이 시작됩니다.\n");
								user[0] = user[1];
								send(user[1], message, strlen(message), 0);
							}
							else if (FD_ISSET(user[0], &reads)) {
								send(user[0], message, strlen(message), 0);
							}
							game_flag = 0;
						}
					} // 문제 맞추는 Client
					else if (i == challenger.socket && FD_ISSET(examiner.socket, &reads) && game_flag == 1)
					{
						if(str_len > 1) message[str_len - 1] = 0;
						if (strcmp(message, ""))
							switch (challenger.flag) {
							case 1:
								if (!strcmp(message, "hint")) {    //힌트요청시
									if (hint_cnt == 0) {    //사용가능힌트가없으면
										strcpy(message, "힌트를 모두 소진하였습니다.\n");
										strcat(message, "소문자 알파벳을 입력하세요 : ");
										send(i, message, strlen(message), 0);
									}
									else {
										strcpy(message, "도전자가 힌트를 요청하였습니다.\n");
										strcat(message, "힌트를 입력하세요 : ");
										send(examiner.socket, message, strlen(message), 0);
										examiner.flag = 3;
										challenger.flag = 0;
									}
									break;
								}
								iscontinue = 0; iscorrect = 0;
								if (strlen(message) == 1) { //알파벳 답 확인 
									for (j = 0; j < strlen(word); j++) {
										if (word[j] == message[0]) {
											iscorrect = 1;
											question[j] = word[j];
										}
										else if (question[j] == '*')
											iscontinue = 1;
									}
									strcat(message, "\n");
									send(examiner.socket, "도전자의 답 입력 : ", strlen("도전자의 답 입력 : "), 0);
									send(examiner.socket, message, strlen(message), 0);

									if (iscorrect == 1)    //맞췄을때
										strcpy(message, "맞췄습니다!\n");
									else {            //틀렸을때
										strcpy(message, "틀렸습니다!\n");
										hang_cnt++;
									}    //결과알림
									strcat(message, "- 문제 : ");
									strcat(message, question); strcat(message, "\n");
									strcat(message, drawHangman(hang_cnt));
									send(examiner.socket, message, strlen(message), 0);
									send(challenger.socket, message, strlen(message), 0);
									if (hang_cnt == 6) {    //마지막 기회일때 
										strcpy(message, "마지막 기회입니다.\n");
										send(examiner.socket, message, strlen(message), 0);
										send(challenger.socket, message, strlen(message), 0);
									}
									if (iscontinue == 0) {    //다 맞췄을때 
										strcpy(message, "단어완성!\n도전자의 승리!\n");
									}
									else if (hang_cnt == 7) {    //기회를 모두 소진했을때 
										strcpy(message, "행맨이 죽었습니다..ㅠㅠ\n출제자의 승리!\n");
									}
									else {    //그렇지 않은 경우-> 다시 답을 입력받음 
										strcpy(message, "도전자가 답을 입력중입니다.\n");
										send(examiner.socket, message, strlen(message), 0);
										strcpy(message, "* 'hint'입력시 힌트사용가능.\n소문자 알파벳을 입력하세요 : ");
										send(challenger.socket, message, strlen(message), 0);
										break;
									}    // 게임종료 (클라이언트가 나가지않으면 게임은 계속됨)
									strcat(message, "게임이 종료되었습니다.\n");
									strcat(message, "다음 게임에서는 역할이 바뀝니다.\n이번 게임의 출제자는~?\n");
									send(examiner.socket, message, strlen(message), 0);
									send(challenger.socket, message, strlen(message), 0);
									//게임 재시작
									// challenger client - examiner client switch
									hint_cnt = HINT; hang_cnt = 0;
									challenger.socket = examiner.socket;
									examiner.socket = i;
									strcpy(message, "상대방입니다! (참고로 저는 사형집행인입니다 ^-^)\n");
									strcat(message, "출제자가 문제를 낼 동안 잠시만 기다려주세요!\n");
									send(challenger.socket, message, strlen(message), 0);
									strcpy(message, "당신입니다! (참고로 저는 사형집행인입니다 ^-^)\n");
									strcat(message, "출제할 영어단어를 소문자 알파벳으로 입력하세요 : ");
									send(examiner.socket, message, strlen(message), 0);
									examiner.flag = 1;
									challenger.flag = 0;
									break;
								}
							default:
								break;
							}
					}   // 문제 출제하는 Client
					else if (i == examiner.socket && FD_ISSET(challenger.socket, &reads) && game_flag == 1)
					{
						if(str_len >1 ) message[str_len - 1] = 0;
						if (strcmp(message, ""))
							switch (examiner.flag) {
							case 1:
								strcpy(word, message);
								printf("%s\n", word);  
								strcpy(message, "**출제단어 : ");
								strcat(message, word);
								strcat(message, "\n맞으면 'y' / 틀리면 'n' 입력 : ");
								send(examiner.socket, message, strlen(message), 0);
								examiner.flag = 2;
								break;
							case 2:    //출제단어확인작업 
								if (!strcmp(message, "y")) {
									examiner.flag = 0;
									challenger.flag = 1;
									for (j = 0; j < strlen(word); j++)
										question[j] = '*';
									question[j] = '\0';

									strcpy(message, "- 문제 : ");
									strcat(message, question); strcat(message, "\n");
									send(examiner.socket, message, strlen(message), 0);
									send(challenger.socket, message, strlen(message), 0);

									strcpy(message, drawHangman(0));
									send(examiner.socket, message, strlen(message), 0);
									send(challenger.socket, message, strlen(message), 0);

									strcpy(message, "도전자가 답을 입력중입니다.\n");
									send(examiner.socket, message, strlen(message), 0);
									strcpy(message, "소문자 알파벳을 입력하세요 : ");
									send(challenger.socket, message, strlen(message), 0);
								}
								else if (!strcmp(message, "n")) {
									strcpy(message, "출제할 영어단어를 소문자 알파벳으로 입력하세요 : ");
									send(examiner.socket, message, strlen(message), 0);
									examiner.flag = 1;
								}
								else {
									strcpy(message, "다시입력하세요.\n맞으면 'y' / 틀리면 'n' 입력 : ");
									send(examiner.socket, message, strlen(message), 0);
								}
								break;
							case 3:    //힌트요청시
								strcat(message, "\n");
								send(challenger.socket, message, strlen(message), 0);
								hint_cnt--;
								strcpy(message, "도전자가 답을 입력중입니다.\n");
								send(examiner.socket, message, strlen(message), 0);
								strcpy(message, "소문자 알파벳을 입력하세요 : ");
								send(challenger.socket, message, strlen(message), 0);
								examiner.flag = 0;
								challenger.flag = 1;
								break;
							default:
								break;
							}
					}
				}
			}
		}
	}

	_close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

const char* drawHangman(int num) {
	const char* hangman = { 0 };
	switch (num)
	{
	case 0:
		hangman = "┌───┐\n│\n│\n│\n│\n└──────\n";
		break;
	case 1:
		hangman = "┌───┐\n│　○\n│\n│\n│\n└──────\n";
		break;
	case 2:
		hangman = "┌───┐\n│　○\n│　 |\n│\n│\n└──────\n";
		break;
	case 3:
		hangman = "┌───┐\n│　○\n│　/|\n│\n│\n└──────\n";
		break;
	case 4:
		hangman = "┌───┐\n│　○\n│　/|＼\n│　\n│\n└──────\n";
		break;
	case 5:
		hangman = "┌───┐\n│　○\n│　/|＼\n│　/\n│\n└──────\n";
		break;
	case 6:
		hangman = "┌───┐\n│　○\n│　/|＼\n│　/＼\n│\n└──────\n";
		break;
	case 7:
		hangman = "┌───┐\n│　○\n│　 X\n│　/|＼\n│　/＼\n└──────\n";
		break;
	default:
		hangman = "drawing error\n";
		break;
	}
	return hangman;
}