/*
*
* NER suite V. 1.0
*  
*/


#include "nersuite.h"
#include "optparse.nersuite.h"


using namespace std;


int main(int argc, char* argv[])
{
	if( argc < 4 ) {
		print_usage( argv[0] );
		return 1;
	}
	
	// 1. Get the directory to the nersuite and the running mode
	string		ner_dir = argv[0], mode = argv[1];
	ner_dir = ner_dir.substr( 0, ner_dir.find_last_of("/") );

	// 2. Parse option arguments
	nersuite_optparse	ner_opt;
	try {
		ner_opt.parse( &argv[2], argc - 2 );
	} catch( const optparse::unrecognized_option& e ) {
                cout << "unrecognized option: " << e.what() << endl;
		print_usage( argv[0] );
                return 1;
        } catch( const optparse::invalid_value& e ) {
                cout << "invalid value: " << e.what() << endl;
		print_usage( argv[0] );
                return 1;
        }

	V2_STR				one_sent;
	COLUMN_INFO			COL_INFO;
	set_column_info( mode, COL_INFO );

	
	if( mode == "learn" ) {
		learn_crfsuite( cin, COL_INFO, ner_opt );

	}else if( mode == "tag" ) {						// Tag each sentence
		FEATURE_EXTRACTOR	FExtor;
		crf_model_t		*crf_model = NULL;	

		if( crf_create_instance_from_file( ner_opt.fn_model.c_str(), (void**)&crf_model) ) {
			cerr << "Can not create a model instance from " << ner_opt.fn_model << endl;
			return 1;
		}

		map<string, int>	term_idx;
		V2_STR		sent_feats;
		
		while ( get_sent( cin, one_sent ) != 0 ) {
			sent_feats.clear();
			
			// 1) Feature extraction
			pad_answer(mode, one_sent, sent_feats);						// CRFsuite needs that first columns are answer or dummy tags (in both training and test)

			FExtor.ext_WORD_feats( COL_INFO, one_sent, sent_feats );
			FExtor.ext_LEMMA_feats( COL_INFO, one_sent, sent_feats );
			FExtor.ext_ORTHO_feats( COL_INFO, one_sent, sent_feats );
			FExtor.ext_POS_feats( COL_INFO, one_sent, sent_feats );
			FExtor.ext_LEMMA_POS_feats( COL_INFO, one_sent, sent_feats );
			FExtor.ext_CHUNK_feats( COL_INFO, one_sent, sent_feats );
	       
			FExtor.ext_DIC_feats( COL_INFO, one_sent, sent_feats, 0 );			// Use dictionaries as a default option (the last argument turn on lexicalized dictionary features)

			// 2) NER tagging
			tag_crfsuite( one_sent, sent_feats, crf_model, term_idx, COL_INFO, ner_opt );
		}

		SAFE_RELEASE(crf_model);

	}else {
		cerr << " The first argument is either \"learn\" or \"tag\"" << endl;
		exit(1);
	}


	return 0;
}


