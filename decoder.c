/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * decode.c --  
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * 29-Feb-2000	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified to allow runtime choice between 3-state and 5-state HMM
 * 		topologies (instead of compile-time selection).
 * 
 * 13-Aug-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added -maxwpf.
 * 
 * 10-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */

/** \file decode.c
 * \brief main function for application decode
 */
#include "wx/wxprec.h"
// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
//#include "info.h"
//#include "unlimit.h"
#include "utt.h"
#include "kb.h"
#include "corpus.h"
#include "srch_output.h"
#include "srch.h"
#include "bio.h"
#include "filename.h" 
#include "cmdln_macro.h"
#include "wx/socket.h"
#include "wx/textfile.h"
#include "wx/string.h"
#include <wx/process.h>
#include <wx/wfstream.h>
#include <wx/dir.h>
#include "ASRSrvApp.h"
//#include "wx/msw/ole/automtn.h"

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
#define CMD_STOP 2
#define CEPDIR	"mfc"
WX_DECLARE_STRING_HASH_MAP(wxString, PhraseBookEn);
WX_DECLARE_STRING_HASH_MAP(wxString, PhraseBookRu);


void delay(){

	float f = 1.1f;
	int i;
	for(i=0; i< 200000; f*=.98 * exp(f), i++);//Sleep(1000); 
}

wxString GetLastLine(){

	wxTextFile f;
	wxString s;

	if (wxFile::Exists(DECODEDTEXT)){
		f.Open(DECODEDTEXT);
		if (f.GetLineCount()>0){
			s = f.GetLastLine().BeforeLast('(');
			s.Trim().Append(_T(" "));
		} else {
			s = _T("SIL");
		}
		f.Close();
		return s;
	}
	return _T("");

}

void fillHashMaps(PhraseBookEn& tableEn, PhraseBookRu& tableRu, wxString fileName){

	wxTextFile f;
	wxString sk, se, sr;
	
	if (wxFile::Exists(fileName)){
		f.Open(fileName);
		if (f.GetLineCount()>0){
			f.GetFirstLine();// first line in the file must be bogus line !
			while (!f.Eof()){
				if (!f.Eof())
					se = f.GetNextLine();
				if (!f.Eof())
					sk = f.GetNextLine();
				if (!f.Eof())
					sr = f.GetNextLine();
				else
					break;
				tableEn[sk] = se;
				tableRu[sk] = sr;
			}
		}
		else {
			E_WARN("HashMap is empty !");
		}
		f.Close();
	}

}

int testsock(){

	wxSocketServer *server;
	wxIPV4address addr;
	wxSocketBase* sock = NULL;
	char buf[21];
	wxUint32 sz = 21;
	addr.Service(3000);
  // Create the socket
	server = new wxSocketServer(addr);

  // We use Ok() here to see if the server is really listening
	if (! server->Ok()){
		E_ERROR("Server is down !\n");
		return -2;
	}

	if (sock==NULL || sock->IsDisconnected()){
		E_INFO("Waiting for connection\n");
		sock = server->Accept(true);
		sock->SetFlags(wxSOCKET_WAITALL);
	}
	E_INFO("Waiting\n");
	memset(buf, 0, sz);
	sock->ReadMsg(buf, sz-1);
	return 0;
}


int ReceiveFile( wxSocketBase *sock){

	char buf[BUFLEN];
	int len = 0;
	int res = -1;
	wxString file = wxGetCwd().Append(_("/")).Append(_(CEPDIR)).Append(_("/recorded.mfc"));
	wxFileOutputStream fo(file);

	while (true){
		sock->ReadMsg(buf, BUFLEN);
		len = sock->LastCount();
		if (len > 0)
			res = 0;
		if (len<BUFLEN)
			break;
		fo.Write(buf, len);
	}
	if (len>0){
		fo.Write(buf, len);
		if (buf[0]=='0' && len ==1)
			res = CMD_STOP;
	}
	fo.Close();
	wxCopyFile(file, _("recorded.mfc.bak"));
	return res;
}

