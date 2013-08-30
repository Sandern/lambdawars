
#include "cbase.h"
#include "deferred/deferred_shared_common.h"
#include "deferred/vgui/vgui_deferred.h"

#include "vgui_controls/radiobutton.h"
#include "vgui_controls/checkbutton.h"
#include "vgui_controls/fileopendialog.h"
#include "vgui_controls/querybox.h"
#include "vgui_controls/combobox.h"

#include "filesystem.h"

using namespace vgui;

ConVar deferred_lighteditor_defaultvmfpath( "deferred_lighteditor_defaultvmfpath", "", FCVAR_ARCHIVE );

CVGUILightEditor_Controls::CVGUILightEditor_Controls( Panel *pParent )
	: BaseClass( pParent, "LightEditorControls" )
{
	m_pFileVmf = NULL;

	m_pCBoxDbg = new ComboBox( this, "cboxdbg", 6, false );
	m_pCBoxDbg->AddItem( "None", NULL );
	m_pCBoxDbg->AddItem( "Lighting", NULL );
	m_pCBoxDbg->AddItem( "Depth", NULL );
	m_pCBoxDbg->AddItem( "Normals", NULL );

	LoadControlSettings( "resource/deferred/lighteditor_controls.res" );

	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetTitle( "Main controls", false );

	OnLevelSpawn();
}

CVGUILightEditor_Controls::~CVGUILightEditor_Controls()
{
}

void CVGUILightEditor_Controls::OnLevelSpawn()
{
	m_pCBoxDbg->ActivateItem( 0 );

	CheckButton *pCheckEnabled = assert_cast<CheckButton*>( FindChildByName( "check_editor_enable" ) );
	if ( pCheckEnabled != NULL )
		pCheckEnabled->SetSelected( true );

	RadioButton *pSelectLight = assert_cast<RadioButton*>( FindChildByName( "action_select" ) );
	if ( pSelectLight != NULL )
		pSelectLight->SetSelected( true );

	CheckButton *pCheckDebugStats = assert_cast<CheckButton*>( FindChildByName( "check_def_stats" ) );
	if ( pCheckDebugStats != NULL )
		pCheckDebugStats->SetSelected( deferred_lightmanager_debug.GetBool() );
}

void CVGUILightEditor_Controls::PerformLayout()
{
	BaseClass::PerformLayout();

	int sw, sh;
	engine->GetScreenSize( sw, sh );

	int w, h;
	GetSize( w, h );

	SetPos( 5, sh / 2 - h / 2 );
}

void CVGUILightEditor_Controls::OnRadioButtonChecked( Panel *panel )
{
	RadioButton *pRadioButton = assert_cast< RadioButton* >( panel );

	int iSubPos = pRadioButton->GetSubTabPosition();

	Assert( iSubPos >= 0 && iSubPos < CLightingEditor::EDITORINTERACTION_COUNT );

	GetLightingEditor()->SetEditorInteractionMode( (CLightingEditor::EDITORINTERACTION_MODE)iSubPos );
}

void CVGUILightEditor_Controls::OnCheckButtonChecked( Panel *panel )
{
	if ( !panel )
		return;

	const char *pszName = panel->GetName();
	CheckButton *pCheck = assert_cast< CheckButton* >( panel );
	bool bChecked = pCheck->IsSelected();

	if ( !Q_stricmp( pszName, "check_editor_enable" ) )
	{
		GetLightingEditor()->SetEditorActive( bChecked, false, true );
	}
	else if ( !Q_stricmp( pszName, "check_def_stats" ) )
	{
		deferred_lightmanager_debug.SetValue( ( bChecked ? 1 : 0 ) );
	}
}

void CVGUILightEditor_Controls::OnTextChanged( Panel *panel )
{
	if ( m_pCBoxDbg == panel )
	{
		CLightingEditor::EDITOR_DBG_MODES dbgMode =
			(CLightingEditor::EDITOR_DBG_MODES)m_pCBoxDbg->GetActiveItem();
		GetLightingEditor()->SetDebugMode( dbgMode );

		RequestFocus();
	}
}

void CVGUILightEditor_Controls::OnCommand( const char *pCmd )
{
	if ( !Q_stricmp( pCmd, "loadvmf" ) )
	{
		OnLoadVmf();
	}
	else if ( !Q_stricmp( pCmd, "savevmf" ) )
	{
		if ( GetLightingEditor()->GetCurrentVmfPath() &&
			*GetLightingEditor()->GetCurrentVmfPath() )
		{
			GetLightingEditor()->SaveCurrentVmf();
		}
	}
	else if ( !Q_stricmp( pCmd, "autoload_confirm" ) )
	{
		char tmp[MAX_PATH*4];
		BuildCurrentVmfPath( tmp, sizeof(tmp) );

		AssertMsg( g_pFullFileSystem->FileExists( tmp ), "Expected file unavailable (moved, deleted)." );

		LoadVmf( tmp );
	}
	else if ( !Q_stricmp( pCmd, "autoload_abort" ) )
	{
		OpenVmfFileDialog();
	}
	else if ( !Q_stricmp( pCmd, "toggleprops" ) )
	{
		PostActionSignal( new KeyValues( "ToggleEditorProperties" ) );
	}
	else if ( !Q_stricmp( pCmd, "editglobal" ) )
	{
		PostActionSignal( new KeyValues( "EditGlobalLight" ) );
	}
	else
		BaseClass::OnCommand( pCmd );
}