/**
* CRF training functions
*/
int learn_crfsuite( istream &is, const COLUMN_INFO &COL_INFO, const nersuite_optparse &ner_opt )
{
	int			ret = 0; //, arg_used = 0;
	char			timestamp[80];
	time_t			ts;
	clock_t			clk_begin, clk_current;
	FILE			*fpo = stdout, *fpe = stderr;
	learn_option_t		opt;
	callback_data_t		cd;
	crf_data_t		data_train, data_test;
	crf_evaluation_t	eval;
	crf_trainer_t		*trainer = NULL;
	crf_dictionary_t	*attrs = NULL, *labels = NULL;

	// Initialize the default sigma value
	char			name[] = "regularization.sigma", value[128];
	sprintf(value, "%f", ner_opt.sigma);


	/* Initializations. */
	learn_option_init(&opt);
	crf_data_init(&data_train);
	crf_data_init(&data_test);
	crf_evaluation_init(&eval, 0);

	/* Set a training file. */
	//opt.training = mystrdup(tr_filename.c_str());

	/* Create dictionaries for attributes and labels. */
	ret = crf_create_instance("dictionary", (void**)&attrs);
	if (!ret) {
		fprintf(fpe, "ERROR: Failed to create a dictionary instance.\n");
		ret = 1;
		goto learn_crf_force_exit;
	}
	ret = crf_create_instance("dictionary", (void**)&labels);
	if (!ret) {
		fprintf(fpe, "ERROR: Failed to create a dictionary instance.\n");
		ret = 1;
		goto learn_crf_force_exit;
	}

	/* Create a trainer instance. */
	ret = crf_create_instance("trainer.crf1m", (void**)&trainer);
	if (!ret) {
		fprintf(fpe, "ERROR: Failed to create a trainer instance.\n");
		ret = 1;
		goto learn_crf_force_exit;
	}

	/* Set the default sigma value. */
	{	// for g++ 4.3.0 
		crf_params_t* params = trainer->params(trainer);
		params->set(params, name, value);
		params->release(params);
	}

	/* Log the start time. */
	time(&ts);
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&ts));
	fprintf(fpo, "Start time of the training: %s\n", timestamp);
	fprintf(fpo, "\n");

	/* Read the training data. */
	fprintf(fpo, "Reading the training data\n");
	clk_begin = clock();
	read_data( is , COL_INFO, &data_train, attrs, labels );
	clk_current = clock();

	/* Report the statistics of the training data. */
	fprintf(fpo, "Number of instances: %d\n", data_train.num_instances);
	fprintf(fpo, "Total number of items: %d\n", crf_data_totalitems(&data_train));
	fprintf(fpo, "Number of attributes: %d\n", labels->num(attrs));
	fprintf(fpo, "Number of labels: %d\n", labels->num(labels));
	fprintf(fpo, "Seconds required: %.3f\n", (clk_current - clk_begin) / (double)CLOCKS_PER_SEC);
	fprintf(fpo, "\n");
	fflush(fpo);

	/* Fill the supplementary information for the data. */
	data_train.num_labels = labels->num(labels);
	data_train.num_attrs = attrs->num(attrs);
	data_train.max_item_length = crf_data_maxlength(&data_train);

	/* Initialize an evaluation object. */
	crf_evaluation_finish(&eval);
	crf_evaluation_init(&eval, labels->num(labels));

	/* Fill the callback data. */
	cd.fpo = fpo;
	cd.eval = &eval;
	cd.attrs = attrs;
	cd.labels = labels;
	cd.data = &data_test;

	/* Set callback procedures that receive messages and taggers. */
	trainer->set_message_callback(trainer, &cd, message_callback);
	trainer->set_evaluate_callback(trainer, &cd, evaluate_callback);

	/* Start training. */
	if ((ret = (trainer->train(
		trainer,
		data_train.instances,
		data_train.num_instances,
		labels->num(labels),
		attrs->num(attrs)
		)))) {
		goto learn_crf_force_exit;
	}

	/* Write out the obtained model. */
	opt.model = mystrdup( ner_opt.fn_model.c_str() );
	if (opt.model != NULL) {
		if ((ret = trainer->save(trainer, opt.model, attrs, labels))) {
			goto learn_crf_force_exit;
		}
	}

	/* Log the end time. */
	time(&ts);
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&ts));
	fprintf(fpo, "End time of the training: %s\n", timestamp);
	fprintf(fpo, "\n");

learn_crf_force_exit:
	SAFE_RELEASE(trainer);
	SAFE_RELEASE(labels);
	SAFE_RELEASE(attrs);

	crf_data_finish(&data_test);
	crf_data_finish(&data_train);
	crf_evaluation_finish(&eval);
	learn_option_finish(&opt);

	return ret;
}

