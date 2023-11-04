// UDP Hole Punching - server
#include <winsock2.h>  //header
#include <Ws2tcpip.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <map>

#pragma comment(lib,"ws2_32.lib") // Library Links

#define PORT 1234           //port number
#define PACKET_SIZE 1024    // packet size

SOCKET serverSocket;

std::map<int, SOCKADDR_IN> current; //Save Information for Connected Users

//Reformat the ip information of the connected user to the string
std::string NormalizedIPString(SOCKADDR_IN addr) {
	char host[16];
	//Macros filling memory area with 0X00
	ZeroMemory(host, 16);
	//It converts IPv4 and IPv6 addresses from binary to text
	inet_ntop(AF_INET, &addr.sin_addr, host, 16);
	//It changes the value received as a factor to the Big Endian format.
	USHORT port = ntohs(addr.sin_port);

	int realLen = 0;
	//Actual length of the packet
	for (int i = 0; i < 16; i++) {
		if (host[i] == '\00') {
			break;
		}
		realLen++;
	}
	//Insert ip address and port number in res
	std::string res(host, realLen);
	res += ":" + std::to_string(port);

	return res;
}

//Pass the other peer's ip address and port number to the peer
void SendResponse(SOCKADDR_IN addr, SOCKADDR_IN receiver) {
	int size = sizeof(addr);
	//Save other peer information
	std::string msg = NormalizedIPString(addr);
	//Change other peer to peer
	sendto(serverSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&receiver, sizeof(receiver));
}

int main() {
	//Set Console Title
	SetConsoleTitleA(("Server [" + std::to_string(PORT) + "]").c_str());
	//Create WSADATA Object
	WSADATA wsaData;
	//Start using Winsoket 2.2 version
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 0;
	}
	//Specify socket IPv4, UDP 
	serverSocket = socket(AF_INET, SOCK_DGRAM, 0); // IPv4, UDP
	//Specify the port number to be used by the server, the ip address system, and the ip address of the LAN card
	SOCKADDR_IN sockAddr;
	sockAddr.sin_port = htons(PORT);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Set socket address information and length as default factor for server socket
	if (bind(serverSocket, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR) {
		return 0;
	}

	int val = 64 * PACKET_SIZE;
	//Specify send and receive buffer sizes
	setsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, (char*)&val, sizeof(val));
	setsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, (char*)&val, sizeof(val));

	// Create receive queue
	listen(serverSocket, 1000);  // The size of the receive queue : 1000
	//Specify buffer, buffer size
	char buffer[PACKET_SIZE];
	int bufferLength = PACKET_SIZE;

	while (true) {
		SOCKADDR_IN clientAddr;
		int clientSize = sizeof(clientAddr);
		//Receiving data from the client
		int iResult = recvfrom(serverSocket, buffer, bufferLength, 0, (sockaddr*)&clientAddr, &clientSize);
		//Run when you receive the data -> entered peer
		if (iResult > 0) {
			try {
				//peer's Identificationnumber information
				int id = stoi(std::string(buffer, buffer + iResult));
				//if the identificationnumber of the peer you are connected to is in the receive queue
				if (current.find(id) != current.end()) {
					SOCKADDR_IN other = current[id];
					//Send each other's information to peers with the same identification number
					SendResponse(other, clientAddr);
					SendResponse(clientAddr, other);
					//print
					std::cout << "Linked" << std::endl << "   ID: " << id << std::endl
						<< "   Endpoint1: " << NormalizedIPString(clientAddr) << std::endl
						<< "   Endpoint2: " << NormalizedIPString(other) << std::endl << std::endl;
					//id erase
					current.erase(id);
				}
				//if the identification number of the peer you are connected to isn't in the receive queue
				else {
					//insert peer's information in receive queue
					current.insert(std::make_pair(id, clientAddr));
					//print
					std::cout << "Registered" << std::endl << "   ID: " << id << std::endl
						<< "   Endpoint: " << NormalizedIPString(clientAddr) << std::endl << std::endl;
				}
			}
			catch (std::invalid_argument& e) {}
			catch (std::out_of_range& e) {}
			catch (...) {}
		}

		Sleep(1);
	}
	closesocket(serverSocket);
	WSACleanup(); // = destructor
}