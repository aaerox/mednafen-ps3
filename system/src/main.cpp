#include <ps3_system.h>

Logger*				es_log = 0;
ESVideo*			es_video = 0;
ESAudio*			es_audio = 0;
ESInput*			es_input = 0;
PathBuild*			es_paths = 0;

namespace
{
	volatile bool	want_to_die = false;
	void			(*ExitFunction)() = 0;
};

void				ESSUB_Init				();
void				ESSUB_Quit				();
ESVideo*			ESSUB_MakeVideo			();
ESAudio*			ESSUB_MakeAudio			();
ESInput*			ESSUB_MakeInput			();
bool				ESSUB_WantToDie			();
bool				ESSUB_WantToSleep		();
std::string			ESSUB_GetBaseDirectory	();

void				Abort					(const char* aMessage)
{
	printf("ABORT: %s\n", aMessage);
	
	if(ExitFunction)
	{
		ExitFunction();
	}
	
	abort();
}

void				InitES					(void (*aExitFunction)())
{
	ExitFunction = aExitFunction;

	ESSUB_Init();

	es_paths = new PathBuild(ESSUB_GetBaseDirectory());

	es_video = ESSUB_MakeVideo();
	es_audio = ESSUB_MakeAudio();
	es_input = ESSUB_MakeInput();

	FontManager::InitFonts();

	es_log = new Logger();
}

void				QuitES					()
{
	delete es_log;

	FontManager::QuitFonts();

	delete es_input;
	delete es_audio;
	delete es_video;

	ESSUB_Quit();
	
	delete es_paths;
}

volatile bool		WantToDie				()
{
	want_to_die = ESSUB_WantToDie();

	if(want_to_die)
	{
		ExitFunction();
		exit(1);
	}
	
	return want_to_die;
}

volatile bool		WantToSleep				()
{
	return ESSUB_WantToSleep();
}
