#include "Server.h"

Server::Server(int PORT)
{
	WSADATA wsaData;

	ListenSocket = INVALID_SOCKET;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(0);
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, std::to_string(PORT).c_str(), &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(0);
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(0);
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
	}

	freeaddrinfo(result);

	//listen for incoming connection
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
	}

	serverPtr = this;
}

bool Server::ListenForNewConnection()
{
	SOCKET ClientSocket = INVALID_SOCKET;
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}
	else // sucessfull connection
	{
		
			
			cout << "Client connected" << std::endl;
			Connections[ConnectionCounter] = ClientSocket;
			
			Getusername(ConnectionCounter);

			connectionThreads[ConnectionCounter] = std::thread(ClientHandler, ConnectionCounter);
			ConnectionCounter++;
			return true;
		
	}
}

bool Server::SendInt(int id, int value)
{
	int returnCheck = send(Connections[id], (char*)&value, sizeof(int), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::GetInt(int id, int& value)
{
	int returnCheck = recv(Connections[id], (char*)&value, sizeof(int), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::SendBool(int id, bool value)
{
	int returnCheck = send(Connections[id], (char*)&value, sizeof(bool), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::GetBool(int id, bool& value)
{
	int returnCheck = recv(Connections[id], (char*)&value, sizeof(bool), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::SendPacketType(int id, const PACKET& packetType)
{
	int returnCheck = send(Connections[id], (char*)&packetType, sizeof(PACKET), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::GetPacketType(int id, PACKET& packetType)
{
	int returnCheck = recv(Connections[id], (char*)&packetType, sizeof(PACKET), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::SendString(int id, const std::string& value)
{
	if (!SendPacketType(id, P_ChatMessage))
		return false;

	int bufferLength = value.size();
	if (!SendInt(id, bufferLength))
		return false;

	int returnCheck = send(Connections[id], value.c_str(), bufferLength, NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}
bool Server::SendLogIn(int id, const std::string& value)
{
	if (!SendPacketType(id, P_logIn))
		return false;

	int bufferLength = value.size();
	if (!SendInt(id, bufferLength))
		return false;

	int returnCheck = send(Connections[id], value.c_str(), bufferLength, NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::GetString(int id, std::string& value)
{
	int bufferLength = 0;
	if (!GetInt(id, bufferLength))
		return false;

	char* buffer = new char[bufferLength + 1]; // +1 tro allow for terminating '/0'

	int returnCheck = recv(Connections[id], buffer, bufferLength, NULL);
	buffer[bufferLength] = '\0';
	value = buffer;
	delete[] buffer;

	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::ProcessPacket(int index, PACKET packetType)
{
	switch (packetType)
	{
	case P_ChatMessage:
	{
		std::string message;
		if (!GetString(index, message))
			return false;
		for (int i = 0; i < ConnectionCounter; i++)
		{
			if (i == index)
				continue;
			//Add user to start of message
			string newMessage = users[index].username + ": " + message;
			if (!SendString(i, newMessage))
				std::cout << "Failed to send message from " << index << " to " << i << std::endl;
		}

		std::cout << "Processed messages for user. ID = " << index << std::endl;
		break;
	}

	case P_DirectMessage:
	{
		std::cout << "DM Message" << std::endl;
		std::string user;
		std::string message;

		std::string value;

		int usernameIndex = -1;
		bool userExists = false;

		//get user
		if (!GetString(index, value))
			return false;

		int val = 0;
		//get desired user
		while (value[val] != ' ')
		{
			user += value[val];
			val++;
		}

		//Check if user Exists
		for (int i = 0; i < ConnectionCounter; i++)
		{
			if (users[i].username == user)
			{
				userExists = true;
				usernameIndex = i;
				break;
			}
		}

		if (userExists)
		{
			//get message
			for (int i = val; i < value.size(); i++)
			{
				message += value[i];
			}
		}

		SendPacketType(index, P_DirectMessage);
		SendBool(index, userExists);

		if (userExists)
		{
			std::string fullMessage = "PM from " + users[index].username + ": " + message;

			SendString(usernameIndex, fullMessage);
		}

		break;
	}

	default:
		std::cout << "Unrecognized packet: " << packetType << std::endl;
		break;
	}
	return true;
}

bool Server::CloseConnection(int index)
{
	if (closesocket(Connections[index]) == SOCKET_ERROR)
	{
		std::cout << "Failed closing Error: " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

void Server::Getusername(int index)
{
	user temp;
	temp.username = "";
	temp.pass = "";
	PACKET packetType;
	serverPtr->GetPacketType(index, packetType);
	switch (packetType)
	{
	case P_ChatMessage :
	{
		bool usernameExist = false;
		do {
			string username;
			serverPtr->GetString(index, username);
			usernameExist = false;
			for (int i = 0; i < serverPtr->users.size(); i++)
			{
				if (serverPtr->users[i].username == username)
				{
					usernameExist = true;
				}
			}
			if (!usernameExist)
			{
				temp.username = username;
				std::cout << "Username " << username << " stored at " << serverPtr->users.size() << endl;
			}
			serverPtr->SendBool(index, !usernameExist);
			if (usernameExist)
			{
				serverPtr->GetPacketType(index, packetType);
			}
			
			
		} while (usernameExist);
		
		
		if (!usernameExist)
		{
			if ((!serverPtr->GetPacketType(index, packetType)) || !packetType == P_ChatMessage)
			{
				serverPtr->SendBool(index, false);
				cout << "Getting pass is not a message Package" << std::endl;
			}
			else
			{
				string pass;
				serverPtr->GetString(index, pass);

				temp.pass = pass;
				temp.connectingAt = index;
				cout << "pass stored at " << index << endl;
				serverPtr->users.push_back(temp);

				serverPtr->SendBool(index, true);
			}
		}break;

	}
	case P_logIn:
	{
		bool usernameExist = false;
		int pos;
		do {
			
			string username;
			serverPtr->GetString(index, username);
			usernameExist = false;
			for (int i = 0; i < serverPtr->users.size(); i++)
			{
				if (serverPtr->users[i].username == username)
				{
					usernameExist = true;
					pos = i;
				}
			}
			serverPtr->SendBool(index, usernameExist);
			if (!usernameExist)
			{
				serverPtr->GetPacketType(index, packetType);
			}
		} while (!usernameExist);


		if (usernameExist)
		{
			if ((!serverPtr->GetPacketType(index, packetType)) || !packetType == P_logIn)
			{
				serverPtr->SendBool(index, false);
				cout << "Getting pass is not a message Package" << std::endl;
			}
			else
			{
				bool truePass = false;
				do
				{
					truePass = false;
					string pass;
					serverPtr->GetString(index, pass);
					if (serverPtr->users[pos].pass == pass)
					{
						truePass = true;
						cout << "Client at " << index << " logIn successfully!" << endl;
						serverPtr->users[pos].connectingAt = index;
					}
					serverPtr->SendBool(index, truePass);
					if (!truePass)
					{
						serverPtr->GetPacketType(index, packetType);
						
					}
				} while (!truePass);
			}
		}
	}
	}
	
}

//Bulk of work
void Server::ClientHandler(int index)
{
	PACKET packetType;
	while (true)
	{
		//Receive Messages
		if (!serverPtr->GetPacketType(index, packetType))
			break;
		if (!serverPtr->ProcessPacket(index, packetType))
			break;
	}

	std::cout << "Lost contact with client: id = " << index << std::endl;
	serverPtr->CloseConnection(index);
}