#ifndef hl2r_baseoptions_h
#define hl2r_baseoptions_h
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/CheckButton.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ScrollableEditablePanel.h>

// The purpose of all these is to contain instructions to create elements.
// Not contain the elements themselves

enum eElementType
{
	TYPE_NULL = 0,

	TYPE_CVAR_CHECKBOX,
	TYPE_CVAR_RADIOBUTTON,
	TYPE_CVAR_BOXBUTTON,
	TYPE_CVAR_TICKSLIDER,

	TYPE_CVAR_LABEL,
};

//------------------------------------------------------------------------------
// Purpose: This class is for streamlining the process of 
// creating/updating a vgui control into just two functions.
//------------------------------------------------------------------------------
class CControlElement
{
public:
	virtual void Init( Panel *parent )
	{

	}
	virtual void Update( void )
	{

	}
	virtual Panel *GetPanel( void )
	{
		return NULL;
	}
};

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
#define MAX_PARAMETERS			10
#define PARAMETER_MAX_LENGTH	128

struct GenericElement_t
{
	int type;
	char name[128];
	char convar[128];

	char parameters[MAX_PARAMETERS][PARAMETER_MAX_LENGTH];
	CControlElement *element;
};

//------------------------------------------------------------------------------
// Check Buttons
//------------------------------------------------------------------------------
class CConvarCheckButtonElement : public CControlElement
{
public:
	// Initialize
	virtual void Init( Panel *parent )
	{
		checkbutton = new CheckButton(parent, Name, "");

		ConVarRef var(Convar);
		checkbutton->SetSelected(GetStateFromValue(var.GetFloat()));
	}

	// Update ConVar
	virtual void Update()
	{
		ConVarRef var(Convar);
		var.SetValue(GetValueFromState(checkbutton->IsSelected()));
	}

	virtual Panel *GetPanel( void )
	{
		return checkbutton;
	}

	CheckButton *GetButton( void )
	{
		return checkbutton;
	}

	bool GetStateFromValue( float value )
	{
		// Get our value list
		char szValues[8];
		V_strcpy_safe(szValues, Values);

		char *pszToken = strtok( szValues, " " );
		if ( pszToken == NULL )
		{
			DevMsg("Warning! CheckButton [%s] doesn't have a valid value list!\n", Name);
			return false; 
		}

		// First token always contains our "unckecked" value
		if ( atof(pszToken) == value )
			return false;
		
		pszToken = strtok( NULL, " " );

		if ( pszToken == NULL )
		{
			DevMsg("Warning! CheckButton [%s] doesn't have a second elenemt in its value list!\n", Name);
			return false; 
		}

		// Second token always contains our "checked" value
		if ( atof(pszToken) == value )
			return true;

		DevMsg("Warning! CheckButton %s's ConVar is set to an invalid value!\n", Name);
		return false;
	}

	float GetValueFromState( bool state )
	{
		// Get our value list
		char szValues[8];
		V_strcpy_safe(szValues, Values);

		char *pszToken = strtok( szValues, " " );
		if ( pszToken == NULL )
		{
			DevMsg("Warning! CheckButton [%s] doesn't have a valid value list!\n", Name);
			return -1; 
		}

		// First token always contains our "unckecked" value
		if ( !state )
			return atof(pszToken);
		
		pszToken = strtok( NULL, " " );

		if ( pszToken == NULL )
		{
			DevMsg("Warning! CheckButton [%s] doesn't have a second elenemt in its value list!\n", Name);
			return -1; 
		}

		// Second token always contains our "checked" value
		if ( state )
			return atof(pszToken);

		return -1;
	}

public:
	char		Name[32];
	char		Convar[128];
	char		Values[8];

private:
	CheckButton	*checkbutton;
};
//------------------------------------------------------------------------------
// Box Buttons
//------------------------------------------------------------------------------
#if 1

#define MAX_BOX_ELEMENTS 5

#define ELEMENT_NAMES_LENGTH 256
#define ELEMENT_VALUES_LENGTH 32

