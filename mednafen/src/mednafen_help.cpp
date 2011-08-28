#include <mednafen_includes.h>
#include "savestates.h"
#include "cheatmenu.h"
#include "cheatsearcher.h"
#include "ers.h"
#include "inputhandler.h"
#include "mednafen_help.h"
#include "settingmenu.h"
#include "textviewer.h"
#include "FastCounter.h"

#include "src/utility/TextViewer.h"	//TODO: Cleaner path for es includes

namespace
{
	//Emulator loaders
	extern "C" MDFNGI*				nestGetEmulator			(uint32_t aIndex);
	extern "C" MDFNGI*				gmbtGetEmulator			(uint32_t aIndex);
	extern "C" MDFNGI*				vbamGetEmulator			(uint32_t aIndex);
	extern "C" MDFNGI*				pcsxGetEmulator			(uint32_t aIndex);
	extern "C" MDFNGI*				stellaGetEmulator		(uint32_t aIndex);
	extern "C" MDFNGI*				desmumeGetEmulator		(uint32_t aIndex);		//Disalbed 
	extern "C" MDFNGI*				lsnesGetEmulator		(uint32_t aIndex);		//Disabled because it requires a user provided build of libsnes and conflicts with the build in snes emulator
	extern "C" MDFNGI*				yabauseGetEmulator		(uint32_t aIndex);		//Disabled because the c68k emu conflicts with the built in megadrive emulator

	const MDFNSetting_EnumList	AspectEnumList[] =
	{
		{"auto", 0, "Autodetect (Broken)", ""},
		{"pillarbox", -1, "Pillarbox", ""},
		{"fullframe", 1, "Fill Screen", ""},
		{"1to1", 2, "Scale image with no correction", ""},
		{0, 0, 0, 0}
	};

	MDFNSetting_EnumList*		BuildShaderEnum					()
	{
		static MDFNSetting_EnumList* results = 0;
		int onresult = 1;

		if(results == 0)
		{
			std::vector<std::string> shaders;
			if(ESVideo::SupportsShaders() && Utility::ListDirectory(es_paths->Build("assets/presets"), shaders))
			{
				std::sort(shaders.begin(), shaders.end());

				results = new MDFNSetting_EnumList[shaders.size() + 2];
				results[0].string = "Standard";
				results[0].number = -1;
				results[0].description = "Standard";
				results[0].description_extra = "";

				for(std::vector<std::string>::iterator i = shaders.begin(); i != shaders.end(); i ++)
				{
					if((*i) != "." && (*i) != "..")
					{
						results[onresult].string = strdup(i->c_str());
						results[onresult].number = onresult;
						results[onresult].description = results[onresult].string;
						results[onresult++].description_extra = 0;
					}
				}

				memset(&results[onresult], 0, sizeof(results[0]));
			}
			else
			{
				results = new MDFNSetting_EnumList[2];
				results[0].string = "Standard";
				results[0].number = -1;
				results[0].description = "Standard";
				results[0].description_extra = "";
				memset(&results[1], 0, sizeof(results[1]));
			}
		}

		return results;
	}


	#define SETTINGNAME(b) ((std::string(GameInfo->shortname) + ".es." + b).c_str())

	MDFNSetting SystemSettings[] = 
	{
		{"display.fps", MDFNSF_NOFLAGS, "Display frames per second in corner of screen", NULL, MDFNST_BOOL, "0" },
		{"display.aspect", MDFNSF_NOFLAGS, "Override screen aspect correction", NULL, MDFNST_ENUM, "auto", NULL, NULL, NULL, NULL, AspectEnumList },
		{"display.underscanadjust", MDFNSF_NOFLAGS, "Value to add to underscan from General Settings.", NULL, MDFNST_INT, "0", "-50", "50" },
		{"autosave", MDFNSF_NOFLAGS, "Save state at exit", NULL, MDFNST_BOOL, "0"},
		{"speed.rewind", MDFNSF_NOFLAGS, "Enable Rewind Support", NULL, MDFNST_BOOL, "0"},
		{"speed.normalrate", MDFNSF_NOFLAGS, "Set speed multiplier for non fast forward mode.", NULL, MDFNST_UINT, "1", "1", "16" },
		{"speed.fastrate", MDFNSF_NOFLAGS, "Set speed multiplier for fast forward mode.", NULL, MDFNST_UINT, "4", "1", "16" },
		{"speed.toggle", MDFNSF_NOFLAGS, "Make the fast forward button a toggle.", NULL, MDFNST_BOOL, "0" },
		//HACK: 4294967051 = ES_BUTTON_AUXRIGHT2
		{"speed.button", MDFNSF_CAT_INPUT, "Button used for fast forward.", NULL, MDFNST_UINT, "4294967051" }
	};

