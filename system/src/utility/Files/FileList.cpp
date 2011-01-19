#include <ps3_system.h>

namespace
{
	bool								AlphaSortC								(ListItem* a, ListItem* b)
	{
		if(((FileListItem*)a)->IsDirectory() && !((FileListItem*)b)->IsDirectory())			return true;
		if(((FileListItem*)b)->IsDirectory() && !((FileListItem*)a)->IsDirectory())			return false;
	
		return a->GetText() < b->GetText();
	}
}

										FileList::FileList						(const std::string& aHeader, const std::string& aPath, std::vector<std::string>& aBookmarks, MenuHook* aInputHook) : WinterfaceList(std::string("[") + aHeader + "]" + aPath, true, true, aInputHook), BookMarks(aBookmarks)
{
	FileEnumerator& enumer = Enumerators::GetEnumerator(aPath);

	Path = aPath;

	if(aPath.empty())
	{
		for(int i = 0; i != BookMarks.size(); i ++)
		{
			std::string nicename = BookMarks[i];
	
			//TODO: Validate path
	
			if(nicename.empty())
			{
				continue;
			}
			
			bool directory = false;
			if(nicename[nicename.length() - 1] != '/')
			{
				nicename = nicename.substr(nicename.rfind('/') + 1);
			}
			else
			{
				nicename = nicename.substr(0, nicename.length() - 1);
				nicename = nicename.substr(nicename.rfind('/') + 1);
				nicename.push_back('/');
				directory = true;
			}
			
			Items.push_back(new FileListItem(nicename, BookMarks[i], directory, true));
		}
	}

	std::vector<std::string> filters;
	enumer.ListPath(aPath, filters, Items);
	
	SideItems.push_back(new InputListItem("Navigate", ES_BUTTON_UP));	
	SideItems.push_back(new InputListItem("Select Item", ES_BUTTON_ACCEPT));
	SideItems.push_back(new InputListItem("Previous Dir", ES_BUTTON_CANCEL));
	SideItems.push_back(new InputListItem("Toggle Bookmark", ES_BUTTON_AUXRIGHT2));
	SideItems.push_back(new InputListItem("Settings", ES_BUTTON_AUXRIGHT3));	//HACK: Only with the mednafen hook!!!!
	SideItems.push_back(new InputListItem("Show Readme", ES_BUTTON_AUXLEFT3));	//HACK: Only with the mednafen hook!!!!

	std::sort(Items.begin(), Items.end(), AlphaSortC);
}

bool									FileList::Input							()
{
	if(es_input->ButtonDown(0, ES_BUTTON_AUXRIGHT2))
	{
		FileListItem* item = (FileListItem*)GetSelected();
		std::vector<std::string>::iterator bookmark = std::find(BookMarks.begin(), BookMarks.end(), item->GetPath());
		
		if(bookmark != BookMarks.end())
		{
			BookMarks.erase(bookmark);
			item->SetBookMark(0);
		}
		else
		{
			BookMarks.push_back(item->GetPath());
			item->SetBookMark(1);
		}
	}

	return 	WinterfaceList::Input();
}

std::string								FileList::GetFile						()
{
	if(WasCanceled())
	{
		throw FileException("FileList::GetFile: Can't get a file if the list was canceled");
	}

	return ((FileListItem*)GetSelected())->GetPath();
}

bool									FileList::IsDirectory					()
{
	if(WasCanceled())
	{
		throw FileException("FileList::IsDirectory: Can't get a file if the list was canceled");
	}
	
	return ((FileListItem*)GetSelected())->IsDirectory();
}
