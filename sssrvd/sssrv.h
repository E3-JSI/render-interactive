
/*
 * sssrv.h
 *
 *  Created on: Nov 14, 2011
 *      Author: tadej
 */

#include <mong_srv.h>

#include <net/sappsrv.h>
#include <net/sappsrv.cpp>

#include "json.h"
#include "ss_controller.h"

class TSSCtrlSrvFun: public TSAppSrvFun {
protected:
	PSemiSuperviserController Ctrl;
	
	int GetInt(const TStrKdV& FldNmValPrV, const TStr& Nm) {
		const TStr& Val = GetFldVal(FldNmValPrV, Nm);
		return Val.GetInt(-1);
	}
	
public:
	

	TSSCtrlSrvFun(PSemiSuperviserController C, const TStr& Name, TSAppOutType Out) :
			TSAppSrvFun(Name, Out), Ctrl(C) {
	}

	TSSCtrlSrvFun(PSemiSuperviserController C, const TStr& Name) :
			TSAppSrvFun(Name, saotJSon), Ctrl(C) {
	}

};

class TCreateModel: public TSSCtrlSrvFun {
public:
	TCreateModel(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "addmodel") {
	}
	// executes function using parameters passed after ? and returns JSon doc
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Type = GetFldVal(FldNmValPrV, "type");
		const TStr Name = GetFldVal(FldNmValPrV, "name");
		const TStr Task = GetFldVal(FldNmValPrV, "task");

		if (Type.Empty() || Name.Empty() || Task.Empty()) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		}

		Ctrl->CreateModel(Type, Name, Task);
		return "";
	}
};

class TUpdateModel: public TSSCtrlSrvFun {
public:
	TUpdateModel(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "updatemodel") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Name = GetFldVal(FldNmValPrV, "name");
		const TStr Task = GetFldVal(FldNmValPrV, "task");
		if (Name.Empty() || Task.Empty()) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		}
		Ctrl->TryUpdateModel(Name, Task);
		return "";
	}
};

class TAddTask: public TSSCtrlSrvFun {
public:
	TAddTask(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "addtask") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Task = GetFldVal(FldNmValPrV, "task");
		const TStr Classes = GetFldVal(FldNmValPrV, "classes");
		if (Task.Empty() || Classes.Empty()) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		}
		int TaskId = Ctrl->Dataset->GetTaskId(Task);
		TStrV ClassV;
		Classes.SplitOnAllAnyCh(", ", ClassV, true);
		Ctrl->Dataset->AddTaskClasses(TaskId, ClassV);
		
		return "";
	}

};

class TAddLabel: public TSSCtrlSrvFun {
public:
	TAddLabel(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "addlabel") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Task = GetFldVal(FldNmValPrV, "task");
		const TStr Label = GetFldVal(FldNmValPrV, "label");
		const TStr Doc = GetFldVal(FldNmValPrV, "docid");
		const TStr Model = GetFldVal(FldNmValPrV, "model");

		uint64 DocId;
		if (Task.Empty() || Label.Empty() || !Doc.IsUInt64(DocId)) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		} else {
			Ctrl->AddLabel(Task, Label, Model, DocId);
		}

		return "";
	}
};

class TSearch: public TSSCtrlSrvFun {
public:
	TSearch(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "search") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Query = GetFldVal(FldNmValPrV, "query");
		const TStr N = GetFldVal(FldNmValPrV, "n", "20");
		int  Number;
		if (Query.Empty() || !N.IsInt(Number)) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		} else {

			TVec < TPair<TUInt64, TStr> > Docs;
			Ctrl->Search(Query, Number, Docs);
			return TJson::Dumps(Docs, "docid", "content");

		}
		return "";
	}
};

class TSearchAndLabel: public TSSCtrlSrvFun {
public:
	TSearchAndLabel(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "searchandlabel") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Query = GetFldVal(FldNmValPrV, "query");
		const TStr Model = GetFldVal(FldNmValPrV, "model");
		const TStr N = GetFldVal(FldNmValPrV, "n", "20");
		