	MDFNSetting ESSettings[] =
	{
		{"underscan", MDFNSF_NOFLAGS, "Reduce size of screen to compensate for display overscan.", NULL, MDFNST_INT, "5", "-50", "50" },
		{"es.bookmarks", MDFNSF_NOFLAGS, "Bookmarks for the file browser.", NULL, MDFNST_STRING, "" },
		{"net.es.username", MDFNSF_NOFLAGS, "User name for netplay.", NULL, MDFNST_STRING, "Me" },		
		{"net.es.password", MDFNSF_NOFLAGS, "Password for the netplay Server.", NULL, MDFNST_STRING, "sexybeef" },
		{"net.es.host", MDFNSF_NOFLAGS, "Hostname for netplay Server.", NULL, MDFNST_STRING, "192.168.0.115" },
		{"net.es.port", MDFNSF_NOFLAGS, "Port for netplay Server.", NULL, MDFNST_UINT, "4046" },
		{"net.es.gameid", MDFNSF_NOFLAGS, "Game ID for netplay Server.", NULL, MDFNST_STRING, "doing" },
	};
}

//TODO: Find this a new home
extern bool					NetplayOn;

void						MednafenEmu::Init				()
{
	if(!IsInitialized && Settings.size() == 0)
	{
		//TODO: Build directory trees, if these don't exist we can't save games

		//Get the external emulators
		std::vector<MDFNGI*> externalSystems;
		externalSystems.push_back(nestGetEmulator(0));
		externalSystems.push_back(gmbtGetEmulator(0));
		externalSystems.push_back(vbamGetEmulator(0));
		externalSystems.push_back(vbamGetEmulator(1));
		externalSystems.push_back(pcsxGetEmulator(0));
		externalSystems.push_back(stellaGetEmulator(0));
		externalSystems.push_back(yabauseGetEmulator(0));

#ifdef TEST_MODULES
		externalSystems.push_back(desmumeGetEmulator(0));
		externalSystems.push_back(lsnesGetEmulator(0));
#endif
		MDFNI_InitializeModules(externalSystems);

		//Put settings for the user interface
		for(int i = 0; i != sizeof(ESSettings) / sizeof(MDFNSetting); i++)
		{
			Settings.push_back(ESSettings[i]);
		}

		//Make settings for each system
		GenerateSettings(Settings);
		InputHandler::GenerateSettings(Settings);

		//Initialize mednafen and go
		MDFNI_Initialize(es_paths->Build("mednafen").c_str(), Settings);
		IsInitialized = true;
	}
}

void						MednafenEmu::Quit				()
{
	if(IsInitialized)
	{
		CloseGame();
		MDFNI_Kill();
	}
}