void CVGUILightEditor_Controls::OnLoadVmf()
{
	char tmp[MAX_PATH*4];
	bool bValidAutoPath = false;

	if ( BuildCurrentVmfPath( tmp, sizeof(tmp) ) )
	{
		bValidAutoPath = g_pFullFileSystem->FileExists( tmp );
	}

	if ( bValidAutoPath )
	{
		QueryBox *pQueryBox = new QueryBox( "Load VMF", VarArgs( "Do you want to load: %s?", tmp ), this );

		pQueryBox->AddActionSignalTarget( this );
		pQueryBox->SetOKCommand( new KeyValues( "Command", "command", "autoload_confirm" ) );
		pQueryBox->SetCancelCommand( new KeyValues( "Command", "command", "autoload_abort" ) );

		pQueryBox->DoModal();
	}
	else
	{
		OpenVmfFileDialog();
	}
}

void CVGUILightEditor_Controls::BuildVmfPath( char *pszOut, int maxlen, bool bMakeRelative )
{
	char tmp[MAX_PATH*4];
	char tmp2[MAX_PATH*4];

	if( Q_strcmp( deferred_lighteditor_defaultvmfpath.GetString(), "" ) != 0 &&
		g_pFullFileSystem->IsDirectory( deferred_lighteditor_defaultvmfpath.GetString() ) )
	{
		Q_strcpy( tmp, deferred_lighteditor_defaultvmfpath.GetString() );
	}
	else
	{
		Q_snprintf( tmp, sizeof(tmp), "%s/mapsrc", engine->GetGameDirectory() );
		Q_FixSlashes( tmp );
	}

	if ( bMakeRelative && g_pFullFileSystem->FullPathToRelativePath( tmp, tmp2, sizeof(tmp2) ) )
		Q_strcpy( tmp, tmp2 );
	else if ( !bMakeRelative && g_pFullFileSystem->RelativePathToFullPath( tmp, "MOD", tmp2, sizeof(tmp2) ) )
		Q_strcpy( tmp, tmp2 );

	Q_snprintf( pszOut, maxlen, "%s", tmp );
}

bool CVGUILightEditor_Controls::BuildCurrentVmfPath( char *pszOut, int maxlen )
{
	const char *pszLevelname = engine->GetLevelName();

	if ( !pszLevelname || !*pszLevelname )
		return false;

	char szBaseLevelName[MAX_PATH];
	Q_FileBase( pszLevelname, szBaseLevelName, sizeof(szBaseLevelName) );

	char tmp[MAX_PATH*4];
	char tmp2[MAX_PATH*4];

	BuildVmfPath( tmp2, sizeof(tmp2) );
	Q_snprintf( tmp, sizeof(tmp), "%s/%s.vmf", tmp2, szBaseLevelName );
	Q_FixSlashes( tmp );

	if ( g_pFullFileSystem->FullPathToRelativePath( tmp, tmp2, sizeof(tmp2) ) )
		Q_strcpy( tmp, tmp2 );

	Q_snprintf( pszOut, maxlen, "%s", tmp );
	return true;
}

void CVGUILightEditor_Controls::OpenVmfFileDialog()
{
	char szVmfPath[MAX_PATH*4];
	BuildVmfPath( szVmfPath, sizeof( szVmfPath ), false );

	if ( m_pFileVmf != NULL )
		m_pFileVmf->DeletePanel();

	m_pFileVmf = new FileOpenDialog( this, "Open vmf", FOD_OPEN );

	m_pFileVmf->SetDeleteSelfOnClose( false );
	m_pFileVmf->AddFilter( "*.vmf", "*.vmf", true );
	m_pFileVmf->AddActionSignalTarget( this );

	m_pFileVmf->SetStartDirectoryContext( "VMFContext", szVmfPath );
	m_pFileVmf->DoModal( false );
	m_pFileVmf->Activate();
}

void CVGUILightEditor_Controls::OnFileSelected( KeyValues *pKV )
{
	const char *pszFullpath = pKV->GetString( "fullpath" );

	LoadVmf( pszFullpath );
}

void CVGUILightEditor_Controls::LoadVmf( const char *pszPath )
{
	GetLightingEditor()->LoadVmf( pszPath );
	PostActionSignal( new KeyValues( "VmfPathChanged" ) );
}