void read_data( istream &is, const COLUMN_INFO &COL_INFO, crf_data_t* data, crf_dictionary_t* attrs, crf_dictionary_t* labels )
{
	int			lid = -1;
	crf_sequence_t		inst;
	crf_item_t		item;
	crf_content_t		cont;

	int			k_sents = 0;
	string			_attr = "", _value = "";
	size_t			_pos = string::npos;
	V2_STR			one_sent, sent_feats;

	FEATURE_EXTRACTOR	FExtor;
	
    
	/* Initialize the instance.*/
	crf_sequence_init(&inst);

	cerr << "Start feature extraction";
	while( get_sent( is, one_sent ) != 0 ) {
		sent_feats.clear();
		
		// 1) Feature extraction
		pad_answer( string("learn"), one_sent, sent_feats );				// CRFsuite needs that first columns are answer or dummy tags (in both training and test)

		FExtor.ext_WORD_feats( COL_INFO, one_sent, sent_feats );
		FExtor.ext_LEMMA_feats( COL_INFO, one_sent, sent_feats );
		FExtor.ext_ORTHO_feats( COL_INFO, one_sent, sent_feats );
		FExtor.ext_POS_feats( COL_INFO, one_sent, sent_feats );
		FExtor.ext_LEMMA_POS_feats( COL_INFO, one_sent, sent_feats );
		FExtor.ext_CHUNK_feats( COL_INFO, one_sent, sent_feats );
       
		FExtor.ext_DIC_feats( COL_INFO, one_sent, sent_feats, 0 );			// Use dictionaries as a default option (the last argument turn on lexicalized dictionary features)

  		for( V2_STR::const_iterator i_row = sent_feats.begin(); i_row != sent_feats.end(); ++i_row ) {
			// 1) Initialize an item (IWA_BOI)
			crf_item_init( &item );

			// 2) Put a label first (IWA_ITEM #1)
			lid = labels->get( labels, (i_row->front()).c_str() );

			// 3) Attribute fields (IWA_ITEM #2)
			for( V1_STR::const_iterator i_attr = i_row->begin() + 1; i_attr != i_row->end(); ++i_attr ) {
				crf_content_init( &cont );	
				
				// Divide an attribute name and its value
				if( (_pos = i_attr->find_first_of(":")) == string::npos ) {
					_attr = *i_attr;
				}else {
					_attr = i_attr->substr( 0, _pos );
					_value = i_attr->substr( _pos + 1, i_attr->length() - _pos - 1 );
				}

				cont.aid = attrs->get( attrs, _attr.c_str() );
				if ( _pos != string::npos ) {
					cont.scale = atof( _value.c_str() );
				}else {
					cont.scale = 1.0;
				}

				crf_item_append_content( &item, &cont );
			}

			// 4) Append the item to a training instance (IWA_EOI)
			crf_sequence_append( &inst, &item, lid );
			crf_item_finish( &item );
		}

		crf_data_append( data, &inst );
		crf_sequence_finish( &inst );
		
		++k_sents;
		if( (k_sents % 1000) == 0 )
			cerr << ".";
	}
	cerr << endl;
}

static int message_callback(void *instance, const char *format, va_list args)
{
    callback_data_t* cd = (callback_data_t*)instance;
    FILE *fpo = cd->fpo;
    vfprintf(fpo, format, args);
    fflush(fpo);
    return 0;
}

static int evaluate_callback(void *instance, crf_tagger_t* tagger)
{
    int i, ret = 0;
    crf_output_t output;
    callback_data_t* cd = (callback_data_t*)instance;
    FILE *fpo = cd->fpo;
    crf_data_t* data = cd->data;
    crf_dictionary_t* labels = cd->labels;

    /* Do nothing if no test data was given. */
    if (data->num_instances == 0) {
        return 0;
    }

    /* Clear the evaluation table. */
    crf_evaluation_clear(cd->eval);

    /* Tag the evaluation instances and accumulate the performance. */
    for (i = 0;i < data->num_instances;++i) {
        /* An instance to be tagged. */
        crf_sequence_t* instance = &data->instances[i];

        crf_output_init(&output);

        /* Tag an instance (ignoring any error occurrence). */
        ret = tagger->tag(tagger, instance, &output);

        /* Accumulate the tagging performance. */
        crf_evaluation_accmulate(cd->eval, instance, &output);
    }

    /* Compute the performance. */
    crf_evaluation_compute(cd->eval);

    /* Report the performance. */
    crf_evaluation_output(cd->eval, labels, fpo);

    return 0;
}


