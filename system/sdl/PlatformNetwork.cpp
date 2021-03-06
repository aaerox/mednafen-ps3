#include <es_system.h>
#include "src/ESException.h"
#include <SDL/SDL_net.h>

struct						ESPlatformSocketPrivate
{
	TCPsocket				Socket;
	IPaddress				Connection;
};

							ESSocket::ESSocket				(const char* aHost, uint32_t aPort) : Data(new ESPlatformSocketPrivate)
{
	ESException::ErrorCheck(-1 == SDLNet_ResolveHost(&Data->Connection, aHost, aPort), "SDL_net failed to resolve host: %s", SDLNet_GetError());

	Data->Socket = SDLNet_TCP_Open(&Data->Connection);
	ESException::ErrorCheck(Data->Socket != 0, "SDL_net failed to open connection: %s", SDLNet_GetError());
}

							ESSocket::~ESSocket				()
{
	SDLNet_TCP_Close(Data->Socket);

	delete Data;
}

uint32_t					ESSocket::ReadString			(void* aBuffer, uint32_t aLength)
{
	uint8_t* buff = (uint8_t*)aBuffer;

	for(int i = 0; i != aLength; i ++)
	{
		int count = SDLNet_TCP_Recv(Data->Socket, &buff[i], 1);

		if(0 == count || buff[i] == 0x0A)
		{
			return i;
		}

		ESException::ErrorCheck(count >= 0, "SDL_net failed to read socket: %s", SDLNet_GetError());
	}

	return aLength;
}

uint32_t					ESSocket::Read					(void* aBuffer, uint32_t aLength)
{
	uint8_t* buff = (uint8_t*)aBuffer;

	int count = SDLNet_TCP_Recv(Data->Socket, aBuffer, aLength);

	ESException::ErrorCheck(count >= 0, "SDL_net failed to read socket: %s", SDLNet_GetError());

	return count;
}

void						ESSocket::Write					(const void* aBuffer, uint32_t aLength)
{
	ESException::ErrorCheck(aLength == SDLNet_TCP_Send(Data->Socket, aBuffer, aLength), "SDL_net failed to write socket: %s", SDLNet_GetError());
}

void						ESNetwork::Initialize			()
{
	ESException::ErrorCheck(-1 != SDLNet_Init(), "SDL_net failed to initialize: %s", SDLNet_GetError());
}

void						ESNetwork::Shutdown				()
{
	SDLNet_Quit();
}

ESSocket*					ESNetwork::OpenSocket			(const char* aHost, uint32_t aPort)
{
	return new ESSocket(aHost, aPort);
}