int
utt_decode2(void *data, utt_res_t * ur, int32 sf, int32 ef, char *uttid)
{
	kb_t *kb;
	kbcore_t *kbcore;
	cmd_ln_t *config;
	int32 num_decode_frame;
	int32 total_frame;
	stat_t *st;
	srch_t *s;
	int res = 0;

	num_decode_frame = 0;
	E_INFO("Processing: %s\n", uttid);

	kb = (kb_t *)data;
	kbcore = kb->kbcore;
	config = kbcore_config(kbcore);
	kb_set_uttid(uttid, ur->uttfile, kb);
	st = kb->stat;

	/* Convert input file to cepstra if waveform input is selected */
	if (cmd_ln_boolean_r(config, "-adcin")) {
		int16 *adcdata;
		size_t nsamps = 0;

		if ((adcdata = bio_read_wavfile(cmd_ln_str_r(config, "-cepdir"),
			ur->uttfile,
			cmd_ln_str_r(config, "-cepext"),
			cmd_ln_int32_r(config, "-adchdr"),
			strcmp(cmd_ln_str_r(config, "-input_endian"), "big"),
			&nsamps)) == NULL) {
			res = -1;
			E_ERROR("Cannot read file %s\n", ur->uttfile);
		}
		if (kb->mfcc) {
			ckd_free_2d((void **)kb->mfcc);
		}
		fe_start_utt(kb->fe);
		if (fe_process_utt(kb->fe, adcdata, nsamps, &kb->mfcc, &total_frame) < 0) {
			res = -1;
			E_ERROR("MFCC calculation failed\n", ur->uttfile);
		}
		ckd_free(adcdata);
		if (total_frame > S3_MAX_FRAMES) {
			res = -1;
			E_ERROR("Maximum number of frames (%d) exceeded\n", S3_MAX_FRAMES);
		}
		if ((total_frame = feat_s2mfc2feat_live(kbcore_fcb(kbcore),
			kb->mfcc,
			&total_frame,
			TRUE, TRUE,
			kb->feat)) < 0) {
			res = -1;
			E_ERROR("Feature computation failed\n");
		}
	}
	else {
		/* Read mfc file and build feature vectors for entire utterance */
		if ((total_frame = feat_s2mfc2feat(kbcore_fcb(kbcore), ur->uttfile,
			cmd_ln_str_r(config, "-cepdir"),
			cmd_ln_str_r(config, "-cepext"), sf, ef,
			kb->feat, S3_MAX_FRAMES)) < 0) {
			res = -1;
			E_ERROR("Cannot read file %s. Forced exit\n", ur->uttfile);
		}
	}

	/* Also need to make sure we don't set resource if it is the same. Well, this mechanism
	could be provided inside the following function.
	*/
	s = (srch_t*)kb->srch;
	if (ur->lmname != NULL)
		srch_set_lm(s, ur->lmname);
	if (ur->regmatname != NULL)
		kb_setmllr(ur->regmatname, ur->cb2mllrname, kb);
	/* These are necessary! */
	s->uttid = kb->uttid;
	s->uttfile = kb->uttfile;

	utt_begin(kb);
	utt_decode_block(kb->feat, total_frame, &num_decode_frame, kb);
	utt_end(kb);

	st->tot_fr += st->nfr;
	return res;
}


