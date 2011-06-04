#include <es_system.h>

namespace
{
	bool			want_to_die = false;
};

void				SetExit					()
{
	want_to_die = true;
}

void				ESSUB_Init				()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
}

void				ESSUB_Quit				()
{
	SDL_Quit();
}

ESThreads*			ESSUB_MakeThreads		()
{
	return new SDLThreads();
}

bool				ESSUB_WantToDie			()
{
	return want_to_die;
}

volatile bool		ESSUB_WantToSleep		()
{
	return false;
}

std::string			ESSUB_GetBaseDirectory	()
{
#ifndef __WIN32__
	return std::string(getenv("HOME")) + "/.mednafen/";
#else
	char path[MAX_PATH];
	SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, path);
	return std::string(path) + "/mednafen/";
#endif
}

