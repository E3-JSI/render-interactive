/*
 * mong_srv.h
 *
 *  Created on: Jul 6, 2010
 *      Author: tadej
 */

#ifndef MONG_SRV_H_
#define MONG_SRV_H_


#include <base.h>
#include <concurrent/thread.h>

extern "C" {
	#include "mongoose/mongoose.h"
}

/**
 * Client context for a web server.
 * Should replace const int& SockId sometimes in the future.
 */
ClassTP(TWebCliContext, PWebCliContext) // { 
public:
	virtual ~TWebCliContext() {}
	virtual TStr GetPeerIpNum() const = 0;
	virtual void Respond(const PHttpResp& HttpResp) = 0;
};

/**
 * A specialization of a client context for a Mongoose
 * web server. Override when implementing another back-end.
 */
class TMongCliContext : public TWebCliContext {
protected:
	struct mg_connection *Conn;
	const struct mg_request_info *RequestInfo;

public:
	TMongCliContext(struct mg_connection *C, const struct mg_request_info *Ri);

	TStr GetPeerIpNum() const;
	void Respond(const PHttpResp& HttpResp);
};

/**
 * TWebSrv replacement, based on Mongoose web server.
 * http://code.google.com/p/mongoose/
 *
 * - multi-threaded
 * - portable
 * - MIT License
 *
 * Drop-in replacement for TWebSrv.
 *
 */
ClassTPV(TMongSrv, PMongSrv, TMongSrvV)//{

private:

  /** Mongoose context */
  mg_context *Ctx;

  char *ports;
  char *num_threads;

  /** Static mapping of port->server instances */
  static THash<TInt, PMongSrv> Registry;

  UndefDefaultCopyAssign(TMongSrv);

protected:
  PNotify Notify;
  int PortN;
  TStr HomeNrFPath;
  THash<TInt, PWebCliContext> Clients;
  TCriticalSection *ClientsLock;
  TRnd Rnd;
  
  TMongSrv(
    const int& _PortN, const bool& FixedPortNP=true, const PNotify& _Notify=NULL, int Threads = 1);

  /** Register a new client, and safely construct the relevant background object and return his id. */
  int NewClient(struct mg_connection *conn,
			const struct mg_request_info *request_info);
  /** Release resources for a given client */
  void DropClient(int ClientId);

public:

  static PHttpResp ServeFile(const int& Client, const PHttpRq& Request);
  
  static PMongSrv& Get(const PUrl& Url);
  static void *HandleRequest(enum mg_event event, struct mg_connection *conn,
			const struct mg_request_info *request_info);

  static PMongSrv New(const int& PortN, const bool& FixedPortNP=true, const PNotify& Notify=NULL, int Threads = 4);
  void Shutdown();
  virtual ~TMongSrv();
  TMongSrv(TSIn&){Fail;}
  static PMongSrv Load(TSIn&){Fail; return NULL;}
  void Save(TSOut&){Fail;}

  PNotify GetNotify() const {return Notify;}
  int GetPortN() const {return PortN;}
  TStr GetHomeNrFPath() const {return HomeNrFPath;}


  TStr GetPeerIpNum(const int& Client) const;

  virtual void OnHttpRq(const int& Client, const PHttpRq& HttpRq);
  void SendHttpResp(const int& Client, const PHttpResp& HttpResp);

};

/* If TWebSrv is not known, masquerade. */
#ifndef TWebSrv
typedef PMongSrv PWebSrv;
typedef TMongSrv TWebSrv;
#endif


#if !defined(NET_SAPPSRV_H_) || !defined(SAPPSRV_H_)
	#define NET_SAPPSRV_H_
	#include <net/sappsrv.h>
	//#include <net/sappsrv.cpp>
#endif



#endif /* MONG_SRV_H_ */
