/*
 * Copyright 2009, 2010, The Pink Petal Development Team.
 * The Pink Petal Devloment Team are defined as the game's coders 
 * who meet on http://pinkpetal.org     // old site: http://pinkpetal .co.cc
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "cSlider.h"
#include "CGraphics.h"
#include "CLog.h"
#include "DirPath.h"

extern cSlider* g_DragSlider;

//these static vars defined in the header file need to be specified here
SDL_Surface* cSlider::m_ImgRailDefault=nullptr;
SDL_Surface* cSlider::m_ImgRailDisabled=nullptr;
SDL_Surface* cSlider::m_ImgButtonOff=nullptr;
SDL_Surface* cSlider::m_ImgButtonOn=nullptr;
SDL_Surface* cSlider::m_ImgButtonDisabled=nullptr;
SDL_Surface* cSlider::m_ImgMarker=nullptr;

cSlider::cSlider(int ID, int x, int y, int width, int min, int max, int increment, int value, float height):
    cUIWidget(ID, x ,y, width, CalcHeight(height)), m_MinVal(min), m_MaxVal(max), m_Value(value), m_IncrementAmount(increment),
    BGLeft(new SDL_Rect), BGRight(new SDL_Rect)
{
	m_MaxOffset = m_Width - m_ImgButtonOff->w;

    Disable(false);
    ValueToOffset();

    // set up SDL_Rects indicating left and right halves of displayed background from source background images
    BGLeft->x = BGLeft->y = BGRight->y = 0;
    BGLeft->h = BGRight->h = m_ImgRailDefault->h;
    BGLeft->w = (m_Width / 2);
    BGRight->w = m_Width - BGLeft->w;
    BGRight->x = m_ImgRailDefault->w - BGRight->w;
}

cSlider::~cSlider() = default;

void cSlider::LoadInitial()
{  // load static class-wide shared base images into memory; only called once by first slider created
	m_ImgButtonDisabled = LoadAlphaImageFromFile("SliderButtonDisabled.png");
	m_ImgButtonOff = LoadAlphaImageFromFile("SliderButtonOff.png");
	m_ImgButtonOn = LoadAlphaImageFromFile("SliderButtonOn.png");
	m_ImgRailDefault = LoadAlphaImageFromFile("SliderRail.png");
	m_ImgRailDisabled = LoadAlphaImageFromFile("SliderRailDisabled.png");
	m_ImgMarker = LoadAlphaImageFromFile("SliderMarker.png");
	if (m_ImgMarker == nullptr) m_ImgMarker = LoadAlphaImageFromFile("SliderButtonDisabled.png");
}

SDL_Surface* cSlider::LoadAlphaImageFromFile(string filepath)
{
	SDL_Surface* TmpImg;
	SDL_Surface* surface;
	TmpImg = IMG_Load(ImagePath(filepath));
	if (TmpImg == nullptr)
	{
		g_LogFile.ss() << "Error Loading Slider image " << filepath; g_LogFile.ssend();
		return nullptr;
	}
	surface = SDL_DisplayFormatAlpha(TmpImg);
	SDL_FreeSurface(TmpImg);
	return surface;
}

int cSlider::SetRange(int min, int max, int value, int increment)
{
	m_MinVal = min;
	m_MaxVal = max;
	m_IncrementAmount = increment;
	return Value(value);
}

int cSlider::Value(int NewValue, bool TriggerEvent)
{
	if(NewValue < m_MinVal)
		NewValue = m_MinVal;
	if(NewValue > m_MaxVal)
		NewValue = m_MaxVal;

	int oldVal = m_Value;
	m_Value = NewValue;
	ValueToOffset();

	// if value changed, trigger update event if requested
	if(TriggerEvent && oldVal != m_Value)
		OnValueUpdate();

	return m_Value;
}

void cSlider::SetMarker(int value)
{
	m_ShowMarker = true;
	// see notes on offset calculation below in ValueToOffset()
	double PercMult = double(value - m_MinVal) / double(m_MaxVal - m_MinVal);
	m_MarkerOffset = int((m_MaxOffset * PercMult) + 0.5);
}

void cSlider::RemoveMarker()
{
	m_ShowMarker = false;
	m_MarkerOffset = 0;
}

void cSlider::Disable(bool disable) 
{
	m_Disabled = disable;
	if(disable)
	{
		m_ImgButton = m_ImgButtonDisabled;
		m_ImgRail = m_ImgRailDisabled;
	}
	else
	{
		m_ImgButton = m_ImgButtonOff;
		m_ImgRail = m_ImgRailDefault;
	}
}

void cSlider::ValueToOffset()
{  // determine m_Offset based on m_Value
	// get percentage multiplier... ex. Value = 50, MinVal = 0, MaxVal = 100: 50%, so PercMult = 0.5
	double PercMult = double(m_Value - m_MinVal) / double(m_MaxVal - m_MinVal);
	// simple enough; the extra 0.5 is used to round the value (instead of "floor")
	m_Offset = int((m_MaxOffset * PercMult) + 0.5);
}

void cSlider::OffsetToValue()
{  // determine m_Value based on m_Offset
	// get percentage multiplier... ex. Offset = 500, MaxOffset = 1000: 50%, so PercMult = 0.5
	double PercMult = double(m_Offset) / double(m_MaxOffset);
	// slightly more complicated, but same principle as used to get offset from value
	m_Value = int((double(m_MaxVal - m_MinVal) * PercMult) + m_MinVal + 0.5);
}

bool cSlider::IsOver(int x, int y)
{
	if(m_Disabled || m_Hidden)
		return false;

	bool over = false;
	if(x > m_XPos && y > m_YPos && x < m_XPos+m_Width && y < m_YPos+m_Height)
	{
		m_ImgButton = m_ImgButtonOn;
		over = true;
	}
	else
		m_ImgButton = m_ImgButtonOff;

	return over;
}

bool cSlider::IsActive(bool active)
{
	if (m_Disabled || m_Hidden)
		return false;

	if (active)
	{
		m_ImgButton = m_ImgButtonOn;
	}
	else
		m_ImgButton = m_ImgButtonOff;

	return active;
}


bool cSlider::MouseDown(int x, int y)
{  // this function is needed to initiate dragging of the slider
	if(m_Disabled || m_Hidden)
		return true;
	
	m_DragInitXPos = x - m_XPos;  // mouse X position within entire slider section

	// let's just assume that wherever they click in the slider, that's where they want it to go
	if(IsOver(x,y))
	{
		g_DragSlider = this;  // g_DragSlider is used in main.cpp to reference whichever slider is being dragged (if any)
		m_LastOffset = m_DragInitXPos - (m_ImgButtonOff->w/2);  // store starting offset for drag handling
		DragMove(x);  // refresh button position to proper x value
	}

	return true;
}

void cSlider::DragMove(int x)
{  // Slider is being dragged so update button position, and send out an update event if LiveUpdate is enabled
	// this function is called only by main.cpp mousemove when drag was initiated
	int NPos = x - m_XPos;  // new mouse position
	int NOffset = m_LastOffset + (NPos - m_DragInitXPos);  // new button offset

	if(NOffset < 0)  // make sure offset is within bounds
		NOffset = 0;
	if(NOffset > m_MaxOffset)
		NOffset = m_MaxOffset;

	m_Offset = NOffset;
	int oldVal = m_Value;
	OffsetToValue();  // set the value based on the new offset

	if(m_LiveUpdate && oldVal != m_Value)  // if doing live updates and value changed, trigger update event
		OnValueUpdate();
}

void cSlider::EndDrag()
{  // user has stopped dragging the slider
	if(!m_LiveUpdate)
		OnValueUpdate();
	ValueToOffset();  // pop button position over to exact position for value
}

bool cSlider::ButtonClicked(int x, int y, bool mouseWheelDown, bool mouseWheelUp)
{
	if(m_Disabled)
		return false;

	if(IsOver(x,y))
	{
		int oldVal = m_Value;

		if (mouseWheelUp)
			Value(m_Value + 1, true);
		else if (mouseWheelDown)
			Value(m_Value - 1, true);
		// clicked in slider area to the left of actual drag button; decrement large amount
		else if (x - m_XPos < m_Offset)
			Value(m_Value - m_IncrementAmount, true);
		// clicked in slider area to the right of actual drag button; increment large amount
		else if (x - m_XPos > m_Offset + m_ImgButtonOff->w)
			Value(m_Value + m_IncrementAmount, true);

		return true;
	}
	return false;
}

void cSlider::DrawWidget(const CGraphics& gfx)
{
	if(!m_ImgButton)
		return;

	SDL_Rect dstRect;
	dstRect.x = m_XPos;
	dstRect.y = m_YPos;

	// draw left half of background rail
    int error = gfx.BlitSurface(m_ImgRail, BGLeft.get(), &dstRect);
	if(error == -1)
	{
		LogSliderError("Error blitting slider background (left half)");
		return;
	}
	// draw right half of background rail
	dstRect.x += BGLeft->w;
	error = gfx.BlitSurface(m_ImgRail, BGRight.get(), &dstRect);
	if(error == -1)
	{
		LogSliderError("Error blitting slider background (right half)");
		return;
	}

	// draw marker if it's enabled
	if (m_ShowMarker)
	{
		dstRect.x = m_XPos + m_MarkerOffset;
		error = gfx.BlitSurface(m_ImgMarker, nullptr, &dstRect);
		if (error == -1)
		{
			LogSliderError("Error blitting slider marker");
			return;
		}
	}

	// draw slider drag button
	dstRect.x = m_XPos + m_Offset;
	error = gfx.BlitSurface(m_ImgButton, nullptr, &dstRect);
	if(error == -1)
	{
		LogSliderError("Error blitting slider drag button");
		return;
	}
}

void cSlider::LogSliderError(const string& description)
{
	CLog l;
	l.ss() << description << " - " << SDL_GetError();
	l.ssend();
}

int cSlider::CalcHeight(float height) {
    // see if static class-wide default images are loaded; if not, do so
    if (!m_ImgRailDefault)
        LoadInitial();

    return int(m_ImgRailDefault->h * height);
}

void cSlider::SetCallback(std::function<void(int)> cb)
{
    m_Callback = std::move(cb);
}

void cSlider::OnValueUpdate()
{
    if(m_Callback)
        m_Callback(Value());
}
