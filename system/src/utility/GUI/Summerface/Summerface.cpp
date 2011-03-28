#include <es_system.h>
#include "Summerface.h"

Summerface_Ptr								Summerface::Create									()
{
	return boost::make_shared<Summerface>();
}


Summerface_Ptr								Summerface::Create									(const std::string& aName, SummerfaceWindow_Ptr aWindow)
{
	Summerface_Ptr sface = boost::make_shared<Summerface>();
	sface->AddWindow(aName, aWindow);
	return sface;
}


bool										Summerface::Draw									()
{
	uint32_t screenW = es_video->GetScreenWidth();
	uint32_t screenH = es_video->GetScreenHeight();

	es_video->SetClip(Area(0, 0, screenW, screenH));

	if(BackgroundCallback)
	{
		BackgroundCallback();
	}
	else
	{
		es_video->FillRectangle(Area(0, 0, screenW, screenH), Colors::Border);
	}

	for(std::map<std::string, SummerfaceWindow_Ptr>::iterator i = Windows.begin(); i != Windows.end(); i ++)
	{
		if(i->second && i->second->PrepareDraw())
		{
			return true;
		}
	}

	return false;
}

bool										Summerface::Input									()
{
	if(Windows.find(ActiveWindow) != Windows.end())
	{
		return Windows[ActiveWindow]->Input();
	}

	return false;
}

void										Summerface::AddWindow								(const std::string& aName, SummerfaceWindow_Ptr aWindow)
{
	ErrorCheck(aWindow, "Summerface::AddWindow: Window is not a valid pointer. [Name: %s]", aName.c_str());
	ErrorCheck(Windows.find(aName) == Windows.end(), "Summerface::AddWindow: Window with name is already present. [Name: %s]", aName.c_str());

	Windows[aName] = aWindow;
	aWindow->SetInterface(shared_from_this(), aName);
	ActiveWindow = aName;
}

void										Summerface::RemoveWindow							(const std::string& aName)
{
	ErrorCheck(Windows.find(aName) != Windows.end(), "Summerface::RemoveWindow: Window with name is not present. [Name: %s]", aName.c_str());
	Windows.erase(aName);
}

SummerfaceWindow_Ptr						Summerface::GetWindow								(const std::string& aName)
{
	ErrorCheck(Windows.find(aName) != Windows.end(), "Summerface::GetWindow: Window with name is not present. [Name: %s]", aName.c_str());
	return Windows[aName];
}

void										Summerface::SetActiveWindow							(const std::string& aName)
{
	ErrorCheck(Windows.find(aName) != Windows.end(), "Summerface::SetActiveWindow: Window with name is not present. [Name: %s]", aName.c_str());
	ActiveWindow = aName;
}

void										(*Summerface::BackgroundCallback)					() = 0;

