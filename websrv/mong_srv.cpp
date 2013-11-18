/*
 * mong_srv.cpp
 *
 *  Created on: Jul 6, 2010
 *      Author: tadej
 */

#include "mong_srv.h"
#include <stdlib.h>
#include <concurrent/thread.h>
#include <string.h>

THash<TInt, PMongSrv> TMongSrv::Registry = THash<TInt, PMongSrv>();

/*
 * Make sure we have ho zombies from CGIs
 */
static void signal_handler(int sig_num) {
#ifndef _WIN32	
	switch (sig_num) {
	case SIGCHLD:
		while (waitpid(-1, &sig_num, WNOHANG) > 0)
			;
		break;
	default:
		break;
	}

#endif /* !_WIN32 */
}

TMongCliContext::TMongCliContext(struct mg_connection *C,
		const struct mg_request_info *Ri) :
		Conn(C), RequestInfo(Ri) {
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, &signal_handler);
#endif /* !_WIN32 */
}

TStr TMongCliContext::GetPeerIpNum() const {
	in_addr IpAddress;
	memcpy(&IpAddress.s_addr, &RequestInfo->remote_ip, 4);
	return inet_ntoa(IpAddress);
	//return TStr(inet_ntoa(Conn->client.rsa.u.sin.sin_addr));
}

void TMongCliContext::Respond(const PHttpResp& HttpResp) {
	const TMem& Body = HttpResp->GetBodyAsMem();
	TStr ResponseHeader = HttpResp->GetHdStr();
	mg_write(Conn, ResponseHeader.CStr(), ResponseHeader.Len());
	mg_write(Conn, Body.GetBf(), Body.Len());
}

PMongSrv TMongSrv::New(const int& PortN, const bool& FixedPortNP,
		const PNotify& Notify, int Threads) {
	return PMongSrv(new TMongSrv(PortN, FixedPortNP, Notify, Threads));
}

void TMongSrv::Shutdown() {
	TNotify::OnNotify(
			Notify,
			ntInfo,
			"Web-Server: Removing server instance from registry, will shutdown once no references exist.");
	Registry.DelIfKey(PortN);
}

PMongSrv& TMongSrv::Get(const PUrl& Url) {
	IAssert(Registry.IsKey(Url->GetPortN()));
	return Registry.GetDat(Url->GetPortN());
}

int TMongSrv::NewClient(struct mg_connection *conn,
		const struct mg_request_info *request_info) {
	PWebCliContext Client = new TMongCliContext(conn, request_info);
	ClientsLock->Enter();
	int ClientId = Rnd.GetUniDevInt(TInt::Mx);
	Clients.AddDat(ClientId, Client);
	ClientsLock->Leave();
	return ClientId;
}

void TMongSrv::DropClient(int ClientId) {
	ClientsLock->Enter();
	Clients.DelIfKey(ClientId);
	ClientsLock->Leave();
}

void *TMongSrv::HandleRequest(enum mg_event event, struct mg_connection *conn,
		const struct mg_request_info *request_info) {

	// Since this is a static request handler, find out for which
	// server instance this is for. The URL should tell that.

	
	TChA UrlStr = "http://";
	const char *Host = mg_get_header(conn, "Host");
	UrlStr += Host != NULL ? Host : "localhost";
	UrlStr += request_info->uri;
	if (request_info->query_string != NULL) {
		UrlStr += "?";
		UrlStr += request_info->query_string;
	}
	
	void *processed = (void *) "yes";
	TStr UrlS = UrlStr;
	PUrl Url = TUrl::New(UrlS);

	if (!Url->IsOk(usHttp)) {
		TNotify::OnNotify(TNotify::StdNotify, ntErr,
				TStr("Invalid URI: ") + UrlStr);
		return NULL;
	}

	PMongSrv& Server = TMongSrv::Get(Url);

	if (request_info->log_message != NULL) {
		TNotify::OnNotify(Server->GetNotify(), ntErr,
				TStr(request_info->log_message));
		return NULL;
	}

	THttpRqMethod Method = hrmUndef;

	if (strncmp(request_info->request_method, "POST", 4) == 0) {
		Method = hrmPost;
	} else if (strncmp(request_info->request_method, "GET", 3) == 0) {
		Method = hrmGet;
	} else if (strncmp(request_info->request_method, "HEAD", 4) == 0) {
		Method = hrmHead;
	} else {
		return NULL;
	}

	TStr ContentType = mg_get_header(conn, THttp::ContTypeFldNm.CStr());
	const char *ContentLenStr = mg_get_header(conn, "Content-Length");
	PHttpRq Rq;
	if (Method == hrmPost) {

		int ContentLength = atoi(ContentLenStr);
		char *Body = new char[ContentLength];
		mg_read(conn, Body, ContentLength);
		Rq = THttpRq::New(Method, Url, ContentType, TMem(Body, ContentLength));
		delete[] Body;
	} else {
		Rq = THttpRq::New(Method, Url, ContentType, TMem());
	}
	

	int ClientId = Server->NewClient(conn, request_info); 
			
	try {
		Server->OnHttpRq(ClientId, Rq);
		Server->DropClient(ClientId);
		return processed;
	} catch (PExcept& Exception) {
		TNotify::OnNotify(Server->GetNotify(), ntErr, Exception->GetStr());
		Server->DropClient(ClientId);
		return NULL;
	}

}

