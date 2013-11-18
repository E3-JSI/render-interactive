/*
 * ssc.cpp
 *
 *  Created on: Feb 2, 2011
 *      Author: tadej
 */

#include "ss_km.h"


TSemiSupKMeans::TSemiSupKMeans(const TBowMatrix *Docs, int K, int MaxItr, PNotify Not) :
	TBowKMeans(Docs,K,MaxItr,Not), Constraints(Docs->GetRows()) {

}


int TSemiSupKMeans::DistinctFixedClusters(const TIntFltKdV& FixedClusters) {
	TIntSet Known;
	for (int i = 0; i < FixedClusters.Len(); i++) {
		if (FixedClusters[i].Key > -1) {
			// -1 means non fixed
			Known.AddKey(FixedClusters[i].Key);
		}
	}
	return Known.Len();
}


void TSemiSupKMeans::SetConstraints(const TIntFltKdV& Const) {
	Constraints = Const;

	int Uniq = DistinctFixedClusters(Const);
	if (Uniq == k) {
		TNotify::OnNotify(Notify, ntWarn, "Warning - k is the same as set of constraints, will not create new clusters");
	}
	EAssertR(Uniq >= k, "Warning - k is bigger as set of constraints, this will fail");

}


bool TSemiSupKMeans::ShouldReassign(double sim, double bestSim, int docIndex) const {
	return sim - bestSim > Constraints[docIndex].Dat;
}
