/*
 * ss_controller.cpp
 *
 *  Created on: Nov 10, 2011
 *      Author: tadej
 */
#include "stdafx.h"
#include "ss_controller.h"


PSemiSupervisor TSemiSuperviserController::ConstructModel(const TStr& Type, const TStr& Name, int TaskId) {
	PSemiSupervisor Model;

	TVec<TIntV> Seeds;
	
	PBowDocBs Bow = Dataset->GetBowBsAll();

	TIntStrPrV ClassV;
	Dataset->GetTaskClasses(TaskId, ClassV);
		
	TVec<TUInt64V> DocIdsForClass(ClassV.Len());
	TVec<TPair<TUInt64, TInt> > DocLabelIds;
	Dataset->GetDocsLabelsForTaskId(TaskId, DocLabelIds);

	TIntH ClassIdClassN;
	for (int i = 0; i < ClassV.Len(); i++) {
		ClassIdClassN.AddDat(ClassV[i].Val1, i);
	}
	for (int i = 0; i < DocLabelIds.Len(); ++i) {
		const TPair<TUInt64, TInt>& DocIdLabelId = DocLabelIds[i];
		int ClassN = ClassIdClassN.GetDat(DocIdLabelId.Val2);
		DocIdsForClass[ClassN].Add(DocIdLabelId.Val1);
	}
		
	TNotify::OnNotify(Notify, ntInfo, TStr::Fmt("Have %d items", Bow->GetDocs()));
	Seeds.Gen(ClassV.Len());

	for (int c = 0; c < ClassV.Len(); c++) {
		TIntV& ClassDIds = Seeds[c];
		const TUInt64V& DocIds = DocIdsForClass[c];
		for (int i = 0; i < DocIds.Len(); ++i) {
			const TStr IdStr = TUInt64::GetStr(DocIds[i]);
			const int DId = Bow->GetDId(IdStr);
			if (DId == -1) {
				TNotify::OnNotify(Notify, ntInfo, TStr::Fmt("Unknown document for class %s: %I64u - %s", ClassV[c].Val2.CStr(), DocIds[i], IdStr.CStr()));
				continue;
			}
			ClassDIds.Add(DId);
		}
	}

		
	for (int i = 0; i < Seeds.Len(); ++i) {
		TNotify::OnNotify(Notify, ntInfo, TStr::Fmt("%s: %d", ClassV[i].Val2.CStr(), Seeds[i].Len()));
	}
		

	return ConstructModel(Type, Name, Seeds, Bow);
}

PSemiSupervisor TSemiSuperviserController::ConstructModel(const TStr& Type, const TStr& Name, const TStr& Task) {
	return ConstructModel(Type, Name, Dataset->GetTaskId(Task));
}

PSemiSupervisor TSemiSuperviserController::ConstructModel(const TStr& Type, const TStr& Name, const TVec<TIntV>& Seeds, const PBowDocBs& Bow) {
	PSemiSupervisor Model;
	if (Type == "NB2") {
		Model = new TNBA2ClassSupervisor(Bow, Seeds[0], Seeds[1], 0.5);
	} else if (Type == "NBMulti") {
		Model = new TNBAMultiClassSupervisor(Bow, Seeds);
	}
	return Model;
}


PSemiSupervisor TSemiSuperviserController::LoadModel(const TStr& Type, const PMem& Mem) {
	PSemiSupervisor Model;
	if (Type == "NB2") {
		PSIn MemIn = TMemIn::New(Mem);
		Model = new TNBA2ClassSupervisor(*MemIn);
	}
	return Model;
}

void TSemiSuperviserController::AddLabel(const TStr& Task, const TStr& Label, const TStr& CurrentModel,  uint64 DocId) {
	DsLock.Enter();
	Dataset->SetLabelForDocs(Task, Label, TUInt64V::GetV(DocId));
	DsLock.Leave();
	if (!CurrentModel.Empty()) {
		TryUpdateModel(CurrentModel, Task);	
	}
}