TMongSrv::TMongSrv(const int& _PortN, const bool& FixedPortNP,
		const PNotify& _Notify, int Threads) :
		Notify(_Notify), PortN(_PortN), HomeNrFPath(
				TStr::GetNrFPath(TDir::GetCurDir())), Rnd(TTm::GetMSecsFromOsStart()) {

	ClientsLock = new TCriticalSection();
	const char *options[] = {
			//"document_root",  HomeNrFPath.GetCStr(),
			"listening_ports", TInt::GetStr(PortN).GetCStr(), "num_threads",
			TInt::GetStr(Threads).GetCStr(),
			//"enable_keep_alive", "yes",
			//"auth_domain", TStr("").GetCStr(),
			0 };

	Ctx = mg_start(&HandleRequest, NULL, options);

	IAssertR(Ctx != NULL, "Web-Server: Server failed to start!");

	TChA MsgChA;
	MsgChA += "Web-Server: Started at port ";
	MsgChA += TInt::GetStr(PortN);
	MsgChA += ".";
	TNotify::OnNotify(Notify, ntInfo, MsgChA);

	PMongSrv Pt(this);
	Registry.AddDat(PortN, Pt);

}

TMongSrv::~TMongSrv() {
	Shutdown();
	mg_stop(Ctx);
	TNotify::OnNotify(Notify, ntInfo, "Web-Server: Stopped.");
	delete ClientsLock;
}

void TMongSrv::OnHttpRq(const int& SockId, const PHttpRq& HttpRq) {
	// check http-request correctness - return if error
	if (!HttpRq->IsOk()) {
		TNotify::OnNotify(Notify, ntInfo, "Web-Server: Bad Http Request.");
		return;
	}
	// check url correctness - return if error
	PUrl RqUrl = HttpRq->GetUrl();
	if (!RqUrl->IsOk()) {
		TNotify::OnNotify(Notify, ntInfo, "Web-Server: Bad Url Requested.");
		return;
	}

	// construct http-response
	PHttpResp HttpResp;
	if (!RqUrl->GetPathStr().Empty()) {
		// get request-file-name
		TStr ExeFPath = TSysProc::GetExeFNm().GetFPath();
		TStr RqFNm = RqUrl->GetPathStr();
		if (RqFNm.LastCh() == '/') {
			RqFNm = RqFNm + "default.htm";
		}
		if ((RqFNm[0] == '/') || (RqFNm[0] == '\\')) {
			RqFNm.DelSubStr(0, 0);
		}
		RqFNm = ExeFPath + RqFNm;
		// open file
		bool RqFOpened = false;
		PSIn RqSIn = TFIn::New(RqFNm, RqFOpened);
		if (!RqFOpened) {
			// prepare default html with time
			TChA HtmlChA;
			HtmlChA += "<html><title>Error - Not Found</title><body>";
			HtmlChA += "File: ";
			HtmlChA += RqUrl->GetPathStr();
			HtmlChA += " not found.";
			HtmlChA += "</body></html>";
			PSIn BodySIn = TMIn::New(HtmlChA);
			HttpResp = PHttpResp(
					new THttpResp(THttp::ErrNotFoundStatusCd,
							THttp::TextHtmlFldVal, false, BodySIn, ""));
		} else {
			// file successfully opened
			PSIn BodySIn = RqSIn;
			if (THttp::IsHtmlFExt(RqFNm.GetFExt())) {
				// send text/html mime type if Html filemg_callback_t
				HttpResp = PHttpResp(
						new THttpResp(THttp::OkStatusCd, THttp::TextHtmlFldVal,
								false, BodySIn, ""));
			} else if (THttp::IsGifFExt(RqFNm.GetFExt())) {
				// send image/gif mime type if Gif file
				HttpResp = PHttpResp(
						new THttpResp(THttp::OkStatusCd, THttp::ImageGifFldVal,
								false, BodySIn, ""));
			} else {
				// send application/octet mime type
				HttpResp = PHttpResp(
						new THttpResp(THttp::OkStatusCd, THttp::AppOctetFldVal,
								false, BodySIn, ""));
			}
		}
	} else {
		// prepare default html with time
		TChA HtmlChA;
		HtmlChA += "<html><title>Welcome to TWebSrv (powered by mongoose 3.1)</title><body>";
		HtmlChA += TSecTm::GetCurTm().GetStr();
		HtmlChA += "</body></html>";
		PSIn BodySIn = TMIn::New(HtmlChA);
		HttpResp = THttpResp::New(THttp::OkStatusCd, THttp::TextHtmlFldVal,
				false, BodySIn);
	}

	// construct & send response
	SendHttpResp(SockId, HttpResp);
	// notify
	if (RqUrl->IsOk()) {
		TChA MsgChA;
		MsgChA += "Web-Server: Request for '";
		MsgChA += RqUrl->GetUrlStr();
		MsgChA += "'.";
		TNotify::OnNotify(Notify, ntInfo, MsgChA);
	}
}

void TMongSrv::SendHttpResp(const int& Client, const PHttpResp& HttpResp) {
	ClientsLock->Enter();
	if (Clients.IsKey(Client)) {
		PWebCliContext& CCtx = Clients.GetDat(Client);
		ClientsLock->Leave();
		CCtx->Respond(HttpResp);
	} else {
		ClientsLock->Leave();
		TExcept::Throw(TStr::Fmt("Cannot send HTTP response: client %d does not exist anymore.", Client));
	}
	
}

TStr TMongSrv::GetPeerIpNum(const int& Client) const {
	ClientsLock->Enter();
	if (Clients.IsKey(Client)) {
		PWebCliContext CCtx = Clients.GetDat(Client);
		ClientsLock->Leave();
		return CCtx->GetPeerIpNum();
	} else {
		ClientsLock->Leave();
		TExcept::Throw(TStr::Fmt("Cannot obtain client IP: client %d does not exist anymore.", Client));
		return "";
	}
}

