/*
 * nbc.h
 *
 *  Created on: Nov 10, 2011
 *      Author: tadej
 */

#ifndef NBC_H_
#define NBC_H_

#include <base.h>
#include <mine.h>

class NB_AL {
protected:
	void calcBackground();
	void add(int doc, int c);

public:
	PBowDocBs Bow;
	TVec<TInt> cat;					// razred i-tega primera -1/0/1 negative/unknown/positive

	double uncert;					// target probability for uncertainty sampling
	int wordsN, Docs;				// stevilo besed, dokumentov
	TInt pos,neg,unl;					// stevilo pozitivnih in negativnih primerov
	TFlt p, loglike0;				// verjetnost pozitivnega razreda
	TVec<TInt> posFreq, negFreq, unlFreq;	// st. pojavitev i-te besede v pozitivnih oz. negativnih primerih
	TVec<TFlt> loglike;
	TFltV probs;
	TRnd rnd;

	NB_AL();
	NB_AL(PBowDocBs BowDocBsX, TIntV posSeed, TIntV negSeed, TFlt us=0.5);
	NB_AL(TSIn& SIn);
	NB_AL(PBowDocBs pbow, TSIn& SIn);

	void init();
	void calc();

	void Save(TSOut& SOut);
	void SaveCat(TSOut& SOut);

	void calcLogLike();

	void put(int doc, int c);
	int query();
	int query(int n);
	double get(int doc);
	double get(const TIntV& Tokens);
	double get(const TStrV& Tokens);

	static void GetBowWIds(const PBowDocBs& Bow, const TStrV& Tokens, TIntV& TokenIds);
};

class NB_AL_Senti {
public:
	PBowDocBs Bow;
	NB_AL NBp, NBn, NBo;
	TInt qCount;

	NB_AL_Senti();
	NB_AL_Senti(PBowDocBs BowDocBs0, TIntV posEx, TIntV negEx, TIntV objEx);
	NB_AL_Senti(TSIn& SIn);
	void Save(TSOut& SOut);
	void put(int id, int c);
	int query();
	TPair<TFlt, TInt> get(int id);


	TPair<TFlt, TInt> get(const TIntV& Tokens);
	TPair<TFlt, TInt> get(const TStrV& Tokens);


};

class NB_AL_Multi {

public:
	PBowDocBs Bow;
protected:
	TVec<NB_AL> Models;
	TStrV Classes;
	TInt n;
	TInt qCount;

public:
	NB_AL_Multi();
	NB_AL_Multi(PBowDocBs BowDocBs, const TVec<TIntV>& seeds);
	NB_AL_Multi(TSIn& SIn);
	/**
	 * Construct a new multi-class classifier by injecting a bow with your favorite tokenizer and
	 * stopword set. Then, supply a set of input streams and class names, each containing a line-separated
	 * source of examples for each class.
	 */
	NB_AL_Multi(PBowDocBs FreshBow, TVec<PSIn>& Inputs, PSIn& InputUnlabeled, const TStrV& Classes);
	void Save(TSOut& SOut);
	void SetClassNm(const int c, const TStr& Cls);
	const TStr& GetClassNm(const int c) const;
	void put(int id, int c);
	int query();
	TPair<TFlt, TInt> get(int id);
	TPair<TFlt, TInt> get(const TIntV& Tokens);
	TPair<TFlt, TInt> get(const TStrV& Tokens);
	void GetTopK(const TStrV& Tokens, TIntFltPrV& Results, int k);
};





#endif /* NBC_H_ */
