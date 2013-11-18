/*
 * ss_dataset.cpp
 *
 *  Created on: Nov 11, 2011
 *      Author: tadej
 */
#include "ss_dataset.h"
#include <qminer.h>

PBowDocBs TOgDsProvider::GetBow(
		const TVec<TPair<TUInt64, TStr> >& IdStrV) const {
	PBowDocBs BowBs = TBowDocBs::New(SwSet, Stemmer, NULL);
	for (int j = 0; j < IdStrV.Len(); ++j) {
		const uint64 TweetId = IdStrV[j].Val1;
		const TStr& Content = IdStrV[j].Val2;
		TStrV Tokens;
		Tok->GetTokens(Content, Tokens);
		BowBs->AddDoc(TUInt64::GetStr(TweetId), TStrV(), Tokens, TStr());
	}
	return BowBs;
}

TOgDsProvider::TOgDsProvider(const TStr& FNm) : DbNm(FNm) {

	SwSet = TSwSet::New(swstNone);
	Stemmer = TStemmer::New(stmtNone);

	TUnicodeDef::Load("UnicodeDef.Bin");
	Tok = TOgUnicodeHtmlTokenizer::New(SwSet, Stemmer);

	Conn = TSQLConnection::New(FNm);
	Qc = new TSQLQueryCache(Conn);
	Miner = TOgBase::New();

	TOgFieldDescV Fields;

	/* from qminer.h:
	 // QMiner-Field-Description
	 typedef enum {
	 oftInt		= 0,
	 oftStr		= 1,
	 oftStrV		= 2,
	 oftBool		= 4,
	 oftFlt		= 5,
	 oftFltPr	= 6,
	 oftTm		= 7,
	 oftUInt64	= 8
	 } TOgFieldType;

	 typedef enum {
	 offgtNone		= 0,
	 offgtNumeric	= 1,
	 offgtNominal	= 2,
	 offgtMultiNom	= 3,
	 offgtToken		= 4,
	 offgtSpNum		= 5,
	 offgtTm			= 6
	 } TOgFieldFtrGenType;

	 typedef enum {
	 ofatNone		= 0,
	 ofatHistogram	= 1,
	 ofatPiechart	= 2,
	 ofatTimeline	= 3,
	 ofatKeywords	= 4,
	 ofatScalar		= 5
	 } TOgFieldAggrType;

	 typedef enum {
	 ofdtNone	= 0,
	 ofdtText	= 1,
	 ofdtMap		= 2
	 } TOgFieldDisplayType;
	 */

	Fields.Add(
			TOgFieldDesc("tweet_id", oftUInt64, offtNominal, ofatNone,
					ofdtText));
	Fields.Add(
			TOgFieldDesc("tweet_text", oftStr, offtToken, ofatNone, ofdtText));
	Fields.Add(
			TOgFieldDesc("created_at", oftTm, offtTm, ofatTimeline, ofdtText));
	Fields.Add(
			TOgFieldDesc("first_seen", oftTm, offtTm, ofatTimeline, ofdtText));
	Fields.Add(
			TOgFieldDesc("retweeted", oftBool, offtNominal, ofatNone,
					ofdtText));
	Fields.Add(
			TOgFieldDesc("retweet_count", oftInt, offtNumeric, ofatHistogram,
					ofdtText));
	Fields.Add(
			TOgFieldDesc("favorited", oftBool, offtNominal, ofatNone,
					ofdtText));
	Fields.Add(TOgFieldDesc("source", oftStr, offtToken, ofatNone, ofdtText));
	Fields.Add(
			TOgFieldDesc("place_name", oftStr, offtToken, ofatNone, ofdtText));
	Fields.Add(
			TOgFieldDesc("place_country", oftStr, offtToken, ofatNone,
					ofdtText));
	Fields.Add(
			TOgFieldDesc("place_full_name", oftStr, offtToken, ofatNone,
					ofdtText));
	Fields.Add(
			TOgFieldDesc("in_reply_to_screen_name", oftStr, offtNominal,
					ofatNone, ofdtText));
	Fields.Add(
			TOgFieldDesc("in_reply_to_status_id", oftUInt64, offtNumeric,
					ofatNone, ofdtText));
	Fields.Add(
			TOgFieldDesc("user_id", oftUInt64, offtNumeric, ofatNone,
					ofdtText));
	Fields.Add(
			TOgFieldDesc("ds_id", oftUInt64, offtNumeric, ofatNone,
					ofdtText));
	//Fields.Add(TOgFieldDesc("coordinates", oftFltPr, offgtNumeric, ofatNone, ofdtMap));
	//Fields.Add(TOgFieldDesc("geo", oftFltPr, offgtNumeric, ofatNone, ofdtMap));
	//Fields.Add(TOgFieldDesc("place", oftFltPr, offgtNumeric, ofatNone, ofdtMap));

	const uchar TweetStore = 0;
	Miner->AddStore(
			new TSQLiteStore(TweetStore, Conn, "tweet", "tweet_id", "tweet_id",
					Fields));

	POgIndexVoc Voc;
	POgIndex Ix;
	TFAccess Access;
	if (!TOgIndex::Exists(FNm + ".index/")) {
		CreateDirectory(TStr(FNm + ".index").CStr(), NULL);
		Access = faCreate;
		Voc = TOgIndexVoc::New();
		const int IxId = Voc->AddKeyNm("tweet_text");
		Voc->AddTextKey(IxId);
		Voc->AddStore(TweetStore);
		Voc->AddStore(IxId, TweetStore);
	} else {
		Access = faRdOnly;
		Voc = TOgIndexVoc::Load(*TFIn::New(FNm + ".index/Voc.Bin"));
	}
	Ix = TOgIndex::New(FNm + ".index", Access, Voc, 128 * 1024 * 1024);

	Miner->PutIndex(Ix);
	Miner->Init();
	Miner->AddOp(TOgOpSearch::New());
}

