
#include "cbase.h"
#include "deferred/deferred_shared_common.h"
#include "deferred/vgui/vgui_deferred.h"

#include "ienginevgui.h"
#include "collisionutils.h"
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/ISurface.h>

#include <vgui_controls/RadioButton.h>

using namespace vgui;

class CVGUILightEditor : public Panel
{
	DECLARE_CLASS_SIMPLE( CVGUILightEditor, Panel );
	DECLARE_REFERENCED_CLASS( CVGUILightEditor );

public:

	static void DestroyEditor();
	static void ToggleEditor();
	static bool IsEditorVisible();
	static VPANEL GetEditorPanel();

	~CVGUILightEditor();

protected:
	void ApplySchemeSettings( IScheme *scheme );
	void PerformLayout();

	void OnMousePressed( MouseCode code );
	void OnMouseReleased( MouseCode code );
	void OnMouseCaptureLost();

	void OnCursorMoved_Internal( int &x, int &y );

	void OnKeyCodePressed_Internal( KeyCode code );
	void OnKeyCodeReleased_Internal( KeyCode code );

	void OnThink();
	void Paint();

	MESSAGE_FUNC( OnEditGlobalLight, "EditGlobalLight" );
	MESSAGE_FUNC( OnToggleEditorProperties, "ToggleEditorProperties" );
	MESSAGE_FUNC_PARAMS( OnPropertiesChanged_Light, "PropertiesChanged_Light", pKV );

	MESSAGE_FUNC( OnVmfPathChanged, "VmfPathChanged" ){
		UpdateCurrentMapName();

		PostActionSignal( new KeyValues( "FileLoaded" ) );
	};

private:

	void PrepareDrag();
	void StartDrag( int mode );
	void StopDrag();
	void GetDragBounds( int *pi2_min, int *pi2_max );

	static CUtlReference< CVGUILightEditor > m_refInstance;
	CVGUILightEditor( VPANEL pParent );

	CLightingEditor *m_pEditorSystem;

	enum
	{
		EDITORDRAG_NONE = 0,
		EDITORDRAG_VIEW,
		EDITORDRAG_TRANSLATE,
		EDITORDRAG_ROTATE,
		EDITORDRAG_SELECT,
	};

	int m_iDragMode;
	int m_iLastMouse_x, m_iLastMouse_y;
	int m_iDragStartPos[2];

	bool m_bDragSelectArmed;

	CLightingEditor::EDITORINTERACTION_MODE m_iLastInteractionMode;
	float m_flLastInteractionTypeKeyPress;
	bool ActivateEditorInteractionMode( CLightingEditor::EDITORINTERACTION_MODE mode );

	Vector m_vecLastDragWorldPos;
	Vector GetDragWorldPos( int x, int y );

	CVGUILightEditor_Properties *m_pEditorProps;
#if DEFCG_LIGHTEDITOR_ALTERNATESETUP
	MenuBar*	m_pMenuBar;
#else
	CVGUILightEditor_Controls *m_pMainControls;
#endif
	KeyValues *m_pCurrentProperties;

	Label *m_pCurrentMapName;
	void UpdateCurrentMapName();
};

CUtlReference< CVGUILightEditor > CVGUILightEditor::m_refInstance = CUtlReference< CVGUILightEditor >();

void CVGUILightEditor::DestroyEditor()
{
	CVGUILightEditor *pPanel = CVGUILightEditor::m_refInstance.GetObject();

	if ( pPanel )
		pPanel->DeletePanel();
}

void CVGUILightEditor::ToggleEditor()
{
	CVGUILightEditor *pPanel = CVGUILightEditor::m_refInstance.GetObject();

	if ( pPanel == NULL )
	{
		VPANEL editorParent = enginevgui->GetPanel( PANEL_INGAMESCREENS );

		pPanel = new CVGUILightEditor( editorParent );
		pPanel->MakeReadyForUse();
		pPanel->SetVisible( true );

		CVGUILightEditor::m_refInstance.Set( pPanel );
		GetLightingEditor()->SetEditorActive( true );
	}
	else
		pPanel->SetVisible( !pPanel->IsVisible() );

	Assert( pPanel != NULL );

	GetLightingEditor()->AbortEditorMovement( true );

	GetLightingEditor()->SetEditorActive( pPanel->IsVisible(), true, false );

	pPanel->UpdateCurrentMapName();
}

