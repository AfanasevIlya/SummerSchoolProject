#pragma comment(lib, "Ws2_32.lib")
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

SOCKET sock;
string name;
const int COMMAND_COUNT = 2;
const char* commands[] = { "exit", "set.new_name" }; //keep local command, we can also add smthg new




// Exit function
void EXIT() { 
	char ask;
	bool loop = true;
	while (loop) {
		std::cout << "Do you want to exit?" << std::endl << "Y/N?" << std::endl;
		std::cin >> ask;
		switch (ask) {
		case 'y': closesocket(sock); WSACleanup(); exit(1);
		case 'Y': closesocket(sock); WSACleanup(); exit(1);
		case 'n': loop = false; break;
		case 'N': loop = false; break;
		default: continue;
		}
	}
}

void NEW_N() {  
	cout << "Input new name: ";
	getline(cin, name);
}

// Menu ( compare input of user and local commans
bool Menu(const char* command) {  
	for (int i = 0; i < COMMAND_COUNT; ++i) {
		if (strcmp(command, commands[i]) == 0) {
			switch (i) {
			case 0: EXIT(); return true;
			case 1: NEW_N(); return true;
			}
		}
	}
	return false;
}

// Send thread function
int send0(SOCKET s) {
	string buf;
	do {
		cout << name << " : "; getline(cin, buf);
		if (Menu(buf.c_str())) continue; 
		if (buf.length() > 0) {
			buf = name + ": " + buf;
			if (send(s, buf.c_str(), strlen(buf.c_str()), 0) == SOCKET_ERROR) { // Check for sock errors
				cout << "send failed: " << WSAGetLastError() << endl; 
				closesocket(s);
				WSACleanup();
				getchar();
				return 1;
			}
		}
	} while (buf.length() > 0);
	return 0;
}

// Receive thread function
int receive(SOCKET s) {
	while (true) {
		char recvbuf[4096];
		ZeroMemory(recvbuf, 4096);
		int result = recv(s, recvbuf, 4096, 0);
		if (result > 0) {
			cout << '\r';
			cout << recvbuf << endl;
			cout << name << ": ";
		}
		else if (result == 0) {
			cout << "Connection closed" << endl;
			return 1;
		}
		else {
			cout << "recv failed: " << WSAGetLastError() << endl;
			closesocket(s);
			WSACleanup();
			getchar();
			return 2;
		}
	}
}

int main() {
	//Initialize Winsock
	int result;	
	WSADATA wsaData;
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		cout << "Initialize Winsock Fail: " << result << endl;
		getchar();
		return 1;
	}
	else cout << "Initialize Winsock OK" << endl;

	// Resolve the server address and port
	addrinfo hints, * res;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	string host; cout << "Host: "; getline(cin, host); 
	string port; cout << "Port: "; getline(cin, port);
	result = getaddrinfo(host.c_str(), port.c_str(), &hints, &res); // Check inputed host & port
	if (result != 0) {
		cout << "getaddrinfo failed: " << result << endl;
		getchar();
		return 1;
	}
	else cout << "getaddrinfo OK" << endl;

	// Create connecting Sock
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock == INVALID_SOCKET) {
		cout << "Create connect sock failed: " << WSAGetLastError() << endl;
		WSACleanup();
		getchar();
		return 1;
	}
	else cout << "Create connect sock OK" << endl;

	// Connect 
	result = connect(sock, res->ai_addr, (int)res->ai_addrlen);
	if (result != 0) {
		cout << "connect failed: " << WSAGetLastError() << endl;
		getchar();
		closesocket(sock);
		WSACleanup();
		return 1;
	}
	else cout << "connect OK" << endl;

	freeaddrinfo(res);
	cout << endl << endl;
	/*
		Done 
	*/


	cout << "What is your nickname? "; getline(cin, name);
	send(sock, name.c_str(), strlen(name.c_str()), 0);


	// Send thread
	thread send_thread(send0, sock);
	send_thread.detach();

	// Receive thread
	thread receive_thread(receive, sock);
	receive_thread.join();

	// Cleanup
	closesocket(sock);
	WSACleanup();
	return 0;
}