bool						MednafenEmu::LoadGame			(std::string aFileName, void* aData, int aSize)
{
	if(IsInitialized)
	{
		if(IsLoaded)
		{
			CloseGame();
		}

		//Load the game
		//TODO: Support other casing of '.cue'
		if(strstr(aFileName.c_str(), ".cue"))
		{
			GameInfo = MDFNI_LoadCD(0, aFileName.c_str());
		}
		else
		{
			GameInfo = MDFNI_LoadGame(0, aFileName.c_str(), aData, aSize);
		}

		//Display log on error!
		if(!GameInfo)
		{
			Summerface("Log", es_log, false).Do();
			free(aData);
			return false;
		}

		//HACK: Attach a default border
		if(ESVideo::SupportsShaders())
		{
			ESVideo::AttachBorder(ImageManager::GetImage("GameBorder"));
		}

		//HACK: Put this before IsLoaded = true to prevent crash on PS3
		Inputs = new InputHandler(GameInfo);

		//Reset states
		MDFND_NetworkClose();
		SkipCount = 0;
		SkipNext = false;
		IsLoaded = true;
		SuspendDraw = false;
		RecordingVideo = false;
		RecordingWave = false;

		Syncher.SetEmuClock(GameInfo->MasterClock >> 32);

		ReadSettings(true);

		//Create the helpers for this game
		Buffer = ESVideo::CreateTexture(GameInfo->fb_width, GameInfo->fb_height);
		Surface = new MDFN_Surface((void*)0, GameInfo->fb_width, GameInfo->fb_height, GameInfo->fb_width, MDFN_PixelFormat(MDFN_COLORSPACE_RGB, Buffer->GetRedShift(), Buffer->GetGreenShift(), Buffer->GetBlueShift(), Buffer->GetAlphaShift()));

		Buffer->Clear(0);

		//Load automatic state
		if(MDFN_GetSettingB(SETTINGNAME("autosave")))
		{
			MDFNI_LoadState(0, "mcq");
		}

		//Display emulator name	
		DisplayMessage(GameInfo->fullname);

		//Free the ROM
		free(aData);

		//Done
		return true;
	}
}

void						MednafenEmu::CloseGame			()
{
	if(IsInitialized && IsLoaded)
	{
		//Stop any recordings
		if(RecordingVideo)
		{
			MDFNI_StopAVRecord();
		}

		if(RecordingWave)
		{
			MDFNI_StopWAVRecord();
		}

		//Save any anto states
		if(MDFN_GetSettingB(SETTINGNAME("autosave")))
		{
			MDFNI_SaveState(0, "mcq", 0, 0, 0);
		}

		//Close the game
		MDFNI_CloseGame();
		GameInfo = 0;

		//Clean up
		MDFND_Rumble(0, 0);
	
		delete Inputs; Inputs = 0;
		delete Buffer; Buffer = 0;
		delete Surface; Surface = 0;
		delete TextFile; TextFile = 0;


		IsLoaded = false;
	}
}

bool						MednafenEmu::Frame				()
{
	assert(IsGameLoaded());

	//Update the rewind state
	//TODO: I had to hack MDNFI_EnableStateRewind to do nothing if the value hasn't changed, should do it here instead!
	MDFNI_EnableStateRewind(RewindSetting);

	//Pump the game's inputs
	Inputs->Process();

	//Sync time for netplay
	if(NetplayOn)
	{
		Syncher.Sync();
	}

	//Emulate frame
	ESAudio::SetSpeed(NetplayOn ? 1 : Counter.GetSpeed());

	memset(VideoWidths, 0xFF, sizeof(MDFN_Rect) * 512);
	memset(&EmulatorSpec, 0, sizeof(EmulateSpecStruct));
	EmulatorSpec.surface = Surface;
	EmulatorSpec.LineWidths = VideoWidths;
	EmulatorSpec.soundmultiplier = 1;
	EmulatorSpec.SoundRate = 48000;
	EmulatorSpec.SoundBuf = Samples;
	EmulatorSpec.SoundBufMaxSize = 24000;
	EmulatorSpec.SoundVolume = 1;
	EmulatorSpec.NeedRewind = !NetplayOn && ESInput::ButtonPressed(0, ES_BUTTON_AUXLEFT2);
	EmulatorSpec.skip = NetplayOn ? Syncher.NeedFrameSkip() : (SkipNext && ((SkipCount ++) < (Counter.GetMaxSpeed() + 1)));
	MDFNI_Emulate(&EmulatorSpec);

	Syncher.AddEmuTime(EmulatorSpec.MasterCycles / (NetplayOn ? 1 : Counter.GetSpeed()));
	Counter.Tick(EmulatorSpec.skip);

	//Handle inputs
	if(NetplayOn && ESInput::ButtonDown(0, ES_BUTTON_AUXRIGHT3) && ESInput::ButtonPressed(0, ES_BUTTON_AUXRIGHT2))
	{
		MDFND_NetworkClose();
	}
	else if(ESInput::ButtonDown(0, ES_BUTTON_AUXRIGHT3))
	{
		DoCommands();
		return false;
	}
	//Handle the frame
	else
	{
		//VIDEO
		if(!EmulatorSpec.skip)
		{
			SkipCount = 0;
			Blit();

			//Show FPS
			if(DisplayFPSSetting)
			{
				char buffer[128];
				uint32_t fps, skip;
				fps = Counter.GetFPS(&skip);
				snprintf(buffer, 128, "%d (%d)", fps, skip);
				FontManager::GetBigFont()->PutString(buffer, 10, 10, 0xFFFFFFFF);
			}

			//Show message
			if(MDFND_GetTime() - MessageTime < 5000)
			{
				FontManager::GetBigFont()->PutString(Message.c_str(), 10, 10 + FontManager::GetBigFont()->GetHeight(), 0xFFFFFFFF);
			}
		}

		//AUDIO
		//TODO: I'm trying to remember where I got this from... I figured from mednafen proper but can't find it in the source.......................
		SkipNext = ESAudio::GetBufferAmount() < EmulatorSpec.SoundBufSize * (2 * (NetplayOn ? 1 : Counter.GetSpeed()));
		Sync(&EmulatorSpec, false);
	}

	return !EmulatorSpec.skip;
}