class CConvarBoxButtonElement : public CControlElement
{
public:
	// Initializes a given BoxButton array.
	virtual void Init( Panel *parent )
	{
		box = new ComboBox(parent, Name, MAX_BOX_ELEMENTS, false);
			
		char szElementNames[ELEMENT_NAMES_LENGTH];
		V_strcpy_safe(szElementNames, ElementNames);

		// Handle elements contained in the box:
		for ( char *pszToken = strtok( szElementNames, " " ); pszToken != NULL; pszToken = strtok( NULL, " " ))
		{
			box->AddItem( pszToken, NULL );
		}
			
		ConVarRef var( Convar );
		int iIndex = GetElementFromValue( var.GetInt() );
		box->ActivateItem( iIndex );
	}
	
	// Updates ConVars related to a given BoxButton array.
	virtual void Update()
	{
		float flValue = GetValueFromElement( box->GetActiveItem() );
			
		ConVarRef var( Convar );
		var.SetValue(flValue);
	}

	virtual Panel *GetPanel( void )
	{
		return box;
	}

	ComboBox *GetBoxButton( void )
	{
		return box;
	}

	// return the element index associated with the inputed value
	int GetElementFromValue( float value )
	{
		// Get element value list
		char szElementValues[ELEMENT_VALUES_LENGTH];
		V_strcpy_safe(szElementValues, ElementValues);

		int iElement = 0;
		for ( char *pszToken = strtok( szElementValues, " " ); pszToken != NULL; pszToken = strtok( NULL, " " ))
		{
			if ( atof(pszToken) == value )
				return iElement;

			iElement++;
		}

		DevMsg("Warning! Value [%f] doesn't have an associated element!\n", value);
		return -1;
	}

	// return the value associated with the inputed element index
	float GetValueFromElement( int element )
	{
		// Get element value list
		char szElementValues[ELEMENT_VALUES_LENGTH];
		V_strcpy_safe(szElementValues, ElementValues);

		int iElement = 0;
		for ( char *pszToken = strtok( szElementValues, " " ); pszToken != NULL; pszToken = strtok( NULL, " " ))
		{
			if ( iElement == element )
				return atof(pszToken);

			iElement++;
		}

		DevMsg("Warning! Element [%i] doesn't have an associated value!\n", element);
		return -1.0f;
	}

public:
	char		Name[32];
	char		Convar[128];
	
	char 		ElementNames[ELEMENT_NAMES_LENGTH];
	char 		ElementValues[ELEMENT_VALUES_LENGTH];

private:
	ComboBox	*box;
};
#endif

//------------------------------------------------------------------------------
// Tick Sliders
//------------------------------------------------------------------------------
class CConvarTickSliderElement : public CControlElement
{
public:
	// Initialize
	virtual void Init( Panel *parent )
	{
		slider = new Slider(parent, Name);

		slider->SetRange(0, numticks - 1); 
		slider->SetNumTicks(numindicators - 1);

		ConVarRef var( Convar );
		int iSelectedTick = (int)RemapVal(var.GetFloat(), min, max, 0, numticks);

		slider->SetValue( iSelectedTick );
	}
	
	// Update ConVar
	virtual void Update()
	{
		ConVarRef var( Convar );
		var.SetValue( RemapVal(slider->GetValue(), 0.0f, numticks - 1, min, max) );
	}

	virtual Panel *GetPanel( void )
	{
		return slider;
	}

	virtual Slider *GetTickSlider( void )
	{
		return slider;
	}

public:
	char		Name[32];
	char		Convar[128];

	int			numticks;		// Number of selectable ticks on the slider.
	int			numindicators;	// Number of visible line indicators on the slider.

	float		min, max; // Value range

private:
	Slider	*slider;
};

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
class CSubPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CSubPanel, vgui::EditablePanel);