bool CVGUILightEditor::IsEditorVisible()
{
	CVGUILightEditor *pPanel = CVGUILightEditor::m_refInstance.GetObject();

	return pPanel && pPanel->IsVisible();
}

VPANEL CVGUILightEditor::GetEditorPanel()
{
	if ( !m_refInstance.IsValid() )
		return (VPANEL)0;

	return m_refInstance->GetVPanel();
}

CVGUILightEditor::CVGUILightEditor( VPANEL pParent )
	: BaseClass( NULL, "LightEditorRoot" )
{
	SetParent( pParent );
	SetConsoleStylePanel( true );

	m_pEditorSystem = GetLightingEditor();

	m_iDragMode = EDITORDRAG_NONE;
	m_bDragSelectArmed = false;
	m_iLastMouse_x = 0;
	m_iLastMouse_y = 0;

	m_pCurrentMapName = new Label(this, "", "");
	UpdateCurrentMapName();

	m_iLastInteractionMode = CLightingEditor::EDITORINTERACTION_SELECT;
	m_flLastInteractionTypeKeyPress = 0;

	m_vecLastDragWorldPos.Init();

#if DEFCG_LIGHTEDITOR_ALTERNATESETUP
	m_pMenuBar = new MenuBar( this, "MenuBar" );
	
	Menu* pMenu = new Menu( m_pMenuBar, "FileMenu" );
	pMenu->AddMenuItem( "OpenVMF", "Open VMF", "loadvmf", this );
	pMenu->AddMenuItem( "SaveVMF", "Save VMF", "savevmf", this );

	m_pMenuBar->AddMenu( "File", pMenu );

	pMenu = new Menu( m_pMenuBar, "PreferencesMenu" );
	pMenu->AddMenuItem( "SetDefaultVMFPath", "Default VMF Director", "setdefaultvmfpath", this );

	m_pMenuBar->AddMenu( "Preferences", pMenu );

	GetLightingEditor()->SetEditorInteractionMode( CLightingEditor::EDITORINTERACTION_SELECT );
	GetLightingEditor()->SetEditorActive( true, false );
#else
	m_pMainControls = new CVGUILightEditor_Controls( this );
	m_pMainControls->Activate();
	m_pMainControls->AddActionSignalTarget( this );
#endif

	m_pEditorProps = new CVGUILightEditor_Properties( this );
	m_pEditorProps->AddActionSignalTarget( this );
	AddActionSignalTarget( m_pEditorProps );

	m_pCurrentProperties = NULL;
	m_pEditorProps->SendPropertiesToRoot();
}

CVGUILightEditor::~CVGUILightEditor()
{
	if ( m_pCurrentProperties != NULL )
		m_pCurrentProperties->deleteThis();
}

void CVGUILightEditor::UpdateCurrentMapName()
{
	const char *pszPath = m_pEditorSystem->GetCurrentVmfPath();

	if ( !pszPath || !*pszPath )
		m_pCurrentMapName->SetText( "No VMF loaded." );
	else
	{
		//char tmp[MAX_PATH];
		//Q_FileBase( pszPath, tmp, sizeof(tmp) );
		//m_pCurrentMapName->SetText( tmp );

		m_pCurrentMapName->SetText( pszPath );
	}
}

void CVGUILightEditor::OnEditGlobalLight()
{
	m_pEditorProps->OnRequestPropertyLayout( CVGUILightEditor_Properties::PROPERTYMODE_GLOBAL );
}

void CVGUILightEditor::OnToggleEditorProperties()
{
	if ( m_pEditorProps->IsVisible() )
		m_pEditorProps->Close();
	else
		m_pEditorProps->Activate();
}

void CVGUILightEditor::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	MakeReadyForUse();

	SetPaintBackgroundEnabled( false );

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );

	m_pCurrentMapName->SetMouseInputEnabled(false);
	m_pCurrentMapName->SetKeyBoardInputEnabled(false);
	m_pCurrentMapName->SetProportional(false);
}

