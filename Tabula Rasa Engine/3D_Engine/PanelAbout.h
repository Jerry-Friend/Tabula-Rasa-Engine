#ifndef __PANEL_ABOUT_H__
#define __PANEL_ABOUT_H__

// Editor Panel to show the properties of a single GameObject and its components
#include "Panel.h"

class PanelAbout : public Panel
{
public:
	PanelAbout();
	virtual ~PanelAbout();

	void Draw() override;

private:

};

#endif// __PANELABOUT_H__