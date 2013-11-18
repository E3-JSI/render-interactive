#include "nbc.h"

NB_AL::NB_AL() {
}
NB_AL::NB_AL(PBowDocBs BowDocBsX, TIntV posSeed, TIntV negSeed, TFlt us) {
	Bow = BowDocBsX;
	Docs = Bow->GetDocs();
	wordsN = Bow->GetWords();
	posFreq = TVec < TInt > (wordsN);
	negFreq = TVec < TInt > (wordsN);
	unlFreq = TVec < TInt > (wordsN);
	probs = TFltV(Docs);
	probs.PutAll(-1.0);
	cat = TVec < TInt > (Docs);
	loglike = TVec < TFlt > (wordsN);
	posFreq.PutAll(0);
	negFreq.PutAll(0);
	cat.PutAll(0);
	loglike.PutAll(0);
	pos = 1;
	neg = 1;
	unl = 1;
	uncert = us;
	for (int i = 0; i < posSeed.Len(); i++)
		add(posSeed[i], 1);
	for (int i = 0; i < negSeed.Len(); i++)
		add(negSeed[i], 0);
	calcBackground();
	calcLogLike();
}

NB_AL::NB_AL(TSIn& SIn) :
		Bow(SIn), cat(SIn) {
	init();
	calc();
}

NB_AL::NB_AL(PBowDocBs pbow, TSIn& SIn) :
		cat(SIn) {
	Bow = pbow;
	init();
	calc();
}

void NB_AL::init() {
	Docs = Bow->GetDocs();
	wordsN = Bow->GetWords();
	posFreq = TVec < TInt > (wordsN);
	negFreq = TVec < TInt > (wordsN);
	unlFreq = TVec < TInt > (wordsN);
	probs = TFltV(Docs);
	probs.PutAll(-1.0);
	pos = 1;
	neg = 1;
	unl = 1;
	uncert = 0.5;
	loglike = TVec < TFlt > (wordsN);
	loglike.PutAll(0);
}

void NB_AL::calc() {
	for (int doc = 0; doc < Docs; doc++) {
		if (cat[doc] != 0) {
			if (cat[doc] == -1)
				neg++;
			else
				pos++;
			for (int j = 0; j < Bow->GetDocWIds(doc); j++) {
				int id = Bow->GetDocWId(doc, j);
				if (cat[doc] == 1)
					posFreq[id]++;
				else
					negFreq[id]++;
			}
		} 
	}
	calcBackground();
	calcLogLike();
}

void NB_AL::Save(TSOut& SOut) {
	Bow.Save(SOut);
	SaveCat(SOut);
}

void NB_AL::SaveCat(TSOut& SOut) {
	cat.Save(SOut);
}

void NB_AL::calcBackground() {
	unl = Docs - neg - pos + 3; // + 3 compensates for -2 on neg/pos counts smoothing + 1 for smoothing for unl
	for (int i = 0; i < Bow->GetWords(); i++) {
		unlFreq[i] = Bow->GetWordFq(i) - posFreq[i] - negFreq[i];
	}
}

void NB_AL::calcLogLike() {
	p = (double) pos / (pos + neg/* + unl*/);
	loglike0 = log(p / (1 - p));
	for (int id = 0; id < Bow->GetWords(); id++) {
		double posi = ((double) posFreq[id] + p * 2) / pos;
		double negi = ((double) negFreq[id] + (1 - p) * 2) / neg;
		loglike[id] = log(posi) - log(negi);
	}
}

void NB_AL::add(int doc, int c) {
	Assert(c == 0 || c == 1);
	c = 2 * c - 1;
	if (doc < 0 || doc >= Bow->GetDocs())
		return;
	cat[doc] = c;
	if (c == 1)
		pos++;
	else
		neg++;
	for (int i = 0; i < Bow->GetDocWIds(doc); i++) {
		int id = Bow->GetDocWId(doc, i);
		if (cat[doc] == 1)
			posFreq[id]++;
		else
			negFreq[id]++;
	}
}