/**
* CRF tagging functions
*/
int tag_crfsuite(V2_STR &one_sent, V2_STR &sent_feat, crf_model_t *model, map<string, int> &term_idx, const COLUMN_INFO &COL_INFO, const nersuite_optparse &ner_opt)
{

    int				N = 0, L = 0, ret = 0, lid = -1;
    clock_t			clk0, clk1;
    crf_sequence_t		inst;
    crf_item_t			item;
    crf_content_t		cont;
    crf_output_t		output;
    crf_evaluation_t	eval;
    // _iwa_token_t		*token = (_iwa_token_t*)malloc(sizeof(_iwa_token_t));
    crf_tagger_t		*tagger = NULL;
    crf_dictionary_t	*attrs = NULL, *labels = NULL;
 
    /* Obtain the dictionary interface representing the labels in the model. */
    if ((ret = model->get_labels(model, &labels))) {
        goto tag_crf_force_exit;
    }

    /* Obtain the dictionary interface representing the attributes in the model. */
    if ((ret = model->get_attrs(model, &attrs))) {
        goto tag_crf_force_exit;
    }

    /* Obtain the tagger interface. */
    if ((ret = model->get_tagger(model, &tagger))) {
        goto tag_crf_force_exit;
    }

    /* Initialize the objects for instance and evaluation. */
    L = labels->num(labels);
    crf_sequence_init(&inst);
    crf_evaluation_init(&eval, L);

	/* Read the input data and assign labels. */
	clk0 = clock();

	// Initialize the item variable
	crf_item_init(&item);

   	for(V2_STR::iterator i = sent_feat.begin(); i != sent_feat.end(); ++i) {
		// Label part (first column)
		lid = labels->to_id(labels, (*(i->begin())).c_str() );
		if(lid < 0)
			lid = L;

		// Attribute part (second ~ last-1 column)
		for(vector<string>::iterator j = (i->begin() + 1); j != i->end(); ++j) {
			size_t pos = j->find_first_of(":");
			string _attr, _value;
			
			if(pos == string::npos) {
				_attr = *j;
			}else {
				_attr = j->substr(0, pos);
				_value = j->substr(pos + 1, j->length() - pos - 1);
			}
			
			/* Fields after the first field present attributes. */
			int aid = attrs->to_id(attrs, _attr.c_str());

			/* Ignore attributes 'unknown' to the model. */
		    if (0 <= aid) {
                /* Associate the attribute with the current item. */
				if (pos != string::npos) {
                    crf_content_set(&cont, aid, atof(_value.c_str()));
                } else{ 
                    crf_content_set(&cont, aid, 1.0);
                }
                crf_item_append_content(&item, &cont);
            }
		}

		// End the item variable (last column)
		crf_sequence_append(&inst, &item, lid);
		crf_item_finish(&item);
	}


	if (!crf_sequence_empty(&inst)) {
		/* Initialize the object to receive the tagging result. */
		crf_output_init(&output);

		/* Tag the instance. */
		if ((ret = tagger->tag(tagger, &inst, &output))) {
			goto tag_crf_force_exit;
		}
		++N;

		if (ner_opt.is_standoff == true) {
			output_result_standoff(stdout, &output, labels, one_sent, term_idx, COL_INFO);
		}else {
			output_result_conll(stdout, &output, labels, one_sent, COL_INFO);
		}

		crf_output_finish(&output);
		crf_sequence_finish(&inst);
	}
	
	clk1 = clock();

tag_crf_force_exit:
    crf_sequence_finish(&inst);
    crf_evaluation_finish(&eval);

    SAFE_RELEASE(tagger);
    SAFE_RELEASE(attrs);
    SAFE_RELEASE(labels);

	return ret;
}

