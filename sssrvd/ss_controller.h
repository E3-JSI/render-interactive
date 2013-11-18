/*
 * ss_controller.h
 *
 *  Created on: Nov 10, 2011
 *      Author: tadej
 */

#ifndef SS_CONTROLLER_H_
#define SS_CONTROLLER_H_


#include <base.h>
#include <concurrent/thread.h>
#include <ss_dataset.h>
#include <semisupervised.h>

ClassTP(TSemiSuperviserController, PSemiSuperviserController) // {

protected:
	THash<TStr, PSemiSupervisor> Models;
	TCriticalSection ModelsLock;
	TCriticalSection RebuildingLock;
	TCriticalSection DsLock;

	PNotify Notify;
	
	PSemiSupervisor ConstructModel(const TStr& Type, const TStr& Name, int TaskId);
	PSemiSupervisor ConstructModel(const TStr& Type, const TStr& Name, const TStr& Task);
	PSemiSupervisor ConstructModel(const TStr& Type, const TStr& Name, const TVec<TIntV>& Seeds, const PBowDocBs& Bow);
	PSemiSupervisor LoadModel(const TStr& Type, const PMem& Mem);
	PSemiSupervisor GetModel(const TStr& Name);
	void TestModel(const PSemiSupervisor& Model, const TStr& ModelName, int SampleSize, int TaskId, TStrV& Classes, TCfyResV& CfyRes);
public:

	POgDsProvider Dataset;

	TSemiSuperviserController(POgDsProvider DsP, PNotify Not) : Dataset(DsP), Notify(Not) {}

	void CreateModel(const TStr& Type, const TStr& Name, const TStr& Task);
	bool TryUpdateModel(const TStr& Name, const TStr& Task);
	void Search(const TStr& DsNm, const TStr& Query, const int N, TVec<TPair<TUInt64, TStr> >& IdStrV);
	void SearchAndLabel(const TStr& DsNm, const TStr& Query, const int N, const TStr& ModelName, const TStrKdV& Filters, TVec<TQuad<TUInt64, TStr, TStr, TFlt> >& IdStrLabelV);
	void SearchAndGetUncertain(const TStr& DsNm, const TStr& Query, const TStr& ModelName, const int N, const TStrKdV& Filters, TVec<TPair<TUInt64, TStr> >& IdStrV);
	void GetUncertain(const TStr& ModelName, const int N, const TStrKdV& Filters, TVec<TPair<TUInt64, TStr> >& IdStrV);
	void AddLabel(const TStr& Task, const TStr& Label, const TStr& CurrentModel, uint64 DocId);
	void TestModel(const TStr& Model, int SampleSize, TStrV& Classes, TCfyResV& CfyRes);
	void Filter(const TStrKdV& Filters, const TVec<TPair<TUInt64, TStr> >& Docs, const TVec<TStrV>& Tokens, TIntV& Select);
};


#endif /* SS_CONTROLLER_H_ */