void NB_AL::put(int doc, int c) {
	add(doc, c);
	calcLogLike();
}

int NB_AL::query(int n) {
	double bestP = -1;
	int bestId = -1;
	for (int i = 0; i < min(n, Docs); i++) {
		int id = rnd.GetUniDevInt(Docs);
		if (cat[id] != 0) {
			continue;
		}
		double p = get(id);
		if (abs(uncert - p) < abs(uncert - bestP)) {
			bestP = p;
			bestId = id;
		}
	}
	return bestId;
}

int NB_AL::query() {
	double bestP = -1;
	int bestId = -1;
	for (int i = 0; i < min(Docs, 1000000); i++) {
		int id = rnd.GetUniDevInt(Docs);
		if (cat[id] != 0) {
			continue;
		}
		double p = get(id);
		if (abs(uncert - p) < abs(uncert - bestP)) {
			bestP = p;
			bestId = id;
		}
	}
	return bestId;
}

double NB_AL::get(const TIntV& Tokens) {
	double like = loglike0;

	for (int j = 0; j < Tokens.Len(); j++) {
		int id = Tokens[j];
		like += loglike[id];
	}
	return exp(like) / (1 + exp(like));
}

void NB_AL::GetBowWIds(const PBowDocBs& Bow, const TStrV& Tokens,
		TIntV& TokenIds) {
	for (int j = 0; j < Tokens.Len(); j++) {
		int id = Bow->GetWId(Tokens[j].GetLc());
		if (id != -1) {
			TokenIds.Add(id);
		}
	}
}

double NB_AL::get(const TStrV& Tokens) {
	TIntV TokenIds(Tokens.Len(), 0);
	GetBowWIds(Bow, Tokens, TokenIds);
	return get(TokenIds);
}

double NB_AL::get(int doc) {
	double prob = probs[doc];
	if (prob != -1.0) {
		return prob;
	}

	double like = loglike0;
	int WIds =  Bow->GetDocWIds(doc);
	for (int j = 0; j < WIds; j++) {
		int id = Bow->GetDocWId(doc, j);
		like += loglike[id];
	}
	double expLike = exp(like);
	prob = expLike / (1 + expLike);
	probs[doc] = prob;
	return prob;
}

NB_AL_Senti::NB_AL_Senti() {
}
NB_AL_Senti::NB_AL_Senti(PBowDocBs BowDocBs0, TIntV posEx, TIntV negEx,
		TIntV objEx) {
	TIntV posNegEx = posEx;
	posNegEx.AddV(negEx);
	TIntV posObjEx = posEx;
	posObjEx.AddV(objEx);
	TIntV negObjEx = negEx;
	negObjEx.AddV(objEx);
	NBp = NB_AL(BowDocBs0, posEx, negObjEx);
	NBn = NB_AL(BowDocBs0, negEx, posObjEx);
	NBo = NB_AL(BowDocBs0, objEx, posNegEx);
	qCount = 0;
}

NB_AL_Senti::NB_AL_Senti(TSIn& SIn) { // load
	Bow = PBowDocBs(SIn);
	qCount = TInt(SIn);
	NBp = NB_AL(Bow, SIn);
	NBn = NB_AL(Bow, SIn);
	NBo = NB_AL(Bow, SIn);
}

void NB_AL_Senti::Save(TSOut& SOut) { // save
	NBp.Bow.Save(SOut);
	qCount.Save(SOut);
	NBp.SaveCat(SOut);
	NBn.SaveCat(SOut);
	NBo.SaveCat(SOut);
}

void NB_AL_Senti::put(int id, int c) {
	if (c == -1) {
		NBp.put(id, 0);
		NBn.put(id, 1);
		NBo.put(id, 0);
	} else if (c == 1) {
		NBp.put(id, 1);
		NBn.put(id, 0);
		NBo.put(id, 0);
	} else {
		NBp.put(id, 0);
		NBn.put(id, 0);
		NBo.put(id, 1);
	}
}

int NB_AL_Senti::query() {
	qCount++;
	if (qCount % 3 == 0)
		return NBp.query();
	else if (qCount % 3 == 1)
		return NBn.query();
	else
		return NBo.query();
}

