/*
 * to_string.h
 *
 *  Created on: Jun 29, 2010
 *      Author: tadej
 */

#ifndef TO_STRING_H_
#define TO_STRING_H_

#include <base.h>

class TToString {
public:


	template<class K, class V>
	static TStr JoinKDV(const TVec<TKeyDat<K, V > >& Vec, const TStr& Sep = ", ") {
		TChA Buf = TChA(Vec.Len() * 4);
		JoinKDV(Vec, Buf, Sep);
		return TStr(Buf);
	}


	template<class K, class V>
	static void JoinKDV(const TVec<TKeyDat<K, V > >& Vec, TChA& Buf, const TStr& Sep = ", ") {
		for (int i = 0; i < Vec.Len(); i++) {
			const TKeyDat<K, V>& Kd = Vec[i];

			Buf += Kd.Key;
			Buf += ":";
			Buf += Kd.Dat;

			if (i < Vec.Len() - 1) {
				Buf += Sep;
			}
		}
	}



	template<class T>
	static void Join(const TVec<T>& Vec, TChA& Buf, const TStr& Sep = ", ") {
		for (int i = 0; i < Vec.Len(); i++) {
			Buf += Vec[i].GetStr();
			if (i < Vec.Len() - 1) {
				Buf += Sep;
			}
		}
	}

	template<class T>
	static TStr Join(const TVec<T>& Vec, const TStr& Sep = ", ") {
		TChA Buf = TChA(Vec.Len() * 4);
		Join(Vec, Buf, Sep);
		return TStr(Buf);
	}

	template<class K, class V>
	static void Join(const THash<K,V>& Hash, TChA& Buf) {
		for (int i = 0; i < Hash.Len(); i++) {
			K& Key = Hash.GetKey(i);
			V& Val = Hash[i];

			Buf += Key;
			Buf += ":";
			Buf += Val;

			if (i < Hash.Len() - 1) {
				Buf += ", ";
			}
		}
	}

	template<class K, class V>
	static TStr Join(const THash<K,V>& Hash) {
		TChA Buf = TChA(Hash.Len() * 4);
		ToStr(Hash, Buf);
		return TStr(Buf);
	}

	template<class T>
	TStr Join(const TVec<T>& Tokens, const TStrV& Delims) {
		TChA Buf;
		IAssertR(Tokens.Len() == Delims.Len() - 1, "For n tokens, there are n + 1 delimiters: i.e. dtdtdtd");
		for (int i = 0; i < Tokens.Len(); i++) {
			Buf += Delims[i].GetStr();
			Buf += Tokens[i];
		}
		Buf += Delims.Last();
		return Buf;
	}

};

#endif /* TO_STRING_H_ */
