// UDP Hole Punching - client1
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <thread>
#include <ctime>
#include <cstdlib>
//using namespace std;

#define IP "222.112.97.52"
#define PORT 1234

// problem candidates
std::string list[] = { "nat", "router", "dhcp", "network", "tcp", "udp", "protocol", "host", "subnet","cidr", "prefix", "switch","buffer",
"sdn", "firewall", "packet","middlebox","ipv4","bellmanford", "datagram" };

int r;
std::string str = list[r];
std::string problem;
std::string msg;
boolean flag = false;
int score = -1;
int myscore;
#pragma comment(lib,"ws2_32.lib")

const int BUFFERLENGTH = 1024;

char buffer[BUFFERLENGTH];

SOCKET connectSocket;
SOCKADDR_IN otherAddr;
int otherSize;

// Convert network information (big-endian binary data) to character string (little-endian) according to the protocol (IPV4)
std::string NormalizedIPString(SOCKADDR_IN addr) {
	char host[16];
	ZeroMemory(host, 16);
	inet_ntop(AF_INET, &addr.sin_addr, host, 16);

	USHORT port = ntohs(addr.sin_port);

	int realLen = 0;
	for (int i = 0; i < 16; i++) {
		if (host[i] == '\00') {
			break;
		}
		realLen++;
	}

	std::string res(host, realLen);
	res += ":" + std::to_string(port);

	return res;
}

// Check if the entered word is correct
void CheckWord(char ch) {
	int count = 0;
	for (int i = 0; i < str.length(); i++) {
		if (ch == str[i]) {
			problem[i] = ch;
			count++;
		}
	}// Score +1 if not correct
	if (count == 0) score += 1;
	msg = problem;
	sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
	// If you match all the words, send success and score message to peer2 (opponent peer)
	if (str == problem) {
		msg = "Success";
		sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
		msg = "Score:" + to_string(score);
		sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
		flag = true;
	}
}
// Thread executed in TaskRec
void TaskRec() {
	while (true) {
		SOCKADDR_IN remoteAddr;
		int   remoteAddrLen = sizeof(remoteAddr);
		//Receiving data from the other peer
		int iResult = recvfrom(connectSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&remoteAddr, &remoteAddrLen);
		//Run when you receive the data
		if (iResult > 0) {
			//Output relative information and received data
			std::cout << NormalizedIPString(remoteAddr) << " -> " << std::string(buffer, buffer + iResult) << std::endl;
			//If the material you received is score, send the result to your opponent
			size_t nPos = string(buffer, buffer + iResult).find("Score");
			if (nPos != string::npos) {
				myscore = stoi(string(buffer, buffer + iResult).substr(string(buffer, buffer + iResult).find(':') + 1));

				//when peer1 win
				if (score < myscore) {
					msg = "End game You win " + to_string(score) + ":" + to_string(myscore);
					sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
					msg = "End game You lose " + to_string(myscore) + ":" + to_string(score);
					cout << msg << endl;
				}
				//when draw
				else if (score == myscore) {
					msg = "End game Draw " + to_string(score) + ":" + to_string(myscore);
					sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
					msg = "End game Draw " + to_string(myscore) + ":" + to_string(score);
					cout << msg << endl;
				}
				//when peer1 lose
				else {
					msg = "End game You lose " + to_string(score) + ":" + to_string(myscore);
					sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
					msg = "End game You win " + to_string(myscore) + ":" + to_string(score);
					cout << msg << endl;
				}
			}
			//Word check
			else if (!flag) {
				char chr[1];
				strcpy(chr, string(buffer, buffer + iResult).c_str());
				CheckWord(chr[0]);
			}
		}
		//Output a message at the end of a relative connection
		else {
			std::cout << "Error: Peer closed." << std::endl;
		}
	}
}

int main() {
	SetConsoleTitleA("Client1");
	srand(time(NULL));
	// Randomly choose one of the words to match
	r = rand() % 20;
	str = list[r];
	for (int i = 0; i < str.length(); i++) {
		problem += '_';
	}
	// initialized WS2_32.DLL 
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {    // Socket version 2.2
		return 0;
	}
	// set serverSocket
	SOCKADDR_IN serverAddr;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(IP);

	int serverSize = sizeof(serverAddr);

	connectSocket = socket(AF_INET, SOCK_DGRAM, 0); // IPv4, UDP

	// ser clientSocket
	SOCKADDR_IN clientAddr;
	clientAddr.sin_port = 0;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = INADDR_ANY;
	// allocate address to connectsocket (register socket to kernel - wait for connection)
	if (bind(connectSocket, (LPSOCKADDR)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
		return 0;
	}

	int val = 64 * 1024;
	// Specifies the size of the kernel's receive and send sockets.
	// SOL_SOCKET level - specify buffer space for SO_SNDBUF data transmission
	setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&val, sizeof(val));
	setsockopt(connectSocket, SOL_SOCKET, SO_RCVBUF, (char*)&val, sizeof(val));
	// Each peer must enter the same identification number
	std::string request = "1";
	std::cout << "Identificationnumber: ";  std::cin >> request;

	// request to server - send identificationnumber to server
	sendto(connectSocket, request.c_str(), request.length(), 0, (sockaddr*)&serverAddr, serverSize);

	bool notFound = true;

	std::string endpoint;

	while (notFound) {
		SOCKADDR_IN remoteAddr;
		int   remoteAddrLen = sizeof(remoteAddr);
		// Receive peer data from server
		int iResult = recvfrom(connectSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&remoteAddr, &remoteAddrLen);

		if (iResult > 0) {  // In case of successful reception (number of bytes actually transmitted except for -1)
			endpoint = std::string(buffer, buffer + iResult);
			// print opponent peer's IP address
			std::cout << "Peer-to-peer Endpoint: " << endpoint << std::endl;

			notFound = false;
		}
		else {

			std::cout << WSAGetLastError();
		}
	}
	// opponent peer
	// front part-host number split
	std::string host = endpoint.substr(0, endpoint.find(':'));
	// back part-port number split
	int port = stoi(endpoint.substr(endpoint.find(':') + 1));
	// set socket for connect to peer
	otherAddr.sin_port = htons(port);
	otherAddr.sin_family = AF_INET;
	otherAddr.sin_addr.s_addr = inet_addr(host.c_str());

	otherSize = sizeof(otherAddr);
	char ch;
	// create thread 
	std::thread t1(TaskRec);
	std::cout << "A Word given to the Peer2 : " << str << std::endl;
	msg = "hello my name is peer1";
	sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
	msg = "Guess the word : ";
	msg += problem;
	sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
	while (true) {
		// Enter word to match hangmanGame
		std::cin >> ch;
		msg = ch;
		sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
	}
	getchar();
	// Words that the opponent peer must match
	closesocket(connectSocket);
	WSACleanup(); // = destructor
}