		int Number;
		if (Query.Empty() || Model.Empty() || !N.IsInt(Number)) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		} else {

			TVec < TQuad<TUInt64, TStr, TStr, TFlt> > IdStrLabelV;
			Ctrl->SearchAndLabel(Query, Number, Model, FldNmValPrV, IdStrLabelV);
			TIntStrPrV ClassV;
			Ctrl->Dataset->GetTaskClasses(Ctrl->Dataset->GetTaskIdForModel(Model), ClassV);
			TStrV ClassNmV(ClassV.Len());
			for (int i = 0 ;i < ClassV.Len(); i++) {
				ClassNmV[i] = ClassV[i].Val2;
			}
			PMem Mem = TMem::New(1024);
			TMemOut Out(Mem);
			Out.PutStr("{\"classes\": ");
			TJson::Write(ClassNmV, Out);
			Out.PutStr(", \"results\": ");
			TJson::Write(IdStrLabelV, "docid", "content", "label", "score", Out);
			Out.PutStr("}\n");
			return Mem->GetAsStr();
		}
		return "";
	}
};

class TGetUncertain: public TSSCtrlSrvFun {
public:
	TGetUncertain(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "getuncertain") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Model = GetFldVal(FldNmValPrV, "model");
		const TStr N = GetFldVal(FldNmValPrV, "n", "20");
		int Number;
		if (Model.Empty() || !N.IsInt(Number)) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		} else {

			TVec < TPair<TUInt64, TStr> > Docs;
			Ctrl->GetUncertain(Model, Number, FldNmValPrV, Docs);
		
			TIntStrPrV ClassV;
			Ctrl->Dataset->GetTaskClasses(Ctrl->Dataset->GetTaskIdForModel(Model), ClassV);
			TStrV ClassNmV(ClassV.Len());
			for (int i = 0 ;i < ClassV.Len(); i++) {
				ClassNmV[i] = ClassV[i].Val2;
			}
			PMem Mem = TMem::New(1024);
			TMemOut Out(Mem);
			Out.PutStr("{\"classes\": ");
			TJson::Write(ClassNmV, Out);
			Out.PutStr(", \"results\": ");
			TJson::Write(Docs, "docid", "content", Out);
			Out.PutStr("}\n");
			return Mem->GetAsStr();



		}
		return "";
	}
};

class TSearchAndGetUncertain: public TSSCtrlSrvFun {
public:
	TSearchAndGetUncertain(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "searchandgetuncertain") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		const TStr Model = GetFldVal(FldNmValPrV, "model");
		const TStr Dataset = GetFldVal(FldNmValPrV, "dataset");
		const TStr Query = GetFldVal(FldNmValPrV, "query");
		const TStr N = GetFldVal(FldNmValPrV, "n", "20");
		int Number;
		if (Model.Empty() || !N.IsInt(Number) || Query.Empty()) {
			TExcept::Throw(
					"Invalid request parameters: "
							+ RqEnv->GetHttpRq()->GetStr());
		} else {

			TVec < TPair<TUInt64, TStr> > Docs;
			Ctrl->SearchAndGetUncertain(Dataset, Query, Model, Number, FldNmValPrV, Docs);
		
			TIntStrPrV ClassV;
			Ctrl->Dataset->GetTaskClasses(Ctrl->Dataset->GetTaskIdForModel(Model), ClassV);
			TStrV ClassNmV(ClassV.Len());
			for (int i = 0 ;i < ClassV.Len(); i++) {
				ClassNmV[i] = ClassV[i].Val2;
			}
			PMem Mem = TMem::New(1024);
			TMemOut Out(Mem);
			Out.PutStr("{\"classes\": ");
			TJson::Write(ClassNmV, Out);
			Out.PutStr(", \"results\": ");
			TJson::Write(Docs, "docid", "content", Out);
			Out.PutStr("}\n");
			return Mem->GetAsStr();
		}
		return "";
	}
};

