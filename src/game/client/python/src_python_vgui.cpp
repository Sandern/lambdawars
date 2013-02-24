//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python_vgui.h"
#include "src_python.h"
#include <vgui_controls/Panel.h>
#include "clientmode_hl2wars.h"
#include <ienginevgui.h>
#include <vgui/ISystem.h>

#include "hl2wars/hl2wars_baseminimap.h"
#include "hl2wars/vgui_video_general.h"
#include "hl2wars/teamcolor_proxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

int g_PythonPanelCount = 0; // For debugging

static CWrapSurface gWrapSurface;
static CWrapIPanel gWrapIPanel;
static PyLocalize pylocalize;

CWrapSurface *wrapsurface() { return &gWrapSurface; }
CWrapIPanel *wrapipanel() { return &gWrapIPanel; }
PyLocalize *g_pylocalize = &pylocalize;

CUtlVector< PyPanel * > g_PyPanels;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PyPanel::PyPanel()
{
	g_PyPanels.AddToTail( this );
	m_bPyDeleted = false;

	m_bUseSurfaceCallBuffer = false;
	m_bFlushedByParent = false;

	m_PaintCallBuffer.m_bIsRecording = true;
	m_PaintCallBuffer.m_iNumSBufferCall = 0;
	m_PaintCallBuffer.m_bShouldRecordBuffer = false;
	m_PaintCallBuffer.m_SurfaceCallRecordFrameTime = -1;
	m_PaintCallBuffer.m_SurfaceCallDrawFrameTime = -1;

	m_PaintBackgroundCallBuffer.m_bIsRecording = true;
	m_PaintBackgroundCallBuffer.m_iNumSBufferCall = 0;
	m_PaintBackgroundCallBuffer.m_bShouldRecordBuffer = false;
	m_PaintBackgroundCallBuffer.m_SurfaceCallRecordFrameTime = -1;
	m_PaintBackgroundCallBuffer.m_SurfaceCallDrawFrameTime = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PyPanel::~PyPanel()
{
	g_PyPanels.FindAndRemove( this );
	ClearAllSBuffers();
	//m_bPyDeleted = true;
}

static ConVar cl_disablesbuffer( "cl_disablesbuffer", "0", 0 );
static ConVar cl_sbuffernoflush( "cl_sbuffernoflush", "0", 0 );
static ConVar cl_sbuffernodraw( "cl_sbuffernodraw", "0", 0 );
bool PyPanel::IsSBufferEnabled( void )
{
	return m_bUseSurfaceCallBuffer && !cl_disablesbuffer.GetBool();
}

void PyPanel::ClearSBuffer( CallBuffer_t &CallBuffer )
{
	for( int i = 0; i < CallBuffer.m_SCallBuffer.Count(); i++ )
		CallBuffer.m_SCallBuffer[i].PurgeAndDeleteElements();
	CallBuffer.m_SCallBuffer.Purge();
}

void PyPanel::FlushSBuffer( void ) 
{ 
	if( m_bFlushedByParent || cl_sbuffernoflush.GetBool() )
		return;
	//m_PaintBorderCallBuffer.m_bShouldRecordBuffer = true; 
	m_PaintCallBuffer.m_bShouldRecordBuffer = true; 
	m_PaintBackgroundCallBuffer.m_bShouldRecordBuffer = true; 
}

void PyPanel::ParentFlushSBuffer( void )
{
	if( cl_sbuffernoflush.GetBool() )
		return;
	//m_PaintBorderCallBuffer.m_bShouldRecordBuffer = true; 
	m_PaintCallBuffer.m_bShouldRecordBuffer = true; 
	m_PaintBackgroundCallBuffer.m_bShouldRecordBuffer = true; 
}

bool PyPanel::ShouldRecordSBuffer( CallBuffer_t &CallBuffer )
{ 
	// Recording this frame, so true (there might be multiple calls to paint during a frame)
	if( CallBuffer.m_SurfaceCallRecordFrameTime == vgui::system()->GetFrameTime() )
	{
		// Start recording
		CallBuffer.m_bIsRecording = true;
		CallBuffer.m_SCallBuffer.AddToTail();
		SBufferStartRecording( &CallBuffer.m_SCallBuffer.Tail() );
		return true;
	}

	// Decide if we should record or draw from the buffer
	if( !CallBuffer.m_bShouldRecordBuffer )
	{
		//DevMsg("Paint from buffer %s %s %d\n", GetName(), GetClassName(), m_SurfaceCallBuffer.Count() );
		return false;
	}

	// Clear buffer
	//DevMsg("Flushing sbuffer %f %s %s\n", m_SurfaceCallRecordFrameTime, GetName(), GetClassName());
	CallBuffer.m_SurfaceCallRecordFrameTime = vgui::system()->GetFrameTime();

	ClearSBuffer( CallBuffer );
	CallBuffer.m_SCallBuffer.AddToTail();

	// Subsequent calls will check the tick, so we can set it to false
	CallBuffer.m_bShouldRecordBuffer = false;
	
	// Start recording
	CallBuffer.m_bIsRecording = true;
	SBufferStartRecording( &CallBuffer.m_SCallBuffer.Tail() );
	return true;
}

bool PyPanel::IsRecordingSBuffer( )
{
	return m_PaintCallBuffer.m_SurfaceCallRecordFrameTime == vgui::system()->GetFrameTime() ||
		    m_PaintBackgroundCallBuffer.m_SurfaceCallRecordFrameTime == vgui::system()->GetFrameTime();
}

void PyPanel::FinishRecordSBuffer( CallBuffer_t &CallBuffer )
{
	if( !IsSBufferEnabled() || !CallBuffer.m_bIsRecording )
		return;
	CallBuffer.m_bIsRecording = false;
	SBufferFinishRecording();
}

void PyPanel::DrawFromSBuffer( CallBuffer_t &CallBuffer )
{
	if( cl_sbuffernodraw.GetBool() )
		return;

	// Update which set we are drawing (might end up with calling Paint multiple times per frame).
	if( CallBuffer.m_SurfaceCallDrawFrameTime != vgui::system()->GetFrameTime() )
	{
		CallBuffer.m_iNumSBufferCall = 0;
		CallBuffer.m_SurfaceCallDrawFrameTime = vgui::system()->GetFrameTime();
	}
	else
	{
		CallBuffer.m_iNumSBufferCall++;
	}

	if( !CallBuffer.m_SCallBuffer.IsValidIndex( CallBuffer.m_iNumSBufferCall ) )
		return;

	// Draw from buffer
	for( int i = 0; i < CallBuffer.m_SCallBuffer[CallBuffer.m_iNumSBufferCall].Count(); i++ )
	{
		CallBuffer.m_SCallBuffer[CallBuffer.m_iNumSBufferCall][i]->Draw();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clears all python panels
//-----------------------------------------------------------------------------
void DestroyPyPanels()
{
	DevMsg("Clearing %d python panels\n", g_PyPanels.Count());

	for( int i = g_PyPanels.Count()-1; i >= 0; i-- )
	{
		PyDeletePanel( dynamic_cast<Panel *>(g_PyPanels[i]), g_PyPanels[i]->GetPySelf(), i ); 
	}

	vgui::ivgui()->RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Panel handle implementation
//			Returns a pointer to a valid panel, NULL if the panel has been deleted
//-----------------------------------------------------------------------------
Panel *PyBaseVGUIHandle::Get() const
{
	if (m_hPanelHandle != INVALID_PANEL)
	{
		VPANEL panel = ivgui()->HandleToPanel(m_hPanelHandle);
		if (panel)
		{
			return ipanel()->GetPanel(panel, GetControlsModuleName());
		}
	}
	return NULL;
}

const PyBaseVGUIHandle& PyBaseVGUIHandle::Set( IClientPanel *pPanel )
{
	if (pPanel)
	{
		m_hPanelHandle = ivgui()->PanelToHandle(pPanel->GetVPanel());
	}
	else
	{
		m_hPanelHandle = INVALID_PANEL;
	}

	return *this; 
}

#define TEST_PANEL( type, pPanel ) \
	{							   \
	bp::wrapper< type > *wrapper = dynamic_cast<bp::wrapper< type > *>( pPanel ); \
	pObject = wrapper ? bp::detail::wrapper_base_::get_owner(*wrapper) : NULL; \
	if( pObject ) \
		return pObject; \
	}

PyObject *GetPyPanel( Panel *pPanel )
{
	PyObject *pObject;

	TEST_PANEL( Panel, pPanel );
	TEST_PANEL( EditablePanel, pPanel );
	TEST_PANEL( Frame, pPanel );
	TEST_PANEL( TextEntry, pPanel );
	TEST_PANEL( ScrollBar, pPanel );
	TEST_PANEL( ScrollBarSlider, pPanel );
	TEST_PANEL( AnimationController, pPanel );
	TEST_PANEL( RichText, pPanel );
	TEST_PANEL( CBaseMinimap, pPanel );
	TEST_PANEL( VideoGeneralPanel, pPanel );

	return NULL;
}

void PyDeletePanel( Panel *pPanel, PyObject *pPyPanel, int iRemoveIdx )
{
	if( !pPanel || !pPyPanel )
	{
		Warning("PyDeletePanel called with NULL panels %d %d\n", pPanel, pPyPanel);
		return;
	}

	// Do not cleanup twice
	PyPanel *pIPyPanel = dynamic_cast<PyPanel *>(pPanel);
	//Msg("Is py deleted: %d\n", pIPyPanel->m_bPyDeleted);
	if( pIPyPanel->m_bPyDeleted )
		return;

	// Allow c++ code to cleanup
	pIPyPanel->m_bPyDeleted = true;
	pIPyPanel->ClearAllSBuffers();
	//Msg("Removing py panel (%d active)\n", g_PyPanels.Count());
	// Remove from list
	if( iRemoveIdx != -1 )
		g_PyPanels.Remove( iRemoveIdx );
	else
		g_PyPanels.FindAndRemove( pIPyPanel );

	if( !SrcPySystem()->IsPythonRunning() )
	{
		//DevMsg("PyDeletePanel called while python is not running (%s)\n", pPanel->GetName());
		pPanel->SetParent((VPANEL)NULL);
		while (ipanel()->GetChildCount(pPanel->GetVPanel()))
		{
			VPANEL child = ipanel()->GetChild(pPanel->GetVPanel(), 0);
			ipanel()->SetParent(child, NULL);
		}

		return; // Already deleted everything
	}

	if( SrcPySystem()->IsPythonFinalizing() )
	{
		//DevMsg("PyDeletePanel: Python is finalizing. Not bothering to clean up panel\n");
		pPanel->SetParent((VPANEL)NULL);
		return;
	}

	bp::object pyPanel(bp::handle<>(boost::python::borrowed(pPyPanel)));

	// Tell panel it is being deleted
	boost::python::object updateondelete;
	try {
		updateondelete = pyPanel.attr("UpdateOnDelete");
	} catch(boost::python::error_already_set &) {
		PyErr_Clear();
	}
	if( updateondelete.ptr() != Py_None )
	{
		try {
			updateondelete();
		} catch(boost::python::error_already_set &) {
			PyErr_Print();
			PyErr_Clear();
		}
	}

	// remove panel from any list
	pPanel->SetParent((VPANEL)NULL);

	// Stop our children from pointing at us, and delete them if possible
	while (ipanel()->GetChildCount(pPanel->GetVPanel()))
	{
		VPANEL child = ipanel()->GetChild(pPanel->GetVPanel(), 0);
		if (ipanel()->IsAutoDeleteSet(child))
		{
			//ipanel()->DeletePanel(child);
			ipanel()->SetParent(child, NULL); // FIXME: can't do this if it's a python panel
		}
		else
		{
			ipanel()->SetParent(child, NULL);
		}
	}

	// delete VPanel
	// NOTE: Can't do this. Let's just make it invisible and remove any tick signal.
	//ivgui()->FreePanel(pPanel->GetVPanel());
	pPanel->SetVisible( false );
	pPanel->SetEnabled( false );
	pPanel->SetPaintBackgroundEnabled( false );
	pPanel->SetPaintEnabled( false );
	pPanel->SetPaintBorderEnabled( false );
	pPanel->SetMouseInputEnabled( false );
	pPanel->SetKeyBoardInputEnabled( false );
	ivgui()->RemoveTickSignal( pPanel->GetVPanel() );

	try {
		// Cause an attribute error when trying to access methods of this class
		// Calling methods after deletion might be dangerous, this prevents that...
		if( _vguicontrols.ptr() != Py_None )
			setattr(pyPanel, "__class__",  _vguicontrols.attr("DeadPanel"));
		else
			Warning("PyDeletePanel: Python shutting down while destroying panel %s\n", pPanel->GetName());
	} catch( boost::python::error_already_set &) {
		Warning("PyDeletePanel: error occurred while deleting Python panel %s\n", pPanel->GetName());
		PyErr_Print();
		PyErr_Clear();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Panel_DispatchMessage(CUtlDict<py_message_entry_t, short> &messageMap, const KeyValues *params, VPANEL fromPanel)
{
	short lookup = messageMap.Find(params->GetName());
	if( lookup == messageMap.InvalidIndex() )
	{
		// Not handled by the python map
		return false;
	}

	py_message_entry_t &entry = messageMap.Element(lookup);

	try {
		switch (entry.numParams)
		{
		case 0:
			{
				entry.method();
				break;
			}

		case 1:
			{
				KeyValues *param1 = params->FindKey(entry.firstParamSymbol);
				if (!param1)
				{
					param1 = const_cast<KeyValues *>(params);
				}

				switch ( entry.firstParamType )
				{
				case DATATYPE_INT:
					entry.method( param1->GetInt() );
					break;

				case DATATYPE_FLOAT:
					entry.method( param1->GetFloat() );
					break;

				case DATATYPE_UINT64:
					entry.method( param1->GetUint64() );
					break;

				case DATATYPE_CONSTCHARPTR:
					entry.method( param1->GetString() );
					break;

				case DATATYPE_CONSTWCHARPTR:
					entry.method( param1->GetWString() );
					break;

				case DATATYPE_KEYVALUES:
					typedef void (Panel::*MessageFunc_KeyValues_t)(KeyValues *);
					if ( entry.firstParamName )
					{
						entry.method( *((KeyValues *)param1->GetPtr()) );
					}
					else
					{
						// no param set, so pass in the whole thing
						entry.method( *(const_cast<KeyValues *>(params)) );
					}
					break;

				case DATATYPE_PTR:
					if (!stricmp(entry.firstParamName, "panel"))
					{
						// Use this as a special case. Always assume 'panel' is a Panel
						entry.method( *((Panel *)param1->GetPtr()) );
						break;
					}
					else
					{
						char buf[512];
						Q_snprintf( buf, 512, "DATATYPE_PTR: vgui python message function with argument name %s unsupported", entry.firstParamName );
						PyErr_SetString(PyExc_Exception, buf);
						throw boost::python::error_already_set(); 
					}

				default:
					//Assert(!("No handler for vgui message function"));
					PyErr_SetString(PyExc_Exception, "No handler for vgui message function" );
					throw boost::python::error_already_set(); 
					break;
				}
				break;
			}
		case 2:
			{
				KeyValues *param1 = params->FindKey(entry.firstParamSymbol);
				if (!param1)
				{
					param1 = const_cast<KeyValues *>(params);
				}
				KeyValues *param2 = params->FindKey(entry.secondParamSymbol);
				if (!param2)
				{
					param2 = const_cast<KeyValues *>(params);
				}

				if ( (DATATYPE_INT == entry.firstParamType) && (DATATYPE_INT == entry.secondParamType) )
				{
					entry.method( param1->GetInt(), param2->GetInt() );
				}
				else if ( (DATATYPE_PTR == entry.firstParamType) && (DATATYPE_INT == entry.secondParamType) )
				{
					if (!stricmp(entry.firstParamName, "panel"))
					{
						// Use this as a special case. Always assume 'panel' is a Panel
						entry.method( *((Panel *)param1->GetPtr()), param2->GetInt() );
						break;
					}
					else
					{
						char buf[512];
						Q_snprintf( buf, 512, "DATATYPE_PTR: vgui python message function with argument name %s unsupported", entry.firstParamName );
						PyErr_SetString(PyExc_Exception, buf);
						throw boost::python::error_already_set(); 
					}
				}
				else if ( (DATATYPE_CONSTCHARPTR == entry.firstParamType) && (DATATYPE_INT == entry.secondParamType) )
				{
					entry.method( param1->GetString(), param2->GetInt() );
				}
				else if ( (DATATYPE_CONSTCHARPTR == entry.firstParamType) && (DATATYPE_CONSTCHARPTR == entry.secondParamType) )
				{
					entry.method( param1->GetString(), param2->GetString() );
				}
				else if ( (DATATYPE_INT == entry.firstParamType) && (DATATYPE_CONSTCHARPTR == entry.secondParamType) )
				{
					entry.method( param1->GetInt(), param2->GetString() );
				}
				break;
			}
		default:
			break;
		}
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Maybe not very efficient, but it's nice and safe for python usage
//-----------------------------------------------------------------------------
void CWrapSurface::DrawFilledRectArray( boost::python::list rects )
{
	int i, n;
	n = boost::python::len( rects );
	IntRect *pRects = new IntRect[n];
	for(i=0; i<n; i++)
	{
		pRects[i] = boost::python::extract<IntRect>(rects[i]);
	}
	surface()->DrawFilledRectArray(pRects, n);
	delete pRects;
}

boost::python::tuple CWrapSurface::GetTextSize( HFont font, boost::python::object unistr )
{
	if (!PyString_Check(unistr.ptr()))
	{
		PyErr_SetString(PyExc_ValueError, "DrawUnicodeString: First argument must be a string.");
		throw boost::python::error_already_set(); 
	}

	const char* value = PyString_AsString(unistr.ptr());
	Py_ssize_t l = PyString_Size(unistr.ptr());
	if (value == 0) {
		PyErr_SetString(PyExc_ValueError, "DrawUnicodeString: String allocation error."); 
		throw boost::python::error_already_set();
	}
	wchar_t* w = new wchar_t[l+1];
	for(int i=0;i<l;i++){
		w[i] = value[i];
	}
	w[l] = '\0';

	int wide, tall;
	surface()->GetTextSize(font, w, wide, tall); 

	delete w;
	return bp::make_tuple(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Maybe not very efficient, but it's nice and safe for python usage
//-----------------------------------------------------------------------------
void CWrapSurface::DrawTexturedPolyLine( boost::python::list vertices )
{
	int i, n;
	Vertex_t *pVertices;
	try 
	{
		n = boost::python::len( vertices );
		pVertices = new Vertex_t[n];
		for(i=0; i<n; i++)
			pVertices[i] = boost::python::extract<Vertex_t>(vertices[i]);
	}
	catch(...)
	{
		PyErr_Print();
		return;
	}
	vgui::surface()->DrawTexturedPolyLine(pVertices, n);

	delete pVertices;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWrapSurface::DrawTexturedPolygon( boost::python::list vertices )
{
	int i, n;
	Vertex_t *pVertices;
	try 
	{
		n = boost::python::len( vertices );
		pVertices = new Vertex_t[n];
		for(i=0; i<n; i++)
			pVertices[i] = boost::python::extract<Vertex_t>(vertices[i]);
	}
	catch(...)
	{
		PyErr_Print();
		return;
	}
	vgui::surface()->DrawTexturedPolygon(n, pVertices);

	delete pVertices;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWrapSurface::DrawUnicodeString( boost::python::object unistr, FontDrawType_t drawType )
{
	if (!PyString_Check(unistr.ptr()))
	{
		PyErr_SetString(PyExc_ValueError, "DrawUnicodeString: First argument must be a string.");
		throw boost::python::error_already_set(); 
	}

	const char* value = PyString_AsString(unistr.ptr());
	Py_ssize_t l = PyString_Size(unistr.ptr());
	if (value == 0) {
		PyErr_SetString(PyExc_ValueError, "DrawUnicodeString: String allocation error."); 
		throw boost::python::error_already_set();
	}
	wchar_t* w = new wchar_t[l+1];
	for(int i=0;i<l;i++){
		w[i] = value[i];
	}
	w[l] = '\0';
	surface()->DrawUnicodeString(w, drawType);

	delete w;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWrapSurface::SetProxyUITeamColor( const Vector &vTeamColor )
{
	CSurfaceBuffered *pBufferSurface = dynamic_cast<CSurfaceBuffered *> ( surface() );
	if( pBufferSurface )
		pBufferSurface->SetProxyUITeamColor( vTeamColor );
	else
		::SetProxyUITeamColor( vTeamColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWrapSurface::ClearProxyUITeamColor( void )
{
	CSurfaceBuffered *pBufferSurface = dynamic_cast<CSurfaceBuffered *> ( surface() );
	if( pBufferSurface )
		pBufferSurface->ClearProxyUITeamColor();
	else
		::ClearProxyUITeamColor();
}

//=============================================================================
// ILocalize
//=============================================================================
// converts an english string to unicode
// returns the number of wchar_t in resulting string, including null terminator
boost::python::tuple PyLocalize::ConvertANSIToUnicode(const char *ansi) 
{ 
	wchar_t unicode[1024];
	int n = g_pVGuiLocalize->ConvertANSIToUnicode(ansi, unicode, 1024); 
	return boost::python::make_tuple( boost::python::object(unicode), n );
}

// converts an unicode string to an english string
// unrepresentable characters are converted to system default
// returns the number of characters in resulting string, including null terminator
boost::python::tuple PyLocalize::ConvertUnicodeToANSI(const wchar_t *unicode)
{ 
	char ansi[1024];
	int n = g_pVGuiLocalize->ConvertUnicodeToANSI(unicode, ansi, 1024); 
	return boost::python::make_tuple( boost::python::object(ansi), n );
}

boost::python::object PyLocalize::ConstructString(const char *tokenName, KeyValues *localizationVariables)
{	
	wchar_t unicodeOutput[1024];
	g_pVGuiLocalize->ConstructString(unicodeOutput, 1024, tokenName, localizationVariables);
	return boost::python::object(unicodeOutput);
}
boost::python::object PyLocalize::ConstructString(StringIndex_t unlocalizedTextSymbol, KeyValues *localizationVariables)	
{
	wchar_t unicodeOutput[1024];
	g_pVGuiLocalize->ConstructString(unicodeOutput, 1024, unlocalizedTextSymbol, localizationVariables);
	return boost::python::object(unicodeOutput);
}


//=============================================================================
// Misc
//=============================================================================
bool PyIsGameUIVisible()
{
	return enginevgui->IsGameUIVisible();
}

VPANEL PyGetPanel( VGuiPanel_t type )
{
	return enginevgui->GetPanel( type );
}

CON_COMMAND(py_debug_panel_count, "Debug command for getting the number of python panels.")
{
	Msg("Python Panels: %d\n", g_PythonPanelCount );
}