TPair<TFlt, TInt> NB_AL_Senti::get(int id) {
	double pP = NBp.get(id);
	double pN = NBn.get(id);
	double pO = NBo.get(id);
	double f = max(pP + pN + pO, 0.1);
	pP /= f;
	pN /= f;
	pO /= f;
	if (pP >= pN && pP >= pO)
		return TPair < TFlt, TInt > (pP, 1);
	if (pN >= pP && pN >= pO)
		return TPair < TFlt, TInt > (pN, -1);
	return TPair < TFlt, TInt > (pO, 0);
}

TPair<TFlt, TInt> NB_AL_Senti::get(const TStrV& Tokens) {
	TIntV TokenIds(Tokens.Len(), 0);
	NB_AL::GetBowWIds(Bow, Tokens, TokenIds);
	return get(TokenIds);
}

TPair<TFlt, TInt> NB_AL_Senti::get(const TIntV& TokenIds) {
	double pP = NBp.get(TokenIds);
	double pN = NBn.get(TokenIds);
	double pO = NBo.get(TokenIds);
	double f = max(pP + pN + pO, 0.1);
	pP /= f;
	pN /= f;
	pO /= f;
	if (pP >= pN && pP >= pO)
		return TPair < TFlt, TInt > (pP, 1);
	if (pN >= pP && pN >= pO)
		return TPair < TFlt, TInt > (pN, -1);
	return TPair < TFlt, TInt > (pO, 0);
}

NB_AL_Multi::NB_AL_Multi() {

}
NB_AL_Multi::NB_AL_Multi(PBowDocBs BowDocBs, const TVec<TIntV>& seeds) : Bow(BowDocBs) {
	n = seeds.Len();
	Models.Gen(n);
	Classes.Gen(n);
	for (int i = 0; i < n; ++i) {
		const TIntV& SeedsI = seeds[i];
		TIntV AntiSeeds;
		for (int j = 0; j < n; ++j) {
			if (i == j)
				continue;
			AntiSeeds.AddV(seeds[j]);
		}
		Models[i] = NB_AL(BowDocBs, SeedsI, AntiSeeds);
		Classes[i] = TStr("class ") + TInt::GetStr(i);
	}
	qCount = 0;
}
NB_AL_Multi::NB_AL_Multi(TSIn& SIn) {
	Bow = PBowDocBs(SIn);
	Classes = TStrV(SIn);
	qCount = TInt(SIn);
	n = Classes.Len();
	Models.Gen(n);
	for (int i = 0; i < n; ++i) {
		Models[i] = NB_AL(Bow, SIn);
	}
}
void NB_AL_Multi::Save(TSOut& SOut) {
	Bow.Save(SOut);
	Classes.Save(SOut);
	qCount.Save(SOut);
	for (int i = 0; i < n; ++i) {
		Models[i].SaveCat(SOut);
	}
}

NB_AL_Multi::NB_AL_Multi(PBowDocBs FreshBow, TVec<PSIn>& Inputs, PSIn& InputUnlabeled,
		const TStrV& ClassNames) :
		Bow(FreshBow), Classes(ClassNames) {
	IAssert(Inputs.Len() == Classes.Len());
	n = Inputs.Len();
	Models.Gen(n);
	int DocN = 0;

	// load unlabeled background data
	TIntV Unlabeled;
	if (!InputUnlabeled.Empty()) {
		TChA Ln;
		while (InputUnlabeled->GetNextLn(Ln)) {
			int DId = Bow->AddHtmlDoc(TInt::GetStr(Bow->GetDocs()), TStrV(),
				TStr(Ln.ToUc()), true);
			Unlabeled.Add(DId);
		}
	}

	// construct bow and gather docIds for each class
	TVec < TIntV > Seeds(n);
	for (int clas = 0; clas < n; ++clas) {
		PSIn& SIn = Inputs[clas];
		TChA Ln;
		TIntV& ClassSeeds = Seeds[clas];
		while (SIn->GetNextLn(Ln)) {
			int DId = Bow->AddHtmlDoc(TInt::GetStr(Bow->GetDocs()), TStrV(),
					TStr(Ln.ToUc()), true);
			ClassSeeds.Add(DId);
		}
	}

	for (int i = 0; i < n; ++i) {
		const TIntV& SeedsI = Seeds[i];
		TIntV AntiSeeds(Unlabeled);
		for (int j = 0; j < n; ++j) {
			if (i == j)
				continue;
			AntiSeeds.AddV(Seeds[j]);
		}
		Models[i] = NB_AL(Bow, SeedsI, AntiSeeds);
	}
}

