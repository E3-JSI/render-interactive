/*
 * ssc.h
 *
 *  Created on: Feb 2, 2011
 *      Author: tadej
 */

#ifndef SSC_H_
#define SSC_H_

#include <base/base.h>
#include <mine/mine.h>
#include <mine/kmpp.h>

class TSemiSupKMeans: public TBowKMeans {

protected:
	// (cluster, weight) constraint for each element
	TIntFltKdV Constraints;

	virtual bool ShouldReassign(double dist, double bestDist, int docIndex) const;

public:
	friend class TPt<TSemiSupKMeans>;
	TSemiSupKMeans(const TBowMatrix *Docs, int K, int MaxItr, PNotify Not);

	static int DistinctFixedClusters(const TIntFltKdV& FixedClusters);
	void SetConstraints(const TIntFltKdV& Constraints);

};

typedef TPt<TSemiSupKMeans> PSemiSupKMeans;

#endif /* SSC_H_ */
