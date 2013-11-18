/*
 * ss_dataset.h
 *
 *  Created on: Nov 10, 2011
 *      Author: tadej
 */

#ifndef SS_DATASET_H_
#define SS_DATASET_H_

#include <base.h>
#include <extension/qminer_sqlite.h>
#include <thread.h>

typedef TVec<TPair<TUInt64, TStr> > TUInt64StrPrV;

ClassTP(TOgDsProvider, POgDsProvider) // {
protected:

	TStr DbNm;

	//PNGramBs Ngrams;
	PSQLConnection Conn;
	POgBase Miner;
	PBowDocBs All;
	
	PSQLQueryCache Qc;
	TCriticalSection IndexLock;
	TCriticalSection DbLock;
	TCriticalSection BowLock;
public:
	PSwSet SwSet;
	PStemmer Stemmer;
	POgTokenizer Tok;
	
	TOgDsProvider(const TStr& FNm);
	~TOgDsProvider();

	void Init();
	PBowDocBs GetBowBs(const TStr& Query);
	PBowDocBs GetBowBsAll();
	PBowDocBs GetBow(const TVec<TPair<TUInt64, TStr> >& IdStrV) const;
	void Search(const TStr& DsNm, const TStr& Query, int n,TVec<TPair<TUInt64, TStr> >& IdStrV);
	void GetDocsForLabel(const TStr& Task, const TStr& Label, TUInt64V& DocIds);
	void GetLabelList(const TStr& Task, TStrV& Labels) ;
	void SetLabelForDocs(const TStr& Task, const TStr& Label, const TUInt64V& DocIds);
	void GetDocsLabelsForTaskId(const int TaskId, TVec<TPair<TUInt64, TInt> >& DocIds);
	int GetTaskId(const TStr& Task);
	bool IsGetTaskId(const TStr& Task, int& TaskId);
	void AddTaskClasses(const int TaskId, const TStrV& Classes);
	int GetTaskIdForModel(const TStr& Model);
	void GetTaskClasses(const int TaskId, TIntStrPrV& ClassV);
	int GetModelId(const TStr& Model);
	void GetTasks(TUInt64StrPrV& Tasks);
	void GetModels(const TStr& Task, TUInt64StrPrV& Models);
	void GetDocs(const TUInt64V& DocIds, TStrV& Contents);
	void DelModel(const TStr& Model);
	void DelTask(const TStr& Task);
	void DelLabel(int LabelId);
	void SaveModel(const TStr& ModelNm, const TStr& ModelType, const TStr& TaskNm, const PMem& ModelSerialized);
	void GetModel(const TStr& ModelNm, PMem& Mem, TStr& Type);
	const TStr GetTaskNm(const int TaskId);
};



#endif /* SS_DATASET_H_ */
