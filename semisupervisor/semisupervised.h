/*
 * ss.h
 *
 *  Created on: Nov 10, 2011
 *      Author: tadej
 */

#ifndef SEMISUPERVISED_H_
#define SEMISUPERVISED_H_

#include <base.h>
#include "nbc.h"


ClassTP(TSemiSupervisor, PSemiSupervisor)
public:

	virtual const PBowDocBs& GetBow() const = 0;
	virtual void Set(int DocId, int Class) = 0;
	virtual void Get(int DocId, double& Score, int& Class) = 0;
	virtual void Classify(const TStrV& Tokens, double& Score, int& Class) = 0;
	virtual bool FNextQuery(int &QId) = 0;
	virtual int FFirstQuery() = 0;
	virtual void Save(TSOut& Out) = 0;
	virtual ~TSemiSupervisor() {}
	virtual TStr GetModelType() const = 0; 

	TCfyRes Test(const TIntV& TestDIds, const TIntV& RealClassNs, int MeasuredClassN) {
		TCountCfyRes Result;
		IAssert(TestDIds.Len() == RealClassNs.Len());
		for (int i = 0; i < TestDIds.Len(); i++) {
			const int DId = TestDIds[i];
			const int RealClass = RealClassNs[i];
			if (RealClass == -1) 
				continue;

			double Score;
			int Class;
			Get(DId, Score, Class);
			Result.Add(Class == MeasuredClassN ? 1.0 : -1.0, RealClass == MeasuredClassN ? 1.0 : - 1.0);
		}
		return Result.ToTCfyRes();
	}

};

class TNBA2ClassSupervisor : public TSemiSupervisor {
protected:
	NB_AL model;

public:

	TNBA2ClassSupervisor(PBowDocBs BowDocBsX, const TIntV& posSeed, const TIntV& negSeed, TFlt us=0.5) : model(BowDocBsX, posSeed, negSeed, us) {
		model.calc();
	}
	
	TNBA2ClassSupervisor(TSIn& In) : model(In) {
		
	}

	TStr GetModelType() const {
		return "NB2";
	}

	const PBowDocBs& GetBow() const  {
		return model.Bow;
	}

	void Set(int DocId, int Class) {
		model.put(DocId, Class);
	}
	void Get(int DocId, double& Score, int& Class) {
		Score = model.get(DocId);
		Class = Score > 0.6 ? 1 : 0;
	}
	void Classify(const TStrV& Tokens, double& Score, int& Class) {
		Score = model.get(Tokens);
		Class = Score > 0.6 ? 1 : 0;
	}
	int FFirstQuery() {
		return model.query();
	}
	bool FNextQuery(int &QId) {
		QId = model.query();
		return true;
	}
	
	void Save(TSOut& Out) {
		model.Save(Out);
	}
};



class TNBAMultiClassSupervisor : public TSemiSupervisor {
protected:
	NB_AL_Multi ActLearn;

public:
	TNBAMultiClassSupervisor(PBowDocBs BowDocBsX, const TVec<TIntV>& seeds) : ActLearn(BowDocBsX, seeds) {
	}

	const PBowDocBs& GetBow() const  {
		return ActLearn.Bow;
	}
	
	TStr GetModelType() const {
		return "NBMulti";
	}
	void Set(int DocId, int Class) {
		ActLearn.put(DocId, Class);
	}
	void Get(int DocId, double& Score, int& Class) {
		const TFltIntPr& Res = ActLearn.get(DocId);
		Score = Res.Val1;
		Class = Res.Val2;
	}
	void Classify(const TStrV& Tokens, double& Score, int& Class) {
		const TFltIntPr& Res = ActLearn.get(Tokens);
		Score = Res.Val1;
		Class = Res.Val2;
	}

	int FFirstQuery() {
		return ActLearn.query();
	}
	bool FNextQuery(int &QId) {
		QId = ActLearn.query();
		return true;
	}
	
	void Save(TSOut& Out) {
		ActLearn.Save(Out);
	}
};

class TBowALSupervisor : public TSemiSupervisor {
protected:
	PBowAL ActLearn;
	PBowDocBs Bow;
	TIntV PosDocs;
	TIntH DocToQuery;

public:
	TBowALSupervisor(PBowAL AL, PBowDocBs B) : ActLearn(AL), Bow(B) {
	}
	~TBowALSupervisor() {};


	const PBowDocBs& GetBow() const {
		return Bow;
	}
	TStr GetModelType() const {
		return "BowAL";
	}

	void Set(int DocId, int Class) {
		ActLearn->MarkQueryDoc(DocId, Class == 0 ? false : true);
		PosDocs.Clr();
		ActLearn->GetAllPosDocs(PosDocs);
	}
	void Get(int DocId, double& Score, int& Class) {
		if (PosDocs.SearchBin(DocId) == -1) {
			Score = -1.0;
			Class = 0;
		} else {
			Score = 1.0;
			Class = 1;
		}
	}

	void Classify(const TStrV& Tokens, double& Score, int& Class) {
		PosDocs.Clr();
		ActLearn->GetAllPosDocs(PosDocs);
		// TODO
	}

	int FFirstQuery() {
		DocToQuery.Clr();
		if (!ActLearn->GenQueryDIdV(false)) {
			return -1;
		} else {
			int FirstDoc = -1;
			for (int i = 0; i < ActLearn->GetQueryDIds(); i++) {
				int DId;
				double Dist;
				ActLearn->GetQueryDistDId(i, Dist, DId);
				DocToQuery.AddDat(DId,i);
				if (FirstDoc == -1) {
					FirstDoc = DId;
				}
			}
			return FirstDoc;
		}
	}

	bool FNextQuery(int &DocId) {
		int QId = DocToQuery.GetDat(DocId);
		if (QId < ActLearn->GetQueryDIds()) {
			double Dist;
			ActLearn->GetQueryDistDId(QId + 1, Dist, DocId);
			return true;
		} else {
			return false;
		}
	}
	
	void Save(TSOut& Out) {
		ActLearn.Save(Out);
	}
};


#endif /* SEMISUPERVISED_H_ */