PSemiSupervisor TSemiSuperviserController::GetModel(const TStr& Name) {
	PSemiSupervisor Model;
	if (!Models.IsKeyGetDat(Name, Model)) {
		/*PMem Mem = TMem::New();
		TStr Type;
		Dataset->GetModel(Name, Mem, Type);
		if (Mem->Len() > 0) {
			Model = LoadModel(Type, Mem);
		}*/
	}
	return Model;
}
void TSemiSuperviserController::CreateModel(const TStr& Type, const TStr& Name, const TStr& Task) {
	
	
	PSemiSupervisor Model = ConstructModel(Type, Name, Task);
	ModelsLock.Enter();
	Models.AddDat(Name, Model);
	//
	
	PMem Mem = TMem::New();
	TMemOut Out(Mem);

	Model.Save(Out);
	Model.Clr();	
	ModelsLock.Leave();


	Dataset->SaveModel(Name, Type, Task, Mem);
	
}

bool TSemiSuperviserController::TryUpdateModel(const TStr& Name, const TStr& Task) {
	// todo
	if (RebuildingLock.TryEnter()) {
		TNotify::OnNotify(Notify, ntInfo, "Updating model " + Name + " for " + Task);
		CreateModel("NBMulti", Name, Task);
		RebuildingLock.Leave();
		return true;
	}
	TNotify::OnNotify(Notify, ntInfo, "Busy - skipping update of model " + Name + " for " + Task);
	return false;
}
void TSemiSuperviserController::Search(const TStr& DsNm, const TStr& Query, const int N, TVec<TPair<TUInt64, TStr> >& Docs) {
	DsLock.Enter();
	try {
		Dataset->Search(DsNm, Query, N, Docs);
	} catch (PExcept& Ex) {
		printf("%s\n", Ex->GetStr().CStr());
	}
	DsLock.Leave();
}
void TSemiSuperviserController::SearchAndLabel(const TStr& DsNm, const TStr& Query, const int N, const TStr& ModelName, const TStrKdV& Filters, TVec<TQuad<TUInt64, TStr, TStr, TFlt> >& IdStrLabelV) {
	TVec<TPair<TUInt64, TStr> > Docs;
	Search(DsNm, Query, N * 100, Docs);

	printf("Found %d docs for query %s\n", Docs.Len(), Query.CStr());
	for (int i = 0; i < Filters.Len(); i++) {
		printf("Filter %s = %s\n", Filters[i].Key.CStr(), Filters[i].Dat.CStr());
	}

	TIntStrPrV ClassV;
	int TaskId = Dataset->GetTaskIdForModel(ModelName);
	const TStr TaskNm = Dataset->GetTaskNm(TaskId);
	Dataset->GetTaskClasses(TaskId, ClassV);
	

	ModelsLock.Enter();
	PSemiSupervisor Model = GetModel(ModelName);
	if (Model.Empty()) {
		CreateModel("NBMulti", ModelName, TaskNm);
		Model = GetModel(ModelName);
	}
	ModelsLock.Leave();


	TVec<TStrV> Tokens(Docs.Len());
	for (int i = 0; i < Docs.Len(); i++) {
		Dataset->Tok->GetTokens(Docs[i].Val2, Tokens[i]);
	}

	TIntV Selected;
	Filter(Filters, Docs, Tokens, Selected);

	for (int i = 0; i < Selected.Len(); i++) {
		if (i == N)  {
			break;
		}

		int DocId = Selected[i];
		double Score; int Class; TStr ClassStr;
		Model->Classify(Tokens[DocId], Score, Class);
		if (Class == -1) {
			ClassStr = "";
		} else {
			ClassStr = ClassV[Class].Val2;
		}
		const TPair<TUInt64, TStr>& Doc = Docs[DocId];
		
		IdStrLabelV.Add(TQuad<TUInt64, TStr, TStr, TFlt>(Doc.Val1, Doc.Val2, ClassStr, Score));
	}

	ModelsLock.Enter();
	Model.Clr();

	ModelsLock.Leave();
	


}