void						MednafenEmu::Sync				(const EmulateSpecStruct* aSpec, bool aInputs)
{
	//Timer
	Syncher.AddEmuTime((aSpec->MasterCycles - aSpec->MasterCyclesALMS) / (NetplayOn ? 1 : Counter.GetSpeed()));

	//Audio
	uint32_t* realsamps = (uint32_t*)(aSpec->SoundBuf + (aSpec->SoundBufSizeALMS * GameInfo->soundchan));

	if(GameInfo->soundchan == 1)
	{
		for(int i = 0; i != aSpec->SoundBufSize - aSpec->SoundBufSizeALMS; i ++)
		{
			SamplesUp[i * 2] = Samples[i];
			SamplesUp[i * 2 + 1] = Samples[i];
		}

		realsamps = (uint32_t*)SamplesUp;
	}

	ESAudio::AddSamples(realsamps, aSpec->SoundBufSize - aSpec->SoundBufSizeALMS);

	//Input if needed
	if(aInputs)
	{
		Inputs->Process();
	}
}

void						MednafenEmu::Blit				(uint32_t* aPixels, uint32_t aWidth, uint32_t aHeight, uint32_t aPitch)
{
	assert(IsGameLoaded());

	//Map the ES texture
	uint32_t* pixels = Buffer->Map();
	uint32_t pitch = Buffer->GetPitch();

	//Display either the frame from mednafen or the frame passed in the arguments
	Area output;
	if(aPixels)
	{
		output = Area(0, 0, aWidth, aHeight);
		for(int i = 0; i != output.Height; i ++)
		{
			memcpy(&pixels[(output.Y + i) * pitch + output.X], &aPixels[(output.Y + i) * aPitch + output.X], output.Width * 4);
		}
	}
	else
	{
		output = Area(VideoWidths[0].w != ~0 ? VideoWidths[0].x : EmulatorSpec.DisplayRect.x, EmulatorSpec.DisplayRect.y, VideoWidths[0].w != ~0 ? VideoWidths[0].w : EmulatorSpec.DisplayRect.w, EmulatorSpec.DisplayRect.h);
		for(int i = 0; i != output.Height; i ++)
		{
			memcpy(&pixels[(output.Y + i) * pitch + output.X], &Surface->pixels[(output.Y + i) * Surface->pitchinpix + output.X], output.Width * 4);
		}
	}

	//Unmap and present the texture
	Buffer->Unmap();
	ESVideo::PresentFrame(Buffer, output);
}

