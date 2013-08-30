
#include "cbase.h"
#include "deferred/deferred_shared_common.h"
#include "deferred/vgui/vgui_deferred.h"

#include "vgui_controls/label.h"
#include "vgui_controls/button.h"
#include "vgui_controls/combobox.h"
#include "vgui_controls/checkbutton.h"
#include "vgui_controls/slider.h"
#include "vgui_controls/panellistpanel.h"
#include "vgui_controls/scrollbar.h"
#include "vgui_controls/textentry.h"
#include "matsys_controls/colorpickerpanel.h"

using namespace vgui;

class VisibilityClient
{
public:
	virtual bool ShouldDraw() = 0;
	virtual VisibilityClient *ShallowCopy() = 0;
};

class Visibility_CheckButton : public VisibilityClient
{
public:
	Visibility_CheckButton( CheckButton *pTarget )
	{
		m_pTarget = pTarget;
	};

	virtual bool ShouldDraw()
	{
		return m_pTarget &&
			m_pTarget->IsSelected();
	};

	virtual VisibilityClient *ShallowCopy()
	{
		return new Visibility_CheckButton( *this );
	};
private:
	CheckButton *m_pTarget;
};

class Visibility_ComboBox : public VisibilityClient
{
public:
	Visibility_ComboBox( ComboBox *pTarget, int iValue )
	{
		m_pTarget = pTarget;
		m_iValue = iValue;
	};

	virtual bool ShouldDraw()
	{
		return m_pTarget &&
			m_pTarget->GetActiveItem() == m_iValue;
	};

	virtual VisibilityClient *ShallowCopy()
	{
		return new Visibility_ComboBox( *this );
	};
private:
	ComboBox *m_pTarget;
	int m_iValue;
};

class PropertyClient
{
public:
	PropertyClient( KeyValues *pValue )
	{
		m_pValue = pValue;
		m_bMultiSelection = false;
		*m_szLabelName = 0;
		m_pLabel = NULL;
	};

	virtual ~PropertyClient()
	{
		delete m_pLabel;
		m_pValue->deleteThis();
		m_hVisRules.PurgeAndDeleteElements();
		m_hEnabledRules.PurgeAndDeleteElements();
	};

	void CopyVisiblityRule( VisibilityClient *pRule )
	{
		m_hVisRules.AddToTail( pRule->ShallowCopy() );
	};

	void CopyEnabledRule( VisibilityClient *pRule )
	{
		m_hEnabledRules.AddToTail( pRule->ShallowCopy() );
	};

	bool ShouldDraw()
	{
		for ( int i = 0; i < m_hVisRules.Count(); i++ )
			if ( !m_hVisRules[i]->ShouldDraw() )
				return false;
		return true;
	};

	bool ShouldBeEnabled()
	{
		for ( int i = 0; i < m_hEnabledRules.Count(); i++ )
			if ( !m_hEnabledRules[i]->ShouldDraw() )
				return false;
		return true;
	};

	virtual KeyValues *GetValue()
	{
		return m_pValue;
	};

	virtual void OnValuesChanged( KeyValues **pValues, int iCount )
	{
		bool bFoundAnything = false;
		bool bWasMultiSelected = m_bMultiSelection;
		m_bMultiSelection = false;

		for ( int i = 0; i < iCount; i++ )
		{
			bool bFoundValue = false;
			for ( KeyValues *pValue = pValues[i]->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
			{
				if ( !Q_stricmp( m_pValue->GetName(), pValue->GetName() ) )
				{
					if ( i > 0 )
					{
						if ( Q_stricmp( m_pValue->GetString(), pValue->GetString() ) )
							m_bMultiSelection = true;
					}

					m_pValue->deleteThis();
					m_pValue = pValue->MakeCopy();
					bFoundAnything = true;
					bFoundValue = true;
					break;
				}
			}

			if ( !bFoundValue && bFoundAnything )
				m_bMultiSelection = true;
		}

		if ( !bFoundAnything )
			m_bMultiSelection = bWasMultiSelected;

		ReadValue();
	};

	bool IsMultiSelected()
	{
		return m_bMultiSelection;
	};

	void ResetMultiSelect()
	{
		m_bMultiSelection = false;
	};

	const Color GetWarningColor()
	{
		return Color( 200, 64, 64, 196 );
	};

	void SetLabelName( const char *pszName )
	{
		if ( m_pLabel == NULL )
			m_pLabel = new Label(NULL,"","");
		m_pLabel->SetText( pszName );
	};

	Label *GetLabel()
	{
		return m_pLabel;
	};

	virtual void ReadValue() = 0;
	virtual void WriteValue() = 0;

	void OnPropertyChanged()
	{
		Panel *panel = dynamic_cast<Panel*>(this);
		Assert(panel);

		panel->PostActionSignal( new KeyValues( "PropertyChanged" ) );
	};

private:
	KeyValues *m_pValue;

	bool m_bMultiSelection;
	char m_szLabelName[32];
	Label *m_pLabel;

	CUtlVector< VisibilityClient* > m_hVisRules;
	CUtlVector< VisibilityClient* > m_hEnabledRules;
};

class PropertyComboBox : public ComboBox, public PropertyClient
{
public:
	PropertyComboBox( KeyValues *prop )
		: ComboBox( NULL, "", 10, false ), PropertyClient( prop )
	{
		m_iLastActivated = 0;
	};

