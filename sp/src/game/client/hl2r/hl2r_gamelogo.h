#ifndef hl2r_gamelogo
#define hl2r_gamelogo
#ifdef _WIN32
#pragma once
#endif

using namespace vgui;
#include <vgui_controls/EditablePanel.h>

class CFixedGameLogo : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CFixedGameLogo, vgui::EditablePanel);

public:
	CFixedGameLogo(const char *pGameLogoFileDir, bool bUseMirroredLogo = false);
	~CFixedGameLogo() {}

	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall) { bScreenSizeChanged = true; }
	virtual void ApplySchemeSettings(IScheme *pScheme);

	bool ScreenSizeChanged( void ) { return bScreenSizeChanged; }

private:
	bool bScreenSizeChanged;

	ImagePanel	*pLogo;
	ImagePanel	*pLogoMirrored;
};

#endif