void CVGUILightEditor::PerformLayout()
{
	BaseClass::PerformLayout();

	int w,h;
	engine->GetScreenSize( w, h );
	SetBounds( 0, 0, w, h );

#if DEFCG_LIGHTEDITOR_ALTERNATESETUP
	m_pMenuBar->SetWide( w );
#endif

	m_pCurrentMapName->SetBounds( 12, h - 24, w, 24 );
}

void CVGUILightEditor::OnMousePressed( MouseCode code )
{
	StopDrag();

	if ( code == MOUSE_RIGHT )
	{
		// HACK: can't REMOVE FOCUS from anywhere. WHY?!
		// ALL of these stupid problems would be gone if the root panel could accept input
		// autonomically without being either a popup or a frame.
		Panel *pFocused = ipanel()->GetPanel( input()->GetFocus(), GetModuleName() );

		if ( pFocused && pFocused->GetParent() )
			pFocused->GetParent()->RequestFocus();

		input()->SetMouseCapture( GetVPanel() );

		StartDrag( EDITORDRAG_VIEW );

		m_iLastMouse_x = GetWide() / 2;
		m_iLastMouse_y = GetTall() / 2;
		input()->SetCursorPos( m_iLastMouse_x, m_iLastMouse_y );

		input()->SetCursorOveride( dc_none );
	}
	else if ( code == MOUSE_LEFT )
	{
		int mx, my;
		input()->GetCursorPos( mx, my );

		switch ( m_pEditorSystem->GetEditorInteractionMode() )
		{
		case CLightingEditor::EDITORINTERACTION_SELECT:
			{
				m_bDragSelectArmed = true;
			}
			break;
		case CLightingEditor::EDITORINTERACTION_ADD:
			{
				if ( m_pCurrentProperties != NULL )
					m_pEditorSystem->AddLightFromScreen( mx, my, m_pCurrentProperties );
			}
			break;
		case CLightingEditor::EDITORINTERACTION_TRANSLATE:
			{
				if ( m_pEditorSystem->GetCurrentSelectedAxis() != CLightingEditor::EDITORAXIS_NONE )
				{
					StartDrag( EDITORDRAG_TRANSLATE );
					m_vecLastDragWorldPos = GetDragWorldPos( mx, my );
				}
			}
			break;
		case CLightingEditor::EDITORINTERACTION_ROTATE:
			{
				if ( m_pEditorSystem->GetCurrentSelectedAxis() != CLightingEditor::EDITORAXIS_NONE )
				{
					Assert( m_pEditorSystem->GetCurrentSelectedAxis() < CLightingEditor::EDITORAXIS_FIRST_PLANE );
					StartDrag( EDITORDRAG_ROTATE );
					m_vecLastDragWorldPos = GetDragWorldPos( mx, my );
					m_pEditorSystem->SetSelectionCenterLocked( true );
				}
			}
			break;
		}
	}
}

void CVGUILightEditor::OnMouseReleased( MouseCode code )
{
	bool bDoLightUpdate = false;

	if ( code == MOUSE_LEFT &&
		m_iDragMode == EDITORDRAG_TRANSLATE ||
		m_iDragMode == EDITORDRAG_ROTATE )
		bDoLightUpdate = true;

	bool bSelectBox = m_iDragMode == EDITORDRAG_SELECT;

	StopDrag();

	if ( code == MOUSE_LEFT )
	{
		int mx, my;
		input()->GetCursorPos( mx, my );

		bool bShift = input()->IsKeyDown( KEY_LSHIFT ) ||
			input()->IsKeyDown( KEY_RSHIFT );
		bool bCtrl = input()->IsKeyDown( KEY_LCONTROL ) ||
			input()->IsKeyDown( KEY_RCONTROL );

		switch ( m_pEditorSystem->GetEditorInteractionMode() )
		{
		case CLightingEditor::EDITORINTERACTION_SELECT:
			{
				if ( bSelectBox )
				{
					int dragMin[2], dragMax[2];
					GetDragBounds( dragMin, dragMax );

					if ( m_pEditorSystem->SelectLights( dragMin, dragMax, bShift, bCtrl ) )
						bDoLightUpdate = true;
				}
				else if ( m_pEditorSystem->SelectLight( mx, my, bShift, bCtrl ) )
					bDoLightUpdate = true;
			}
			break;
		}
	}

	if ( bDoLightUpdate )
		PostActionSignal( new KeyValues( "LightSelectionChanged" ) );
}