TOgDsProvider::~TOgDsProvider() {
}

void TOgDsProvider::Init() {
	POgStore Tweets = Miner->GetStoreByStoreNm("tweet");
	TOgExcept::Check(!Tweets.Empty(), "No tweet table.");
	POgIndex Ix = Miner->GetIndex();
	TOgExcept::Check(!Ix.Empty(), "No index.");
	POgIndexVoc Voc = Ix->GetIndexVoc();
	TOgExcept::Check(!Voc.Empty(), "No vocabulary.");

	const int TextFieldId = Tweets->GetFieldId("tweet_text");
	const int DsIdFieldId = Tweets->GetFieldId("ds_id");
	const int IxId = Voc->GetKeyId("tweet_text");
	const int DsIxId = Voc->GetKeyId("ds_id");
	const uchar StoreId = Tweets->GetStoreId();

	POgStoreIter It = Tweets->GetIter();
	while (It->Next()) {
		const uint64 Rec = It->GetRecId();
		const TStr& Content = Tweets->GetFieldStr(Rec, TextFieldId);
		const int DsId = Tweets->GetFieldInt(Rec, DsIdFieldId);
		TStrV Tokens;
		Tok->GetTokens(Content, Tokens);
		for (int i = 0; i < Tokens.Len(); ++i) {
			//printf("%s\n", Tokens[i].CStr());
			Voc->AddWordStr(IxId, Tokens[i]);
		}
		Ix->Index(IxId, Tokens, StoreId, Rec);
		Ix->Index(DsIxId, DsId, StoreId, Rec, 1);
	}

	Miner->GetIndexVoc()->Save(*TFOut::New(Ix->GetIndexFPath() + "/Voc.Bin"));

	/*It = Tweets->GetIter();
	 while (It->Next()) {
	 uint64 Rec = It->GetRecId();
	 const TStr& Content = Tweets->GetFieldStr(Rec, TextFieldId);
	 TStrV Tokens;
	 Tok->GetTokens(Content, Tokens);

	 }*/
}

PBowDocBs TOgDsProvider::GetBowBs(const TStr& Query) {
	TVec < TPair<TUInt64, TStr> > IdStrV;
	Search("", Query, -1, IdStrV);

	BowLock.Enter();
	PBowDocBs BowBs = TBowDocBs::New(SwSet, Stemmer, NULL);
	for (int j = 0; j < IdStrV.Len(); ++j) {
		const uint64 TweetId = IdStrV[j].Val1;
		const TStr& Content = IdStrV[j].Val2;
		TStrV Tokens;
		Tok->GetTokens(Content, Tokens);
		BowBs->AddDoc(TUInt64::GetStr(TweetId), TStrV(), Tokens, TStr());
	}
	BowLock.Leave();
	return BowBs;
}