	virtual void ReadValue()
	{
		ActivateItem( GetValue()->GetInt() );
		m_iLastActivated = GetValue()->GetInt();
	};

	virtual void WriteValue()
	{
		GetValue()->SetInt( NULL, GetActiveItem() );
	};

	void OnMenuItemSelected()
	{
		// >_>
		int id = GetActiveItem();
		bool bUpdate = id != m_iLastActivated;

		ComboBox::OnMenuItemSelected();

		// <_<
		if ( bUpdate )
		{
			m_iLastActivated = id;

			WriteValue();
			OnPropertyChanged();
		}
	};

	Color GetBgColor()
	{
		if ( IsMultiSelected() )
			return GetWarningColor();
		return ComboBox::GetBgColor();
	};

private:
	int m_iLastActivated;
};

class PropertyCheckButton : public CheckButton, public PropertyClient
{
public:
	PropertyCheckButton( KeyValues *prop, int iFlag, const char *pszText )
		: CheckButton( NULL, "", pszText ), PropertyClient( prop )
	{
		m_iFlag = iFlag;
		m_bWasChecked = false;
	};

	virtual void ReadValue()
	{
		SetSelected( ( GetValue()->GetInt() & m_iFlag ) != 0 );
		m_bWasChecked = IsSelected();
	};

	virtual void WriteValue()
	{
		int iValue = GetValue()->GetInt();
		if ( IsSelected() )
			iValue |= m_iFlag;
		else
			iValue &= ~m_iFlag;
		GetValue()->SetInt( NULL, iValue );
	};

	void OnCheckButtonChecked()
	{
		WriteValue();
		OnPropertyChanged();
	};

	void OnThink()
	{
		// >_>_>_>_>_>
		if ( IsSelected() != m_bWasChecked )
		{
			OnCheckButtonChecked();
			m_bWasChecked = IsSelected();
		}
	};

	Color GetFgColor()
	{
		if ( IsMultiSelected() )
			return GetWarningColor();
		return CheckButton::GetFgColor();
	};

private:
	int m_iFlag;
	bool m_bWasChecked;
};

class PropertyTextEntry : public TextEntry, public PropertyClient
{
public:
	enum PTENTRY_MODE
	{
		PTENTRY_FLOAT = 0,
		PTENTRY_INT,
		PTENTRY_STRING,
	};

	PropertyTextEntry( KeyValues *prop, PTENTRY_MODE mode, bool bAcceptChars = false )
		: TextEntry( NULL, "" ), PropertyClient( prop )
	{
		m_Mode = mode;

		bHasMin = false;
		bHasMax = false;

		if ( !bAcceptChars )
			SetAllowNumericInputOnly( true );
	};

	virtual void ReadValue()
	{
		switch ( m_Mode )
		{
		case PTENTRY_FLOAT:
			SetText( VarArgs( "%f", GetValue()->GetFloat() ) );
			break;
		case PTENTRY_INT:
			SetText( VarArgs( "%i", GetValue()->GetInt() ) );
			break;
		case PTENTRY_STRING:
			SetText( GetValue()->GetString() );
			break;
		}
	};

	virtual void WriteValue()
	{
		char text[ 256 ];
		GetText( text, sizeof(text) );

		if ( !*text )
		{
			Q_snprintf( text, sizeof( text ), "0" );
		}

		switch ( m_Mode )
		{
		case PTENTRY_FLOAT:
			{
				float flData = atof( text );

				if ( bHasMin )
					flData = MAX( flMin, flData );
				if ( bHasMax )
					flData = MIN( flMax, flData );

				GetValue()->SetFloat( NULL, flData );
			}
			break;
		case PTENTRY_INT:
			{
				int iData = atoi( text );

				if ( bHasMin )
					iData = MAX( iMin, iData );
				if ( bHasMax )
					iData = MIN( iMax, iData );

				GetValue()->SetInt( NULL, iData );
			}
			break;
		case PTENTRY_STRING:
			GetValue()->SetString( NULL, text );
			break;
		}
	};

	void FireActionSignal()
	{
		TextEntry::FireActionSignal();
		WriteValue();
		OnPropertyChanged();
	};

	Color GetBgColor()
	{
		if ( IsMultiSelected() )
			return GetWarningColor();
		return TextEntry::GetBgColor();
	};

	void SetBoundsFloat( bool bMin, float flMin, bool bMax = false, float flMax = 0 )
	{
		Assert( m_Mode == PTENTRY_FLOAT );

		bHasMin = bMin;
		bHasMax = bMax;

		this->flMin = flMin;
		this->flMax = flMax;
	};

	void SetBoundsInt( bool bMin, int iMin, bool bMax = false, int iMax = 0 )
	{
		Assert( m_Mode == PTENTRY_INT );

		bHasMin = bMin;
		bHasMax = bMax;

		this->iMin = iMin;
		this->iMax = iMax;
	};

private:
	PTENTRY_MODE m_Mode;