void TSemiSuperviserController::Filter(const TStrKdV& Filters, const TVec<TPair<TUInt64, TStr> >& Docs, const TVec<TStrV>& Tokens, TIntV& Selected) {
	
	TVec<TTriple<TInt, TInt, PSemiSupervisor> > FilterModels;
		
	for (int i = 0; i < Filters.Len(); i++) {
		const TStr& FilterNm = Filters[i].Key;
		int TaskId;
		if (Dataset->IsGetTaskId(FilterNm, TaskId)) {
			const TStr& FilterVal = Filters[i].Dat;
			TIntStrPrV TaskClasses;
			Dataset->GetTaskClasses(TaskId, TaskClasses);
			for (int j = 0; j < TaskClasses.Len(); j++) {
				if (TaskClasses[j].Val2 == FilterVal) {
					const int TaskVal = TaskClasses[j].Val1;

					TUInt64StrPrV Models;
					Dataset->GetModels(FilterNm, Models);
					if (Models.Len() > 0) { 
						ModelsLock.Enter();
						PSemiSupervisor FilterModel = GetModel(Models[0].Val2); // FIXME Models[0] is not good in general
						if (FilterModel.Empty()) {
							CreateModel("NBMulti", Models[0].Val2, FilterNm);
							FilterModel = GetModel(Models[0].Val2);
						}

						FilterModels.Add(TTriple<TInt, TInt, PSemiSupervisor>(TaskId, j, FilterModel));
						ModelsLock.Leave();
						printf("found filter for %s (%d) = %s (%d), modeled by %s\n", FilterNm.CStr(), TaskId, FilterVal.CStr(), TaskVal, Models[0].Val2.CStr());
						break;
					}
				}
			}
		}
	}
	 
	
	for (int i = 0; i < Docs.Len(); ++i) {

		const TPair<TUInt64, TStr>& Doc = Docs[i];
		TStr ClassStr;
		
		bool IsSelected = true;
		for (int j = 0; j < FilterModels.Len(); j++) {
			const TTriple<TInt, TInt, PSemiSupervisor>& FilterModel = FilterModels[j];
			const int TaskId = FilterModel.Val1;
			const int TargetClass = FilterModel.Val2;
			const PSemiSupervisor& Filter = FilterModel.Val3;

			double PredictedScore;
			int PredictedClass;
			Filter->Classify(Tokens[i], PredictedScore, PredictedClass);
			if (TargetClass != PredictedClass) {
				IsSelected = false;
				break;
			}
		}
		
		if (IsSelected) {
			Selected.Add(i);
		}

		
	}
	ModelsLock.Enter();
	FilterModels.Clr();
	ModelsLock.Leave();
}

void TSemiSuperviserController::GetUncertain(const TStr& ModelName, const int N,  const TStrKdV& Filters, TVec<TPair<TUInt64, TStr> >& IdStrV) {
	PSemiSupervisor Model;
	ModelsLock.Enter();
	if (Models.IsKeyGetDat(ModelName, Model)) {
		ModelsLock.Leave();
		int QId = Model->FFirstQuery();
		PBowDocBs Bow = Model->GetBow();
		TUInt64V DocIds;
		TIntV DIds;
		//ModelsLock.Enter();
		int Iters = 10000;
		do {
			if (!DIds.IsInBin(QId)) {
				DIds.AddMerged(QId);
				ModelsLock.Enter();
				const TStr& DocNm = Bow->GetDocNm(QId);
				ModelsLock.Leave();
				const uint64 Id = DocNm.GetUInt64();
				DocIds.Add(Id);
			}
		} while (Model->FNextQuery(QId) && DocIds.Len() < N && QId != -1 && Iters-- > 0);
		//ModelsLock.Leave();

		TStrV Contents;
		Dataset->GetDocs(DocIds, Contents);

		for (int i = 0; i < DocIds.Len(); ++i) {
			IdStrV.Add(TPair<TUInt64, TStr>(DocIds[i], Contents[i]));
		}
	} else {
		ModelsLock.Leave();
		TNotify::OnNotify(Notify, ntErr, "No model under name" +  ModelName);
	}
}


