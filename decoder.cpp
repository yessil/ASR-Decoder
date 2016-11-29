#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include "sphinxbase/info.h"
#include "sphinxbase/unlimit.h"
#include "utt.h"
#include "kb.h"
#include "srch_output.h"
#include "corpus.h"
#include "filename.h" 
#include "cmdln_macro.h"
#include "wx/socket.h"
#include "wx/textfile.h"
#include "wx/string.h"
#include <wx/process.h>
#include <wx/wfstream.h>
#include <wx/hashmap.h>
#include <wx/tokenzr.h>


WX_DECLARE_STRING_HASH_MAP( wxString, MorphMap );
WX_DECLARE_STRING_HASH_MAP( wxString, TransMap );


static arg_t arg[] = {
    log_table_command_line_macro(),
    waveform_to_cepstral_command_line_macro(),
    cepstral_to_feature_command_line_macro(),

    acoustic_model_command_line_macro(),
    speaker_adaptation_command_line_macro(),
    language_model_command_line_macro(),
    dictionary_command_line_macro(),
    phoneme_lookahead_command_line_macro(),
    histogram_pruning_command_line_macro(),
    fast_GMM_computation_command_line_macro(),
    common_filler_properties_command_line_macro(),
    common_s3x_beam_properties_command_line_macro(),
    common_application_properties_command_line_macro(),
    control_file_handling_command_line_macro(),
    hypothesis_file_handling_command_line_macro(),
    score_handling_command_line_macro(),
    output_lattice_handling_command_line_macro(),
    dag_handling_command_line_macro(),
    second_stage_dag_handling_command_line_macro(),
    input_lattice_handling_command_line_macro(),
    flat_fwd_debugging_command_line_macro(),
    history_table_command_line_macro(),

    cepstral_input_handling_command_line_macro(),
    decode_specific_command_line_macro(),
    search_specific_command_line_macro(),
    search_modeTST_specific_command_line_macro(),
    search_modeWST_specific_command_line_macro(),
    control_lm_mllr_file_command_line_macro(),
    finite_state_grammar_command_line_macro(),
    phone_insertion_penalty_command_line_macro(),

    /* Things are yet to refactored */
#if 0
    /* Commented out; not supported */
    {"-compsep",
     ARG_STRING,
     /* Default: No compound word (NULL separator char) */
     "",
     "Separator character between components of a compound word (NULL if "
     "none)"},
#endif

    {"-phsegdir",
     ARG_STRING,
     NULL,
     "(Allphone mode only) Output directory for phone segmentation files"},

    {"-bestscoredir",
     ARG_STRING,
     NULL,
     "(Mode 3) Directory for writing best score/frame (used to set beamwidth; "
     "one file/utterance)"},

    /** ARCHAN 20050717: The only argument which I didn't refactor,
        reason is it makes sense to make every s3.0 family of tool to
        accept -utt.  DHD 20070525: I have no idea what that means. */

    {"-utt",
     ARG_STRING,
     NULL,
     "Utterance file to be processed (-ctlcount argument times)"},
  { "-port", \
      ARG_INT32, \
      "3000", \
      "listening port for decoder server" },
    { "-timeout", \
      ARG_INT32, \
      "60", \
      "timeout is seconds for decoder server" },
    {NULL, ARG_INT32, NULL, NULL}

};


#define DECODEDTEXT _T("decoded.txt")
#define TEMPFILE _T("temp.txt")
#define BUFLEN 500
#define MAPFILE _T("etc/w2m.txt")
#define TRANSMAPFILE _T("etc/kz2ru.txt")

void delay(){

	float f = 1.1f;
	int i;
	for(i=0; i< 200000; f*=.98 * exp(f), i++);//Sleep(1000); 
}

