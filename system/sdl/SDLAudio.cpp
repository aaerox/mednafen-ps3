#include <es_system.h>

						SDLAudio::SDLAudio				()
{
	SDL_AudioSpec spec;
	spec.freq = 48000;
	spec.format = AUDIO_S16;
	spec.channels = 2;
	spec.samples = 2048;
	spec.callback = ProcessAudioCallback;

	SDL_OpenAudio(&spec, &Format);
	SDL_PauseAudio(0);
}


						SDLAudio::~SDLAudio				()
{
	SDL_CloseAudio();
}

void					SDLAudio::AddSamples			(const uint32_t* aSamples, uint32_t aCount)
{
	SDL_LockAudio();
	Buffer.WriteData(aSamples, aCount);
	SDL_UnlockAudio();
}

void					SDLAudio::ProcessAudioCallback	(void *userdata, Uint8 *stream, int len)
{
	if(es_audio)
	{
		((SDLAudio*)es_audio)->Buffer.ReadDataSilentUnderrun((uint32_t*)stream, len / 4);
	}
}
