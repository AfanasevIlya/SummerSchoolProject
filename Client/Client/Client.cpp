#pragma comment(lib, "Ws2_32.lib")
#include <ws2tcpip.h>
#include <iostream>
#include <WinSock2.h>
#include <string>
#include <thread>
#define DEFAULT_THREAD_SLEEP_TIMER 10

SOCKET g_sock;
std::string g_name;
const int COMMAND_COUNT = 2;
const char* COMMANDS[] = { "exit", "set.new_name" }; //keep local command, we can also add smthg new




// Exit function
void exitProgramm() { 
	char ask;
	bool loop = true;
	while (loop) {
		std::cout << "Do you want to exit?" << std::endl << "Y/N?" << std::endl;
		std::cin >> ask;
		switch (tolower(ask)) {
		case 'y': closesocket(g_sock); WSACleanup(); exit(1);
		case 'n': loop = false; break;
		default: continue;
		}
	}
}

// Set new name function
void newName() {  
	std::cout << "Input new name: ";
	std::getline(std::cin, g_name);
}

// Menu ( compare input of user and local commans
bool menu(const char* command) {  
	for (int i = 0; i < COMMAND_COUNT; ++i) {
		if (strcmp(command, COMMANDS[i]) == 0) {
			switch (i) {
			case 0: exitProgramm(); return true;
			case 1: newName(); return true;
			}
		}
	}
	return false;
}

// Send thread function
int send0(SOCKET s) {
	std::string buf;
	while (true) {
		std::cout << g_name << " : "; std::getline(std::cin, buf);
		if (menu(buf.c_str())) continue; 
		if (buf.length() > 0) {
			buf = g_name + ": " + buf;
			int msg_size = buf.size();
			if (send(s, (char*)&msg_size, sizeof(int), 0) == SOCKET_ERROR) {
				std::cout << "send message size failed: " << WSAGetLastError() << std::endl;
				closesocket(s);
				WSACleanup();
				return 1;
			}
			send(s, buf.c_str(), msg_size, 0);
		}
		Sleep(DEFAULT_THREAD_SLEEP_TIMER);
	} 
	return 0;
}

// Receive thread function
int receive(SOCKET s) {
	while (true) {
		int msg_size;
		if (recv(s, (char*)&msg_size, sizeof(int), 0) <= 0) { // Get msg size from server and check for error
			std::cout << "Connection closed" << std::endl;
			return 1;
		}
		char* msg = new char[msg_size + 1];
		msg[msg_size] = '\0';
		int result = recv(s, msg, msg_size, 0); // Get msg from server and show to client
		if (result > 0) {
			std::cout << '\r';
			std::cout << msg << std::endl;
			std::cout << g_name << ": ";
		}
		else if (result == 0) {
			delete[] msg;
			std::cout << "Connection closed" << std::endl;
			return 1;
		}
		else {
			std::cout << "recv failed: " << WSAGetLastError() << std::endl;
			delete[] msg;
			closesocket(s);
			WSACleanup();
			return 2;
		} delete[] msg;
	} 
	
}

int main() {
	//Initialize Winsock
	int result;	
	WSADATA wsaData;
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cout << "Initialize Winsock Fail: " << result << std::endl;
		return 1;
	}
	else std::cout << "Initialize Winsock OK" << std::endl;

	// Resolve the server address and port
	addrinfo hints, * res;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	std::string host; std::cout << "Host: "; std::getline(std::cin, host);
	std::string port; std::cout << "Port: "; std::getline(std::cin, port);
	result = getaddrinfo(host.c_str(), port.c_str(), &hints, &res); // Check inputed host & port
	if (result != 0) {
		std::cout << "getaddrinfo failed: " << result << std::endl;
		return 1;
	}
	else std::cout << "getaddrinfo OK" << std::endl;

	// Create connecting Sock
	g_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (g_sock == INVALID_SOCKET) {
		std::cout << "Create connect sock failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else std::cout << "Create connect sock OK" << std::endl;

	// Connect 
	result = connect(g_sock, res->ai_addr, (int)res->ai_addrlen);
	if (result != 0) {
		std::cout << "connect failed: " << WSAGetLastError() << std::endl;
		closesocket(g_sock);
		WSACleanup();
		return 1;
	}
	else std::cout << "connect OK" << std::endl;

	freeaddrinfo(res);
	std::cout << std::endl << std::endl;
	/*
		Done 
	*/

	// Get user name and send it to server
	std::cout << "What is your nickname? "; std::getline(std::cin, g_name);
	int name_size = g_name.size();
	send(g_sock, (char*)&name_size, sizeof(int), 0);
	send(g_sock, g_name.c_str(), name_size, 0);


	// Send thread
	std::thread send_thread(send0, g_sock);
	send_thread.detach();

	// Receive thread
	std::thread receive_thread(receive, g_sock);
	receive_thread.join();

	// Cleanup
	closesocket(g_sock);
	WSACleanup();
	return 0;
}