PBowDocBs TOgDsProvider::GetBowBsAll() {
	BowLock.Enter();
	if (All.Empty()) {
		const TStr BowAllFNm = DbNm + "all.bow";
		if (TFile::Exists(BowAllFNm)) {
			// load from file
			All = TBowDocBs::Load(*TFIn::New(BowAllFNm));
		} else {
			// construct from db
			const TStrV Blank;
			const POgStore& Store = Miner->GetStoreByStoreId(0);
			POgStoreIter Itr = Store->GetIter();
			All = TBowDocBs::New(SwSet, Stemmer, NULL);
			const int tweetTextFId = Store->GetFieldId("tweet_text");
			while (Itr->Next()) {
				const uint64 RecId = Itr->GetRecId();
				const TStr Content = Store->GetFieldStr(RecId, tweetTextFId);
				TStrV Tokens;
				Tok->GetTokens(Content.GetLc(), Tokens);
				All->AddDoc(TUInt64::GetStr(RecId), Blank, Tokens);
			}
			All->Save(*TFOut::New(BowAllFNm));
		}
		BowLock.Leave();
	}
	// return from memory
	BowLock.Leave();	
	return All;
}

void TOgDsProvider::Search(const TStr& DsNm, const TStr& Query, int n,
		TVec<TPair<TUInt64, TStr> >& IdStrV) {
	const POgOp& Search = Miner->GetOp("search");
	TOgRecSetV RecSetIn, RecSetOut;
	TStrKdV FldNmValPrV;
	FldNmValPrV.Add(TStrKd("storeid", "0"));
	const POgIndexVoc& Voc = Miner->GetIndexVoc();

	const int IxId = Voc->GetKeyId("tweet_text");
	FldNmValPrV.Add(TStrKd("q", TInt::GetStr(IxId) + "::" + Query));
	POgRecSet Rs;
	IndexLock.Enter();
	try {
		Rs = Search->Exec(Miner, RecSetIn, FldNmValPrV);
	} catch (PExcept& Ex) {
		printf("%s\n", Ex->GetStr().CStr());
	}
	IndexLock.Leave();

	if (!Rs.Empty()) {
		const POgStore& Store = Miner->GetStoreByStoreId(0);

		DbLock.Enter();
		const int tweetTextFId = Store->GetFieldId("tweet_text");
		const int tweetFId = Store->GetFieldId("tweet_id");
		for (int j = 0; j < Rs->GetRecs(); ++j) {
			const uint64 RecId = Rs->GetRecId(j);
			const uint64 TweetId = Store->GetFieldUInt64(RecId, tweetFId);
			const TStr Content = Store->GetFieldStr(RecId, tweetTextFId);
			//printf("%llu %llu %s\n", RecId, TweetId, Content.CStr());
			IdStrV.Add(TPair<TUInt64, TStr>(TweetId, Content));
			if (n != -1 && j == n) {
				break;
			}
		}
		DbLock.Leave();
	}
}

void TOgDsProvider::GetDocsForLabel(const TStr& Task, const TStr& Label,
		TUInt64V& DocIds) {
	const TStr Query =
			"SELECT tweet_id FROM label, task, task_class WHERE label.task_id = task.task_id AND task_class.task_id = task.task_id AND task_class.task_class_id = label.task_class_id AND task.task_name = ? AND task_class.task_class_name = ?";
	TSQLParameterV Params;
	Params.Add(new TSQLStrParameter(Task));
	Params.Add(new TSQLStrParameter(Label));
	PSQLCommand Cmd = Qc->GetQ(Query, Params);
	Cmd->ExecuteQuery();
	while (Cmd->ReadNext()) {
		DocIds.Add(Cmd->GetUInt64(0));
	}
}

