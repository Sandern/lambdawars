
#include "cbase.h"
#include "deferred/vgui/vgui_deferred.h"

#include "vgui/ISurface.h"


using namespace vgui;

REGISTER_PROJECTABLE_FACTORY( CVGUIMarquee, "marquee" );

CVGUIMarquee::CVGUIMarquee( const wchar_t *pszText, const int ispeed )
{
	m_pszText = NULL;
	Q_snprintf( m_szFont, sizeof( m_szFont ), "default" );

	SetText( pszText );
	SetMoveSpeed( ispeed );

	m_Font = 0;
	m_flCurOffset = 0;
}

CVGUIMarquee::~CVGUIMarquee()
{
	delete [] m_pszText;
}

void CVGUIMarquee::SetText( const wchar_t *pszText )
{
	delete [] m_pszText;

	if ( pszText == NULL || *pszText == '\0' )
	{
		pszText = NULL;
		return;
	}

	int textLen = Q_wcslen( pszText ) + 1;
	m_pszText = new wchar_t[ textLen ];
	Q_wcsncpy( m_pszText, pszText, textLen * sizeof( wchar_t ) );
}

void CVGUIMarquee::SetFont( vgui::HFont font )
{
	m_Font = font;
}

void CVGUIMarquee::SetMoveSpeed( const float flspeed )
{
	m_flMoveDir = flspeed;
}

void CVGUIMarquee::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	m_Font = scheme->GetFont( m_szFont );
}

void CVGUIMarquee::ApplyConfig( KeyValues *pKV )
{
	BaseClass::ApplyConfig( pKV );

	SetText( pKV->GetWString( "text", L"No text in marquee script!" ) );
	SetMoveSpeed( pKV->GetFloat( "speed" ) );
	Q_snprintf( m_szFont, sizeof( m_szFont ), "%s", pKV->GetString( "font", m_szFont ) );
}

void CVGUIMarquee::Paint()
{
	BaseClass::Paint();

	if ( m_pszText == NULL )
		return;

	int w, t;
	GetSize( w, t );

	int strWidth, strTall;
	surface()->GetTextSize( m_Font, m_pszText, strWidth, strTall );

	if ( strWidth < 1 )
		return;

	int iWalk = -strWidth + m_flCurOffset;

	surface()->DrawSetTextColor( GetFgColor() );
	surface()->DrawSetTextFont( m_Font );

	while ( iWalk < w )
	{
		surface()->DrawSetTextPos( iWalk, t * 0.5f - strTall * 0.5f );
		surface()->DrawPrintText( m_pszText, Q_wcslen( m_pszText ) );

		iWalk += strWidth;
	}
}

void CVGUIMarquee::OnThink()
{
	if ( m_pszText == NULL )
		return;

	int strWidth, strTall;
	surface()->GetTextSize( m_Font, m_pszText, strWidth, strTall );

	if ( strWidth < 1 )
		return;

	m_flCurOffset += m_flMoveDir * gpGlobals->frametime;

	while ( abs( m_flCurOffset ) > strWidth )
		m_flCurOffset -= strWidth * Sign( m_flCurOffset );
}