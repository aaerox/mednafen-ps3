#pragma once

#include <src/GUI/Colors.h>

///ListItem class for settings.
struct								SettingItem
{
	///Create a new SettingItem.
	///@param aSetting Pointer to setting data. May not be NULL.
									SettingItem							(const MDFNCS* aSetting, const std::string& aGroup) :
		Setting(aSetting), Group(aGroup),
		TextColor("text", Colors::black),
		SelectedColor("selectedtext", Colors::red)
	{
		assert(aSetting);
	}

	///Return a string containing the settings name and value.
	///@return The settings name and value, separated by a tab character.
	std::string						GetText								(); //External

	///Returns an empty string.
	///@return An empty string.
	std::string						GetImage							() {return "";}

	///Return the group for GroupListView
	///@return The group name.
	std::string						GetGroup							() {return Group;}

	///Retruns Colors::Normal.
	///@return Colors::Normal.
	uint32_t						GetNormalColor						() {return TextColor;}

	///Retruns Colors::HighLight.
	///@return Colors::HighLight.
	uint32_t						GetHighLightColor					() {return SelectedColor;}

	const MDFNCS*					Setting;							///<A pointer to the managed setting.
	std::string						Group;								///<The group name for GroupListView.

	Color							TextColor;							///<The text color.
	Color							SelectedColor;						///<The selected text color.
};


///Class used to list and edit mednafen's settings.
class								SettingGroupMenu
{
	typedef GroupListView<SettingItem>							SettingListType;
	typedef AnchoredListView<SummerfaceItem>					EnumListType;

	public:
		typedef std::list<SettingItem*>							SettingGroup;

	public:
		///Create a new SettingGroupMenu.
		///@param aSettings List of settings for the menu to edit. The list is cleared by this function, and the items are passed to delete.
		///@param aSystemName Name of the system, or empty if a general category.
									SettingGroupMenu			(SettingGroup& aSettings, const std::string& aSystemName);

		///Blank virtual destructor for SetttingMenu.
		virtual						~SettingGroupMenu			() {}

		///Run the setting editor.
		void						Do							();

		int							HandleInput					(Summerface* aInterface, const std::string& aWindow, uint32_t aButton);

		///Refresh the parent list's header if needed.
		void						DoHeaderRefresh				();

		bool						HandleButton				(uint32_t aButton, const MDFNCS& aSetting);

		///Handle changing a boolean setting.
		///@param aButton System button that was pressed.
		///@param aSetting Setting to update.
		///@return True if this function has eaten the input, false to continue processing.
		bool						HandleBool					(uint32_t aButton, const MDFNCS& aSetting);

		///Handle changing an int or unsigned int setting.
		///@param aButton System button that was pressed.
		///@param aSetting Setting to update.
		///@return True if this function has eaten the input, false to continue processing.
		bool						HandleInt					(uint32_t aButton, const MDFNCS& aSetting);

		///Handle changing an enumeration setting.
		///@param aButton System button that was pressed.
		///@param aSetting Setting to update.
		///@return True if this function has eaten the input, false to continue processing.
		bool						HandleEnum					(uint32_t aButton, const MDFNCS& aSetting);

	private:
		SettingListType				List;						///<SummerfaceList used for displaying a category of settings.
		Summerface					Interface;					///<Summerface object used for the list.

		bool						RefreshHeader;				///<True if the header needs to be refreshed.
};