	float flMin, flMax;
	int iMin, iMax;
	bool bHasMin, bHasMax;
};

class PropertySlider : public Slider, public PropertyClient
{
public:
	PropertySlider( KeyValues *prop, int iFloatScale = -1 )
		: Slider( NULL, "" ), PropertyClient( prop )
	{
		m_bFloat = iFloatScale > 0;
		m_flFloatScale = iFloatScale;
		m_iComponentIndex = 0;
		m_iComponentCount = 1;

		Assert( !m_bFloat || m_flFloatScale > 0 );
	};

	void SetComponentIndex( int numComponents, int index )
	{
		m_iComponentCount = numComponents;
		m_iComponentIndex = index;
	};

	virtual void ReadValue()
	{
		Assert( m_iComponentCount >= 1 && m_iComponentCount <= 4 );

		if ( m_bFloat )
		{
			float c[4] = {0};
			UTIL_StringToFloatArray( c, m_iComponentCount, PropertyClient::GetValue()->GetString() );
			Slider::SetValue( c[m_iComponentIndex] * m_flFloatScale, false );
		}
		else
		{
			int c[4] = {0};
			UTIL_StringToIntArray( c, m_iComponentCount, PropertyClient::GetValue()->GetString() );
			Slider::SetValue( c[m_iComponentIndex], false );
		}
	};

	virtual void WriteValue()
	{
		Assert( m_iComponentCount >= 1 && m_iComponentCount <= 4 );
		char tmp[128] = {0};

		if ( m_bFloat )
		{
			float c[4] = {0};
			UTIL_StringToFloatArray( c, m_iComponentCount, PropertyClient::GetValue()->GetString() );
			c[m_iComponentIndex] = Slider::GetValue() / m_flFloatScale;

			for ( int i = 0; i < m_iComponentCount; i++ )
			{
				Q_strcat( tmp, VarArgs( "%f", c[i] ), sizeof(tmp) );
				if ( i < m_iComponentCount - 1 )
					Q_strcat( tmp, " ", sizeof(tmp) );
			}
		}
		else
		{
			int c[4] = {0};
			UTIL_StringToIntArray( c, m_iComponentCount, PropertyClient::GetValue()->GetString() );
			c[m_iComponentIndex] = Slider::GetValue();

			for ( int i = 0; i < m_iComponentCount; i++ )
			{
				Q_strcat( tmp, VarArgs( "%i", c[i] ), sizeof(tmp) );
				if ( i < m_iComponentCount - 1 )
					Q_strcat( tmp, " ", sizeof(tmp) );
			}
		}

		PropertyClient::GetValue()->SetString( NULL, tmp );
	};

	virtual void SendSliderMovedMessage()
	{
		Slider::SendSliderMovedMessage();
		WriteValue();
		OnPropertyChanged();
	};

	Color GetBgColor()
	{
		if ( IsMultiSelected() )
			return GetWarningColor();
		return Slider::GetBgColor();
	};

private:
	bool m_bFloat;
	float m_flFloatScale;
	int m_iComponentIndex;
	int m_iComponentCount;
};

class PropertyColorPicker : public Panel, public PropertyClient
{
	DECLARE_CLASS_SIMPLE( PropertyColorPicker, Panel );

public:
	PropertyColorPicker( KeyValues *prop ): 
	Panel( NULL, "" ),
	PropertyClient( prop ),
	m_pButton( new Button( this, "OpenPicker", "Choose", this, "OpenPicker" ) ),
	m_pPreviewPanel( new Panel( this ) ),
	m_pColorPicker(static_cast<CColorPickerFrame*>(0))
	{
		m_pButton->SetDragEnabled( false );
		m_pButton->SetDropEnabled( false );
	};

	MESSAGE_FUNC_PARAMS( OnPreview, "ColorPickerPreview", data )
	{
		Color newColor( data->GetColor( "color" ) );
		newColor[3] = 255;

		m_pPreviewPanel->SetBgColor( newColor );

		m_pValues[0] = newColor.r();
		m_pValues[1] = newColor.g();
		m_pValues[2] = newColor.b();

		WriteValue();

		m_pPreviewPanel->SetBgColor( Color( m_pValues[0], m_pValues[1], m_pValues[2], 255 ) );
		OnPropertyChanged();
	}

	MESSAGE_FUNC_PARAMS( OnPicked, "ColorPickerPicked", data )
	{
		m_pButton->SetEnabled( true );
		
		Color newColor( data->GetColor( "color" ) );
		newColor[3] = 255;

		m_pPreviewPanel->SetBgColor( newColor );

		m_pValues[0] = newColor.r();
		m_pValues[1] = newColor.g();
		m_pValues[2] = newColor.b();

		WriteValue();

		OnPropertyChanged();
	}