wxString GetLastLine(MorphMap &m){

	wxTextFile f;
	wxString s;
	wxString word;
	wxArrayString morphs;
	wxString tmp;
	int j, n=0;

	if ( !wxFile::Exists(DECODEDTEXT))
		return _T("");

	f.Open(DECODEDTEXT);
	if (f.GetLineCount()==0)
		return _T("");
	s = f.GetLastLine();
	morphs = wxStringTokenize(s, _("(")); 
	word=morphs[0];
	word.Trim().Append(_T(" "));
	wxPuts(word);
	f.Close();
	if (m.size()==0)
	    return word;
	morphs = wxStringTokenize(s, _(" ")); 
	tmp =_("");
	n = morphs.Count();
	for (int i=0; i<n; i++){
		int k=4, l=0;
		while (l ==0 && --k>0){
			word =_("");
			for (int ii=0; ii <k && i+ii < n; ii++){
				word += morphs[i+ii]+ _(" ");
				j = i+ii;
			}
			word.Trim();
			l = m[word].Length();
		}
		if (l>0) 
			word = m[word];
		i = j;
		tmp+= word + _(" ");
	}
	return tmp;
}

wxString GetTranslation(TransMap &m){

	wxTextFile f;
	wxString s;
	wxArrayString words;
	wxString tmp;
	int n=0;

	if ( !wxFile::Exists(DECODEDTEXT))
		return _T("");

	f.Open(DECODEDTEXT);
	if (f.GetLineCount()==0)
		return _T("");
	s = f.GetLastLine();
	s.Replace(_("Result:"), _(""));
	s.Trim().Append(_T(" "));
	f.Close();
	if (m.size()==0)
	    return s;
	words = wxStringTokenize(s, _(" ")); 
	tmp =_("");
	n = words.Count();
	for (int i=0; i<n; i++){
	    tmp += m[words[i]] + _(" ");
	}
	return tmp;
}

void FillMap(MorphMap &map){

	wxTextFile f;
	wxString s, w, m;
	int t = -1;

	if ( !wxFile::Exists(MAPFILE))
		return;

	f.Open(MAPFILE);
	if (f.GetLineCount()==0)
		return;
	for (s= f.GetFirstLine(); !f.Eof(); s = f.GetNextLine()){
		t = s.Find(_("\t"));
		w = s.Mid(0, t);
		m = s.Mid(t+1);
		map[m]=w;
	}
	f.Close();
}

void FillTranslator(TransMap &map){

	wxTextFile f;
	wxString s, w, m;
	int t = -1;

	if ( !wxFile::Exists(TRANSMAPFILE))
		return;

	f.Open(TRANSMAPFILE);
	if (f.GetLineCount()==0)
		return;
	for (s= f.GetFirstLine(); !f.Eof(); s = f.GetNextLine()){
		t = s.Find(_("\t"));
		w = s.Mid(0, t);
		m = s.Mid(t+1);
		map[w]=m;
	}
	f.Close();
}


int ReceiveFile( wxSocketBase *sock){

    char buf[BUFLEN];
    int len = 0;
    int res = -1;

    wxFileOutputStream fo(_("feat/recorded.mfc"));
    while (true){
	if (!sock->IsConnected())
	    break;
	sock->ReadMsg(buf, BUFLEN);
	len = sock->LastCount();
	if (len > 0){
	    res = 0;
	    fo.Write(buf, len);
	    if (len<BUFLEN)
		break;
	}
    }
    if (len>0){
        if (buf[0]=='0' && len ==1){
            res = -2;
        }
    } 
    fo.Close();
    return res;
}

int
process_utt(char *uttfile, 
                void (*func) (void *kb, utt_res_t * ur, int32 sf, int32 ef, char *uttid), void *kb, int port, int timeout)
{
    char uttid[4096];
    char base[16384];
    int32 c, res;
    ptmr_t tm;
    utt_res_t *ur;
    wxSocketServer *server;
    wxIPV4address addr;
    wxSocketBase* sock = NULL;
    wxUint32 sz = 0;
    wxString s;
    MorphMap m2w;
    TransMap translator;

	wxApp::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE, "program");

    wxInitializer initializer;
    if ( !initializer )
    {
        E_ERROR("Failed to initialize the wxWidgets library, aborting.\n");
        return -1;
    }

    addr.Service(port);
  // Create the socket
    server = new wxSocketServer(addr);
    server->SetTimeout(timeout);
    E_INFO("Timeout: %d\n", timeout);

  // We use Ok() here to see if the server is really listening
    if (!server->Ok()){
		E_ERROR("Server is down !\n");
		return -1;
    }

    ptmr_init(&tm);
    ur = new_utt_res();