class THelloWorld: public TSSCtrlSrvFun {
public:
	THelloWorld(PSemiSuperviserController C) :
			TSSCtrlSrvFun(C, "hello") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		return "{ \"hello\" : \"world\" }\n";
	}
};

class TGetTasks : public TSSCtrlSrvFun {
public:
	TGetTasks(PSemiSuperviserController C) : TSSCtrlSrvFun(C, "gettasks") {
		
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		TUInt64StrPrV Tasks;
		Ctrl->Dataset->GetTasks(Tasks);
		PMem Mem = TMem::New(1024);
		TMemOut Out(Mem);
		Out.PutStr("[");
		for (int i = 0; i < Tasks.Len(); i++) {
			const TUInt64StrPr& TaskIdTask = Tasks[i];
			TIntStrPrV TaskClasses;
			Ctrl->Dataset->GetTaskClasses((int) TaskIdTask.Val1, TaskClasses);

			Out.PutStrFmt("{\"taskid\":\"%llu\", \"task\":", TaskIdTask.Val1.Val);
			TJson::Write(TaskIdTask.Val2, Out);
			Out.PutStr(", \"classes\":[");
			for (int j = 0; j < TaskClasses.Len(); j++) {
				TJson::Write(TaskClasses[j].Val2, Out);
				if (j  < TaskClasses.Len() - 1) {
					Out.PutStr(", ");
				}
			}	
			Out.PutStr("]}\n");
			if (i < Tasks.Len() - 1) {
				Out.PutStr(",");
			}
		}
		Out.PutStr("]\n");
		printf("%s\n", Mem->GetAsStr().CStr());
		return Mem->GetAsStr();
		//return TJson::Dumps(Tasks, "taskid", "task");
	}		
};

class TGetModels : public TSSCtrlSrvFun {
public:
	TGetModels(PSemiSuperviserController C) : TSSCtrlSrvFun(C, "getmodels") {
		
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		TUInt64StrPrV Models;
		const TStr& TaskNm = GetFldVal(FldNmValPrV, "task"); 
		Ctrl->Dataset->GetModels(TaskNm, Models);
		return TJson::Dumps(Models, "modelid", "model");
	}	
};

class TDelModel : public TSSCtrlSrvFun {
public:
	TDelModel(PSemiSuperviserController C) : TSSCtrlSrvFun(C, "delmodel") {
		
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		Ctrl->Dataset->DelModel(GetFldVal(FldNmValPrV, "model"));
		return "";
	}	
};

class TDelTask : public TSSCtrlSrvFun {
public:
	TDelTask(PSemiSuperviserController C) : TSSCtrlSrvFun(C, "deltask") {
		
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		Ctrl->Dataset->DelTask(GetFldVal(FldNmValPrV, "task"));
		return "";
	}		
};

class TDelLabel : public TSSCtrlSrvFun {
public:
	TDelLabel(PSemiSuperviserController C) : TSSCtrlSrvFun(C, "dellabel") {
		
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		Ctrl->Dataset->DelLabel(GetInt(FldNmValPrV, "labelid"));
		return "";
	}		
};

class TGetLabels : public TSSCtrlSrvFun {
public:
	TGetLabels(PSemiSuperviserController C) : TSSCtrlSrvFun(C, "getlabels") {
	}
	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
		TStrV KnownLabels;
		const TStr& Task = GetFldVal(FldNmValPrV, "task");
		Ctrl->Dataset->GetLabelList(Task, KnownLabels);

		TVec<TTriple<TUInt64, TStr, TStr> > Labeled;
		for (int i = 0; i < KnownLabels.Len(); i++) {
			const TStr& Label = KnownLabels[i];
			TUInt64V DocIds;
			TStrV Contents;
			Ctrl->Dataset->GetDocsForLabel(Task, Label, DocIds);
			Ctrl->Dataset->GetDocs(DocIds, Contents);
			for (int j = 0; j < DocIds.Len(); j++) {
				Labeled.Add(TTriple<TUInt64, TStr, TStr>(DocIds[j], Contents[j], Label));
			}
		}
		return TJson::Dumps(Labeled, "docid", "content", "label");
	}		
};

