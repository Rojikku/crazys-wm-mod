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
#ifndef __CBUTTON_H
#define __CBUTTON_H

#include <string>
#include <memory>
#include <functional>
#include <SDL_keyboard.h>

#include "cInterfaceObject.h"

class CSurface;

class cButton : public cUIWidget
{
public:
	cButton(const std::string& OffImage, const std::string& DisabledImage, const std::string& OnImage, int ID,
	        int x, int y, int width, int height, bool transparency = false, bool cached = false);
	~cButton();

	bool IsOver(int x, int y);
	bool ButtonClicked(int x, int y);
	void OnKeyPress(SDL_keysym key);

	void SetDisabled(bool disable);
	bool IsDisabled() const;

	void DrawWidget(const CGraphics& gfx) override;

    void SetCallback(std::function<void()>);
    void SetHotKey(SDLKey key);
private:
    std::unique_ptr<CSurface> m_OffImage;
    std::unique_ptr<CSurface> m_DisabledImage;
    std::unique_ptr<CSurface> m_OnImage;
    std::function<void()> m_Callback;
    SDLKey m_HotKey = SDLK_UNKNOWN;

    bool m_Disabled = false;
    CSurface* m_CurrImage;
};


#endif