void TSemiSuperviserController::SearchAndGetUncertain(const TStr& DsNm, const TStr& Query, const TStr& ModelName, const int N,  const TStrKdV& Filters, TVec<TPair<TUInt64, TStr> >&IdStrV) {
	TVec<TPair<TUInt64, TStr> > Docs;
	Search(DsNm, Query, 100 * N, Docs);
	printf("Found %d docs for query %s\n", Docs.Len(), Query.CStr());

	TVec<TStrV> Tokens(Docs.Len());
	for (int i = 0; i < Docs.Len(); i++) {
		Dataset->Tok->GetTokens(Docs[i].Val2, Tokens[i]);
	}

	TIntV Selected;
	TVec<TPair<TUInt64, TStr> > DocsFiltered;
	Filter(Filters, Docs, Tokens, Selected);

	for (int i = 0; i < Selected.Len(); i++) {
		DocsFiltered.Add(Docs[Selected[i]]);
	}


	THash<TUInt64, TInt> QueryDocs;
	for (int i = 0; i < DocsFiltered.Len(); i++) {
		QueryDocs.AddDat(DocsFiltered[i].Val1, i);
	}
	PSemiSupervisor Model;
	ModelsLock.Enter();
	if (Models.IsKeyGetDat(ModelName, Model)) {
		ModelsLock.Leave();
		int QId = Model->FFirstQuery();
		const PBowDocBs& Bow = Model->GetBow();
		TUInt64V DocIds;
		TIntV DIds;
		int Iters = 10000;
		//ModelsLock.Enter();
		do {
			if (!DIds.IsInBin(QId)) {
				DIds.AddMerged(QId);
				ModelsLock.Enter();
				const TStr& DocNm = Bow->GetDocNm(QId);
				ModelsLock.Leave();
				const uint64 Id = DocNm.GetUInt64();
				if (QueryDocs.IsKey(Id)) {
					DocIds.Add(Id);
				}
			}
		} while (Model->FNextQuery(QId) && DocIds.Len() < N && QId != -1 && Iters-- > 0);
		//ModelsLock.Leave();

		TStrV Contents;
		Dataset->GetDocs(DocIds, Contents);

		for (int i = 0; i < DocIds.Len(); ++i) {
			IdStrV.Add(TPair<TUInt64, TStr>(DocIds[i], Docs[QueryDocs.GetDat(DocIds[i])].Val2));		}
	} else {
		ModelsLock.Leave();
		TNotify::OnNotify(Notify, ntErr, "No model under name" +  ModelName);
	}


}


void TSemiSuperviserController::TestModel(const TStr& ModelName, int SampleSize, TStrV& Classes, TCfyResV& CfyRes) {
	PSemiSupervisor Model;
	int TaskId = Dataset->GetTaskIdForModel(ModelName);

	ModelsLock.Enter();
	if (Models.IsKeyGetDat(ModelName, Model)) {
		ModelsLock.Leave();
	} else {
		
		Model = ConstructModel("NBMulti", ModelName, TaskId);
		Models.AddDat(ModelName, Model);
		ModelsLock.Leave();
	}

	
	TestModel(Model, ModelName, SampleSize, TaskId, Classes, CfyRes);

}

