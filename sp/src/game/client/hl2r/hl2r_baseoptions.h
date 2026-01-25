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

#define GetElement(elementname) m_ElementList[ GetElementIndexFromName(elementname) ]
#define GetElementPtr(elementclass, elementname) dynamic_cast<elementclass *>(m_ElementList[GetElementIndexFromName(elementname)].element)
#define GetElementPanel(elementname) GetElement(elementname).element->GetPanel()
#define GetNumElements() m_ElementList[0].type

// The number of elements is stored in the 0th index of a GenericElements array.
#define IterateElements(iterator) for (int iterator = 1; iterator < GetNumElements() + 1; iterator++)

#define START_ELEMENT_ARRAY(name, numelements) GenericElement_t name[] ={{numelements},

#define ADD_CHECKBOX(name, convar, onValue, offValue) { TYPE_CVAR_CHECKBOX,	name, convar,	{onValue, offValue} },
#define ADD_BOXBUTTON(name, convar, dropdownnames, dropdownvalues) { TYPE_CVAR_BOXBUTTON, name, convar,	{dropdownnames, dropdownvalues} },
#define ADD_TICKSLIDER(name, convar, numticks, numindicators, minvalue, maxvalue) { TYPE_CVAR_TICKSLIDER, name, convar,	{numticks, numindicators, minvalue, maxvalue} },

#define END_ELEMENT_ARRAY };


enum eElementType
{
	TYPE_NULL = 0,

	TYPE_CVAR_CHECKBOX,
	TYPE_CVAR_BOXBUTTON,
	TYPE_CVAR_TICKSLIDER,
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
		checkbutton->SetSelected( GetStateFromValue() );
	}

	// Update ConVar
	virtual void Update()
	{
		ConVarRef var(Convar);
		var.SetValue(GetValueFromState());
	}

	virtual Panel *GetPanel( void )
	{
		return checkbutton;
	}

	CheckButton *GetButton( void )
	{
		return checkbutton;
	}

	bool GetStateFromValue( void )
	{
		ConVarRef var(Convar);

		// First token always contains our "unckecked" value
		if ( offvalue == var.GetFloat() )
			return false;

		// Second token always contains our "checked" value
		if ( onvalue == var.GetFloat() )
			return true;

		return false;
	}

	float GetValueFromState( void )
	{
		if ( !checkbutton->IsSelected() )
			return offvalue;

		if ( checkbutton->IsSelected() )
			return onvalue;

		return -1;
	}

public:
	char		Name[32];
	char		Convar[128];
	float		onvalue;
	float		offvalue;

