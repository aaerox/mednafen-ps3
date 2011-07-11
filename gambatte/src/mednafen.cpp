#include <src/mednafen.h>
#include <src/git.h>

#include <sstream>

#include "gambatte.h"
#include "resample/resamplerinfo.h"

using namespace Gambatte;

namespace mdfngmbt
{
	GB*						GambatteEmu;
	Resampler*				Resample;
	
	uint32_t				Samples[48000];
	uint32_t				Resamples[48000];
	int32_t					SampleOverflow;

	EmulateSpecStruct*		ESpec;
	uint8_t*				InputPort;

	//Class for processing video data
	class gbblitter : public VideoBlitter
	{
		public:
								gbblitter				()			{buffer.pixels = 0;}
								~gbblitter				()			{delete[] (uint32_t*)buffer.pixels;}
			const PixelBuffer	inBuffer				()			{return buffer;}

			void				setBufferDimensions		(unsigned aWidth, unsigned aHeight)
			{
				delete[] (uint32_t*)buffer.pixels;
				buffer.pixels = new uint32_t[aWidth * aHeight];

				width = aWidth;
				height = aHeight;
				buffer.pitch = aWidth;
			}

			void				blit					()
			{
				uint32_t* sourceptr = (uint32_t*)buffer.pixels;

				for(int i = 0; i != height; i ++)
				{
					for(int j = 0; j != width ; j ++)
					{
						ESpec->surface->pixels[i * ESpec->surface->pitch32 + j] = sourceptr[i * width + j];
					}
				}
			}

		protected:
			uint32_t			width, height;
			PixelBuffer			buffer;
	};
	gbblitter					Blitter;


	//Class for feeded gambatte input data
	class gbinput : public InputStateGetter
	{
		public:
			const InputState& 	operator()			()	{return inputs;};
			InputState			inputs;
	};
	gbinput						Input;
}
using namespace	mdfngmbt;

//Implement MDFNGI:
int				GmbtLoad				(const char *name, MDFNFILE *fp)
{
	//Create gambatte objects
	GambatteEmu = new GB();
	Resample = ResamplerInfo::get(0).create(2097152, 48000, 35112);

	//Init sound values
	SampleOverflow = 0;
	memset(Samples, 0, sizeof(Samples));
	memset(Resamples, 0, sizeof(Resamples));

	//Load the file into gambatte
	std::istringstream file(std::string((const char*)fp->data, (size_t)fp->size), std::ios_base::in | std::ios_base::binary);	
	if(GambatteEmu->load(file, MDFN_GetSettingB("gmbt.forcedmg")))
	{
		delete Resample;
		delete GambatteEmu;

		MDFND_PrintError("gambatte: Failed to load ROM");
		throw 0;
	}

	//Give gambatte it's objects
	GambatteEmu->setVideoBlitter(&Blitter);
	GambatteEmu->setInputStateGetter(&Input);

	//Done
	return 1;
}

bool			GmbtTestMagic			(const char *name, MDFNFILE *fp)
{
	static const uint8 GBMagic[8] = { 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B };
	return fp->size > 0x10C && !memcmp(fp->data + 0x104, GBMagic, 8);
}

void			GmbtCloseGame			(void)
{
	delete Resample;
	delete GambatteEmu;
	GambatteEmu = 0;
}

int				GmbtStateAction			(StateMem *sm, int load, int data_only)
{
	//TODO:
	if(!load)
	{
		std::ostringstream os(std::ios_base::out | std::ios_base::binary);
		GambatteEmu->saveState(os);

		void* buffer = malloc(os.str().size());
		memcpy(buffer, os.str().data(), os.str().size());

		smem_write32le(sm, os.str().size());
		smem_write(sm, buffer, os.str().size());

		free(buffer);

		return 1;
	}
	else
	{
		uint32_t size;
		smem_read32le(sm, &size);

		char* buffer = (char*)malloc(size);
		smem_read(sm, buffer, size);

		std::istringstream iss(std::string((const char*)buffer, (size_t)size), std::ios_base::in | std::ios_base::binary);
		GambatteEmu->loadState(iss);

		free(buffer);

		SampleOverflow = 0;

		return 1;
	}
	return 0;
}

void			GmbtEmulate				(EmulateSpecStruct *espec)
{
	ESpec = espec;

	//SOUND PREP
	if(ESpec->SoundFormatChanged)
	{
		Resample->adjustRate(2097152, (ESpec->SoundRate > 1.0) ? ESpec->SoundRate : 22050);
	}

	//INPUT
	if(InputPort[0])
	{
		Input.inputs.startButton	= (*InputPort & 8) ? 1 : 0;
		Input.inputs.selectButton	= (*InputPort & 4) ? 1 : 0;
		Input.inputs.bButton		= (*InputPort & 2) ? 1 : 0;
		Input.inputs.aButton		= (*InputPort & 1) ? 1 : 0;
		Input.inputs.dpadUp			= (*InputPort & 0x40) ? 1 : 0;
		Input.inputs.dpadDown		= (*InputPort & 0x80) ? 1 : 0;
		Input.inputs.dpadLeft		= (*InputPort & 0x20) ? 1 : 0;
		Input.inputs.dpadRight		= (*InputPort & 0x10) ? 1 : 0;
	}

	//EXECUTE
	uint32_t samps = GambatteEmu->runFor((Gambatte::uint_least32_t*)Samples, 35112 - SampleOverflow);
	SampleOverflow += samps;
	SampleOverflow -= 35112;

	//Grab sound
	uint32_t count = Resample->resample((short*)Resamples, (short*)Samples, samps);

	if(espec->SoundBuf && (espec->SoundBufMaxSize >= count))
	{
		espec->SoundBufSize = count;
		memcpy(espec->SoundBuf, Resamples, espec->SoundBufSize * 4);
	}

	//Set frame size
	espec->DisplayRect.x = 0;
	espec->DisplayRect.y = 0;
	espec->DisplayRect.w = 160;
	espec->DisplayRect.h = 144;

	//TODO: Real timing
	espec->MasterCycles = 1LL * 100;
}