//    path2basename(uttfile, base);
    FillMap(m2w);
    FillTranslator(translator);
    if (m2w.size()==0){
	E_INFO("Full word model\n");
    } else {
	E_INFO("Subword model\n");
    }

    if (translator.size()==0){
	E_INFO("No translation\n");
    } else {
	E_INFO("Translator file %s\n", TRANSMAPFILE);
    }

    c=1;
    while(true){

	E_INFO("Waiting for connection\n");
        while (!(sock && sock->IsConnected())){
//  		sock->SetTimeout(timeout);
		sock = server->Accept();
	}
		E_INFO("Waiting for %s, #: %d\n", uttfile, c++);
		res = ReceiveFile(sock);
		if (res==0){
//			delay();
			sprintf(uttid, "%s_%08d", base, c);
			/* Process this utterance */
			ptmr_start(&tm);
			if (func) {
				utt_res_set_uttfile(ur, uttfile);
				(*func) (kb, ur, 0, -1, uttid);
			}
			ptmr_stop(&tm);
			E_INFO("%s: %6.1f sec CPU, %6.1f sec Clk;  TOT: %8.1f sec CPU, %8.1f sec Clk\n\n",
					 uttid, tm.t_cpu, tm.t_elapsed, tm.t_tot_cpu, tm.t_tot_elapsed);
			ptmr_reset(&tm);
			s = GetLastLine(m2w);// wxString::FromUTF8(srch_output_result());
//			s = GetTranslation(translator);
			wxPuts(s);
			sz = s.Len()*sizeof(wxChar)+2;
		        while (!(sock && sock->IsConnected())){
			    sock = server->Accept();
			}
//			sock->WriteMsg(s.c_str(), sz);
printf("sz= %d\n", sz);
			sock->WriteMsg(s, sz);
//			sock->WriteMsg(_("Test test"), 10);
		
			sock->Close();
			E_INFO("Result sent %d\n", s.Length());
			
		}
		if (res==-2)
		    break;
    }
    E_INFO(" >> Exiting loop <<\n");
    if (ur)
        free_utt_res(ur);
	if (sock!=NULL)
		sock->Destroy();
    delete server;
    return res;
}

kb_t kb;

int decode(char** argv){

	int res, port, timeout;
	cmd_ln_t *config;
	const char *logfile;
	
	config = cmd_ln_parse_file_r(NULL, arg, argv[1], FALSE);
	unlimit();

	if (logfile = cmd_ln_str_r(config, "-logfn")){
		remove(logfile);
		err_set_logfile(logfile);
	}
	port = cmd_ln_int_r(config, "-port");
	timeout = cmd_ln_int_r(config, "-timeout");

	kb_init(&kb, config);
	/* When -utt is specified, corpus.c will wait for the utterance to change */
	res = 0 ;

	res = process_utt((char*)cmd_ln_str_r(config,"-utt"), utt_decode, &kb, port, timeout);

	if (kb.matchsegfp)
		fclose(kb.matchsegfp);
	if (kb.matchfp)
		fclose(kb.matchfp);
	if (kb.kbcore != NULL){
		kb_free(&kb);
	}

	cmd_ln_free_r(config);
	if (res==1)
		E_INFO(">> Stopped <<");
	return res;
}

int main(int argc, char** argv){

    int res = 0;
    while (res==0){
        res = decode(argv);
    }
    printf("\nThat's all folks !\n");
    return 0;
}
