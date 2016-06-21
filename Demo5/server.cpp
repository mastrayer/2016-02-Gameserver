#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

const int SERVER_PORT = 19999;
const int BUFFER_SIZE = 512;

struct SOCKET_DATA {
	SOCKET socket;
	char buffer[BUFFER_SIZE+1];
	int recv_bytes;
	int send_bytes;
	bool isDestroyed;
};

BOOL AddSocket(std::vector<SOCKET_DATA> &list, SOCKET socket)
{
	if (list.size() >= FD_SETSIZE)
		return FALSE;

	SOCKET_DATA item;
	item.socket = socket;
	item.isDestroyed = false;
	item.recv_bytes = item.send_bytes = 0;

	list.push_back(item);

	return TRUE;
}
void RemoveSocket(std::vector<SOCKET_DATA> &list, int index)
{
	SOCKADDR_IN client_address;
	int address_length = sizeof(client_address);

	getpeername(list[index].socket, (SOCKADDR *)&client_address, &address_length);

	char clientIP[32] = { 0, };
	inet_ntop(AF_INET, &(client_address.sin_addr), clientIP, 32 - 1);

	std::cout
		<< "DISCONNECTED [ "
		<< clientIP << ":" << ntohs(client_address.sin_port) << "]"
		<< std::endl;

	closesocket(list[index].socket);
	list[index].isDestroyed = true;
}

int main() 
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
		return 1;

	SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == INVALID_SOCKET)
		return 1;

	SOCKADDR_IN server_address;
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(SERVER_PORT);

	if (bind(listen_socket, (SOCKADDR *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
		return 1;

	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR)
		return 1;

	u_long on = 1;
	if (ioctlsocket(listen_socket, FIONBIO, &on) == SOCKET_ERROR)
		return 1;


	std::vector<SOCKET_DATA> sockets;
	FD_SET read_set, write_set;
	SOCKET client_socket;
	SOCKADDR_IN client_address;
	int address_length;

	while (1)
	{
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		FD_SET(listen_socket, &read_set);

		for (int i=0; i<sockets.size(); ++i)
		{
			if (sockets[i].recv_bytes > sockets[i].send_bytes)
				FD_SET(sockets[i].socket, &write_set);
			else
				FD_SET(sockets[i].socket, &read_set);
		}

		if (select(0, &read_set, &write_set, NULL, NULL) == SOCKET_ERROR)
			return 1;

		if (FD_ISSET(listen_socket, &read_set))
		{
			address_length = sizeof(client_address);
			client_socket = accept(listen_socket, (SOCKADDR *)&client_address, &address_length);

			if (client_socket == INVALID_SOCKET)
				return 1;
			else
			{
				char clientIP[32] = { 0, };
				inet_ntop(AF_INET, &(client_address.sin_addr), clientIP, 32 - 1);
				std::cout 
					<< "CONNECT [" 
					<< clientIP << ":" << ntohs(client_address.sin_port) << "]" 
					<< std::endl;

				AddSocket(sockets, client_socket);
			}
		}

		int i = 0;
		for(auto &iter : sockets)
		{
			if (FD_ISSET(iter.socket, &read_set))
			{
				int read = recv(iter.socket, iter.buffer, BUFFER_SIZE, 0);

				if (read == SOCKET_ERROR || read == 0) {
					RemoveSocket(sockets, i);
					continue;
				}

				iter.recv_bytes = read;	
				iter.buffer[read] = '\0';

				address_length = sizeof(client_address);
				getpeername(iter.socket, (SOCKADDR *)&client_address, &address_length);

				char clientIP[32] = { 0, };
				inet_ntop(AF_INET, &(client_address.sin_addr), clientIP, 32 - 1);

				std::cout
					<< "Receive ["
					<< clientIP << ":" << ntohs(client_address.sin_port) << "] "
					<< iter.buffer
					<< std::endl;
			}

			if (FD_ISSET(iter.socket, &write_set))
			{
				int ret = send(iter.socket, iter.buffer + iter.send_bytes, iter.recv_bytes - iter.send_bytes, 0);

				if (ret == SOCKET_ERROR) {
					RemoveSocket(sockets, i);
					continue;
				}

				iter.send_bytes += ret;

				if (iter.recv_bytes == iter.send_bytes) {
					iter.recv_bytes = iter.send_bytes = 0;
				}
			}

			++i;
		}

		for (int i = 0; i < sockets.size(); ++i) {
			if (sockets[i].isDestroyed)
				sockets.erase(sockets.begin() + i);
		}
	}

	// 윈속 종료
	WSACleanup();
	return 0;
}