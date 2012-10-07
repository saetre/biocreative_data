#ifndef		_NERSUITE_MAIN_HEADER_
#define		_NERSUITE_MAIN_HEADER_
	

#include <iostream>
#include <map>
#include <list>
#include <stdio.h>
#include <string.h>

#include "optparse.nersuite.h"

// BEGIN: utils
#include "string_utils.h"
#include "text_loader.h"
// END: utils

// BEGIN: feature extractor
#include "typedefs.h"
#include "FExtor.h"
// END: feature extractor

// BEGIN: CRFSuite wrapping
#include <stdlib.h>
#include <string>
#include <time.h>

typedef         double                          floatval_t;

#include "crfsuite.h"

#define    SAFE_RELEASE(obj)    if ((obj) != NULL) { (obj)->release(obj); (obj) = NULL; }

typedef struct {
    FILE *fpo;
    crf_data_t* data;
    crf_evaluation_t* eval;
    crf_dictionary_t* attrs;
    crf_dictionary_t* labels;
} callback_data_t;

typedef struct {
    char *model;
    char *training;
    char *evaluation;

    int help;

    int num_params;
    char **params;
} learn_option_t;
// END:  for CRFSuite 0.10


using namespace std;

typedef         vector<string>          V1_STR;
typedef         vector< V1_STR >        V2_STR;
typedef         vector< V2_STR >        V3_STR;


// Main program functions
void set_column_info( const string &mode, COLUMN_INFO &COL_INFO );
int pad_answer( const string &mode, const V2_STR &one_sent, V2_STR &sent_feats );
void print_usage( char exe_name[] );

// CRFSuite functions
int learn_crfsuite( istream &is, const COLUMN_INFO &COL_INFO, const nersuite_optparse &ner_opt );
        static int message_callback( void *instance, const char *format, va_list args );
        static int evaluate_callback( void *instance, crf_tagger_t* tagger );
        void read_data( istream &is, const COLUMN_INFO &COL_INFO, crf_data_t* data, crf_dictionary_t* attrs, crf_dictionary_t* labels );

int tag_crfsuite( V2_STR &one_sent, V2_STR &sent_feat, crf_model_t *model, map<string, int> &term_idx, const COLUMN_INFO &COL_INFO, const nersuite_optparse &ner_opt );
        void output_result_standoff( FILE *fpo, crf_output_t *output, crf_dictionary_t *labels, vector<vector<string> > &one_sent, map<string, int> &term_idx, const COLUMN_INFO &COL_INFO );
        void output_result_conll( FILE *fpo, crf_output_t *output, crf_dictionary_t *labels, vector<vector<string> > &one_sent, const COLUMN_INFO &COL_INFO );

static void learn_option_init( learn_option_t* opt );
static char* mystrdup( const char *src );
static void learn_option_finish( learn_option_t* opt );


#endif