	MESSAGE_FUNC( OnCancelled, "ColorPickerCancel" )
	{
		m_pButton->SetEnabled( true );
		
		m_pValues[0] = m_pOldValues[0];
		m_pValues[1] = m_pOldValues[1];
		m_pValues[2] = m_pOldValues[2];
		m_pValues[3] = m_pOldValues[3];

		WriteValue();

		m_pPreviewPanel->SetBgColor( Color( m_pValues[0], m_pValues[1], m_pValues[2], 255 ) );

		OnPropertyChanged();
	}

	void OnCommand( const char *command )
	{
		if( !Q_strcmp( command, "OpenPicker" ) )
		{
			ReadValue();

			m_pButton->SetEnabled( false );

			CColorPickerFrame* pColorPickerFrame = new CColorPickerFrame( this, "Select Color" ); 
			//pColorPickerFrame->SetName( "ColorPicker" );
			pColorPickerFrame->AddActionSignalTarget( this );
			pColorPickerFrame->DoModal( Color( m_pOldValues[0], m_pOldValues[1], m_pOldValues[2], 255 ) );
		}
		else
		{
			Panel::OnCommand( command );
		}
	}

	void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings( pScheme );

		Color bgColor( PropertyClient::GetValue()->GetColor() );
		bgColor[3] = 255;

		m_pPreviewPanel->SetBgColor( bgColor );

		m_pPreviewPanel->SetPos( m_pButton->GetWide() + 2, 0 );

		m_pPreviewPanel->SetBorder( pScheme->GetBorder( "GenericPanelListBorder" ) );		
	}

	virtual void ReadValue()
	{
		UTIL_StringToIntArray( m_pValues, 4, PropertyClient::GetValue()->GetString() );

		//could use the sse intrinsics here to do this all in one
		m_pOldValues[0] = m_pValues[0];
		m_pOldValues[1] = m_pValues[1];
		m_pOldValues[2] = m_pValues[2];
		m_pOldValues[3] = m_pValues[3];
	}

	virtual void WriteValue()
	{
		PropertyClient::GetValue()->SetString
		( 
			NULL,
			VarArgs
			( 
				"%i %i %i %i", 
				m_pValues[0], 
				m_pValues[1], 
				m_pValues[2], 
				m_pValues[3]
			) 
		);
	};

	Color GetBgColor()
	{
		if ( IsMultiSelected() )
			return GetWarningColor();
		return Panel::GetBgColor();
	};

private:
	Button*	m_pButton;
	Panel*	m_pPreviewPanel;
	CColorPickerFrame* m_pColorPicker;

	int		m_pValues[4];
	int		m_pOldValues[4];
};

CVGUILightEditor_Properties::CVGUILightEditor_Properties( Panel *pParent )
	: BaseClass( pParent, "LightEditorProperties" )
{
	SetMinimumSize( 300, 100 );
	SetPropertyMode( PROPERTYMODE_LIGHT );

	m_pListRoot = new PanelListPanel( this, "list_root" );
	m_pListRoot->SetShowScrollBar( true );
	m_pListRoot->SetNumColumns( 1 );
	m_pListRoot->SetFirstColumnWidth( 88 );
	m_pListRoot->OverrideChildPanelWidth( true );

	LoadControlSettings( "resource/deferred/lighteditor_properties.res" );

	SetTitle( "Properties", false );

	CreateProperties();

	MakeReadyForUse();

	int sw, sh;
	engine->GetScreenSize( sw, sh );

	int w, h;
	w = 300;
	h = sh  * 0.8f;
	SetSize( w, h );
	SetPos( sw - w - 5, sh / 2 - h / 2 );
}

CVGUILightEditor_Properties::~CVGUILightEditor_Properties()
{
	FlushProperties();
}

CVGUILightEditor_Properties::PROPERTYMODE CVGUILightEditor_Properties::GetPropertyMode()
{
	return m_iPropMode;
}

void CVGUILightEditor_Properties::OnRequestPropertyLayout( PROPERTYMODE mode )
{
	if ( GetPropertyMode() == mode )
		return;

	SetPropertyMode( mode );

	CreateProperties();
}

void CVGUILightEditor_Properties::SetPropertyMode( PROPERTYMODE mode )
{
	m_iPropMode = mode;
}

void CVGUILightEditor_Properties::OnSizeChanged( int newWide, int newTall )
{
	BaseClass::OnSizeChanged( newWide, newTall );

	int minx, miny;
	GetMinimumSize( minx, miny );

	// >_>
	if ( newWide != minx )
		SetWide( minx );
}

void CVGUILightEditor_Properties::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CVGUILightEditor_Properties::FlushProperties()
{
	m_pListRoot->RemoveAll();
	m_hProperties.PurgeAndDeleteElements();
}

void CVGUILightEditor_Properties::CreateProperties()
{
	FlushProperties();

	if ( GetPropertyMode() == PROPERTYMODE_LIGHT )
		CreateProperties_LightEntity();
	else
		CreateProperties_GlobalLight();

	for ( int i = 0; i < m_hProperties.Count(); i++ )
		m_hProperties[i]->AddActionSignalTarget( this );

	UpdatePropertyVisibility();

	PropertiesReadAll();
}