public:
	// Constructor
	CSubPanel( vgui::PropertyDialog *parent, GenericElement_t *pElementList ) : EditablePanel(parent, NULL)
	{
		m_GenericElementList = pElementList;
	}

	// Enable the apply button.
	virtual void OnControlModified( void )
	{ 
		PostActionSignal(new KeyValues("ApplyButtonEnable")); 
	}

	// Updates ConVars related to a given GenericElement array.
	virtual void UpdateElements( void )
	{
		if ( m_GenericElementList == NULL )
			return;

		// The number of elements is stored in the 0th index of a GenericElements array.
		int iNumElements = m_GenericElementList[0].type;

		for (int i = 1; i < iNumElements + 1; i++)
			m_GenericElementList[i].element->Update();
	}

	#define CREATE_NEW_ELEMENT(x, y) y *x = new y; V_strcpy_safe(x->Name, element.name); V_strcpy_safe(x->Convar, element.convar);
	virtual void InitElements( Panel *parent, const char *resource )
	{
		if ( m_GenericElementList == NULL )
			return;

		// The number of elements is stored in the 0th index of a GenericElements array.
		int iNumElements = m_GenericElementList[0].type;
		for (int i = 1; i < iNumElements + 1; i++)
		{
			GenericElement_t element = m_GenericElementList[i];
			switch( m_GenericElementList[i].type )
			{
			case TYPE_CVAR_CHECKBOX:
				{
					CREATE_NEW_ELEMENT(checkbutton, CConvarCheckButtonElement);

					// Parameters
					V_strcpy_safe( checkbutton->Values, element.parameters[0] );

					// Init
					checkbutton->Init(parent);
					m_GenericElementList[i].element = checkbutton;
				}
				break;

			case TYPE_CVAR_TICKSLIDER:
				{
					CREATE_NEW_ELEMENT(tickslider, CConvarTickSliderElement);

					// Parameters
					tickslider->numticks =		(int)GetParamValue(element.parameters[0]);
					tickslider->numindicators = (int)GetParamValue(element.parameters[1]);
					tickslider->min = GetParamValue(element.parameters[2]);
					tickslider->max = GetParamValue(element.parameters[3]);

					// Init
					tickslider->Init(parent);
					m_GenericElementList[i].element = tickslider;
				}
				break;

			case TYPE_CVAR_BOXBUTTON:
				{
					CREATE_NEW_ELEMENT(boxbutton, CConvarBoxButtonElement);

					// Parameters
					V_strcpy_safe( boxbutton->ElementNames, element.parameters[0] );
					V_strcpy_safe( boxbutton->ElementValues, element.parameters[1] );

					// Init
					boxbutton->Init(parent);
					m_GenericElementList[i].element = boxbutton;
				}
				break;
			}
		}

		LoadControlSettings( resource );
	}

#define GET_ELEMENT(variable, name) variable = NULL; variable = GetElement(variable, name); variable

// HACKHACKHACKHACKHACK: This is levels of hackiness not seen before by mankind...
// From: https://stackoverflow.com/questions/652815/
#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define GET_ELEMENT_PANEL(type, name) type *MACRO_CONCAT(variable, __LINE__ ); GET_ELEMENT(MACRO_CONCAT(variable, __LINE__ ), name)->GetPanel()

	template <typename T>
	T *GetElement( T *pElement, const char *pName )
	{
		// The number of elements is stored in the 0th index of a GenericElements array.
		int iNumElements = m_GenericElementList[0].type;
		for (int i = 1; i < iNumElements + 1; i++)
		{
			if ( !Q_stricmp( m_GenericElementList[i].name, pName ) )
				return dynamic_cast<T *>(m_GenericElementList[i].element);
		}

		return NULL;
	}

	// Converts an element string into a float, which we can cast as int or bool.
	float GetParamValue( const char *param )
	{
		return atof(param);
	}

	
	void DebugElementList( void )
	{
		// The number of elements is stored in the 0th index of a GenericElements array.
		int iNumElements = m_GenericElementList[0].type;

		DevMsg("Full Element list is:\n\n");
		for (int i = 1; i < iNumElements; i++)
		{
			DevMsg("[%s'n] Parameters are:\n", m_GenericElementList[i].name);
			for (int j = 0; j < 10; j++ )
			{
				DevMsg("	Parameter [%i] is: [%s]\n", j, m_GenericElementList[i].parameters[j]);
			}
			DevMsg("\n");
		}
	}

private:
	// Updates all our control methods.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" ) { UpdateElements(); }

	// Execute OnControlModified() when any action should enable the apply button.
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel) { OnControlModified(); }
	MESSAGE_FUNC_PTR(OnRadioButtonChecked, "RadioButtonChecked", panel) { OnControlModified(); }
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel )				{ OnControlModified(); }
	MESSAGE_FUNC_PTR( OnSliderMoved, "SliderMoved", panel )				{ OnControlModified(); }
	
private:
	GenericElement_t *m_GenericElementList = NULL;
};

#endif