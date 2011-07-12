#pragma once

///Class holding and editing the cheats for the loaded game.
class							CheatMenu
{
	private:
		///Class describing a Cheat as a SummerfaceItem.
		class					Cheat : public SummerfaceItem
		{
			public:
				///Create a new Cheat item.
				///@param aName Display name of the Cheat.
				///@param aAddress Memory address of the Cheat.
				///@param aValue Value of the Cheat.
				///@param aCompare Condition of the Cheat.
				///@param aStatus Determine if the Cheat is active.
				///@param aType Unknown...
				///@param aLength Number of bytes the Cheat should patch.
				///@param aBigEndian True if the Cheat should patch memory as big endian.
								Cheat					(char* aName, uint32_t aAddress, uint64_t aValue, uint64_t aCompare, int aStatus, int8_t aType, uint32_t aLength, bool aBigEndian) :
					SummerfaceItem("", ""),
					Name(aName),
					Address(aAddress),
					Value(aValue),
					Compare(aCompare),
					Status(aStatus),
					Type(aType),
					Length(aLength),
					BigEndian(aBigEndian)
				{
					std::stringstream str;
					str << Name << " (" << std::hex << Address << "->" << Value << ":" << Compare << ")";
					SetText(str.str());
				}

				///Override the image used for displaying the Cheat.
				///@return "CheckIMAGE" if the Cheat is enabled, "ErrorIMAGE" if it is disabled.
				std::string		GetImage				()
				{
					return Status ? "CheckIMAGE" : "ErrorIMAGE";
				}

			public:
				std::string		Name;
				uint32_t		Address;
				uint64_t		Value;
				uint64_t		Compare;
				int				Status;
				int8_t			Type;
				uint32_t		Length;
				bool			BigEndian;
		};


	public:
		///Create a new CheatMenu.
								CheatMenu				()
		{
			//Make the menu
			CheatList = smartptr::make_shared<SummerfaceList>(Area(10, 10, 80, 80));
			CheatList->SetView(smartptr::make_shared<AnchoredListView>(CheatList, true));
			CheatList->SetHeader("Available Cheats");

			//Insert all of the cheats
			//NOTE: These MUST NOT be sorted, their index in the list is passed to mednafen
			MDFNI_ListCheats(AttachCheat, (void*)this);
		}

		///Run the CheatMenu.
		void					Do						()
		{
			Summerface_Ptr sface = Summerface::Create("CHEATS", CheatList);
			sface->AttachConduit(smartptr::make_shared<SummerfaceTemplateConduit<CheatMenu> >(this));
			sface->Do();
		}

		///Handle input on the CheatMenu.
		///@param aInterface Pointer to Summerface making the call.
		///@param aWindow Name of the active SummerfaceWindow.
		///@return 0: Ignore, 1: Eat, -1: Close Interface
		int						HandleInput				(Summerface_Ptr aInterface, const std::string& aWindow)
		{
			if(ESInput::ButtonDown(0, ES_BUTTON_LEFT) || ESInput::ButtonDown(0, ES_BUTTON_RIGHT))
			{
				smartptr::shared_ptr<Cheat> cheat = smartptr::static_pointer_cast<Cheat>(CheatList->GetSelected());
				cheat->Status = !cheat->Status;

				MDFNI_ToggleCheat(CheatList->GetSelection());
				return 1;
			}

			return 0;
		}

	private:
		///Static callback function for MDFNI_ListCheats.
		static int				AttachCheat				(char* aName, uint32_t aAddress, uint64_t aValue, uint64_t aCompare, int aStatus, char aType, uint32_t aLength, bool aBigEndian, void* aData)
		{
			CheatMenu* menu = (CheatMenu*)aData;
			menu->CheatList->AddItem(smartptr::make_shared<Cheat>(aName, aAddress, aValue, aCompare, aStatus, aType, aLength, aBigEndian));
			return 1;
		}

	private:
		SummerfaceList_Ptr		CheatList;				///<List of Cheats.
};