void CVGUILightEditor_Properties::OnPropertyChanged( Panel *panel )
{
	PropertyClient *pActivator = dynamic_cast< PropertyClient* >( panel );
	Assert( pActivator != NULL );

	KeyValues *pChangeValues = new KeyValues("");

	for ( int i = 0; i < m_hProperties.Count(); i++ )
	{
		if ( panel == m_hProperties[i] )
			continue;

		PropertyClient *pClient = dynamic_cast< PropertyClient* >( m_hProperties[i] );
		Assert( pClient != NULL );

		if ( pClient->IsMultiSelected() )
			continue;

		pChangeValues->SetString( pClient->GetValue()->GetName(),
			pClient->GetValue()->GetString() );
	}

	pChangeValues->SetString( pActivator->GetValue()->GetName(),
			pActivator->GetValue()->GetString() );

	CLightingEditor *pEditor = GetLightingEditor();

	if ( GetPropertyMode() == PROPERTYMODE_LIGHT )
		pEditor->ApplyKVToSelection( pChangeValues );
	else
		pEditor->ApplyKVToGlobalLight( pChangeValues );

	for ( int i = 0; i < m_hProperties.Count(); i++ )
	{
		PropertyClient *pClient = dynamic_cast< PropertyClient* >( m_hProperties[i] );
		Assert( pClient != NULL );

		if ( panel == m_hProperties[i] )
		{
			pClient->ResetMultiSelect();
			continue;
		}

		pClient->OnValuesChanged( &pChangeValues, 1 );
	}

	pChangeValues->deleteThis();

	UpdatePropertyVisibility();

	SendPropertiesToRoot();
}

void CVGUILightEditor_Properties::OnFileLoaded()
{
	RefreshGlobalLightData();
}

void CVGUILightEditor_Properties::RefreshGlobalLightData()
{
	if ( GetPropertyMode() == PROPERTYMODE_GLOBAL )
	{
		KeyValues *pKVGlobal = GetLightingEditor()->GetKVGlobalLight();

		if ( !pKVGlobal )
			return;

		FOR_EACH_VEC( m_hProperties, i )
		{
			PropertyClient *pClient = dynamic_cast< PropertyClient* >( m_hProperties[i] );
			pClient->OnValuesChanged( &pKVGlobal, 1 );
		}
	}
}

void CVGUILightEditor_Properties::OnLightSelectionChanged()
{
	OnRequestPropertyLayout( PROPERTYMODE_LIGHT );

	CLightingEditor *pEditor = GetLightingEditor();

	CUtlVector< KeyValues* > hKeyValues;

	pEditor->GetKVFromSelection( hKeyValues );

	if ( hKeyValues.Count() <= 0 )
		return;

	FOR_EACH_VEC( m_hProperties, i )
	{
		PropertyClient *pClient = dynamic_cast< PropertyClient* >( m_hProperties[i] );
		pClient->OnValuesChanged( hKeyValues.Base(), hKeyValues.Count() );
	}

	FOR_EACH_VEC( hKeyValues, i )
		hKeyValues[ i ]->deleteThis();

	UpdatePropertyVisibility();

	SendPropertiesToRoot();
}

void CVGUILightEditor_Properties::SendPropertiesToRoot()
{
	KeyValues *pActiveKV = AllocKVFromVisible();
	KeyValues *pMsg = new KeyValues( "PropertiesChanged_Light" );
	pMsg->SetPtr( "properties", pActiveKV );
	PostActionSignal( pMsg );
}

static KeyValues *MakeKey( const char *pszName, const char *pszValue )
{
	KeyValues *pDummy = new KeyValues( "" );
	pDummy->SetString( pszName, pszValue );
	KeyValues *pRet = pDummy->GetFirstValue();
	pDummy->RemoveSubKey( pRet );
	return pRet;
}