void						MednafenEmu::DoCommands			()
{
	assert(IsGameLoaded());

	//Shut off rumble
	MDFND_Rumble(0, 0);

	//List of commands for the menu
	const char*const commands[] =
	{
		//Display name,			Image name,			Command name
		"Change Game",			"DoReload",			"DoReload",
		"Reset Game",			"DoReset",			"DoReset",
		"Show Text File",		"DoTextFile",		"DoTextFile",
		"Connect Netplay",		"DoNetplay",		"DoNetplay",
		"Change Disk Side",		"DoDiskSide", 		"DoDiskSide",
		"Save State",			"DoSaveState",		"DoSaveStateMenu",
		"Load State",			"DoLoadState",		"DoLoadStateMenu",
		"Take Screen Shot",		"DoScreenShot",		"DoScreenShot",
		"Settings",				"DoSettings",		"DoSettings",
		"Configure Controls",	"DoInputConfig",	"DoInputConfig",
		"Record Video",			"DoRecordVideo",	"DoToggleRecordVideo",
		"Record Audio",			"DoRecordAudio",	"DoToggleRecordWave",
		"Cheat Search",			"DoCheatSearch",	"DoCheatSearch",
		"Cheat Menu",			"DoCheatMenu",		"DoCheatMenu",
		"Exit Mednafen",		"ErrorIMAGE",		"DoExit",
	};

	//Setup the menu
	CommandList grid(Area(25, 25, 50, 50), 5, 3, true, false);
	grid.SetHeader("Choose Action");
	for(int i = 0; i != 15; i ++)
	{
		grid.AddItem(new CommandItem(commands[i * 3], commands[i * 3 + 1], commands[i * 3 + 2]));
	}

	//Setupt and run the interface
	Summerface sface("Grid", &grid, false);
	sface.AttachConduit(new SummerfaceStaticConduit(DoCommand, (void*)0));
	sface.Do();
}