void CVGUILightEditor::OnMouseCaptureLost()
{
	StopDrag();

	BaseClass::OnMouseCaptureLost();
}

Vector CVGUILightEditor::GetDragWorldPos( int x, int y )
{
	Assert( m_pEditorSystem->IsAnythingSelected() );

	Vector picker = m_pEditorSystem->GetPickingRay( x, y );
	Vector viewOrigin = m_pEditorSystem->GetViewOrigin();

	Vector center = m_pEditorSystem->GetSelectionCenter();
	QAngle orientation = m_pEditorSystem->GetSelectionOrientation();

	Vector viewFwd;
	AngleVectors( m_pEditorSystem->GetViewAngles(), &viewFwd );

	Vector ret = center;

	Ray_t pickingRay;
	pickingRay.Init( viewOrigin, viewOrigin + picker * MAX_TRACE_LENGTH );

	CLightingEditor::EDITOR_SELECTEDAXIS axis = m_pEditorSystem->GetCurrentSelectedAxis();

	Assert( axis >= CLightingEditor::EDITORAXIS_FIRST && axis < CLightingEditor::EDITORAXIS_COUNT );

	switch( m_iDragMode )
	{
	default:
		Assert( 0 );
	case EDITORDRAG_TRANSLATE:
		{
			const bool bMoveScreen = axis == CLightingEditor::EDITORAXIS_SCREEN;
			const bool bMovePlane = axis >= CLightingEditor::EDITORAXIS_FIRST_PLANE;

			if ( bMovePlane || bMoveScreen )
			{
				Vector normals[3] = {
					Vector( 0, 0, 1 ),
					Vector( 0, 1, 0 ),
					Vector( 1, 0, 0 ),
				};

				cplane_t plane;
				if ( bMoveScreen )
					plane.normal = viewFwd;
				else
					plane.normal = normals[ axis - CLightingEditor::EDITORAXIS_FIRST_PLANE ];
				plane.dist = DotProduct( plane.normal, center );

				float flDist = IntersectRayWithPlane( pickingRay, plane );

				ret = pickingRay.m_Start + pickingRay.m_Delta * flDist;
			}
			else
			{
				Vector direction( vec3_origin );
				direction[ axis - CLightingEditor::EDITORAXIS_FIRST ] = 1;

				Ray_t axisRay;
				axisRay.Init( center, center + direction );

				float t, s = 0;
				IntersectRayWithRay( pickingRay, axisRay, t, s );

				ret = center + direction * s;
			}
		}
		break;
	case EDITORDRAG_ROTATE:
		{
			Vector normals[4] = {
				Vector( 0, 1, 0 ),
				Vector( 0, 0, 1 ),
				Vector( 1, 0, 0 ),
			};

			normals[3] = viewOrigin - center;
			normals[3].NormalizeInPlace();

			AngleVectors( orientation,
				&normals[2], &normals[0], &normals[1] );

			int iAxis = axis - CLightingEditor::EDITORAXIS_FIRST;

			Assert( iAxis >= 0 && iAxis < 4 );

			Vector planeNormal = normals[ iAxis ];

			cplane_t plane;
			plane.normal = planeNormal;
			plane.dist = DotProduct( planeNormal, center );

			float t = IntersectRayWithPlane( pickingRay, plane );

			ret = pickingRay.m_Start + pickingRay.m_Delta * t;
		}
		break;
	}

	return ret;
}

void CVGUILightEditor::GetDragBounds( int *pi2_min, int *pi2_max )
{
	int iCurPos[2];
	input()->GetCursorPos( iCurPos[0], iCurPos[1] );

	pi2_min[ 0 ] = MIN( iCurPos[0], m_iDragStartPos[0] );
	pi2_min[ 1 ] = MIN( iCurPos[1], m_iDragStartPos[1] );

	pi2_max[ 0 ] = MAX( iCurPos[0], m_iDragStartPos[0] );
	pi2_max[ 1 ] = MAX( iCurPos[1], m_iDragStartPos[1] );
}