class TTestModel : public TSSCtrlSrvFun {
protected:

	void WriteResult(const TStr& ClassNm, const TCfyRes& ClassRes, TSOut& Out) {
			
		Out.PutStr("{");
		Out.PutStr("\"class\": ");
		TJson::Write(ClassNm, Out);
		Out.PutStr(", \"precision\": ");
		TJson::Write(TFlt(ClassRes.Prec()), Out);
		Out.PutStr(", \"recall\": ");
		TJson::Write(TFlt(ClassRes.Rec()), Out);
		Out.PutStr(", \"f1\": ");
		TJson::Write(TFlt(ClassRes.F1()), Out);
		Out.PutStr("}\n");
	}
public:
	TTestModel(PSemiSuperviserController C) : TSSCtrlSrvFun(C, "testmodel") {
	}

	TStr ExecJSon(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {

		const TStr& Model = GetFldVal(FldNmValPrV, "model");
		const TStr& NStr = GetFldVal(FldNmValPrV, "n", "-1");
		int N;
		if (NStr.IsInt(N)) {
			TCfyResV Cfy;
			TStrV Classes;
			PMem Mem = TMem::New(128);
			Ctrl->TestModel(Model, N, Classes, Cfy);
			if (Cfy.Len() > 0) {

				TCfyRes Total;
				for (int i = 0; i < Cfy.Len(); i++) {
					Total.Add(Cfy[i]);
				}
				Total.Def();

				TMemOut Out(Mem);
				Out.PutStr("[");
				for (int ClassN = 0; ClassN < Classes.Len(); ClassN++) {
					const TCfyRes& ClassRes = Cfy[ClassN];
					const TStr& ClassNm = Classes[ClassN];
					WriteResult(ClassNm, ClassRes, Out);
					Out.PutStr(",");
				}
				WriteResult("total", Total, Out);
				Out.PutStr("]\n");

			}
			return Mem->GetAsStr();

		} else {
			return "";
		}
	}
};


class TSemiSupervisedSrv: public TSAppSrv {

	static TSAppSrvFunV GetFuncs(PSemiSuperviserController Ctrl) {
		TSAppSrvFunV Funcs;
		Funcs.Add(new TCreateModel(Ctrl));
		Funcs.Add(new TUpdateModel(Ctrl));
		Funcs.Add(new TAddTask(Ctrl));
		Funcs.Add(new TAddLabel(Ctrl));
		Funcs.Add(new TSearch(Ctrl));
		Funcs.Add(new TSearchAndLabel(Ctrl));
		Funcs.Add(new TGetUncertain(Ctrl));
		Funcs.Add(new TGetModels(Ctrl));
		Funcs.Add(new TGetTasks(Ctrl));
		Funcs.Add(new TGetLabels(Ctrl));
		Funcs.Add(new TDelLabel(Ctrl));
		Funcs.Add(new TDelTask(Ctrl));
		Funcs.Add(new TDelModel(Ctrl));
		Funcs.Add(new TTestModel(Ctrl));
		Funcs.Add(new TSearchAndGetUncertain(Ctrl));
		Funcs.Add(new TSASFunFile("index.html", "index.html", "text/html"));
		Funcs.Add(new TSASFunFile("ss.js", "ss.js", "application/javascript"));
		Funcs.Add(new TSASFunFile("jquery-1.7.1.min.js", "jquery-1.7.1.min.js", "application/javascript"));
		Funcs.Add(new TSASFunFile("bootstrap.min.css", "bootstrap.min.css", "text/css"));
		return Funcs;
	}
	
public:
	TSemiSupervisedSrv(int PortN, PNotify Notify, bool ShowParam,
			PSemiSuperviserController Ctrl) :
			TSAppSrv(PortN, GetFuncs(Ctrl), Notify, ShowParam) {

	}

};
