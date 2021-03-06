#include <es_system.h>
#include "FileSelect.h"

namespace
{
	bool								AlphaSortDirectory					(FileSelect::DirectoryItem* a, FileSelect::DirectoryItem* b)
	{
		if(a->IsDirectory && !b->IsDirectory)		return true;
		if(!a->IsDirectory && b->IsDirectory)		return false;
		return a->GetText() < b->GetText();
	}
}


								FileSelect::DirectoryItem::DirectoryItem	(const std::string& aName, const std::string& aPath, bool aDirectory, bool aFile, bool aBookMark) :
	Name(aName),
	Extension(Utility::GetExtension(aPath)),
	Image(aDirectory ? "FolderICON" : (ImageManager::GetImage(Extension + "ICON") ? Extension + "ICON" : "FileICON")),
	Path(aPath),
	IsDirectory(aDirectory),
	IsFile(aFile),
	IsBookMark(aBookMark)
{
}


										FileSelect::FileSelect				(const std::string& aHeader, const std::string& aBookMarks, const std::string& aPath, SummerfaceInputConduit* aInputHook) :
	List(Area(10, 10, 80, 80)),
	Interface("List", &List, false),
	Header(aHeader)
{
	//Bookmarks
	Utility::StringToVector(BookMarks, aBookMarks, ';');

	//Setup
	if(aInputHook)
	{
		Interface.AttachConduit(aInputHook);
	}

	Interface.AttachConduit(new SummerfaceTemplateConduit<FileSelect>(this));

	Paths.push(aPath);
	LoadList(aPath);
}

int										FileSelect::HandleInput				(Summerface* aInterface, const std::string& aWindow, uint32_t aButton)
{
	//Use the input conduit to toggle bookmarks
	if(aButton == ES_BUTTON_AUXRIGHT2)
	{
		BookmarkList::iterator bookmark = std::find(BookMarks.begin(), BookMarks.end(), List.GetSelected()->Path);
		
		if(bookmark != BookMarks.end())
		{
			BookMarks.erase(bookmark);
			List.GetSelected()->IsBookMark = false;
		}
		else
		{
			BookMarks.push_back(List.GetSelected()->Path);
			List.GetSelected()->IsBookMark = true;
		}

		//Eat the input
		return 1;
	}

	return 0;
}

std::string								FileSelect::GetFile					()
{
	std::string result;

	while(!LibES::WantToDie())
	{
		//Run the list
		Interface.Do();

		//If the list was canceled; try to move up the stack
		if(List.WasCanceled())
		{
			//Go to parent directory
			if(Paths.size() > 1)
			{
				Paths.pop();
				LoadList(Paths.top().c_str());
				continue;
			}
			//No parent, return empty string
			else
			{
				return "";
			}
		}
		
		//If a directory was selected, list it
		if(List.GetSelected()->IsDirectory)
		{
			Paths.push(List.GetSelected()->Path);
			LoadList(Paths.top());
		}
		//If a file was selected, return it
		else if(List.GetSelected()->IsFile)
		{
			return List.GetSelected()->Path;
			break;
		}
	}

	return "";
}

std::string&						FileSelect::GetBookmarks					(std::string& aOutput)
{
	return Utility::VectorToString(aOutput, BookMarks, ';');
}

void								FileSelect::LoadList						(const std::string& aPath)
{
	//Prep the list for this directory
	List.ClearItems();
	List.SetHeader("[%s] %s", Header.c_str(), aPath.c_str());

	//If the path is empty, list the drive selection and bookmarks
	if(aPath.empty())
	{
		//List volumes
		std::list<std::string> items;

		if(Utility::ListVolumes(items) && items.size())
		{
			for(std::list<std::string>::iterator i = items.begin(); i != items.end(); i ++)
			{
				List.AddItem(new DirectoryItem((*i), (*i), true, false, false));
			}

			List.Sort(AlphaSortDirectory);
		}
		else
		{
			List.AddItem(new DirectoryItem("No Volumes Found", "", false, false, false));
		}

		//Load bookmarks
		for(BookmarkList::iterator i = BookMarks.begin(); i != BookMarks.end(); i ++)
		{
			if(!i->empty())
			{
				std::string nicename = *i;

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
				
				List.AddItem(new DirectoryItem(nicename, *i, directory, !directory, true));
			}
		}
	}
	else
	{
		//List files
		std::list<std::string> items;

		if(Utility::ListDirectory(aPath, items) && items.size())
		{
			for(std::list<std::string>::iterator i = items.begin(); i != items.end(); i ++)
			{
				List.AddItem(new DirectoryItem((*i), aPath + *i, (*i)[i->length() - 1] == '/', (*i)[i->length() - 1] != '/', std::find(BookMarks.begin(), BookMarks.end(), aPath + *i) != BookMarks.end()));
			}

			List.Sort(AlphaSortDirectory);
		}
		else
		{
			List.AddItem(new DirectoryItem("Directory is empty", "", false, false, false));
		}
	}
}