void CVGUILightEditor::StartDrag( int mode )
{
	m_iDragMode = mode;
	input()->SetMouseCapture( GetVPanel() );
	input()->GetCursorPos( m_iDragStartPos[0], m_iDragStartPos[1] );
}

void CVGUILightEditor::StopDrag()
{
	m_bDragSelectArmed = false;

	if ( m_iDragMode == EDITORDRAG_NONE )
		return;

	bool bView = m_iDragMode == EDITORDRAG_VIEW;

	if ( input()->GetMouseCapture() == GetVPanel() )
		input()->SetMouseCapture( 0 );

	m_iDragMode = EDITORDRAG_NONE;

	if ( bView )
		input()->SetCursorPos( m_iDragStartPos[0], m_iDragStartPos[1] );

	input()->SetCursorOveride( dc_user );

	m_pEditorSystem->SetSelectionCenterLocked( false );
}

// don't affect ourselves recursively
void CVGUILightEditor::OnCursorMoved_Internal( int &x, int &y )
{
	float delta_x = ( x - m_iLastMouse_x );
	float delta_y = ( y - m_iLastMouse_y );

	bool bShouldReset = false;

	if ( m_bDragSelectArmed )
	{
		m_bDragSelectArmed = false;

		StartDrag( EDITORDRAG_SELECT );
	}

	switch ( m_iDragMode )
	{
	default:
		{
			m_pEditorSystem->UpdateCurrentSelectedAxis( x, y );
		}
		break;
	case EDITORDRAG_VIEW:
		{
			QAngle angles;
			m_pEditorSystem->GetEditorView( NULL, &angles );

			normalizeAngles( angles );

			const float flTurnSpeed = 0.5f;
			angles.x += delta_y * flTurnSpeed;
			angles.y -= delta_x * flTurnSpeed;

			angles.x = clamp( angles.x, -89, 89 );
			angles.y = AngleNormalize( angles.y );

			m_pEditorSystem->SetEditorView( NULL, &angles );

			bShouldReset = true;
		}
		break;
	case EDITORDRAG_TRANSLATE:
	case EDITORDRAG_ROTATE:
		{
			Vector vecCurrentDragPos = GetDragWorldPos( x, y );

			if ( m_iDragMode == EDITORDRAG_TRANSLATE )
			{
				Vector vecMove = vecCurrentDragPos - m_vecLastDragWorldPos;
				m_pEditorSystem->MoveSelectedLights( vecMove );
			}
			else
			{
				Vector center = m_pEditorSystem->GetSelectionCenter();
				QAngle orientation = m_pEditorSystem->GetSelectionOrientation();

				CLightingEditor::EDITOR_SELECTEDAXIS axis = m_pEditorSystem->GetCurrentSelectedAxis();
				Assert( axis >= CLightingEditor::EDITORAXIS_FIRST && axis < CLightingEditor::EDITORAXIS_FIRST_PLANE );

				int iAxis = axis - CLightingEditor::EDITORAXIS_FIRST;
				Assert( iAxis >= 0 && iAxis < 4 );

				Vector vDirections[6];
				AngleVectors( orientation,
					&vDirections[0], &vDirections[1], &vDirections[2] );

				Vector vecViewCenter = center - m_pEditorSystem->GetViewOrigin();
				vecViewCenter.NormalizeInPlace();

				VectorVectors( vecViewCenter, vDirections[3], vDirections[4] );
				vDirections[5] = vecViewCenter;

				int iDirIndices[4][3] = {
					{ 2, 0, 1 },
					{ 0, 1, 2 },
					{ 1, 2, 0 },
					{ 3, 4, 5 },
				};

				Vector vecDelta[2] = {
					m_vecLastDragWorldPos - center,
					vecCurrentDragPos - center,
				};

				vecDelta[0].NormalizeInPlace();
				vecDelta[1].NormalizeInPlace();

				float flAngle[2] = {
					atan2( DotProduct( vDirections[ iDirIndices[iAxis][0] ], vecDelta[0] ),
						DotProduct( vDirections[ iDirIndices[iAxis][1] ], vecDelta[0] ) ),
					atan2( DotProduct( vDirections[ iDirIndices[iAxis][0] ], vecDelta[1] ),
						DotProduct( vDirections[ iDirIndices[iAxis][1] ], vecDelta[1] ) ),
				};

				float flRotate = flAngle[1] - flAngle[0];

				VMatrix matRotate;
				matRotate.Identity();

				MatrixBuildRotationAboutAxis( matRotate,
					vDirections[ iDirIndices[iAxis][2] ],
					RAD2DEG( flRotate ) );

				m_pEditorSystem->RotateSelectedLights( matRotate );
			}

			m_vecLastDragWorldPos = vecCurrentDragPos;
		}
		break;
	}

	m_iLastMouse_x = x;
	m_iLastMouse_y = y;

	if ( bShouldReset )
	{
		int whalf = GetWide() / 2;
		int thalf = GetTall() / 2;

		m_iLastMouse_x = whalf;
		m_iLastMouse_y = thalf;

		// HACK: reset tracker
		x = m_iLastMouse_x;
		y = m_iLastMouse_y;

		input()->SetCursorPos( whalf, thalf );
	}
}

