
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include "wx/intl.h"
//#include "DecoderFrame.h"
// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------
//extern "C" int decode(MyServer* srv);
#define EVT_MY_CUSTOM_COMMAND(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_NEW_TEXT, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#define APPNAME "Dictomash"
// Define a new application type, each program should derive a class from wxApp
class DecoderApp : public wxApp
{
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit();
	virtual int OnExit();
 // 2nd-level exception handling: we get all the exceptions occurring in any
    // event handler here
    virtual bool OnExceptionInMainLoop();

    // 3rd, and final, level exception handling: whenever an unhandled
    // exception is caught, this function is called
    virtual void OnUnhandledException();

    // and now for something different: this function is called in case of a
    // crash (e.g. dereferencing null pointer, division by 0, ...)
    virtual void OnFatalException();

	
private:
	wxLocale m_locale;


};

IMPLEMENT_APP(DecoderApp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
int decode();

bool DecoderApp::OnInit()
{
    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;

	wxHandleFatalExceptions(false);
	wxLocale::AddCatalogLookupPathPrefix(wxT("./lang"));
	bool check = m_locale.Init(wxLANGUAGE_KAZAKH, wxLOCALE_CONV_ENCODING);
	check = m_locale.AddCatalog(_(APPNAME));

	SetVendorName(_T(APPNAME));
	int l = m_locale.GetLanguage();
	wxString s = m_locale.GetLanguageName(l);

	m_locale.Init(wxLANGUAGE_UNKNOWN, wxLOCALE_CONV_ENCODING);
	if (decode()<0){
		wxMessageBox(_T("Server error"), _T("System messages"), wxOK | wxICON_ERROR, NULL);
	} else
		wxMessageBox(_T("Service completed"), _T("System messages"), wxOK | wxICON_INFORMATION, NULL);

	return false;
 //   return true;
}
int DecoderApp::OnExit(){

  return -1;
}
bool DecoderApp::OnExceptionInMainLoop()
{
    try
    {
        throw;
    }
    catch ( ... )
    {
        throw;
    }

}

void DecoderApp::OnUnhandledException()
{
    try
    {
        throw;
    }
    catch ( ... )
    {
        wxMessageBox(_T("Помогите !!! Умираю !!!"),
                     _T("Смертельная ошибка"), wxOK | wxICON_ERROR);
    }
}

void DecoderApp::OnFatalException()
{
    wxMessageBox(_T("Fatal error"),
                 _T("Server error"), wxOK | wxICON_ERROR);
	if (::wxFileExists(_("semafor")))
		::wxRemoveFile(_("semafor"));
	exit(-1);
}

DECLARE_APP(DecoderApp)