void output_result_standoff(
	FILE						*fpo, 
	crf_output_t				*output, 
	crf_dictionary_t			*labels, 
	vector<vector<string> >		&one_sent, 
	map<string, int>			&term_idx,
	const COLUMN_INFO			&COL_INFO	
	)
{
    int								i, cnt;
	string							ne_term = "", ne_class = "", beg = "", end = "";
	map<string, int>::iterator		check;
		
    for (i = 0;i < output->num_labels;++i) {
        const char *label = NULL;
        labels->to_string(labels, output->labels[i], &label);
		
		// print begin position, end position, word and NE information
		string s_label = label;
		if (s_label == "O") {
			if (ne_term != "") {
				if ((check = term_idx.find(ne_class)) == term_idx.end()) {
					term_idx.insert( pair<string,int>( ne_class, 1 ) );
					cnt = 1;
				}else {
					cnt = ++(check->second);
				}

				//fprintf(fpo, "T%d\t%s %s %s\t%s\n", cnt, ne_class.c_str(), beg.c_str(), end.c_str(), ne_term.c_str());
				fprintf(fpo, "%s\t%s\tentity_name\tid=\"entity-%d\" type=\"%s\"\n", beg.c_str(), end.c_str(), cnt, ne_class.c_str());
				ne_term = "";
			}
		}else if (s_label.substr(0, 1) == "B") {
			if (ne_term != "") {
				if ((check = term_idx.find(ne_class)) == term_idx.end()) {
					term_idx.insert( pair<string,int>( ne_class, 1 ) );
					cnt = 1;
				}else {
					cnt = ++(check->second);
				}

				//fprintf(fpo, "T%d\t%s %s %s\t%s\n", cnt, ne_class.c_str(), beg.c_str(), end.c_str(), ne_term.c_str());				
				fprintf(fpo, "%s\t%s\tentity_name\tid=\"entity-%d\" type=\"%s\"\n", beg.c_str(), end.c_str(), cnt, ne_class.c_str());
			}

			ne_term = one_sent[i][COL_INFO.WORD];
			ne_class = s_label.substr(2, s_label.length() - 2);
			beg = one_sent[i][COL_INFO.BEG].c_str();
			end = one_sent[i][COL_INFO.END].c_str();
		}else if (s_label.substr(0, 1) == "I") {
			if (ne_term != "") {
				if ((i != 0) && (one_sent[i - 1][1] == one_sent[i][0])) {
					ne_term = ne_term + one_sent[i][COL_INFO.WORD];
				}else if (i != 0) {
					ne_term = ne_term + " " + one_sent[i][COL_INFO.WORD];
				}

				end = one_sent[i][COL_INFO.END].c_str();
			}else {															// ***Exception) NE begins with I- label.
				ne_term = one_sent[i][COL_INFO.WORD];
				ne_class = s_label.substr(2, s_label.length() - 2);
				beg = one_sent[i][COL_INFO.BEG].c_str();
				end = one_sent[i][COL_INFO.END].c_str();
			}
		}else {
			cerr << "Can not reach here: BIO-" << endl;
			exit(1);
		}

		labels->free(labels, label);		
	}
	if (ne_term != "") {			// If the last token is "B" or "I"
		if ((check = term_idx.find(ne_class)) == term_idx.end()) {
			term_idx.insert( pair<string,int>( ne_class, 1 ) );
			cnt = 1;
		}else {
			cnt = ++(check->second);
		}

		//fprintf(fpo, "T%d\t%s %s %s\t%s\n", cnt, ne_class.c_str(), beg.c_str(), end.c_str(), ne_term.c_str());
		fprintf(fpo, "%s\t%s\tentity_name\tid=\"entity-%d\" type=\"%s\"\n", beg.c_str(), end.c_str(), cnt, ne_class.c_str());
		
		ne_term = "";
	}

    //fprintf(fpo, "\n");
}

void output_result_conll(
	FILE				*fpo, 
	crf_output_t			*output, 
	crf_dictionary_t		*labels, 
	vector<vector<string> >		&one_sent, 
	const COLUMN_INFO		&COL_INFO	
	) 
{
	for (int i = 0;i < output->num_labels;++i) {
		const char *label = NULL;
		labels->to_string(labels, output->labels[i], &label);

		fprintf(fpo, "%s\t%s\t%s\t%s\t%s\t%s\t%s\n", one_sent[i][COL_INFO.BEG].c_str(), one_sent[i][COL_INFO.END].c_str(), one_sent[i][COL_INFO.WORD].c_str(),
			one_sent[i][COL_INFO.LEMMA].c_str(), one_sent[i][COL_INFO.POS].c_str(), one_sent[i][COL_INFO.CHUNK].c_str(), label);
	}
	fprintf(fpo, "\n");
}


