#include <es_system.h>

						TextViewer::TextViewer					(const Area& aRegion, const std::string& aFileName, bool aFile) : SummerfaceWindow(aRegion)
{
	std::istream* stream;

	if(aFile)
	{
		stream = new std::ifstream(aFileName.c_str());
	}
	else
	{
		stream = new std::stringstream(aFileName);
	}

	if(stream->fail())
	{
		Lines.push_back("Couldn't open file");
		LongestLine = Lines[0].length();
		Top = Left = LinesDrawn = 0;
		return;
	}

	LoadStream(stream);	
	
	SetHeader("[Text Viewer]");

	delete stream;
}
						
								
						TextViewer::~TextViewer					()
{
						
}
		
bool					TextViewer::Draw						()
{
	uint32_t lineheight = FontManager::GetFixedFont()->GetHeight();
	LinesDrawn = es_video->GetClip().Height / lineheight;
	
	for(int i = 0; i != LinesDrawn; i ++)
	{
		if(i + Top >= Lines.size())
		{
			break;
		}
		
		if(Lines[Top + i].length() > Left)
		{
			FontManager::GetFixedFont()->PutString(Lines[Top + i].substr(Left).c_str(), 2, i * lineheight, Colors::Normal);
		}
	}
	
	return false;	
}

bool					TextViewer::Input						()
{
	Top += es_input->ButtonPressed(0, ES_BUTTON_DOWN) ? 1 : 0;
	Top -= es_input->ButtonPressed(0, ES_BUTTON_UP) ? 1 : 0;
	Top += es_input->ButtonPressed(0, ES_BUTTON_AUXRIGHT1) ? LinesDrawn : 0;
	Top -= es_input->ButtonPressed(0, ES_BUTTON_AUXLEFT1) ? LinesDrawn : 0;
	Top = Utility::Clamp(Top, 0, (int32_t)Lines.size() - 1);

	Top = es_input->ButtonPressed(0, ES_BUTTON_AUXRIGHT2) ? Lines.size() - 1 : Top;
	Top = es_input->ButtonPressed(0, ES_BUTTON_AUXLEFT2) ? 0 : Top;
	
	Left += es_input->ButtonPressed(0, ES_BUTTON_RIGHT) ? 1 : 0;
	Left -= es_input->ButtonPressed(0, ES_BUTTON_LEFT) ? 1 : 0;	
	Left = Utility::Clamp(Left, 0, (int32_t)LongestLine);

	return es_input->ButtonDown(0, ES_BUTTON_ACCEPT);
}
		
void					TextViewer::LoadStream					(std::istream* aStream)
{
	std::string line;

	Top = 0;
	Left = 0;
	LongestLine = 0;
	LinesDrawn = 0;
		
	while(!aStream->eof())
	{
		std::getline(*aStream, line);
		Lines.push_back(line);
		
		LongestLine = (line.length() > LongestLine) ? line.length() : LongestLine;
	}
}