private:
	CheckButton	*checkbutton;
};
//------------------------------------------------------------------------------
// Box Buttons
//------------------------------------------------------------------------------
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
		box->ActivateItem( GetElementFromValue() );
	}
	
	// Updates ConVars related to a given BoxButton array.
	virtual void Update()
	{
		ConVarRef var( Convar );
		var.SetValue(GetValueFromElement());
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
	int GetElementFromValue( void )
	{
		ConVarRef var( Convar );

		// Get element value list
		char szElementValues[ELEMENT_VALUES_LENGTH];
		V_strcpy_safe(szElementValues, ElementValues);

		int iElement = 0;
		for ( char *pszToken = strtok( szElementValues, " " ); pszToken != NULL; pszToken = strtok( NULL, " " ))
		{
			if ( atof(pszToken) == var.GetFloat() )
				return iElement;

			iElement++;
		}

		DevMsg("Warning! Value [%f] doesn't have an associated element!\n", var.GetFloat());
		return -1;
	}

	// return the value associated with the inputed element index
	float GetValueFromElement( void )
	{
		// Get element value list
		char szElementValues[ELEMENT_VALUES_LENGTH];
		V_strcpy_safe(szElementValues, ElementValues);

		int iElement = 0;
		for ( char *pszToken = strtok( szElementValues, " " ); pszToken != NULL; pszToken = strtok( NULL, " " ))
		{
			if ( iElement ==  box->GetActiveItem() )
				return atof(pszToken);

			iElement++;
		}

		DevMsg("Warning! Element [%i] doesn't have an associated value!\n",  box->GetActiveItem());
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
		slider->SetValue( GetSliderPosFromValue() );
	}
	
	// Update ConVar
	virtual void Update()
	{
		ConVarRef var( Convar );
		var.SetValue( GetValueFromSliderPos() );
	}

	virtual Panel *GetPanel( void )
	{
		return slider;
	}

	virtual Slider *GetTickSlider( void )
	{
		return slider;
	}

	float GetValueFromSliderPos( void )
	{
		return RemapVal(slider->GetValue(), 0.0f, numticks - 1, min, max);
	}

	int GetSliderPosFromValue( void )
	{
		ConVarRef var( Convar );
		return RemapVal(var.GetFloat(), min, max, 0, numticks);
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
		m_ElementList = pElementList;
	}

	// Enable the apply button.
	virtual void OnControlModified( void )
	{ 
		PostActionSignal(new KeyValues("ApplyButtonEnable")); 
	}

	// Updates ConVars related to a given GenericElement array.
	virtual void UpdateElements( void )
	{
		if ( m_ElementList == NULL )
			return;

		IterateElements(i)
			m_ElementList[i].element->Update();
	}

	virtual void InitElements( Panel *parent, const char *resource )
	{
		if ( m_ElementList == NULL )
			return;

		IterateElements(i)
		{
			GenericElement_t element = m_ElementList[i];
			switch( m_ElementList[i].type )
			{
			case TYPE_CVAR_CHECKBOX:
				{
					CConvarCheckButtonElement *checkbutton = new CConvarCheckButtonElement; 
					V_strcpy_safe(checkbutton->Name, element.name); 
					V_strcpy_safe(checkbutton->Convar, element.convar);

					// Parameters
					checkbutton->offvalue = GetParamValue(element.parameters[0]);
					checkbutton->onvalue = GetParamValue(element.parameters[1]);

					// Init
					checkbutton->Init(parent);
					m_ElementList[i].element = checkbutton;
				}
				break;

			case TYPE_CVAR_TICKSLIDER:
				{
					CConvarTickSliderElement *tickslider = new CConvarTickSliderElement; 
					V_strcpy_safe(tickslider->Name, element.name); 
					V_strcpy_safe(tickslider->Convar, element.convar);

					// Parameters
					tickslider->numticks =		(int)GetParamValue(element.parameters[0]);
					tickslider->numindicators = (int)GetParamValue(element.parameters[1]);
					tickslider->min = GetParamValue(element.parameters[2]);
					tickslider->max = GetParamValue(element.parameters[3]);

					// Init
					tickslider->Init(parent);
					m_ElementList[i].element = tickslider;
				}
				break;

			case TYPE_CVAR_BOXBUTTON:
				{
					CConvarBoxButtonElement *boxbutton = new CConvarBoxButtonElement; 
					V_strcpy_safe(boxbutton->Name, element.name); 
					V_strcpy_safe(boxbutton->Convar, element.convar);

					// Parameters
					V_strcpy_safe( boxbutton->ElementNames, element.parameters[0] );
					V_strcpy_safe( boxbutton->ElementValues, element.parameters[1] );

					// Init
					boxbutton->Init(parent);
					m_ElementList[i].element = boxbutton;
				}
				break;
			}
		}

		LoadControlSettings( resource );
	}

	int GetElementIndexFromName( const char *pName )
	{
		IterateElements(i)
		{
			if ( !Q_stricmp( m_ElementList[i].name, pName ) )
				return i;
		}

		return NULL;
	}

	// Converts an element string into a float, which we can cast as int or bool.
	float GetParamValue( const char *param )
	{
		return atof(param);
	}

private:
	// Updates all our control methods.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" ) { UpdateElements(); }

	// Execute OnControlModified() when any action should enable the apply button.
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel) { OnControlModified(); }
	MESSAGE_FUNC_PTR(OnRadioButtonChecked, "RadioButtonChecked", panel) { OnControlModified(); }
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel )				{ OnControlModified(); }
	MESSAGE_FUNC_PTR( OnSliderMoved, "SliderMoved", panel )				{ OnControlModified(); }
	
public:
	GenericElement_t *m_ElementList = NULL;
};

#endif