void CVGUILightEditor::OnKeyCodePressed_Internal( KeyCode code )
{
	VPANEL gamedll = enginevgui->GetPanel( PANEL_GAMEDLL );
	VPANEL panel_over = input()->GetMouseOver();

	if ( panel_over && panel_over != gamedll )
		return;

	Vector &vecEditorMoveDir = m_pEditorSystem->GetMoveDirForModify();
	bool bUnhandled = true;

	if ( m_iDragMode == EDITORDRAG_VIEW )
	{
		bUnhandled = false;

		switch ( code )
		{
		default:
			bUnhandled = true;
			break;
		case KEY_LEFT:
		case KEY_A:
			vecEditorMoveDir.y = -1;
			break;
		case KEY_RIGHT:
		case KEY_D:
			vecEditorMoveDir.y = 1;
			break;
		case KEY_UP:
		case KEY_W:
			vecEditorMoveDir.x = 1;
			break;
		case KEY_DOWN:
		case KEY_S:
			vecEditorMoveDir.x = -1;
			break;
		case KEY_LSHIFT:
			vecEditorMoveDir.z = 1;
			break;

		}
	}

	if ( bUnhandled && m_iDragMode == EDITORDRAG_NONE )
	{
		CLightingEditor::EDITORINTERACTION_MODE lastMode = m_pEditorSystem->GetEditorInteractionMode();
		CLightingEditor::EDITORINTERACTION_MODE curMode = lastMode;

		switch ( code )
		{
		case KEY_DELETE:
			m_pEditorSystem->DeleteSelectedLights();
			break;
		case KEY_X:
			curMode = CLightingEditor::EDITORINTERACTION_SELECT;
			break;
		case KEY_C:
			curMode = CLightingEditor::EDITORINTERACTION_ROTATE;
			break;
		case KEY_V:
			curMode = CLightingEditor::EDITORINTERACTION_TRANSLATE;
			break;
		case KEY_B:
			curMode = CLightingEditor::EDITORINTERACTION_ADD;
			break;
		case KEY_F5:
			engine->ClientCmd( "clear; jpeg" );
			break;
		}

		if ( curMode != lastMode &&
			ActivateEditorInteractionMode( curMode ) )
		{
			m_iLastInteractionMode = lastMode;
			m_flLastInteractionTypeKeyPress = gpGlobals->curtime;
		}
	}

	//BaseClass::OnKeyCodePressed( code );
}

