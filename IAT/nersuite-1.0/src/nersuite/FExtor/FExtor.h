//
//
// This class provides functionalies extracing features
//
//


#ifndef		_FEATURE_EXTRACTOR_
#define		_FEATURE_EXTRACTOR_


#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#include <cstdlib>

#include "typedefs.h"
#include "string_utils.h"


using namespace std;


class FEATURE_EXTRACTOR {
public:
	void ext_WORD_feats(const COLUMN_INFO &COL_INFO, const V2_STR &one_sent, V2_STR &sent_feats);
	void ext_LEMMA_feats(const COLUMN_INFO &COL_INFO, const V2_STR &one_sent, V2_STR &sent_feats);
	void ext_ORTHO_feats(const COLUMN_INFO &COL_INFO, const V2_STR &one_sent, V2_STR &sent_feats);
	void ext_POS_feats(const COLUMN_INFO &COL_INFO, const V2_STR &one_sent, V2_STR &sent_feats);
	void ext_LEMMA_POS_feats(const COLUMN_INFO &COL_INFO, const V2_STR &one_sent, V2_STR &sent_feats);
	void ext_CHUNK_feats(const COLUMN_INFO &COL_INFO, const V2_STR &one_sent, V2_STR &sent_feats);
	void ext_DIC_feats(const COLUMN_INFO &COL_INFO, const V2_STR &one_sent, V2_STR &sent_feats, int opt_dic);

	
	FEATURE_EXTRACTOR(void) {
		string g_alphabets[] = { "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta", "iota", "kappa", "lambda", 
								 "mu", "nu", "xi", "omicron", "pi", "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega" };
		greek_alphabets.insert(g_alphabets, g_alphabets + 24);
	}		

private:
	void get_n_grams(const string &token, const int n, vector<string> &ngrams);
	string get_item(const V2_STR &one_sent, const V2_STR_citr &i_row, const int col, const int rel_pos);
	void find_chunk_range(const V2_STR &one_sent, const V2_STR_citr &i_row, const COLUMN_INFO &COL_INFO, 
						  pair<V2_STR_citr, V2_STR_citr> &chk_range);
	
	set<string> greek_alphabets;
};



#endif