int
process_utt(char *uttfile, int (*func) (void *kb, utt_res_t * ur, int32 sf, int32 ef, char *uttid), void *kb, int port, int timeout)
{
    char uttid[4096];
    char base[16384];
    int32 c, res;
    ptmr_t tm;
    utt_res_t *ur;
	wxSocketServer *server;
	wxIPV4address addr;
	wxSocketBase* sock = NULL;
	wxString s;
	PhraseBookEn en;
	PhraseBookRu ru;

	fillHashMaps(en, ru, _("etc/phrasebook.txt"));
	addr.Service(port);
	s = addr.IPAddress();
  // Create the socket
	server = new wxSocketServer(addr);
	server->SetTimeout(timeout);

  // We use Ok() here to see if the server is really listening
	if (!server->Ok()){
		E_ERROR("Server is down !\n");
		return -2;
	}

    ptmr_init(&tm);
    ur = new_utt_res();
    //path2basename(uttfile, base);
	strcpy(base, path2basename(uttfile));

	c=1;
	E_INFO("Listening on port %d, timeout %d\n", port, timeout);

	while(true){

		while (sock==NULL || sock->IsDisconnected()){
			ptmr_start(&tm);
			E_INFO("Waiting for connection: ");
			sock = server->Accept(true);
			E_INFO("%6.1f sec\n", tm.t_elapsed);
			ptmr_stop(&tm);
		}
		E_INFO("Waiting for %s, #: c %d\n", uttfile, c++);
		res = ReceiveFile(sock);
		if (res==0){
			sprintf(uttid, "%s_%08d", base, c);
     		/* Process this utterance */
			ptmr_start(&tm);

			if (func) {
				utt_res_set_uttfile(ur, uttfile);
				if ((*func) (kb, ur, 0, -1, uttid) <0)
					continue;
			}

			ptmr_stop(&tm);
			E_INFO("%s: %6.1f sec CPU, %6.1f sec Clk;  TOT: %8.1f sec CPU, %8.1f sec Clk\n\n", uttid, tm.t_cpu, tm.t_elapsed, tm.t_tot_cpu, tm.t_tot_elapsed);
			ptmr_reset(&tm);
			E_INFO("Sending results back\n");
			s = GetLastLine().Trim(true); 
			//s.Append(_("\n Eng: ")).Append(en[s]).Append(_("\n Rus: ")).Append(ru[s]);
			int l = s.Len()*sizeof(wxChar)+2;
			sock->WriteMsg(s, l);

		} else 
			if (res==CMD_STOP)
				break;

	}
	E_INFO("That's all folks !\n");
    if (ur)
        free_utt_res(ur);
	if (sock!=NULL)
		sock->Destroy();
	delete server;

    return res;
}


kb_t kb;
int doDecode(char** argv);

int decode(){

	char* argv[] = {"dummy.exe", "etc/decode.cfg"};
	return doDecode(argv);
}


void checkdict(){

	char line[16384];
	FILE* fp = fopen("etc\\missed.txt", "r");
	if (fp == NULL)
		return;

	if (fgets(line, sizeof(line), fp) != NULL) {
		fclose(fp);
		wxProcess *p = wxProcess::Open(_T("cmd /c newdic.bat"));
		while(wxFile::Exists(_T("busy")));
		delete p;
		p = NULL;
	}
	else {
		fclose(fp);
	}
	return;

}


int
doDecode(char** argv)
{
	int res, port, timeout;
    cmd_ln_t *config;
	const char *logfile;
	wxString workDir;
	wxGetEnv(_("PROGRAMDATA"), &workDir);
	workDir.Append(_("\\")).Append(_(APPNAME));
	if (!wxDir::Exists(workDir)) {
		wxMkdir(workDir);
	}
	wxSetWorkingDirectory(workDir);
	//hope
	wxMkdir(_(CEPDIR));

	config = cmd_ln_parse_file_r(NULL, arg, argv[1], FALSE);

	if (logfile = cmd_ln_str_r(config, "-logfn")){
		remove(logfile);
		err_set_logfile(logfile);
	}
	port = cmd_ln_int_r(config, "-port");
	timeout = cmd_ln_int_r(config, "-timeout");

	kb_init(&kb, config);
	/* When -utt is specified, corpus.c will wait for the utterance to change */
	res = 0 ;

	//checkdict();
	//testsock();
	res = process_utt((char*)cmd_ln_str_r(config,"-utt"), utt_decode2, &kb, port, timeout);

	if (kb.matchsegfp)
		fclose(kb.matchsegfp);
	//if (kb.matchfp)
		//fclose(kb.matchfp);
	if (kb.kbcore != NULL){
		kb_free(&kb);
	}

	cmd_ln_free_r(config);
	return res;
}

/*
int main0(int argc, char** argv){

	return decode();
}
*/