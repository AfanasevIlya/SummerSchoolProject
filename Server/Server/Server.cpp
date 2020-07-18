#pragma comment(lib, "Ws2_32.lib")
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996) // For strcat
#pragma warning(disable: 6319) // For FD func

#define DEFAULT_PORT 54000

fd_set master;
std::vector<std::string> userlist;

// Receive message and send to other client socks
int recvAndSendToClients(SOCKET s) {
	int size_name, welcome_msg_size;
	recv(s, (char*)&size_name, sizeof(int), 0);
	char* client_n = new char[size_name+1];
	client_n[size_name] = '\0';
	recv(s, client_n, size_name, 0);  // Get user name
	std::string welcome_msg = "Server: Welcome to the chat server ";
	welcome_msg += client_n; // Concatinate welcome msg with user name
	welcome_msg_size = welcome_msg.size();
	for (int i = 0; i < master.fd_count; i++) {
		send(master.fd_array[i], (char*)&welcome_msg_size, sizeof(int), 0); // Send size of msg
		send(master.fd_array[i], welcome_msg.c_str(), welcome_msg.size(), 0); // Send welcome msg
	}
	std::cout << '\r';
	std::cout << client_n << " connected" << std::endl; 
	std::cout << "Server: ";
	while (true) {
		int msg_size;
		int result = recv(s, (char*)&msg_size, sizeof(int), 0); // Check getted data
		char* msg = new char[msg_size+1];
		msg[msg_size] = '\0';
		if (result > 0) {
			recv(s, msg, msg_size, 0);
			std::cout << '\r';
			std::cout << msg << std::endl;
			std::cout << "Server: ";
			for (int i = 0; i < master.fd_count; i++) {
				if (master.fd_array[i] != s) { 
					send(master.fd_array[i], (char*)&msg_size, sizeof(int), 0);
					send(master.fd_array[i], msg, msg_size, 0); // Send msg to everyone exept source user
				}
			}
		}
		else if (result == 0) {
			// Annouce server about disconnection
			FD_CLR(s, &master);
			std::cout << '\r';
			std::cout << "A client disconnected: " << client_n << std::endl;
			std::cout << "Server: ";
			// Annouce to others
			for (int i = 0; i < master.fd_count; i++) {
				std::string disconnected_msg = "Server: A client disconnected: ";
				disconnected_msg += client_n;
				int disconnected_msg_size = disconnected_msg.size();
				send(master.fd_array[i], (char*)&disconnected_msg_size, sizeof(int), 0);
				send(master.fd_array[i], disconnected_msg.c_str(), disconnected_msg_size, 0);
			}
			return 1;
		}
		else {
			// Show server reciving error
			FD_CLR(s, &master);
			std::cout << '\r';
			std::cout << "a client receive failed: " << WSAGetLastError() << std::endl;
			std::cout << "Server: ";
			// Annouce to others
			for (int i = 0; i < master.fd_count; i++) {
				std::string disconnected_msg = "Server: A client disconnected: ";
				disconnected_msg += client_n;
				int disconnected_msg_size = disconnected_msg.size();
				send(master.fd_array[i], (char*)&disconnected_msg_size, sizeof(int), 0);
				send(master.fd_array[i], disconnected_msg.c_str(), disconnected_msg_size, 0);
			}
			return 2;
		}
		delete[] msg;
	} 
	delete[] client_n;
}
// Accept function 
int accept0(SOCKET listenSOCK) {
	while (true) {
		SOCKET ClientSocket = accept(listenSOCK, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) { // Check socket and give server info
			std::cout << '\r';
			std::cout << "accept error: " << WSAGetLastError() << std::endl;
			std::cout << "Server: ";
			closesocket(ClientSocket);
		}
		else {
			std::thread handle(recvAndSendToClients, ClientSocket); //threat for main loop
			handle.detach();
			FD_SET(ClientSocket, &master);

		}
	}
}
// Send message to all clients
int sendAnouce() {
	std::string buf;
	do {
		std::cout << "Server: ";
		std::getline(std::cin, buf);
		buf = "Server: " + buf;
		int buf_size = buf.size();
		for (int i = 0; i < master.fd_count; i++) {
			int result = send(master.fd_array[i], (char*)&buf_size, sizeof(int), 0);
			if (result == SOCKET_ERROR) { // Check for error sending
				std::cout << '\r';
				std::cout << "send to client " << i << " failed: " << WSAGetLastError() << std::endl;
				std::cout << "Server: ";
			}
			else
			{
				send(master.fd_array[i], buf.c_str(), buf_size, 0);
			}
		}
	} while (true);
	return 0;
}


int main() {

	// Initialize Winsock
	int result;
	WSADATA wsadata;			
	result = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (result != 0) { //Check for winsock err
		std::cout << "WSAStartup error: " << result << std::endl;
		return 1;
	}
	else std::cout << "WSAStartup OK" << std::endl;

	// Create listening SOCK
	SOCKET listenSOCK = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSOCK == INVALID_SOCKET) {
		std::cout << "listenSOCK error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else std::cout << "listenSOCK OK" << std::endl;

	// Bind addr and port to sock	
	struct addrinfo* res = NULL;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	int port = -1;
	std::cout << "Listen on port (default: 54000): ";
	std::cin >> port;
	// Check inputed port
	if (port <= 0 || port > 65535) { 
		std::cout << "Invalid, listen on 54000" << std::endl;
		port = DEFAULT_PORT;								
		result = getaddrinfo(NULL, std::to_string(DEFAULT_PORT).c_str(), &hints, &res);
	}
	else {
		result = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res);
	}
	if (result != 0) { 
		std::cout << "getaddrinfo error: " << WSAGetLastError() << std::endl;
		closesocket(listenSOCK);
		WSACleanup();
		return 1;
	}
	else std::cout << "getaddrinfo OK" << std::endl;
	result = ::bind(listenSOCK, res->ai_addr, (int)res->ai_addrlen);
	if (result == SOCKET_ERROR) {
		std::cout << "bind error: " << WSAGetLastError() << std::endl;
		closesocket(listenSOCK);
		WSACleanup();
		return 1;
	}
	else std::cout << "bind OK" << std::endl;
	freeaddrinfo(res);
	 // Listening	
	result = listen(listenSOCK, SOMAXCONN); 
	if (result == SOCKET_ERROR) {
		std::cout << "listen error: " << WSAGetLastError() << std::endl;
		closesocket(listenSOCK);
		WSACleanup();
		return 1;
	}
	else std::cout << "listen OK" << std::endl;
	std::cout << "Listening on port " << port << std::endl;
	std::cout << std::endl << std::endl;
	/*
		Done bind
	*/

	// Accept thread
	std::thread accept_thread(accept0, listenSOCK);
	accept_thread.detach();

	// Send anouce thread
	std::thread send_thread(sendAnouce);
	send_thread.join();

	// Cleanup
	WSACleanup();
	return 0;
}