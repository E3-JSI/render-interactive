/*
 * json.h
 *
 *  Created on: Nov 17, 2011
 *      Author: tadej
 */

#ifndef JSON_H_
#define JSON_H_

#include <base.h>

class TJson {
public:
	
	template<class T>
	static TStr Dumps(const T& Obj) {
		PMem Mem = TMem::New(64);
		TMemOut MemOut(Mem);
		Write(Obj, Mem);
		return Mem->GetAsStr();
	}

		
	template<class T>
	static TStr Dumps(const TVec<T>& Obj) {
		PMem Mem = TMem::New(64);
		TMemOut MemOut(Mem);
		Write(Obj, Mem);
		return Mem->GetAsStr();
	}


	template<class T>
	static void Write(const TVec<T>& V, TSOut& Buf) {
		bool first = true;
		Buf.PutCh('[');
		for (int i = 0; i < V.Len(); ++i) {
			if (first) {
				first = false;
			} else {
				Buf.PutCh(',');
			}
			Write(V[i], Buf);
		}
		Buf.PutCh(']');
	}
	static void Write(const TStr& Str, TSOut& Buf) {
		bool Unicode = false;
		bool ToEscape = false;
		int Len = Str.Len();
		for (int i = 0; i < Len; ++i) {
			char ch = Str[i];
			if (!isascii(ch)) {
				Unicode = true;
			} else if (ch == '"' || ch == '\n' || ch == '\r') {
				ToEscape = true;
			}
		}
		Buf.PutCh('"');
		if (ToEscape) {
			if (Unicode) {
				WriteUnicode(Str, Len, Buf);
			} else {
				WriteAscii(Str, Len, Buf);
			}
		} else {
			Buf.PutStr(Str);
		}
		Buf.PutCh('"');
	}

	static void WriteAscii(const TStr& Str, int Len, TSOut& Buf) {
		for (int i = 0; i < Len; i++) {
			int ch = Str[i];
			switch (ch) {
			case '\"':
				Buf.PutBf("\\\"", 2);
				break;
			case '\\':
				Buf.PutBf("\\\\", 2);
				break;
			case '\b':
				Buf.PutBf("\\b", 2);
				break;
			case '\f':
				Buf.PutBf("\\f", 2);
				break;
			case '\n':
				Buf.PutBf("\\n", 2);
				break;
			case '\r':
				Buf.PutBf("\\r", 2);
				break;
			case '\t':
				Buf.PutBf("\\t", 2);
				break;
			default:
				Buf.PutCh(ch);
				break;
			}
		}
	}

	static void WriteUnicode(const TStr& Str, int Len, TSOut& Buf) {
		const TUnicode *Unicode = TUnicodeDef::GetDef();
		TIntV UniChA(Len, 0);
		TIntV CleanChA(Len, 0);
		Unicode->DecodeUtf8(Str, UniChA);
		for (int i = 0; i < UniChA.Len(); i++) {
			int ch = UniChA[i];
			switch (ch) {
			case '\"':
				CleanChA.Add('\\');
				CleanChA.Add('"');
				break;
			case '\\':
				CleanChA.Add('\\');
				CleanChA.Add('\\');
				break;
			case '/':
				CleanChA.Add('\\');
				CleanChA.Add('/');
				break;
			case '\b':
				CleanChA.Add('\\');
				CleanChA.Add('f');
				break;
			case '\f':
				CleanChA.Add('\\');
				CleanChA.Add('f');
				break;
			case '\n':
				CleanChA.Add('\\');
				CleanChA.Add('n');
				break;
			case '\r':
				CleanChA.Add('\\');
				CleanChA.Add('r');
				break;
			case '\t':
				CleanChA.Add('\\');
				CleanChA.Add('f');
				break;
			default:
				CleanChA.Add(ch);
				break;
			}
		}
		const TStr OutUtf8 = Unicode->EncodeUtf8Str(CleanChA);
		Buf.PutStr(OutUtf8);
	}

	static void Write(const TFlt& Val, TSOut& Buf) {
		char Bf[32];
		int Len = snprintf(Bf, 32, "%f", Val.Val);
		Buf.PutBf(Bf, Len);
	}
	/*static void Write(const TInt& Val, TSOut& Buf) {
		char Bf[16];
		int Len = snprintf(Bf, 16, "%d", Val.Val);
		Buf.PutBf(Bf, Len);
	}*/
	static void Write(const TUInt64& Val, TSOut& Buf) {
		char Bf[32];
		// 64-bit integers are serialized as strings, since JS uses 64-bit floats to represent all numbers, which leads to 
		// poor precision on identifiers which are large numbers
#ifdef GLib_WIN
		int Len = snprintf(Bf, 32, "\"%I64u\"", Val.Val);
#else
		int Len = snprintf(Bf, 32, "\"%lu\"", Val.Val);
#endif
		Buf.PutBf(Bf, Len);
		//Buf.PutStr(Val.GetStr());
	}


