#include "cbase.h"
using namespace vgui;

#include "hl2r_gamelogo.h"
#include "ienginevgui.h"
#include "vgui_controls/ImagePanel.h"

//------------------------------------------------------------------------------
// Helper functions:
//------------------------------------------------------------------------------
static VPANEL FindChildPanel(VPANEL parent, CUtlStringList& pathNames, int index = 0)
{
	if (index >= pathNames.Size())
	{
		return parent;
	}

	for (auto child : g_pVGuiPanel->GetChildren(parent))
	{
		if (Q_strcmp(g_pVGuiPanel->GetName(child), pathNames[index]) == 0)
		{
			return FindChildPanel(child, pathNames, index + 1);
		}
	}

	return 0;
}
static Panel* FindGameUIChildPanel(const char* pPath)
{
	CUtlStringList pathNames;
	Q_SplitString(pPath, "/", pathNames);
	return g_pVGuiPanel->GetPanel(FindChildPanel(enginevgui->GetPanel(PANEL_GAMEUIDLL), pathNames), "GameUI");
}

//------------------------------------------------------------------------------
// Gamelogo:
//------------------------------------------------------------------------------
CFixedGameLogo::CFixedGameLogo(const char *pGameLogoFileDir, bool bMirrored) : EditablePanel(FindGameUIChildPanel("BaseGameUIPanel"), "GameLogo")
{
	bScreenSizeChanged = false;

	pLogo = new ImagePanel(this, "Logo");
	pLogoMirrored = new ImagePanel(this, "LogoMirrored");

	LoadControlSettings( pGameLogoFileDir );

	pLogo->SetVisible(!bMirrored);
	pLogoMirrored->SetVisible(bMirrored);
}

void CFixedGameLogo::ApplySchemeSettings(IScheme *pScheme) 
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}