void TOgDsProvider::GetDocsLabelsForTaskId(const int TaskId, TVec<TPair<TUInt64, TInt> >& DocIds) {
	const TStr Query =
			"SELECT tweet_id, task_class_id FROM label WHERE label.task_id = ? ";
	TSQLParameterV Params;
	Params.Add(new TSQLIntParameter(TaskId));
	PSQLCommand Cmd = Qc->GetQ(Query, Params);
	Cmd->ExecuteQuery();
	while (Cmd->ReadNext()) {
		TPair<TUInt64, TInt>& IdLabel = DocIds[DocIds.Add()];
		IdLabel.Val1 = Cmd->GetUInt64(0);
		IdLabel.Val2 = Cmd->GetInt(1);
	}
}


void TOgDsProvider::GetLabelList(const TStr& Task, TStrV& Labels) {
	const TStr Query =
			"SELECT DISTINCT(label) FROM label, task WHERE label.task_id = task.task_id AND task.task_name = ?";
	TSQLParameterV Params;
	Params.Add(new TSQLStrParameter(Task));
	PSQLCommand Cmd = Qc->GetQ(Query, Params);
	Cmd->ExecuteQuery();
	while (Cmd->ReadNext()) {
		Labels.Add(Cmd->GetText(0));
	}
}
void TOgDsProvider::SetLabelForDocs(const TStr& Task, const TStr& Label,
		const TUInt64V& DocIds) {
	const int TaskId = GetTaskId(Task);

	if (Label == "none") {
		const TStr Query =
				"DELETE FROM label WHERE tweet_id = ? AND task_id = ?";
		TSQLParameterV Params;
		Params.Add(new TSQLUInt64Parameter(0));
		Params.Add(new TSQLIntParameter(TaskId));
		PSQLCommand Cmd = Qc->GetQ(Query, Params);
		for (int i = 0; i < DocIds.Len(); i++) {
			Cmd->ResetCommand();
			Cmd->Bind(new TSQLUInt64Parameter(DocIds[i]), 1);
			Cmd->ExecuteNonQuery();
		}
	} else {
		const TStr LabelQ = "SELECT task_class_id FROM task_class WHERE task_class_name = ? AND task_id = ?";
		TSQLParameterV LParams;
		LParams.Add(new TSQLStrParameter(Label));
		LParams.Add(new TSQLIntParameter(TaskId));
		PSQLCommand LCmd = Qc->GetQ(LabelQ, LParams);
		LCmd->ExecuteQuery();
		int ClassLabelId = -1;
		if (LCmd->ReadNext()) {
			ClassLabelId = LCmd->GetInt(0);
		}

		if (ClassLabelId != -1) {
			const TStr Query =
					"INSERT OR REPLACE INTO label (task_class_id, tweet_id, task_id) VALUES (?,?,?)";
			TSQLParameterV Params;
			Params.Add(new TSQLIntParameter(ClassLabelId));
			Params.Add(new TSQLUInt64Parameter(0));
			Params.Add(new TSQLIntParameter(TaskId));
			PSQLCommand Cmd = Qc->GetQ(Query, Params);
			for (int i = 0; i < DocIds.Len(); i++) {
				Cmd->ResetCommand();
				Cmd->Bind(new TSQLUInt64Parameter(DocIds[i]), 1);
				Cmd->ExecuteNonQuery();
			}
		}
	}
}

bool TOgDsProvider::IsGetTaskId(const TStr& Task, int& TaskId) {
	TSQLParameterV Params;
	Params.Add(new TSQLStrParameter(Task));
	PSQLCommand Cmd = Qc->GetQ("SELECT task_id FROM task WHERE task.task_name = ?",
			Params);
	Cmd->ExecuteQuery();
	if (Cmd->ReadNext()) {
		TaskId = Cmd->GetInt(0);
		return true;
	} else {
		return false;
	}
}


const TStr TOgDsProvider::GetTaskNm(const int TaskId) {
	TSQLParameterV Params;
	Params.Add(new TSQLIntParameter(TaskId));
	PSQLCommand Cmd = Qc->GetQ("SELECT task_name FROM task WHERE task.task_id = ?",
			Params);
	Cmd->ExecuteQuery();
	TStr TaskNm;
	if (Cmd->ReadNext()) {
		TaskNm = Cmd->GetText(0);
	}
	return TaskNm;
}

