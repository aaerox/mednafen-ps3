#include <mednafen_includes.h>

									MednafenStateItem::MednafenStateItem			(uint32_t aSlot, MednafenStateMenu* aMenu) : GridItem("", "")
{
	if(aSlot >= 9)
	{
		throw ESException("StateItem slot out of range: %d", aSlot);
	}

	Parent = aMenu;

	Slot = aSlot;

	char buffer[32];
	snprintf(buffer, 32, "Slot %d", aSlot + 1);
	Text = buffer;

	snprintf(buffer, 32, "SCRATCH%%%d", aSlot);
	LabelImage = buffer;

	Pixels = 0;
	Width = 0;
	Height = 0;
}

									MednafenStateItem::~MednafenStateItem			()
{
	if(Pixels)
	{
		free(Pixels);
	}
}

bool								MednafenStateItem::Input						()
{
	if(es_input->ButtonDown(0, ES_BUTTON_AUXLEFT2))
	{
		Parent->SelectMednafenState(Slot);

		MednafenEmu::DoCommand("DoSaveState");
		return true;
	}

	if(es_input->ButtonDown(0, ES_BUTTON_AUXRIGHT2))
	{
		Parent->SelectMednafenState(Slot);

		MednafenEmu::DoCommand("DoLoadState");
		return true;
	}

	return false;
}

void								MednafenStateItem::Draw							(uint32_t aX, uint32_t aY, uint32_t aWidth, uint32_t aHeight, bool aSelected)
{
	if(Pixels && ImageManager::GetImage(LabelImage))
	{
		Texture* tex = ImageManager::GetImage(LabelImage);
		uint32_t* texpix = tex->GetPixels();

		for(int i = 0; i != Height; i ++)
		{
			for(int j = 0; j != Width; j ++)
			{
				texpix[i * tex->GetWidth() + j] = Pixels[i * Width + j];
			}
		}
	}

	GridItem::Draw(aX, aY, aWidth, aHeight, aSelected);
}

void								MednafenStateItem::SetImage						(uint8_t* aPixels, uint32_t aWidth, uint32_t aHeight)
{
	if(Pixels)
	{
		free(Pixels);
		Pixels = 0;
	}

	if(aPixels && aWidth > 0 && aHeight > 0 && aWidth <= 512 && aHeight <= 512)
	{
		Pixels = (uint32_t*)malloc(aWidth * aHeight * 4);

		for(int i = 0; i != aHeight; i ++)
		{
			for(int j = 0; j != aWidth; j ++)
			{
				uint32_t r = aPixels[0];
				uint32_t g = aPixels[1];
				uint32_t b = aPixels[2];
				Pixels[i * aWidth + j] = (r << 16) | (g << 8) | (b) | 0xFF000000;
				aPixels += 3;
			}
		}

		Width = aWidth;
		Height = aHeight;

		printf("%d %d\n", Width, Height);
		SetImageArea(0, 0, Width, Height);
	}
}


									MednafenStateMenu::MednafenStateMenu			() : WinterfaceIconGrid("Save States", 3, 3, true)
{
	CurrentState = 0;

	Items.push_back(new MednafenStateItem(0, this));
	Items.push_back(new MednafenStateItem(1, this));
	Items.push_back(new MednafenStateItem(2, this));
	Items.push_back(new MednafenStateItem(3, this));
	Items.push_back(new MednafenStateItem(4, this));
	Items.push_back(new MednafenStateItem(5, this));
	Items.push_back(new MednafenStateItem(6, this));
	Items.push_back(new MednafenStateItem(7, this));
	Items.push_back(new MednafenStateItem(8, this));

	SideItems.push_back(new InputListItem("Navigate", ES_BUTTON_UP));
	SideItems.push_back(new InputListItem("Save State", ES_BUTTON_AUXLEFT2));
	SideItems.push_back(new InputListItem("Load State", ES_BUTTON_AUXRIGHT2));
	SideItems.push_back(new InputListItem("Close Menu", ES_BUTTON_CANCEL));

	MDFNDES_SetStateTarget(this);

	for(int i = 0; i != 9; i ++)
	{
		char buffer[32];
		snprintf(buffer, 32, "SCRATCH%%%d", i);
		if(ImageManager::GetImage(buffer))
		{
			ImageManager::GetImage(buffer)->Clear(0);
		}

		SelectMednafenState(i);
	}
}

									MednafenStateMenu::~MednafenStateMenu			()
{
	MDFNDES_SetStateTarget(0);
}

void								MednafenStateMenu::SetStateStatus				(StateStatusStruct* aStatus)
{
	if(aStatus && aStatus->current >= 0 && aStatus->current < 9)
	{
		((MednafenStateItem*)Items[CurrentState])->SetImage(aStatus->gfx, aStatus->w, aStatus->h);
	}
	else
	{
		((MednafenStateItem*)Items[CurrentState])->SetImage(0, 0, 0);
	}
}

void								MednafenStateMenu::SelectMednafenState			(uint32_t aSlot)
{
	if(aSlot >= 9)
	{
		throw ESException("SelectMednafenState: slot out of range: %d", aSlot);
	}

	CurrentState = aSlot;
	MDFNI_SelectState(aSlot);
}

