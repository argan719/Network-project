// UDP Hole Punching - client2
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <thread>
#include <ctime>
#include <cstdlib>


#define IP "222.112.97.52"
#define PORT 9876

// problem candidates
std::string list[] = { "nat", "router", "dhcp", "network", "tcp", "udp", "protocol", "host", "subnet","cidr", "prefix", "switch","buffer",
"sdn", "firewall", "packet","middlebox","ipv4","bellmanford", "datagram" };

std::string str;
std::string problem;
boolean flag = false;
std::string msg;
#pragma comment(lib,"ws2_32.lib")  // library link

int score = -1;
int myscore;
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
	int check = 0;
	for (int i = 0; i < str.length(); i++) {
		if (ch == str[i]) {
			problem[i] = ch;
			check++;
		}
	} // Score +1 if not correct
	if (check == 0) score += 1;
	msg = problem;
	sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
	// If you match all the words, send success and score message to peer2 (opponent peer)
	if (str == problem) {
		msg = "Success";
		sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
		msg = "Score:" + std::to_string(score);
		sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
	}
}

// Thread executed in TaskRec
void TaskRec() {
	while (true) {
		SOCKADDR_IN remoteAddr;
		int remoteAddrLen = sizeof(remoteAddr);
		//Receiving data from the other peer
		int iResult = recvfrom(connectSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&remoteAddr, &remoteAddrLen);
		//Run when you receive the data
		if (iResult > 0) {
			//Output relative information and received data
			std::cout << NormalizedIPString(remoteAddr) << " -> " << std::string(buffer, buffer + iResult) << std::endl;
			//If the material you received is successful, send the correct question to your opponent
			size_t nPos = std::string(buffer, buffer + iResult).find("Score");
			if (nPos != std::string::npos) {
				myscore = std::stoi(std::string(buffer, buffer + iResult).substr(std::string(buffer, buffer + iResult).find(':') + 1));
				flag = true;
				msg = "Guess the word : ";
				msg += problem;
				sendto(connectSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&otherAddr, otherSize);
			}
			//If you get a message to end the game, the game is over
			nPos = std::string(buffer, buffer + iResult).find("End game");
			if (nPos != std::string::npos) {
				flag = false;
			}
			//Word check
			if (flag) {
				char chr[1];
				strcpy(chr, std::string(buffer, buffer + iResult).c_str());
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
	SetConsoleTitleA("Client2");
	srand(time(NULL));
	// Randomly choose one of the words to match
	int r = rand() % 20;
	str = list[r];
	for (int i = 0; i < str.length(); i++) {
		problem += '_';
	}
	// initialized WS2_32.DLL 
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {  // Socket version 2.2
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
	int val = 64 * BUFFERLENGTH;
	// Specifies the size of the kernel's receive and send sockets.
	// SOL_SOCKET level - specify buffer space for SO_SNDBUF data transmission
	setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&val, sizeof(val));
	setsockopt(connectSocket, SOL_SOCKET, SO_RCVBUF, (char*)&val, sizeof(val));
	// Specify buffer space to receive SO_RCVBUF data
	// Each peer must enter the same identification number
	std::string request = "1";
	std::cout << "Identificationnumber: ";  std::cin >> request;

	// request to server - send identificationnumber to server
	sendto(connectSocket, request.c_str(), request.length(), 0, (sockaddr*)&serverAddr, serverSize);

	bool notFound = true;
	std::string endpoint;

	while (notFound) {
		SOCKADDR_IN remoteAddr;
		int remoteAddrLen = sizeof(remoteAddr);
		// Receive peer data from server
		int iResult = recvfrom(connectSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&remoteAddr, &remoteAddrLen);
		if (iResult > 0) { // In case of successful reception (number of bytes actually transmitted except for -1)
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
	char ch;
	otherSize = sizeof(otherAddr);


	// create thread 
	std::thread t1(TaskRec);
	std::cout << "A Word given to the Peer1 : " << str << std::endl;
	msg = "Hi I'm Peer2";
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