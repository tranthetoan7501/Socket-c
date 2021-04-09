#include "Client.h"



// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

Client* myClient;

#define DEFAULT_PORT 27015

void SecureUsername()
{
	// Make client have a username
	bool usernameAccepted = false;
	do
	{
		cout << "Please provide a username" << std::endl;
		string username;
		getline(cin, username);
		myClient->SendString(username);

		myClient->GetBool(usernameAccepted);

		if (!usernameAccepted)
			std::cout << "Username Taken, Try again" << std::endl;

	} while (!usernameAccepted);

	bool passAccepted = false;
	do
	{
		std::cout << "Please provide a pass" << std::endl;
		std::string pass;
		std::getline(std::cin, pass);
		myClient->SendString(pass);

		myClient->GetBool(passAccepted);

		if (!passAccepted)
			cout << "pass Taken, Try again" << std::endl;

	} while (!passAccepted);
}

void loginUser()
{
	bool usernameAccepted = false;
	do
	{
		std::cout << "Please provide a username" << std::endl;
		std::string username;
		std::getline(std::cin, username);
		myClient->logIn(username);

		myClient->GetBool(usernameAccepted);

		if (!usernameAccepted)
			std::cout << "Username not exist, Try again" << std::endl;

	} while (!usernameAccepted);

	bool passAccepted = false;
	do
	{
		std::cout << "Please provide a pass" << std::endl;
		std::string pass;
		std::getline(std::cin, pass);
		myClient->logIn(pass);

		myClient->GetBool(passAccepted);

		if (!passAccepted)
		{
			cout << "Wrong Pass, Try again" << std::endl;
		}
		else
		{
			cout <<endl<< "LogIn successfully!";
		}

	} while (!passAccepted);
}
int main(int argc, char** argv)
{
	myClient = new Client(argc, argv, DEFAULT_PORT);

	if (!myClient->Connect())
	{
		system("pause");
		return 1;
	}
	int choosen;
	cout << "1.logup/2.login\n";
	cin >> choosen;
	cin.ignore();
	if (choosen == 1)
	{
		SecureUsername();
	}
	if (choosen == 2)
	{
		loginUser();
	}

	myClient->StartSubRoutine();

	//Receive and create messages
	std::string buffer;
	while (true)
	{
		std::getline(std::cin, buffer); // get line from cmd console

		if (buffer[0] == '@')
		{
			buffer.erase(0, 1);
			if (!myClient->SendDirectMessage(buffer))
				break;
		}
		else
		{
			if (!myClient->SendString(buffer))
				break; // leave as server conn lost
		}
	}

	// cleanup
	system("pause");

	WSACleanup();

	system("pause");

	return 0;
}