void CVGUILightEditor_Properties::CreateProperties_LightEntity()
{
	PropertyTextEntry *pTextEntry = NULL;
	PropertyCheckButton *pCheckButton = NULL;
	PropertySlider *pSlider = NULL;
	PropertyComboBox *pComboBox = NULL;
	PropertyColorPicker* pColorPickerPanel = NULL;

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_ORIGIN ), "0 0 0" ),
		PropertyTextEntry::PTENTRY_STRING, true );
	pTextEntry->SetLabelName( "Origin" );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_ANGLES ), "0 0 0" ),
		PropertyTextEntry::PTENTRY_STRING, true );
	pTextEntry->SetLabelName( "Angles" );
	m_hProperties.AddToTail( pTextEntry );

	pComboBox = new PropertyComboBox( MakeKey( GetLightParamName( LPARAM_LIGHTTYPE ), "0" ) );
	pComboBox->AddItem( "Point", NULL );
	pComboBox->AddItem( "Spot", NULL );
	m_hProperties.AddToTail( pComboBox );

	Visibility_ComboBox vis_spotlight = Visibility_ComboBox( pComboBox, DEFLIGHTTYPE_SPOT );

	pColorPickerPanel = new PropertyColorPicker( MakeKey( GetLightParamName( LPARAM_DIFFUSE ), "255 255 255 255" ) );
	pColorPickerPanel->SetLabelName( "Diffuse" );
	m_hProperties.AddToTail( pColorPickerPanel );

	pSlider = new PropertySlider( MakeKey( GetLightParamName( LPARAM_DIFFUSE ), "255 255 255 255" ) );
	pSlider->SetLabelName( "Diffuse Intensity" );
	pSlider->SetRange( 0, 1275 );
	pSlider->SetNumTicks( 10 );
	pSlider->SetComponentIndex( 4, 3 );
	m_hProperties.AddToTail( pSlider );

	pColorPickerPanel = new PropertyColorPicker( MakeKey( GetLightParamName( LPARAM_AMBIENT ), "255 255 255 0" ) );
	pColorPickerPanel->SetLabelName( "Ambient" );
	m_hProperties.AddToTail( pColorPickerPanel );

	pSlider = new PropertySlider( MakeKey( GetLightParamName( LPARAM_AMBIENT ), "255 255 255 0" ) );
	pSlider->SetLabelName( "Ambient Intensity" );
	pSlider->SetRange( 0, 1275 );
	pSlider->SetNumTicks( 10 );
	pSlider->SetComponentIndex( 4, 3 );
	m_hProperties.AddToTail( pSlider );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_RADIUS ), "256" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Radius" );
	m_hProperties.AddToTail( pTextEntry );

	pSlider = new PropertySlider(
		MakeKey( GetLightParamName( LPARAM_RADIUS ), "256" ) );
	pSlider->SetRange( 0, 1024 );
	pSlider->SetNumTicks( 10 );
	m_hProperties.AddToTail( pSlider );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_POWER ), "2" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Falloff" );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_SPOTCONE_INNER ), "35" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Cone inner" );
	pTextEntry->CopyVisiblityRule( &vis_spotlight );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_SPOTCONE_OUTER ), "45" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Cone outer" );
	pTextEntry->CopyVisiblityRule( &vis_spotlight );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_VIS_DIST ), "2048" ),
		PropertyTextEntry::PTENTRY_INT );
	pTextEntry->SetLabelName( "Vis dist" );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_VIS_RANGE ), "512" ),
		PropertyTextEntry::PTENTRY_INT );
	pTextEntry->SetLabelName( "Vis range" );
	m_hProperties.AddToTail( pTextEntry );


	pCheckButton = new PropertyCheckButton(
		MakeKey( GetLightParamName( LPARAM_SPAWNFLAGS ), "1" ),
		DEFLIGHT_SHADOW_ENABLED, "Enable shadows" );
	m_hProperties.AddToTail( pCheckButton );

	Visibility_CheckButton vis_shadows( pCheckButton );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_SHADOW_DIST ), "1536" ),
		PropertyTextEntry::PTENTRY_INT );
	pTextEntry->SetLabelName( "Shadow dist" );
	pTextEntry->CopyVisiblityRule( &vis_shadows );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_SHADOW_RANGE ), "512" ),
		PropertyTextEntry::PTENTRY_INT );
	pTextEntry->SetLabelName( "Shadow range" );
	pTextEntry->CopyVisiblityRule( &vis_shadows );
	m_hProperties.AddToTail( pTextEntry );

	pCheckButton = new PropertyCheckButton(
		MakeKey( GetLightParamName( LPARAM_SPAWNFLAGS ), "1" ),
		DEFLIGHT_VOLUMETRICS_ENABLED, "Enable volumetrics" );
	pCheckButton->CopyEnabledRule( &vis_shadows );
	m_hProperties.AddToTail( pCheckButton );

#if DEFCFG_ADAPTIVE_VOLUMETRIC_LOD || DEFCFG_CONFIGURABLE_VOLUMETRIC_LOD
	Visibility_CheckButton vis_volume( pCheckButton );
