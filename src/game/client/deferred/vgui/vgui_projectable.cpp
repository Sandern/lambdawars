
#include "cbase.h"
#include "deferred/deferred_shared_common.h"
#include "deferred/vgui/vgui_deferred.h"

#include "vgui/ipanel.h"
#include "vgui/isurface.h"


using namespace vgui;


CVGUIProjectable::CVGUIProjectable() : BaseClass( NULL, "" )
{
	m_pszControllableName = NULL;
	m_pszParentName = NULL;
	m_pConfig = NULL;

	SetVisible( true );

	CProjectableFactory::GetProjectableManager()->AddThinkTarget( this );
}

CVGUIProjectable::~CVGUIProjectable()
{
	CProjectableFactory::GetProjectableManager()->RemoveThinkTarget( this );

	delete [] m_pszControllableName;
	delete [] m_pszParentName;

	if ( m_pConfig )
		m_pConfig->deleteThis();
}

void CVGUIProjectable::SetControllableName( const char *name )
{
	delete [] m_pszControllableName;

	if ( name == NULL || *name == '\0' )
		return;

	m_pszControllableName = new char[ Q_strlen( name ) + 1 ];
	Q_strcpy( m_pszControllableName, name );
}

void CVGUIProjectable::SetParentName( const char *name )
{
	delete [] m_pszParentName;

	if ( name == NULL || *name == '\0' )
		return;

	m_pszParentName = new char[ Q_strlen( name ) + 1 ];
	Q_strcpy( m_pszParentName, name );
}

void CVGUIProjectable::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds( 0, 0, PROJECTABLE_VGUI_RES, PROJECTABLE_VGUI_RES );
}

void CVGUIProjectable::LoadProjectableConfig( KeyValues *pKV )
{
	if ( m_pConfig )
		m_pConfig->deleteThis();

	m_pConfig = pKV->MakeCopy();

	ApplyConfig( m_pConfig );
}

void CVGUIProjectable::ApplyConfig( KeyValues *pKV )
{
	SetFgColor( pKV->GetColor( "fgcolor", Color( 255,255,255,255 ) ) );
	SetBgColor( pKV->GetColor( "bgcolor", Color( 0,0,0,0 ) ) );
}

void CVGUIProjectable::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	if ( m_pConfig )
		ApplyConfig( m_pConfig );
}

void CVGUIProjectable::DrawSelfToRT( ITexture *pTarget, bool bClear )
{
	Assert( GetWide() == PROJECTABLE_VGUI_RES );
	Assert( GetTall() == PROJECTABLE_VGUI_RES );
	int px, py;
	GetPos( px, py );
	Assert( px == 0 );
	Assert( py == 0 );

	Assert( pTarget->GetActualWidth() == PROJECTABLE_VGUI_RES );
	Assert( pTarget->GetActualHeight() == PROJECTABLE_VGUI_RES );

	CViewSetup setup;
	setup.x = setup.y = 0;
	setup.width = PROJECTABLE_VGUI_RES;
	setup.height = PROJECTABLE_VGUI_RES;
	setup.m_bOrtho = false;
	setup.m_flAspectRatio = 1;
	setup.fov = 90;
	setup.zFar = 9999;
	setup.zNear = 10;

	Frustum frustum;
	render->Push2DView( setup, 0, pTarget, frustum );

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PushRenderTargetAndViewport( pTarget );

	if ( bClear )
	{
		pRenderContext->ClearColor3ub( 0, 0, 0 );
		pRenderContext->ClearBuffers( true, false );
	}

	surface()->DrawSetAlphaMultiplier( 1 );

	surface()->PushMakeCurrent( GetVPanel(), false );
	ipanel()->PaintTraverse( GetVPanel(), true );
	surface()->PopMakeCurrent( GetVPanel() );

	surface()->SwapBuffers( GetVPanel() );

	pRenderContext->PopRenderTargetAndViewport();
	render->PopView( frustum );
}
