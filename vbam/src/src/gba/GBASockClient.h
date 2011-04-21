#pragma once

//ROBO: No Link support
//#include <SFML/Network.hpp>
#include "../common/Types.h"

//ROBO: No Link support
/*
class GBASockClient : public sf::SocketTCP
{
public:
	GBASockClient(sf::IPAddress server_addr);
	~GBASockClient();

	void Send(std::vector<char> data);
	char ReceiveCmd(char* data_in);

private:
	sf::IPAddress server_addr;
	sf::SocketTCP client;
};*/
class GBASockClient
{
public:
	GBASockClient(uint32_t server_addr);
	~GBASockClient();

	void Send(std::vector<char> data);
	char ReceiveCmd(char* data_in);

private:
	uint32_t server_addr;
	uint32_t client;
};