/**
* ETC functions
*/
static void learn_option_init(learn_option_t* opt)
{
    memset(opt, 0, sizeof(*opt));
    opt->num_params = 0;
    opt->model = mystrdup("crfsuite.model");
}

static char* mystrdup(const char *src)
{
    char *dst = (char*)malloc(strlen(src)+1);
    if (dst != NULL) {
        strcpy(dst, src);
    }
    return dst;
}

static void learn_option_finish(learn_option_t* opt)
{
    int i;

    free(opt->model);
    free(opt->training);
    free(opt->evaluation);

    for (i = 0;i < opt->num_params;++i) {
        free(opt->params[i]);
    }
    free(opt->params);
}

void set_column_info(const string &mode, COLUMN_INFO &COL_INFO)
{
	int		begin = 0;

	if(mode == "learn")	
		begin = 1;

	COL_INFO.BEG	= begin;
	COL_INFO.END	= begin + 1;
	COL_INFO.WORD	= begin + 2;
	COL_INFO.LEMMA	= begin + 3;
	COL_INFO.POS	= begin + 4;
	COL_INFO.CHUNK	= begin + 5;
	COL_INFO.DIC	= begin + 6;	// NE result of GENIA tagger is removed (if it isn't, DIC needs to be + 6)
}

int pad_answer( const string &mode, const V2_STR &one_sent, V2_STR &sent_feats )
{
        vector<string>                  answer_tag(1, "DUMMY");

        if( mode == "tag" ) {            // no answer tag
                for( V2_STR_citr i_row = one_sent.begin(); i_row != one_sent.end(); ++i_row ) {
                        sent_feats.push_back( answer_tag );
                }
        }else if( mode == "learn" ) {
                for( V2_STR_citr i_row = one_sent.begin(); i_row != one_sent.end(); ++i_row ) {
                        answer_tag[ 0 ] = (*i_row)[ 0 ];
                        sent_feats.push_back( answer_tag );
                }
        }else {
                cerr << "invalid mode option! " << endl;
                exit(1);
        }

        return 0;
}

void print_usage( char exe_name[] ) {
	cerr << "Usage: " << exe_name << " <mode> <-m model_filename> [-s sigma_value] [-f output_format]  <  input_file" << endl;
	cerr << "			1. mode " << endl;
	cerr << "				- 'learn' or 'tag' " << endl;
	cerr << "			2. model_filename " << endl;
	cerr << "				- A model file name for storing a trained model in 'train' mode, or for loading a model in 'tag' mode" << endl;
	cerr << "			3. sigma_value  (for learn mode) " << endl;
	cerr << "				- The sigma value of L2-regularization term " << endl;
	cerr << "				- Default value is 1.0 " << endl;
	cerr << "			4. output_format  (for tag mode) " << endl;
	cerr << "				- 'conll' or 'standoff' format " << endl;
	cerr << "				- Default value is 'conll' format " << endl;
	cerr << "			5. input_file " << endl;
	cerr << "				- An input file consists of columns as follows." << endl;
	cerr << "				[1st col.] - the byte position of the first letter of a token. " << endl;
	cerr << "				[2nd col.] - the byte position one past the last letter of a token. " << endl;
	cerr << "				[3rd col.] - raw token (word)" << endl;
	cerr << "				[4th col.] - lemma" << endl;
	cerr << "				[5th col.] - POS tag" << endl;
	cerr << "				[6th col.] - chunk tag" << endl;
	cerr << "				[7th ... ] - any attributes" << endl;
	cerr << "					P.s. With learn option, 1st column is a correct named entity label for each line and other columns will be placed one column after then the original position" << endl;
}	