int TOgDsProvider::GetTaskId(const TStr& Task) {
	
	int TaskId = -1;
	if (IsGetTaskId(Task, TaskId)) {
		return TaskId;
	} else {
		TSQLParameterV Params;
		Params.Add(new TSQLStrParameter(Task));
		Qc->GetQ("BEGIN TRANSACTION")->ExecuteNonQuery();
		Qc->GetQ("INSERT INTO task (task_name) VALUES (?)", Params)->ExecuteNonQuery();
		PSQLCommand CmdLastRowid = Qc->GetQ(
				"SELECT task_id FROM task WHERE task.task_name = ?", Params);
		CmdLastRowid->ExecuteQuery();
		if (CmdLastRowid->ReadNext()) {
			TaskId = CmdLastRowid->GetInt(0);
		}
		Qc->GetQ("COMMIT TRANSACTION")->ExecuteNonQuery();
	}
	
	return TaskId;
}

void TOgDsProvider::AddTaskClasses(const int TaskId, const TStrV& Classes) {
	TSQLParameterV Params;
	Params.Add(new TSQLIntParameter(TaskId));
	Params.Add(new TSQLStrParameter(""));
	
	try {
		Qc->GetQ("BEGIN TRANSACTION")->ExecuteNonQuery();
		PSQLCommand Cmd = Qc->GetQ(
				"INSERT INTO task_class (task_id, task_class_name) VALUES (?,?)",
				Params);
		for (int i = 0; i < Classes.Len(); ++i) {
			Cmd->ResetCommand();
			Cmd->Bind(new TSQLStrParameter(Classes[i]), 1);
			Cmd->ExecuteNonQuery();
		}
		Qc->GetQ("COMMIT TRANSACTION")->ExecuteNonQuery();
	} catch (PExcept& E) {
		printf("%s\n", E->GetStr().CStr());
	}
}

int TOgDsProvider::GetTaskIdForModel(const TStr& Model) {
	TSQLParameterV Params;
	Params.Add(new TSQLStrParameter(Model));
	PSQLCommand Cmd = Qc->GetQ("SELECT task_id FROM model WHERE model.model_name = ?",
			Params);
	Cmd->ExecuteQuery();
	int TaskId = -1;
	if (Cmd->ReadNext()) {
		TaskId = Cmd->GetInt(0);
	}
	return TaskId;
}
void TOgDsProvider::GetTaskClasses(const int TaskId, TIntStrPrV& ClassV) {
	const TStr Query =
			"SELECT task_class_id, task_class_name FROM task_class WHERE task_id = ? ORDER BY task_class_id ASC";
	TSQLParameterV Params;
	Params.Add(new TSQLIntParameter(TaskId));
	PSQLCommand Cmd = Qc->GetQ(Query, Params);
	Cmd->ExecuteQuery();
	while (Cmd->ReadNext()) {
		ClassV.Add(TIntStrPr(Cmd->GetInt(0), Cmd->GetText(1)));
	}
}

int TOgDsProvider::GetModelId(const TStr& Model) {
	TSQLParameterV Params;
	Params.Add(new TSQLStrParameter(Model));
	Qc->GetQ("BEGIN TRANSACTION")->ExecuteNonQuery();
	PSQLCommand Cmd = Qc->GetQ(
			"SELECT model_id FROM model WHERE model.model_name = ?", Params);
	Cmd->ExecuteQuery();
	int ModelId = -1;
	if (Cmd->ReadNext()) {
		ModelId = Cmd->GetInt(0);
	} else {
		Qc->GetQ("INSERT INTO model (model_name) VALUES (?)", Params)->ExecuteNonQuery();
		PSQLCommand CmdLastRowid = Qc->GetQ(
				"SELECT model_id FROM model WHERE model.model_name = ?",
				Params);
		CmdLastRowid->ExecuteQuery();
		if (CmdLastRowid->ReadNext()) {
			ModelId = Cmd->GetInt(0);
		}
	}
	Qc->GetQ("COMMIT TRANSACTION")->ExecuteNonQuery();
	return ModelId;
}