#endif

	pCheckButton = new PropertyCheckButton(
		MakeKey( GetLightParamName( LPARAM_SPAWNFLAGS ), "1" ),
		DEFLIGHT_COOKIE_ENABLED, "Enable cookie" );
	m_hProperties.AddToTail( pCheckButton );

	Visibility_CheckButton vis_cookie( pCheckButton );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_COOKIETEX ), "" ),
		PropertyTextEntry::PTENTRY_STRING, true );
	pTextEntry->SetLabelName( "Cookie name" );
	pTextEntry->CopyVisiblityRule( &vis_cookie );
	m_hProperties.AddToTail( pTextEntry );


	pCheckButton = new PropertyCheckButton(
		MakeKey( GetLightParamName( LPARAM_SPAWNFLAGS ), "1" ),
		DEFLIGHT_LIGHTSTYLE_ENABLED, "Enable lightstyle" );
	m_hProperties.AddToTail( pCheckButton );

	Visibility_CheckButton vis_style( pCheckButton );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_STYLE_SEED ), "-1" ),
		PropertyTextEntry::PTENTRY_INT );
	pTextEntry->SetLabelName( "Seed" );
	pTextEntry->CopyVisiblityRule( &vis_style );
	pTextEntry->SetBoundsInt( true, 0, true, 65536 );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_STYLE_AMT ), "0.5" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Amount" );
	pTextEntry->CopyVisiblityRule( &vis_style );
	pTextEntry->SetBoundsFloat( true, 0, true, 1 );
	m_hProperties.AddToTail( pTextEntry );

	pSlider = new PropertySlider(
		MakeKey( GetLightParamName( LPARAM_STYLE_AMT ), "0.5" ),
		100 );
	pSlider->SetRange( 0, 100 );
	pSlider->SetNumTicks( 5 );
	pSlider->CopyVisiblityRule( &vis_style );
	m_hProperties.AddToTail( pSlider );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_STYLE_SPEED ), "10" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Speed" );
	pTextEntry->CopyVisiblityRule( &vis_style );
	pTextEntry->SetBoundsFloat( true, 0, false );
	m_hProperties.AddToTail( pTextEntry );

	pSlider = new PropertySlider(
		MakeKey( GetLightParamName( LPARAM_STYLE_SPEED ), "10" ),
		100 );
	pSlider->SetRange( 0, 5000 );
	pSlider->SetNumTicks( 10 );
	pSlider->CopyVisiblityRule( &vis_style );
	m_hProperties.AddToTail( pSlider );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_STYLE_SMOOTH ), "0.5" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Smooth" );
	pTextEntry->CopyVisiblityRule( &vis_style );
	pTextEntry->SetBoundsFloat( true, 0, true, 1 );
	m_hProperties.AddToTail( pTextEntry );

	pSlider = new PropertySlider(
		MakeKey( GetLightParamName( LPARAM_STYLE_SMOOTH ), "0.5" ),
		100 );
	pSlider->SetRange( 0, 100 );
	pSlider->SetNumTicks( 5 );
	pSlider->CopyVisiblityRule( &vis_style );
	m_hProperties.AddToTail( pSlider );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_STYLE_RANDOM ), "0.5" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Random" );
	pTextEntry->CopyVisiblityRule( &vis_style );
	pTextEntry->SetBoundsFloat( true, 0, true, 1 );
	m_hProperties.AddToTail( pTextEntry );

	pSlider = new PropertySlider(
		MakeKey( GetLightParamName( LPARAM_STYLE_RANDOM ), "0.5" ),
		100 );
	pSlider->SetRange( 0, 100 );
	pSlider->SetNumTicks( 5 );
	pSlider->CopyVisiblityRule( &vis_style );
	m_hProperties.AddToTail( pSlider );

#if DEFCFG_ADAPTIVE_VOLUMETRIC_LOD
	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_VOLUME_LOD0_DIST ), "256" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Volume LOD 0" );
	pTextEntry->CopyVisiblityRule( &vis_volume );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_VOLUME_LOD1_DIST ), "512" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Volume LOD 1" );
	pTextEntry->CopyVisiblityRule( &vis_volume );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_VOLUME_LOD2_DIST ), "1024" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Volume LOD 2" );
	pTextEntry->CopyVisiblityRule( &vis_volume );
	m_hProperties.AddToTail( pTextEntry );

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_VOLUME_LOD3_DIST ), "2048" ),
		PropertyTextEntry::PTENTRY_FLOAT );
	pTextEntry->SetLabelName( "Volume LOD 3" );
	pTextEntry->CopyVisiblityRule( &vis_volume );
	m_hProperties.AddToTail( pTextEntry );
#endif

#if DEFCFG_CONFIGURABLE_VOLUMETRIC_LOD
	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_VOLUME_SAMPLES ), "50" ),
		PropertyTextEntry::PTENTRY_INT );
	pTextEntry->SetLabelName( "Volume Samples" );
	pTextEntry->CopyVisiblityRule( &vis_volume );
	m_hProperties.AddToTail( pTextEntry );
#endif
}