int							MednafenEmu::DoCommand			(void* aUserData, Summerface* aInterface, const std::string& aWindow, uint32_t aButton)
{
	assert(IsGameLoaded());

	std::string command;

	//If the function is called as part of a UI instance, and the accept button is pressed.
	if(aInterface && aInterface->GetWindow(aWindow) && aButton == ES_BUTTON_ACCEPT)
	{
		CommandList* list = (CommandList*)aInterface->GetWindow(aWindow);
		command = list->GetSelected()->UserData;
	}
	//If the function is called called manually.
	else if(!aInterface)
	{
		command = aWindow;
	}
	//Otherwise nothing
	else
	{
		return 0;
	}

	if(0 == strcmp(command.c_str(), "DoCheatSearch"))		CheatSearcher::Do();
	if(0 == strcmp(command.c_str(), "DoCheatMenu"))			{CheatMenu* menu = new CheatMenu(); menu->Do(); delete menu;}
	if(0 == strcmp(command.c_str(), "DoDiskSide"))			MDFN_DoSimpleCommand(MDFN_MSC_SELECT_DISK);
	if(0 == strcmp(command.c_str(), "DoReload"))			ReloadEmulator("");
	if(0 == strcmp(command.c_str(), "DoSettings"))			{SettingMenu* menu = new SettingMenu(GameInfo->shortname); menu->Do(); delete menu;}
	if(0 == strcmp(command.c_str(), "DoReset"))				MDFNI_Reset();
	if(0 == strcmp(command.c_str(), "DoNetplay"))			MDFND_NetStart();
	if(0 == strcmp(command.c_str(), "DoScreenShot"))		MDFNI_SaveSnapshot(Surface, &EmulatorSpec.DisplayRect, VideoWidths);
	if(0 == strcmp(command.c_str(), "DoSaveState"))			MDFNI_SaveState(0, 0, Surface, &EmulatorSpec.DisplayRect, VideoWidths);
	if(0 == strcmp(command.c_str(), "DoLoadState"))			MDFNI_LoadState(0, 0);
	if(0 == strcmp(command.c_str(), "DoSaveStateMenu"))		{SuspendDraw = true; StateMenu* menu = new StateMenu(false); menu->Do(); delete menu; SuspendDraw = false;}
	if(0 == strcmp(command.c_str(), "DoLoadStateMenu"))		{SuspendDraw = true; StateMenu* menu = new StateMenu(true);  menu->Do(); delete menu; SuspendDraw = false;}
	if(0 == strcmp(command.c_str(), "DoInputConfig"))		Inputs->Configure();
	if(0 == strcmp(command.c_str(), "DoExit"))				Exit();

	if(0 == strcmp(command.c_str(), "DoTextFile"))
	{
		if(!TextFile)
		{
			TextFile = new TextFileViewer();
		}

		TextFile->Display();
	}

	if(0 == strcmp(command.c_str(), "DoToggleRecordVideo"))
	{
		if(RecordingWave)
		{
			DisplayMessage("Can't record video and audio simultaneously.");		
		}
		else
		{
			if(RecordingVideo)
			{
				DisplayMessage("Finished recording video.");
				MDFNI_StopAVRecord();
				RecordingVideo = false;
			}
			else
			{
				if(MDFNI_StartAVRecord(MDFN_MakeFName(MDFNMKF_VIDEO, 0, 0).c_str(), 48000))
				{
					DisplayMessage("Begin recording video.");
					RecordingVideo = true;
				}
				else
				{
					DisplayMessage("Failed to begin recording video.");
				}
			}
		}
	}

	if(0 == strcmp(command.c_str(), "DoToggleRecordWave"))
	{
		if(RecordingVideo)
		{
			DisplayMessage("Can't record video and audio simultaneously.");
		}
		else
		{
			if(RecordingWave)
			{
				DisplayMessage("Finished recording audio.");
				MDFNI_StopWAVRecord();

				RecordingWave = false;
			}
			else
			{

				if(MDFNI_StartWAVRecord(MDFN_MakeFName(MDFNMKF_AUDIO, 0, 0).c_str(), 48000))
				{
					DisplayMessage("Begin recording audio.");
					RecordingWave = true;
				}
				else
				{
					DisplayMessage("Failed to begin recording audio.");
				}
			}
		}
	}

	return -1;
}

void						MednafenEmu::ReadSettings		(bool aOnLoad)
{
	if(IsGameLoaded())
	{
		Inputs->ReadSettings();

		RewindSetting = MDFN_GetSettingB(SETTINGNAME("speed.rewind"));;
		DisplayFPSSetting = MDFN_GetSettingB(SETTINGNAME("display.fps"));

		Counter.SetNormalSpeed(MDFN_GetSettingUI(SETTINGNAME("speed.normalrate")));
		Counter.SetFastSpeed(MDFN_GetSettingUI(SETTINGNAME("speed.fastrate")));
		Counter.SetToggle(MDFN_GetSettingUI(SETTINGNAME("speed.toggle")));
		Counter.SetButton(MDFN_GetSettingUI(SETTINGNAME("speed.button")));

		//Update Video Area
		ESVideo::UpdatePresentArea(MDFN_GetSettingI(SETTINGNAME("display.aspect")), MDFN_GetSettingI("underscan") + MDFN_GetSettingI(SETTINGNAME("display.underscanadjust")), Area(0, 0, 0, 0));

		//Update vsync
		if(ESVideo::SupportsVSyncSelect())
		{
			ESVideo::SetVSync(MDFN_GetSettingB(SETTINGNAME("display.vsync")));
		}

		if(ESVideo::SupportsShaders())
		{
			//Update border
			if(aOnLoad || (BorderSetting != MDFN_GetSettingS(SETTINGNAME("shader.border"))))
			{
				BorderSetting = MDFN_GetSettingS(SETTINGNAME("shader.border"));
				if(Utility::FileExists(BorderSetting))
				{
					ImageManager::Purge();
					ImageManager::LoadImage("GameBorderCustom", BorderSetting);
					ESVideo::AttachBorder(ImageManager::GetImage("GameBorderCustom"));
				}
			}

			//Update shader
			if(aOnLoad || (ShaderSetting != MDFN_GetSettingS(SETTINGNAME("shader.preset"))))
			{
				ShaderSetting = MDFN_GetSettingS(SETTINGNAME("shader.preset"));
				ESVideo::SetFilter(es_paths->Build(std::string("assets/presets/") + ShaderSetting), 1);
			}
		}
	}
}

