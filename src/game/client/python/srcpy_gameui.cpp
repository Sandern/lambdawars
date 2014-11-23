//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_gameui.h"

#include "gameui/wars/basemodpanel.h"
#include "gameui/wars/basemodframe.h"

#ifdef WIN32
#include <winlite.h>
#endif // WIN32

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern boost::python::object s_ref_ui_basemodpanel;

//-----------------------------------------------------------------------------
// Temporary function until everything is ported over to the new html based
// menu.
//-----------------------------------------------------------------------------
void PyGameUICommand( const char *command )
{
	BaseModUI::WINDOW_TYPE type = BaseModUI::CBaseModPanel::GetSingleton().GetActiveWindowType();
	BaseModUI::CBaseModFrame *pActivePanel = BaseModUI::CBaseModPanel::GetSingleton().GetWindow( type );
	if( pActivePanel )
		pActivePanel->OnCommand( command );
}

void PyGameUIOpenWindow( BaseModUI::WINDOW_TYPE windowType, bool hidePrevious, KeyValues *pParameters )
{
	BaseModUI::CBaseModPanel &baseModPanel = BaseModUI::CBaseModPanel::GetSingleton();
	baseModPanel.OpenWindow( windowType, NULL, hidePrevious, pParameters );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
boost::python::object GetMainMenu()
{
	return s_ref_ui_basemodpanel;
}

#ifdef WIN32
static boost::python::object ConvertLangID2LocaleInfo( LCID locale )
{
	boost::python::object localeName;
#ifdef WIN32
	int nchars = GetLocaleInfoW(locale, LOCALE_SISO639LANGNAME, NULL, 0);
	wchar_t* pLanguageCode = new wchar_t[nchars];
	GetLocaleInfoW(locale, LOCALE_SISO639LANGNAME, pLanguageCode, nchars);

	localeName = boost::python::object( boost::python::handle<>(
		PyUnicode_FromWideChar(pLanguageCode, nchars) 
	) );

	delete pLanguageCode;
#else
	localeName = boost::python::object("");
#endif // WIN32
	return localeName;
}
#endif // WIN32

boost::python::object GetKeyboardLangIds()
{
	boost::python::list langids = boost::python::list();
#ifdef WIN32
	HKL buf[1024];
	int n = GetKeyboardLayoutList(sizeof(buf), buf);
	for( int i = 0; i < n; i++ )
	{
		langids.append( ConvertLangID2LocaleInfo( LOWORD(buf[i]) ) );
	}
#endif // WIN32
	return langids;
}

boost::python::object GetCurrentKeyboardLangId()
{
	boost::python::object localeName;
#ifdef WIN32
	HKL currentKb = ::GetKeyboardLayout( 0 );
	localeName = ConvertLangID2LocaleInfo( LOWORD(currentKb) );
#else
	localeName = boost::python::object("");
#endif // WIN32
	return localeName;
}