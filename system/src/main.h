#pragma once

//ES Interfaces with happy macro
#define DEC_AND_DEF(a,b)	class a; extern a* b;

DEC_AND_DEF(ESThreads, es_threads);
DEC_AND_DEF(ESVideo, es_video);
DEC_AND_DEF(ESAudio, es_audio);
DEC_AND_DEF(ESInput, es_input);
DEC_AND_DEF(ESNetwork, es_network);
DEC_AND_DEF(PathBuild, es_paths);
DEC_AND_DEF(Logger, es_log);

#undef DEC_AND_DEF

//Happy functions
void					InitES					(void (*aExitFunction)() = 0);
void					QuitES					();
volatile bool			WantToDie				();
volatile bool			WantToSleep				();
void					Abort					(const char* aMessage);
void					ESSUB_Error				(const char* aMessage);
std::string				ESSUB_GetString			(const std::string& aHeader, const std::string& aMessage);