void CVGUILightEditor::OnKeyCodeReleased_Internal( KeyCode code )
{
	Vector &vecEditorMoveDir = m_pEditorSystem->GetMoveDirForModify();

	switch ( code )
	{
	case KEY_LEFT:
	case KEY_A:
		if ( vecEditorMoveDir.y < 0 )
			vecEditorMoveDir.y = 0;
		break;
	case KEY_RIGHT:
	case KEY_D:
		if ( vecEditorMoveDir.y > 0 )
			vecEditorMoveDir.y = 0;
		break;
	case KEY_UP:
	case KEY_W:
		if ( vecEditorMoveDir.x > 0 )
			vecEditorMoveDir.x = 0;
		break;
	case KEY_DOWN:
	case KEY_S:
		if ( vecEditorMoveDir.x < 0 )
			vecEditorMoveDir.x = 0;
		break;
	case KEY_LSHIFT:
		vecEditorMoveDir.z = 0;
		break;
	}

	if ( code == KEY_X ||
		code == KEY_C ||
		code == KEY_V ||
		code == KEY_B )
	{
		if ( ( gpGlobals->curtime - m_flLastInteractionTypeKeyPress ) > 0.2f )
		{
			ActivateEditorInteractionMode( m_iLastInteractionMode );
		}
	}

	//BaseClass::OnKeyCodePressed( code );
}

bool CVGUILightEditor::ActivateEditorInteractionMode( CLightingEditor::EDITORINTERACTION_MODE mode )
{
	const char *pszTargetRadioButton = NULL;

	switch( mode )
	{
	case CLightingEditor::EDITORINTERACTION_SELECT:
			pszTargetRadioButton = "action_select";
		break;
	case CLightingEditor::EDITORINTERACTION_ADD:
			pszTargetRadioButton = "action_add";
		break;
	case CLightingEditor::EDITORINTERACTION_TRANSLATE:
			pszTargetRadioButton = "action_translate";
		break;
	case CLightingEditor::EDITORINTERACTION_ROTATE:
			pszTargetRadioButton = "action_rotate";
		break;
	default:
		break;
	}

	if ( pszTargetRadioButton != NULL )
	{
#if !DEFCG_LIGHTEDITOR_ALTERNATESETUP
		RadioButton *pButton = assert_cast<RadioButton*>( m_pMainControls->FindChildByName( pszTargetRadioButton ) );
		Assert( pButton );

		if ( pButton != NULL )
		{
			pButton->SetSelected( true );
			m_pEditorSystem->SetEditorInteractionMode( mode );

			return true;
		}
#else
		m_pEditorSystem->SetEditorInteractionMode( mode );
#endif
	}

	return false;
}

void CVGUILightEditor::OnPropertiesChanged_Light( KeyValues *pKV )
{
	KeyValues *pProperties = (KeyValues*)pKV->GetPtr( "properties" );

	if ( pProperties == NULL )
		return;

	if ( m_pCurrentProperties )
		m_pCurrentProperties->deleteThis();

	m_pCurrentProperties = pProperties->MakeCopy();
}

void CVGUILightEditor::Paint()
{
	BaseClass::Paint();

	if ( m_iDragMode != EDITORDRAG_SELECT )
		return;

	int dragMin[2];
	int dragMax[2];

	GetDragBounds( dragMin, dragMax );

	surface()->DrawSetColor( Color( 0, 96, 196, 48 ) );
	surface()->DrawFilledRect( dragMin[0], dragMin[1], dragMax[0], dragMax[1] );

	surface()->DrawSetColor( Color( 0, 64, 128, 196 ) );
	surface()->DrawFilledRect( dragMin[0], dragMin[1], dragMax[0], dragMin[1] + 1 );
	surface()->DrawFilledRect( dragMin[0], dragMax[1] - 1, dragMax[0], dragMax[1] );
	surface()->DrawFilledRect( dragMin[0], dragMin[1] + 1, dragMin[0] + 1, dragMax[1] - 1 );
	surface()->DrawFilledRect( dragMax[0] - 1, dragMin[1] + 1, dragMax[0], dragMax[1] - 1 );
}