void			GmbtSetInput			(int port, const char *type, void *ptr)
{
	if(port == 0)
	{
		InputPort = (uint8_t*)ptr;
	}
}

void			GmbtDoSimpleCommand		(int cmd)
{
	if(cmd == MDFN_MSC_RESET || cmd == MDFN_MSC_POWER)
	{
		GambatteEmu->reset();
	}
}

static const InputDeviceInputInfoStruct IDII[] =
{
	{"a",		"A",		7, IDIT_BUTTON_CAN_RAPID,	NULL},
	{"b",		"B",		6, IDIT_BUTTON_CAN_RAPID,	NULL},
	{"select",	"SELECT",	4, IDIT_BUTTON,				NULL},
	{"start",	"START",	5, IDIT_BUTTON,				NULL},
	{"right",	"RIGHT",	3, IDIT_BUTTON,				"left"},
	{"left",	"LEFT",		2, IDIT_BUTTON,				"right"},
	{"up",		"UP",		0, IDIT_BUTTON,				"down"},
	{"down",	"DOWN",		1, IDIT_BUTTON,				"up"},
};

static InputDeviceInfoStruct InputDeviceInfo[] =	{{"gamepad", "Gamepad", NULL, 8, IDII,}};
static const InputPortInfoStruct PortInfo[] =		{{0, "builtin", "Built-In", 1, InputDeviceInfo, "gamepad"}};
static InputInfoStruct GmbtInput =					{1, PortInfo};

static FileExtensionSpecStruct	extensions[] = 
{
	{".gb",		"Game Boy Rom"},
	{".gbc",	"Game Boy Color Rom"},
	{".cgb",	"Game Boy Color Rom"},
	{0,			0}
};


static MDFNSetting GmbtSettings[] =
{
	{"gmbt.forcedmg",	MDFNSF_NOFLAGS,	"Force GB Mono Mode.",	NULL, MDFNST_BOOL,	"0"},
	{0}
};


static MDFNGI	GmbtInfo = 
{
/*	shortname:*/		"gmbt",
/*	fullname:*/			"Game Boy Color (Gambatte)",
/*	FileExtensions:*/	extensions,
/*	ModulePriority:*/	MODPRIO_EXTERNAL_HIGH,
/*	Debugger:*/			0,
/*	InputInfo:*/		&GmbtInput,

/*	Load:*/				GmbtLoad,
/*	TestMagic:*/		GmbtTestMagic,
/*	LoadCD:*/			0,
/*	TestMagicCD:*/		0,
/*	CloseGame:*/		GmbtCloseGame,
/*	ToggleLayer:*/		0,
/*	LayerNames:*/		0,
/*	InstallReadPatch:*/	0,
/*	RemoveReadPatches:*/0,
/*	MemRead:*/			0,
/*	StateAction:*/		GmbtStateAction,
/*	Emulate:*/			GmbtEmulate,
/*	SetInput:*/			GmbtSetInput,
/*	DoSimpleCommand:*/	GmbtDoSimpleCommand,
/*	Settings:*/			GmbtSettings,
/*	MasterClock:*/		MDFN_MASTERCLOCK_FIXED(6000),
/*	fps:*/				0,
/*	multires:*/			false,
/*	lcm_width:*/		160,
/*	lcm_height:*/		144,
/*	dummy_separator:*/	0,
/*	nominal_width:*/	160,
/*	nominal_height:*/	144,
/*	fb_width:*/			160,
/*	fb_height:*/		144,
/*	soundchan:*/		2
};

#ifdef MLDLL
#define VERSION_FUNC GetVersion
#define GETEMU_FUNC GetEmulator
#ifdef __WIN32__
#define DLL_PUBLIC __attribute__((dllexport))
#else
#define DLL_PUBLIC __attribute__ ((visibility("default")))
#endif
#else
#define VERSION_FUNC gmbtGetVersion
#define GETEMU_FUNC gmbtGetEmulator
#define	DLL_PUBLIC
#endif

extern "C" DLL_PUBLIC	uint32_t		VERSION_FUNC()
{
	return 0x916;
//	return MEDNAFEN_VERSION_NUMERIC;
}
	
extern "C" DLL_PUBLIC	MDFNGI*			GETEMU_FUNC()
{
	return &GmbtInfo;
}

