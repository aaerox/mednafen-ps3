#include <es_system.h>
#include "SummerfaceList.h"

namespace
{
	bool									AlphaSort											(SummerfaceItem* a, SummerfaceItem* b)
	{
		return a->GetText() < b->GetText();
	}
}




											SummerfaceList::SummerfaceList						(const Area& aRegion) : 
	SummerfaceWindow(aRegion),
	SelectedIndex(0),
	Canceled(false),
	LabelFont(FontManager::GetBigFont()),
	View(new AnchoredListView(this))
	
{
}

											SummerfaceList::~SummerfaceList						()
{
	ClearItems();
	delete View;
}

void										SummerfaceList::SetSelection						(uint32_t aIndex)
{
	ErrorCheck(aIndex < Items.size(), "SummerfaceList::SetSelection: Item index out of range [Item %d, Total %d]", aIndex, Items.size());
	SelectedIndex = aIndex;
}

void										SummerfaceList::SetSelection						(const std::string& aText)
{
	for(int i = 0; i != Items.size(); i ++)
	{
		if(Items[i]->GetText() == aText)
		{
			SelectedIndex = i;
			return;
		}
	}

	SelectedIndex = 0;
}

void										SummerfaceList::AddItem								(SummerfaceItem* aItem)
{
	ErrorCheck(std::find(Items.begin(), Items.end(), aItem) == Items.end(), "SummerfaceList::AddItem: Can't add the same item to the list twice");

	Items.push_back(aItem);
}

void										SummerfaceList::ClearItems							()
{
	for(std::vector<SummerfaceItem*>::iterator iter = Items.begin(); iter != Items.end(); iter ++)
	{
		delete (*iter);
	}

	Items.clear();
	SelectedIndex = 0;
}

void										SummerfaceList::SetFont								(Font* aFont)
{
	ErrorCheck(aFont, "SummerfaceList::SetFont: Font must not be null");

	LabelFont = aFont;
}

void										SummerfaceList::SetView								(ListView* aView)
{
	ErrorCheck(aView, "SummerfaceList::SetView: View must not be null");

	delete View;
	View = aView;
}

void										SummerfaceList::Sort								(bool (*aCallback)(SummerfaceItem*, SummerfaceItem*))
{
	std::sort(Items.begin(), Items.end(), aCallback ? aCallback : AlphaSort);
}


											GridListView::GridListView							(SummerfaceList* aList, uint32_t aWidth, uint32_t aHeight, bool aHeader, bool aLabels) :
	List(aList),
	Width(aWidth),
	Height(aHeight),
	FirstItem(0),
	DrawHeader(aHeader),
	RefreshHeader(true),
	DrawLabels(aLabels)
{
	ErrorCheck(Width != 0 && Height != 0 && Width <= 16 && Height <= 16, "GridListView::GridListView: Grid dimensions out of range. [X: %d, Y: %d]", Width, Height);
}


bool										GridListView::Input									()
{
	uint32_t oldIndex = List->GetSelection();
	int32_t XSelection = List->GetSelection() % Width;
	int32_t YSelection = (List->GetSelection() - FirstItem) / Width;

	XSelection += es_input->ButtonPressed(0, ES_BUTTON_RIGHT) ? 1 : 0;
	XSelection -= es_input->ButtonPressed(0, ES_BUTTON_LEFT) ? 1 : 0;
	XSelection = Utility::Clamp(XSelection, 0, (int32_t)Width - 1);

	YSelection += es_input->ButtonPressed(0, ES_BUTTON_DOWN) ? 1 : 0;
	YSelection -= es_input->ButtonPressed(0, ES_BUTTON_UP) ? 1 : 0;

	while(YSelection < 0)
	{
		YSelection ++;
		FirstItem -= Width;
	}	
		
	while(YSelection >= Height)
	{
		YSelection --;
		FirstItem += Width;
	}
	
	while(FirstItem >= 0 && (FirstItem + (Width * Height) >= List->GetItemCount() + Width))
	{
		FirstItem -= Width;
	}

	while(FirstItem < 0)
	{
		FirstItem += Width;
	}
	

	List->SetSelection(FirstItem + (YSelection * Width + XSelection));
	if(List->GetSelection() >= List->GetItemCount())
	{
		List->SetSelection(oldIndex);
	}

	if(DrawHeader && (oldIndex != List->GetSelection() || RefreshHeader))
	{
		RefreshHeader = false;
		List->SetHeader(List->GetSelected()->GetText());
	}

	if(es_input->ButtonDown(0, ES_BUTTON_CANCEL))
	{
		List->SetCanceled(true);
		return true;
	}

	if(List->GetInputConduit() && List->GetSelection() < List->GetItemCount())
	{
		return List->GetInputConduit()->HandleInput(List->GetInterface(), List->GetName()); 
	}
	else if(es_input->ButtonDown(0, ES_BUTTON_ACCEPT))
	{
		List->SetCanceled(false);
		return true;
	}

	return false;
}

