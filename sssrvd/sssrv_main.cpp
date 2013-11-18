/*
 * sssrv_main.cpp
 *
 *  Created on: Nov 14, 2011
 *      Author: tadej
 */


#include "stdafx.h"
#include "../semisupervisor/ss_dataset.h"
#include "ss_controller.h"
#include "sssrv.h"

int main(int argc, char* argv[]) {
	Env=TEnv(argc, argv, TNotify::StdNotify);
	// command line parameters
	Env.PrepArgs("Semi-supervised learning server daemon");
	TStr InF=Env.GetIfArgPrefixStr("-in:", "", "Input-DB");

	bool Index=Env.GetIfArgPrefixBool("-do-index", false, "Do indexing");
	if (Env.IsEndOfRun()){return 0;}
	Try;
	POgDsProvider Provider = new TOgDsProvider(InF);
	if (Index) {
		Provider->Init();
	} else {
		PSemiSuperviserController Ctrl = new TSemiSuperviserController(Provider, TNotify::StdNotify);
		PWebSrv Srv = new TSemiSupervisedSrv(7000, TNotify::StdNotify, true, Ctrl);
		printf("Server started, press any key to exit.");
		getchar();	
		Srv.Clr();
		Ctrl.Clr();
	}
	return 0;
	Catch;
	return 1;
}