void						MednafenEmu::GenerateSettings	(std::vector<MDFNSetting>& aSettings)
{
	//Some platform specific settings
	static const MDFNSetting VSync	= {"display.vsync", MDFNSF_NOFLAGS, "Enable vsync to prevent screen tearing.", NULL, MDFNST_BOOL, "1" };

	static const MDFNSetting Shader	= {"shader.preset", MDFNSF_NOFLAGS, "Shader preset for presenting the display", NULL, MDFNST_ENUM, "Standard", 0, 0, 0, 0, 0 };
	static const MDFNSetting Border	= {"shader.border", MDFNSF_NOFLAGS, "Path to Border to use with appropriate shaders.", NULL, MDFNST_STRING, "" };

	for(int i = 0; i != MDFNSystems.size(); i ++)
	{
		for(int j = 0; j != sizeof(SystemSettings) / sizeof(MDFNSetting); j++)
		{
			std::string myname = std::string(MDFNSystems[i]->shortname) + ".es." + std::string(SystemSettings[j].name);		
	
			MDFNSetting thisone;
			memcpy(&thisone, &SystemSettings[j], sizeof(MDFNSetting));
			//TODO: This strdup will not be freed
			thisone.name = strdup(myname.c_str());
			aSettings.push_back(thisone);
		}

		//Attach platform specific settings
		if(ESVideo::SupportsVSyncSelect())
		{
			MDFNSetting thisone = VSync;
			thisone.name = strdup((std::string(MDFNSystems[i]->shortname) + ".es.display.vsync").c_str());
			aSettings.push_back(thisone);
		}

		if(ESVideo::SupportsShaders())
		{
			MDFNSetting thisone = Shader;
			thisone.name = strdup((std::string(MDFNSystems[i]->shortname) + ".es.shader.preset").c_str());
			thisone.enum_list = BuildShaderEnum();
			aSettings.push_back(thisone);

			thisone = Border;
			thisone.name = strdup((std::string(MDFNSystems[i]->shortname) + ".es.shader.border").c_str());
			aSettings.push_back(thisone);
		}
	}
}

bool						MednafenEmu::IsInitialized = false;
bool						MednafenEmu::IsLoaded = false;
	
Texture*					MednafenEmu::Buffer;
MDFN_Surface*				MednafenEmu::Surface ;
bool						MednafenEmu::SuspendDraw = false;

MDFNGI*						MednafenEmu::GameInfo = 0;
InputHandler*				MednafenEmu::Inputs;
FastCounter					MednafenEmu::Counter;
EmuRealSyncher				MednafenEmu::Syncher;

std::string					MednafenEmu::Message;
uint32_t					MednafenEmu::MessageTime = 0;
bool						MednafenEmu::RecordingVideo = false;
bool						MednafenEmu::RecordingWave = false;

std::vector<MDFNSetting>	MednafenEmu::Settings;

EmulateSpecStruct			MednafenEmu::EmulatorSpec;	
MDFN_Rect					MednafenEmu::VideoWidths[512];
int16_t						MednafenEmu::Samples[48000];
int16_t						MednafenEmu::SamplesUp[48000];
bool						MednafenEmu::SkipNext = false;
uint32_t					MednafenEmu::SkipCount = 0;

TextFileViewer*				MednafenEmu::TextFile;

bool						MednafenEmu::RewindSetting = false;
bool						MednafenEmu::DisplayFPSSetting = false;
std::string					MednafenEmu::ShaderSetting = "";
std::string					MednafenEmu::BorderSetting = "";