bool										GridListView::DrawItem								(SummerfaceItem* aItem, uint32_t aX, uint32_t aY, uint32_t aWidth, uint32_t aHeight, bool aSelected)
{
	Texture* image = ImageManager::GetImage(aItem->GetImage());
	
	//TODO: Make this work!
	if(Utility::FileExists(aItem->Properties["THUMB"]))
	{
		image = ImageManager::GetImage("SCRATCH%0");
		ImageManager::FillScratch(0, aItem->Properties["THUMB"]);
	}
	
	Area ImageArea(0, 0, 0, 0);

	if(image && aWidth && aHeight)
	{
		if(ImageArea.Width == 0)
		{
			ImageArea = Area(0, 0, image->GetWidth(), image->GetHeight());
		}

		if(DrawLabels)
		{
			aHeight -= List->GetFont()->GetHeight();
		}

		uint32_t x = aX, y = aY, w = aWidth, h = aHeight;
		Utility::CenterAndScale(x, y, w, h, ImageArea.Width, ImageArea.Height);
		
		es_video->PlaceTexture(image, Area(x, y, w, h), ImageArea, 0xFFFFFFFF);

		if(DrawLabels)
		{
			List->GetFont()->PutString(aItem->GetText().c_str(), aX, aY + aHeight, aItem->GetNormalColor());
		}
	}
	
	if(aSelected)
	{
		es_video->FillRectangle(Area(aX, aY, aWidth - 2, aHeight - 2), Colors::SpecialBackGround);
	}

	return false;
}

bool										GridListView::Draw									()
{
	uint32_t iconWidth = es_video->GetClip().Width / Width - 4;
	uint32_t iconHeight = es_video->GetClip().Height / Height - 4;	

	uint32_t XSelection = List->GetSelection() % Width;
	uint32_t YSelection = List->GetSelection() / Width;

	for(int i = 0; i != Height; i ++)
	{
		for(int j = 0; j != Width; j ++)
		{
			if(FirstItem + (i * Width + j) >= List->GetItemCount())
			{
				break;
			}
		
			DrawItem(List->GetItem(FirstItem + (i * Width + j)), j * iconWidth + 4, i * iconHeight + 4, iconWidth, iconHeight, List->GetSelection() == FirstItem + (i * Width + j));
		}
	}
	
	return false;
}

											AnchoredListView::AnchoredListView					(SummerfaceList* aList, bool aAnchored, bool aWrap) :
	List(aList),
	FirstLine(0),
	LinesDrawn(0),
	Anchored(aAnchored),
	Wrap(aWrap)
{
}

bool										AnchoredListView::DrawItem							(SummerfaceItem* aItem, uint32_t aX, uint32_t aY, bool aSelected)
{
	Texture* image = ImageManager::GetImage(aItem->GetImage());

	if(image)
	{
		uint32_t width = (uint32_t)((double)image->GetWidth() * ((double)(List->GetFont()->GetHeight() - 4) / (double)image->GetHeight()));

		es_video->PlaceTexture(image, Area(aX, aY + 2, width, List->GetFont()->GetHeight() - 4), Area(0, 0, image->GetWidth(), image->GetHeight()), 0xFFFFFFFF);
		aX += width;
	}

	List->GetFont()->PutString(aItem->GetText().c_str(), aX, aY, aSelected ? aItem->GetHighLightColor() : aItem->GetNormalColor());

	return false;
}

bool										AnchoredListView::Draw								()
{
	if(List->GetItemCount() != 0)
	{
		uint32_t itemheight = List->GetFont()->GetHeight();
		LinesDrawn = es_video->GetClip().Height / itemheight;
		
		uint32_t online = 0;
		int onitem = Anchored ? List->GetSelection() - LinesDrawn / 2 : FirstLine;

		for(int i = 0; i != LinesDrawn + 2; i ++, onitem ++, online ++)
		{
			if(onitem < 0 || onitem >= List->GetItemCount())
			{
				continue;
			}
	
			DrawItem(List->GetItem(onitem), 16, (online * itemheight), onitem == List->GetSelection());
		}
	}

	return false;
}

bool										AnchoredListView::Input								()
{
	int32_t oldIndex = List->GetSelection();
	if(List->GetItemCount() != 0)
	{
		oldIndex += (es_input->ButtonPressed(0, ES_BUTTON_DOWN) ? 1 : 0);
		oldIndex -= (es_input->ButtonPressed(0, ES_BUTTON_UP) ? 1 : 0);
		oldIndex += (es_input->ButtonPressed(0, ES_BUTTON_RIGHT) ? LinesDrawn : 0);
		oldIndex -= (es_input->ButtonPressed(0, ES_BUTTON_LEFT) ? LinesDrawn : 0);
	
		if(Wrap)
		{
			oldIndex = (oldIndex < 0) ? List->GetItemCount() - 1 : oldIndex;
			oldIndex = (oldIndex >= List->GetItemCount()) ? 0 : oldIndex;
		}

		oldIndex = Utility::Clamp(oldIndex, 0, List->GetItemCount() - 1);
		List->SetSelection(oldIndex);

		if(List->GetItemCount() != 1 && !Anchored)
		{
			if(oldIndex >= FirstLine + LinesDrawn)
			{
				FirstLine = oldIndex - LinesDrawn;
			}

			if(oldIndex < FirstLine)
			{
				FirstLine = oldIndex;
			}
		}
	}

	if(List->GetInputConduit())
	{
		return List->GetInputConduit()->HandleInput(List->GetInterface(), List->GetName()); 
	}
	else if(es_input->ButtonDown(0, ES_BUTTON_ACCEPT))
	{
		List->SetCanceled(false);
		return true;
	}

	if(es_input->ButtonDown(0, ES_BUTTON_CANCEL))
	{
		List->SetCanceled(true);
		return true;
	}
	
	return false;
}