void CVGUILightEditor_Properties::CreateProperties_GlobalLight()
{
	PropertyTextEntry *pTextEntry = NULL;
	PropertyCheckButton *pCheckButton = NULL;
	PropertySlider *pSlider = NULL;
	PropertyColorPicker* pColorPickerPanel = NULL;

	pTextEntry = new PropertyTextEntry(
		MakeKey( GetLightParamName( LPARAM_ANGLES ), "0 0 0" ),
		PropertyTextEntry::PTENTRY_STRING, true );
	pTextEntry->SetLabelName( "Angles" );
	m_hProperties.AddToTail( pTextEntry );

	pSlider = new PropertySlider(
		MakeKey( GetLightParamName( LPARAM_ANGLES ), "0" ) );
	pSlider->SetRange( -180, 180 );
	pSlider->SetNumTicks( 10 );
	pSlider->SetLabelName( "Yaw" );
	pSlider->SetComponentIndex( 3, 1 );
	m_hProperties.AddToTail( pSlider );

	pSlider = new PropertySlider(
		MakeKey( GetLightParamName( LPARAM_ANGLES ), "0" ) );
	pSlider->SetRange( -180, 180 );
	pSlider->SetNumTicks( 10 );
	pSlider->SetLabelName( "Pitch" );
	pSlider->SetComponentIndex( 3, 0 );
	m_hProperties.AddToTail( pSlider );

	pColorPickerPanel = new PropertyColorPicker( MakeKey( GetLightParamName( LPARAM_DIFFUSE ), "128 128 128 255" ) );
	pColorPickerPanel->SetLabelName( "Diffuse" );
	m_hProperties.AddToTail( pColorPickerPanel );

	pSlider = new PropertySlider( MakeKey( GetLightParamName( LPARAM_DIFFUSE ), "255 255 255 255" ) );
	pSlider->SetLabelName( "Diffuse Intensity" );
	pSlider->SetRange( 0, 1275 );
	pSlider->SetNumTicks( 10 );
	pSlider->SetComponentIndex( 4, 3 );
	m_hProperties.AddToTail( pSlider );

	pColorPickerPanel = new PropertyColorPicker( MakeKey( GetLightParamName( LPARAM_AMBIENT_LOW ), "0 0 0 0" ) );
	pColorPickerPanel->SetLabelName( "Ambient Low" );
	m_hProperties.AddToTail( pColorPickerPanel );

	pSlider = new PropertySlider( MakeKey( GetLightParamName( LPARAM_AMBIENT_LOW ), "0 0 0 0" ) );
	pSlider->SetLabelName( "Ambient Low Intensity" );
	pSlider->SetRange( 0, 1275 );
	pSlider->SetNumTicks( 10 );
	pSlider->SetComponentIndex( 4, 3 );
	m_hProperties.AddToTail( pSlider );

	pColorPickerPanel = new PropertyColorPicker( MakeKey( GetLightParamName( LPARAM_AMBIENT_HIGH ), "0 0 0 0" ) );
	pColorPickerPanel->SetLabelName( "Ambient High" );
	m_hProperties.AddToTail( pColorPickerPanel );

	pSlider = new PropertySlider( MakeKey( GetLightParamName( LPARAM_AMBIENT_HIGH ), "0 0 0 0" ) );
	pSlider->SetLabelName( "Ambient High Intensity" );
	pSlider->SetRange( 0, 1275 );
	pSlider->SetNumTicks( 10 );
	pSlider->SetComponentIndex( 4, 3 );
	m_hProperties.AddToTail( pSlider );

	pCheckButton = new PropertyCheckButton(
		MakeKey( GetLightParamName( LPARAM_SPAWNFLAGS ), "1" ),
		DEFLIGHTGLOBAL_ENABLED, "Enable light" );
	m_hProperties.AddToTail( pCheckButton );

	pCheckButton = new PropertyCheckButton(
		MakeKey( GetLightParamName( LPARAM_SPAWNFLAGS ), "1" ),
		DEFLIGHTGLOBAL_SHADOW_ENABLED, "Enable shadows" );
	m_hProperties.AddToTail( pCheckButton );

	RefreshGlobalLightData();
}

void CVGUILightEditor_Properties::UpdatePropertyVisibility()
{
	int iSliderPos = m_pListRoot->GetScrollBar()->GetValue();
	m_pListRoot->RemoveAll();

	for ( int i = 0; i < m_hProperties.Count(); i++ )
	{
		PropertyClient *pClient = dynamic_cast< PropertyClient* >( m_hProperties[i] );
		Assert( pClient != NULL );

		Label *pLabel = pClient->GetLabel();

		// <_<
		if ( !pClient->ShouldDraw() )
		{
			m_hProperties[i]->SetVisible( false );
			if ( pLabel )
				pLabel->SetVisible( false );
			continue;
		}
		else
		{
			m_hProperties[i]->SetVisible( true );
			if ( pLabel )
				pLabel->SetVisible( true );
		}

		bool bShouldBeEnabled = pClient->ShouldBeEnabled();
		if ( bShouldBeEnabled != m_hProperties[i]->IsEnabled() )
			m_hProperties[i]->SetEnabled( bShouldBeEnabled );

		AddPropertyToList( m_hProperties[ i ] );
	}

	m_pListRoot->GetScrollBar()->SetValue( iSliderPos );
}

void CVGUILightEditor_Properties::AddPropertyToList( Panel *panel )
{
	int w = m_pListRoot->GetWide();
	panel->SetWide( w );

	PropertyClient *pClient = dynamic_cast< PropertyClient* >( panel );
	Assert( pClient != NULL );

	m_pListRoot->AddItem( pClient->GetLabel(), panel );
}

void CVGUILightEditor_Properties::PropertiesReadAll()
{
	for ( int i = 0; i < m_hProperties.Count(); i++ )
	{
		PropertyClient *pClient = dynamic_cast< PropertyClient* >( m_hProperties[ i ] );
		Assert( pClient != NULL );
		pClient->ReadValue();
	}
}

KeyValues *CVGUILightEditor_Properties::AllocKVFromVisible()
{
	KeyValues *pRet = new KeyValues("");

	for ( int i = 0; i < m_hProperties.Count(); i++ )
	{
		if ( !m_hProperties[i]->IsVisible() )
			continue;

		PropertyClient *pClient = dynamic_cast< PropertyClient* >( m_hProperties[ i ] );
		Assert( pClient != NULL );

		pRet->SetString( pClient->GetValue()->GetName(),
			pClient->GetValue()->GetString() );
	}

	return pRet;
}