void TOgDsProvider::GetDocs(const TUInt64V& DocIds, TStrV& Contents) {
	const TStr Query = "SELECT tweet_text FROM tweet WHERE tweet_id = ?";
	TSQLParameterV Params;
	Params.Add(new TSQLUInt64Parameter(0));
	PSQLCommand Cmd = Qc->GetQ(Query, Params);
	for (int i = 0; i < DocIds.Len(); i++) {
		Cmd->ResetCommand();
		Cmd->Bind(new TSQLUInt64Parameter(DocIds[i]), 0);
		Cmd->ExecuteQuery();
		if (Cmd->ReadNext()) {
			Contents.Add(Cmd->GetText(0));
		} else {
			Contents.Add("");
		}
	}
}

void TOgDsProvider::GetTasks(TUInt64StrPrV& Tasks) {
	const TStr Query = "SELECT task_id, task_name FROM task";
	PSQLCommand Cmd = Qc->GetQ(Query);
	Cmd->ExecuteQuery();
	while (Cmd->ReadNext()) {
		const uint64 TaskId = Cmd->GetUInt64(0);
		const TStr TaskNm = Cmd->GetText(1);
		Tasks.Add(TUInt64StrPr(TaskId, TaskNm));
	}
}

void TOgDsProvider::GetModels(const TStr& Task, TUInt64StrPrV& Models) {
	const TStr Query =
			"SELECT model_id, model_name FROM model, task WHERE model.task_id = task.task_id AND task.task_name = ?";
	PSQLCommand Cmd = Qc->GetQ(Query,
			TSQLParameterV::GetV(new TSQLStrParameter(Task)));
	Cmd->ExecuteQuery();
	while (Cmd->ReadNext()) {
		const uint64 ModelId = Cmd->GetUInt64(0);
		const TStr ModelNm = Cmd->GetText(1);
		Models.Add(TUInt64StrPr(ModelId, ModelNm));
	}
}

void TOgDsProvider::DelModel(const TStr& ModelId) {
	Qc->GetQ("DELETE FROM model WHERE model_name = ?",
			TSQLParameterV::GetV(new TSQLStrParameter(ModelId)))->ExecuteNonQuery();
}
void TOgDsProvider::DelTask(const TStr& TaskId) {
	Qc->GetQ("DELETE FROM task WHERE task_name = ?",
			TSQLParameterV::GetV(new TSQLStrParameter(TaskId)))->ExecuteNonQuery();
}
void TOgDsProvider::DelLabel(int LabelId) {
	Qc->GetQ("DELETE FROM label WHERE label_id = ?",
			TSQLParameterV::GetV(new TSQLIntParameter(LabelId)))->ExecuteNonQuery();
}

void TOgDsProvider::SaveModel(const TStr& ModelNm, const TStr& ModelType,
		const TStr& TaskNm, const PMem& ModelSerialized) {
	int MId = GetModelId(ModelNm);
	int TaskId = GetTaskId(TaskNm);
	Qc->GetQ("BEGIN TRANSACTION")->ExecuteNonQuery();
	PSQLCommand Cmd =
			Qc->GetQ(
					"UPDATE model SET model_class = ?, model_blob = ?, task_id = ? WHERE model_id = ?");
	TSQLParameterV Params;
	Params.Add(new TSQLStrParameter(ModelType));
	Params.Add(new TSQLBlobParameter(ModelSerialized));
	Params.Add(new TSQLIntParameter(TaskId));
	Params.Add(new TSQLIntParameter(MId));
	Cmd->Bind(Params);
	Cmd->ExecuteNonQuery();
	Qc->GetQ("COMMIT TRANSACTION")->ExecuteNonQuery();
}

void TOgDsProvider::GetModel(const TStr& ModelNm, PMem& Mem, TStr& Type) {
	PSQLCommand Cmd =
			Qc->GetQ(
					"SELECT model.model_blob, model.model_class FROM model WHERE model_name = ?",
					TSQLParameterV::GetV(new TSQLStrParameter(ModelNm)));
	Cmd->ExecuteQuery();
	if (Cmd->ReadNext()) {
		Mem = Cmd->GetBlob(0);
		Type = Cmd->GetText(1);
	}
}