void NB_AL_Multi::put(int id, int c) {
	for (int i = 0; i < n; ++i) {
		if (c == i) {
			Models[i].put(id, 1);
		} else {
			Models[i].put(id, 0);
		}
	}
}

int NB_AL_Multi::query() {
	qCount++;
	return Models[qCount % n].query(100);
}
TPair<TFlt, TInt> NB_AL_Multi::get(int id) {
	TFltV Probs(n);
	for (int i = 0; i < n; ++i) {
		Probs[i] = Models[i].get(id);
	}
	double AllP = TLinAlg::SumVec(Probs);
	double f = max(AllP, 0.1);
	TLinAlg::MultiplyScalar(1.0 / f, Probs, Probs);
	int mostLikelyClass = Probs.GetMxValN();
	return TPair < TFlt, TInt > (Probs[mostLikelyClass], mostLikelyClass);
}

TPair<TFlt, TInt> NB_AL_Multi::get(const TStrV& Tokens) {
	TIntV TokenIds(Tokens.Len(), 0);
	NB_AL::GetBowWIds(Bow, Tokens, TokenIds);
	return get(TokenIds);
}

TPair<TFlt, TInt> NB_AL_Multi::get(const TIntV& TokenIds) {
	TFltV Probs(n);
	for (int i = 0; i < n; ++i) {
		Probs[i] = Models[i].get(TokenIds);
	}
	double AllP = TLinAlg::SumVec(Probs);
	double f = max(AllP, 0.1);
	TLinAlg::MultiplyScalar(1.0 / f, Probs, Probs);
	int mostLikelyClass = Probs.GetMxValN();
	if (Probs[mostLikelyClass] > 0.5) {
		return TPair < TFlt, TInt > (Probs[mostLikelyClass], mostLikelyClass);
	} else {
		return TPair < TFlt, TInt > (0.0, -1);
	}
}

void NB_AL_Multi::SetClassNm(const int c, const TStr& Cls) {
	EAssertR(Classes.Len() > c, TStr("Unknown class id ") + TInt::GetStr(c));
	Classes[c] = Cls;
}

const TStr& NB_AL_Multi::GetClassNm(const int c) const {
	return Classes[c];
}

void NB_AL_Multi::GetTopK(const TStrV& Tokens, TIntFltPrV& Results, int k) {
	TIntV TokenIds(Tokens.Len(), 0);
	NB_AL::GetBowWIds(Bow, Tokens, TokenIds);
	TFltV Probs(n);
	for (int i = 0; i < n; ++i) {
		Probs[i] = Models[i].get(TokenIds);
	}
	double AllP = TLinAlg::SumVec(Probs);
	double f = max(AllP, 0.1);
	TLinAlg::MultiplyScalar(1.0 / f, Probs, Probs);


	int bestIndex = -1;
	double best = 1.0;
	for (int j = 0; j < k; ++j) {
		int currentMostLikely = -1;
		double currentBest = 0.0;


		for (int i = 0; i < Probs.Len(); ++i) {
			double p_i = Probs[i];
			if (p_i > currentBest && p_i <= best && i != bestIndex) {
				currentMostLikely = i;
				currentBest = p_i;
			}
		}

		if (currentMostLikely != -1) {
			Results.Add(TIntFltPr(currentMostLikely, currentBest));
			best = currentBest;
			bestIndex = currentMostLikely;
		}
	}
}