	template<class K, class V>
	static void Write(const THash<K, V>& Hash, TSOut& Buf) {
		Buf.PutCh('{');
		bool first = true;
		for (int i = 0; i < Hash.Len(); ++i) {
			if (first) {
				first = false;
			} else {
				Buf.PutCh(',');
			}
			const K& Key = Hash.GetKey(i);
			const V& Val = Hash[i];
			Write(Key, Buf);
			Buf.PutCh(':');
			Write(Val, Buf);
		}
		Buf.PutCh('}');
	}
	
	template<class A, class B>
	static TStr Dumps(const TVec<TPair<A, B> >& V,  const TStr& KeyA
			, const TStr& KeyB ) {
		PMem Mem = TMem::New(64);
		TMemOut MemOut(Mem);
		Write(V, KeyA, KeyB, MemOut);
		return Mem->GetAsStr();
	}

	template<class A, class B>
	static void Write(const TVec<TPair<A, B> >& V, const TStr& KeyA
			, const TStr& KeyB, TSOut& Buf) {
		bool first = true;
		Buf.PutCh('[');
		for (int i = 0; i < V.Len(); ++i) {
			if (first) {
				first = false;
			} else {
				Buf.PutCh(',');
			}
			Buf.PutCh('{');
			Write(KeyA, Buf);
			Buf.PutCh(':');
			Write(V[i].Val1, Buf);
			Buf.PutCh(',');
			Write(KeyB, Buf);
			Buf.PutCh(':');
			Write(V[i].Val2, Buf);
			Buf.PutCh('}');
		}
		Buf.PutCh(']');
	}

	template<class A, class B, class C>
	static TStr Dumps(const TVec<TTriple<A, B, C> >& V,  const TStr& KeyA
			, const TStr& KeyB , const TStr& KeyC) {
		PMem Mem = TMem::New(64);
		TMemOut MemOut(Mem);
		Write(V, KeyA, KeyB, KeyC, MemOut);
		return Mem->GetAsStr();
	}
	
	template<class A, class B, class C>
	static void Write(const TVec<TTriple<A, B, C> >& V, const TStr& KeyA
			, const TStr& KeyB, const TStr& KeyC, TSOut& Buf) {
		bool first = true;
		Buf.PutCh('[');
		for (int i = 0; i < V.Len(); ++i) {
			if (first) {
				first = false;
			} else {
				Buf.PutCh(',');
			}
			Buf.PutCh('{');
			Write(KeyA, Buf);
			Buf.PutCh(':');
			Write(V[i].Val1, Buf);
			Buf.PutCh(',');
			Write(KeyB, Buf);
			Buf.PutCh(':');
			Write(V[i].Val2, Buf);
			Buf.PutCh(',');
			Write(KeyC, Buf);
			Buf.PutCh(':');
			Write(V[i].Val3, Buf);
			Buf.PutCh('}');
		}
		Buf.PutCh(']');
	}

	template<class A, class B, class C, class D>
	static TStr Dumps(const TVec<TQuad<A, B, C, D> >& V,  const TStr& KeyA
			, const TStr& KeyB , const TStr& KeyC, const TStr& KeyD) {
		PMem Mem = TMem::New(64);
		TMemOut MemOut(Mem);
		Write(V, KeyA, KeyB, KeyC, KeyD, MemOut);
		return Mem->GetAsStr();
	}
	
	template<class A, class B, class C, class D>
	static void Write(const TVec<TQuad<A, B, C, D> >& V, const TStr& KeyA
			, const TStr& KeyB, const TStr& KeyC, const TStr& KeyD, TSOut& Buf) {
		bool first = true;
		Buf.PutCh('[');
		for (int i = 0; i < V.Len(); ++i) {
			if (first) {
				first = false;
			} else {
				Buf.PutCh(',');
			}
			Buf.PutCh('{');
			Write(KeyA, Buf);
			Buf.PutCh(':');
			Write(V[i].Val1, Buf);
			Buf.PutCh(',');
			Write(KeyB, Buf);
			Buf.PutCh(':');
			Write(V[i].Val2, Buf);
			Buf.PutCh(',');
			Write(KeyC, Buf);
			Buf.PutCh(':');
			Write(V[i].Val3, Buf);
			Buf.PutCh(',');
			Write(KeyD, Buf);
			Buf.PutCh(':');
			Write(V[i].Val4, Buf);
			Buf.PutCh('}');
		}
		Buf.PutCh(']');
	}

};

#endif /* JSON_H_ */
