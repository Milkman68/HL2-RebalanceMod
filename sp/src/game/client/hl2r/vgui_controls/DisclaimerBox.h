#ifndef DISCLAIMERBOX_H
#define DISCLAIMERBOX_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/MessageBox.h>
#include <vgui_controls/Button.h>

namespace vgui
{
//-----------------------------------------------------------------------------
// Purpose: Creates A Message box with a question in it and yes/no buttons
//-----------------------------------------------------------------------------
class DisclaimerBox : public MessageBox
{
	DECLARE_CLASS_SIMPLE( DisclaimerBox, MessageBox );

public:
	DisclaimerBox(const char *title, const char *queryText,vgui::Panel *parent = NULL );
	DisclaimerBox(const wchar_t *wszTitle, const wchar_t *wszQueryText,vgui::Panel *parent = NULL);
	~DisclaimerBox();

	// Layout the window for drawing 
	virtual void PerformLayout();

	// Set the clientcommand to execute when ok button is hit
	void SetOKClientCommand(const char *command);

	// Set the text on the Cancel button
	void SetDismissButtonText(const char *buttonText);
	void SetDismissButtonText(const wchar_t *wszButtonText);
	void SetDismissConvar( const char *convar );


protected:
	virtual void OnCommand(const char *command);
	CheckButton		*m_pDismissButton;

private:
	const char	*m_pOkCommand;
	const char	*m_pDismissConvar;
};

}
#endif // DISCLAIMERBOX_H