void TSemiSuperviserController::TestModel(const PSemiSupervisor& Model, const TStr& ModelName, int SampleSize, int TaskId, TStrV& Classes, TCfyResV& CfyRes) {
	TIntStrPrV ClassV;
	Dataset->GetTaskClasses(TaskId, ClassV);
	CfyRes.Gen(ClassV.Len());
	TVec<TPair<TUInt64, TInt> > DocIdClassId(ClassV.Len(),0); 
	Dataset->GetDocsLabelsForTaskId(TaskId, DocIdClassId);
	TIntV SubSet(DocIdClassId.Len(),0); 
	const PBowDocBs& Bow = Model->GetBow();
	for (int i = 0; i < DocIdClassId.Len(); i++) {
		int DId = Bow->GetDId(TUInt64::GetStr(DocIdClassId[i].Val1));
		if (DId == -1) {
			printf("Unknown doc id %llu\n", DocIdClassId[i].Val1);	
		} else {
			SubSet.Add(DId);
		}
	}

	TIntH ClassId2ClassN;
	for (int i = 0; i < ClassV.Len(); i++) {
		ClassId2ClassN.AddDat(ClassV[i].Val1, i);
		Classes.Add(ClassV[i].Val2);
		printf("%d %s\n", i, ClassV[i].Val2.CStr());
	}
	TIntH DocId2ClassN;
	for (int i = 0; i < SubSet.Len(); i++) {
		int ClassId = DocIdClassId[i].Val2;
		int ClassN = ClassId2ClassN.GetDat(ClassId);
		DocId2ClassN.AddDat(SubSet[i], ClassN);
	}

	const int Folds = 10;
	TIntV DIdV; TCrossValid::PrepareSubDIdV(SubSet, SampleSize != -1 ? SampleSize : SubSet.Len(), DIdV);
	
	if (DIdV.Len() <= Folds) {
		return;
	}
		
	TVec<TIntV> DIdVFoldV; TCrossValid::PrepareFolds(Folds, DIdV, (int) TTm::GetCurUniMSecs(), DIdVFoldV);
	// Folds-Fold Cross Validation
	TIntV TrainSubDIdV, TestSubDIdV; 

	for (int FoldN = 0; FoldN < Folds; FoldN++) {
		printf("%d.. ", FoldN);
		// prepare train/test split for curent fold
		TCrossValid::PrepareSplit(FoldN, DIdVFoldV, TrainSubDIdV, TestSubDIdV);
		//TIntV InterV; TrainSubDIdV.Intrs(TestSubDIdV, InterV);
		// learn model
		TVec<TIntV> Seeds(ClassV.Len());
		for (int i = 0; i < TrainSubDIdV.Len(); i++) {
			int DId = TrainSubDIdV[i];
			int ClassN = DocId2ClassN.GetDat(DId);
			Seeds[ClassN].Add(ClassN);
		}

		TIntV RealClassV(TestSubDIdV.Len());
		RealClassV.PutAll(-1);
		for (int i = 0; i < TestSubDIdV.Len(); i++) {
			int DId = TestSubDIdV[i];
			int ClassN = DocId2ClassN.GetDat(DId);
			//printf("%d %d %d\n", DId, ClassN, ClassId2ClassN.Len());
			RealClassV[i] = ClassN;
		}

		for (int i = 0; i < Seeds.Len(); i++) {
			printf("Seed %d %s: %d\n", i, ClassV[i].Val2.CStr(), Seeds[i].Len());
		}

		PSemiSupervisor SubModel = ConstructModel(Model->GetModelType(), ModelName, Seeds, Bow);
		/*PSVMModel FoldModel = MakeModel(Linear, ker, 
			TrainSet, TrainSubDIdV, ModelParam, LearnParam);*/

		// validate model
		for (int ClassN = 0; ClassN < ClassV.Len(); ClassN++) {
			TCfyRes FoldCfyRes = SubModel->Test(TestSubDIdV, RealClassV, ClassN);//FoldModel->Test(TrainSet, TestSubDIdV);
			TCfyRes Tmp;
			Tmp.Add(FoldCfyRes);
			Tmp.Def();
			printf("%s %2.4f %2.4f %2.4f\n", ClassV[ClassN].Val2.CStr(), Tmp.Prec(), Tmp.Rec(), Tmp.F1());
			CfyRes[ClassN].Add(FoldCfyRes);
		}
	} printf("\n");
	// finalize the results    
	for (int ClassN = 0; ClassN < ClassV.Len(); ClassN++) {
		CfyRes[ClassN].Def();
	}
}