/*
	HACK: Can't use the popup input crap because it breaks shit.
*/
void CVGUILightEditor::OnThink()
{
	VPANEL gamedll = enginevgui->GetPanel( PANEL_GAMEDLL );
	VPANEL panel_over = input()->GetMouseOver();
	VPANEL panel_capture = input()->GetMouseCapture();
	VPANEL panel_self = GetVPanel();

	if ( ( ipanel()->HasParent( panel_over, panel_self ) ||
		panel_capture != 0 ) && panel_capture != panel_self ||
		panel_over && panel_over != gamedll )
	{
		m_pEditorSystem->AbortEditorMovement( false );
		return;
	}

	const bool bCapturingMouse = panel_capture == panel_self;

	const MouseCode iMouse_Map[] = {
		MOUSE_LEFT,
		MOUSE_RIGHT,
	};
	const int iNumMouse = ARRAYSIZE( iMouse_Map );
	static bool bMouseLeft_Last[iNumMouse] = { false };
	bool bMouseLeft_Cur[iNumMouse] = { false };

	if ( !bCapturingMouse )
		for ( int i = 0; i < iNumMouse; i++ )
			bMouseLeft_Cur[ i ] = input()->IsMouseDown( iMouse_Map[ i ] );

	const KeyCode iKey_Map[] = {
		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,
		KEY_A,
		KEY_W,
		KEY_S,
		KEY_D,
		KEY_LSHIFT,

		KEY_DELETE,
		KEY_X,
		KEY_C,
		KEY_V,
		KEY_B,
		KEY_F5,
	};

	const int iLastDelayedKey = 8;
	const int iNumKeys = ARRAYSIZE( iKey_Map );
	static bool bKey_Last[iNumKeys] = { false };
	bool bKey_Cur[iNumKeys] = { false };

	for ( int i = 0; i < iNumKeys; i++ )
	{
		if ( i <= iLastDelayedKey && m_iDragMode != EDITORDRAG_VIEW )
			continue;

		bKey_Cur[ i ] = input()->IsKeyDown( iKey_Map[ i ] );
	}

	if ( !bCapturingMouse )
	{
		for ( int i = 0; i < iNumMouse; i++ )
		{
			if ( bMouseLeft_Cur[ i ] != bMouseLeft_Last[ i ] )
			{
				if ( bMouseLeft_Cur[ i ] )
					OnMousePressed( iMouse_Map[ i ] );
				else
					OnMouseReleased( iMouse_Map[ i ] );
			}
		}
	}

	for ( int i = 0; i < iNumKeys; i++ )
	{
		if ( bKey_Cur[ i ] != bKey_Last[ i ] )
		{
			if ( bKey_Cur[ i ] )
				OnKeyCodePressed_Internal( iKey_Map[ i ] );
			else
				OnKeyCodeReleased_Internal( iKey_Map[ i ] );
		}
	}

	Q_memcpy( bMouseLeft_Last, bMouseLeft_Cur, sizeof( bMouseLeft_Last ) );
	Q_memcpy( bKey_Last, bKey_Cur, sizeof( bKey_Last ) );

	static int m_last[2] = { 0 };
	int m_cur[2];
	input()->GetCursorPos( m_cur[ 0 ], m_cur[ 1 ] );

	if ( m_last[ 0 ] != m_cur[ 0 ] ||
		m_last[ 1 ] != m_cur[ 1 ] )
		OnCursorMoved_Internal( m_cur[ 0 ], m_cur[ 1 ] );

	Q_memcpy( m_last, m_cur, sizeof( m_last ) );

	BaseClass::OnThink();
}

CON_COMMAND( deferred_LightEditor_toggle, "" )
{
	CVGUILightEditor::ToggleEditor();
}


static class CLightEditorHelper : public CAutoGameSystemPerFrame
{
	void LevelShutdownPostEntity()
	{
		CVGUILightEditor::DestroyEditor();
	};

	void Update( float ft )
	{
		if ( !engine->IsInGame() )
		{
			if ( CVGUILightEditor::IsEditorVisible() )
				CVGUILightEditor::ToggleEditor();

			return;
		}

		static bool bWasTabDown = false;
		bool bIsTabDown = vgui::input()->IsKeyDown( KEY_TAB );

		VPANEL focusedPanel = input()->GetFocus();

		if ( bIsTabDown != bWasTabDown )
		{
			if ( bIsTabDown &&
				( focusedPanel == 0 || CVGUILightEditor::GetEditorPanel() == 0 ||
				ipanel()->HasParent( focusedPanel, CVGUILightEditor::GetEditorPanel() ) ) )
				CVGUILightEditor::ToggleEditor();

			bWasTabDown = bIsTabDown;
		}
	};
} __g_lightEditorHelper;
