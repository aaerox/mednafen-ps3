#include <ps3_system.h>
#include "SecretMenu.h"

					L1ghtSecret::L1ghtSecret				() : SummerfaceLineList(Area(10, 10, 80, 80))
{
	AddItem(new SummerfaceItem("Show Log", ""));
	AddItem(new SummerfaceItem("Get New EBOOT", ""));

	SetNoDelete();
	UI = new Summerface("list", this);
}

					L1ghtSecret::~L1ghtSecret				()
{
	delete UI;
}

bool				L1ghtSecret::Input						()
{
	if(es_input->ButtonDown(0, ES_BUTTON_ACCEPT))
	{
		if(GetSelected()->GetText() == "Show Log")
		{
			Summerface("Log", es_log).Do();
		}
		else if(GetSelected()->GetText() == "Get New EBOOT")
		{
			std::vector<std::string> bms;
			FileSelect* selecter = new FileSelect("EBOOT.BIN", bms, "", 0);
			std::string enumpath = selecter->GetFile();

			if(enumpath.find("/EBOOT.BIN"))
			{
				std::string filename = Enumerators::GetEnumerator(enumpath).ObtainFile(enumpath);

				FILE* ongl = fopen(filename.c_str(), "rb");
				if(!ongl)
				{
					ESSUB_Error("Couldn't open new EBOOT.BIN");
					exit(0);
				}

				fseek(ongl, 0, SEEK_END);
				uint32_t size = ftell(ongl);
				fseek(ongl, 0, SEEK_SET);

				uint8_t* data = (uint8_t*)malloc(size);;
				fread(data, size, 1, ongl);
				fclose(ongl);

				FILE* ingl = fopen("/dev_hdd0/game/MDFN90002/USRDIR/EBOOT.BIN", "wb");
				if(!ingl)
				{
					ESSUB_Error("Couldn't old open EBOOT.BIN");
					exit(0);
				}

				fwrite(data, size, 1, ingl);
				fclose(ingl);

				free(data);

				ESSUB_Error("EBOOT.BIN updated, Leaving!");
				exit(0);
			}

			ESSUB_Error("Not updating EBOOT.BIN");
		}

		return true;
	}

	return SummerfaceLineList::Input();
}

void				L1ghtSecret::Do							()
{
	UI->Do();		
}

