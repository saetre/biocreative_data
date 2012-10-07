//
//
// Typedefs for Feature Extractor 
//
//


#ifndef		_TYPEDEFS_FEATURE_EXTRACTOR_
#define		_TYPEDEFS_FEATURE_EXTRACTOR_

#include <string>
#include <vector>


using namespace std;


typedef		vector< vector<string> >		V2_STR;
typedef		V2_STR::const_iterator			V2_STR_citr;


struct COLUMN_INFO {
	int BEG;
	int END;
	int WORD;
	int LEMMA;
	int POS;
	int CHUNK;
	int DIC;				// The first column of dictionary(ies)
};


#endif
