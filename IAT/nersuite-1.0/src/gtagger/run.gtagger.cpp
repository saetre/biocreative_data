//
//
//
//

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <list>

#include <stdio.h>	
#include <stdlib.h>
#include <time.h>
#include "maxent.h"
#include "common.h"


using namespace std;


typedef		vector<string>		V1_STR;
typedef		vector< V1_STR >	V2_STR;


vector<ME_Model>		vme(16);		// genia pos models
vector<ME_Model>		vme_chunking(16);	// genia chunking models

// Genia 3.0.1 tagger modules
int	genia_init( const string &genia_dir );
string	bidir_postag( const string & s, const vector<ME_Model> & vme, const vector<ME_Model> & cvme, bool dont_tokenize );
void	bidir_chunking( vector<Sentence> & vs, const vector<ME_Model> & vme );
void	init_morphdic( const string &path );

int	get_sent( istream &is, V2_STR &one_sent );
string	assemble_tok_sent( const V2_STR &one_sent );
int	tokenize( V1_STR &one_seg, string &one_line, const string &del );
void	output_result( V2_STR &one_sent, ostream &os );


int main(int argc, char* argv[])
{
	string		genia_dir = "";
	bool		dont_tokenize = true;

	if( argc == 3 ) {
		string	opt = argv[1];
		genia_dir = argv[2];

		if( opt == "-m") {
			if( genia_dir[ genia_dir.length() - 1 ] != '/' )
				genia_dir += "/";
		}else {
			cerr << "Unrecognized option: " << opt << endl;
			exit(1);
		}
	}else {
		cerr << "Usage: " << argv[0] << " <-m  path/to/the/models>  <  an input file" << endl;
		cerr << "        1. <-m  path/to/the/models>" << endl;
		cerr << "              - The folder where GENIA tagger models are placed " << argv[0] << endl;
		cerr << "        2. an input file" << endl;
		cerr << "              - A file consists of a beginning position, a past-the-end position and a token columns." << endl;
		cerr << "              - Each column is tab-separated." << endl;

       	        exit(1);
	}

	// 1. Load POS-tagging, lemmatization and chunking models
	genia_init( genia_dir );

	// 2. Run POS-tagging, Lemmatization and Chunking with Genia tagger ver. 3.01
	int		n = 1;
	V2_STR		one_sent;

	while ( get_sent( cin, one_sent ) != 0 ) {
		string		tok_sent = assemble_tok_sent( one_sent );

		if( tok_sent.size() > 1024 )
	  		cerr << "warning: the sentence seems to be too long at line " << n;

		// 2.1. Run tagging
		string		tagged = bidir_postag( tok_sent, vme, vme_chunking, dont_tokenize );

		// 2.2. Split the result and push them into the data container
		V2_STR::iterator	i_row_sent = one_sent.begin();

		V1_STR		line_items;
		tokenize( line_items, tagged, "\n" );
		for( V1_STR::iterator i_row_tagged = line_items.begin(); i_row_tagged != line_items.end(); ++i_row_tagged) {
			V1_STR		token_items;
			tokenize( token_items, *i_row_tagged, "\t" );
			
			for( V1_STR::iterator i_col_tagged = token_items.begin() + 1; i_col_tagged != token_items.end(); ++i_col_tagged )
				i_row_sent->push_back( *i_col_tagged );
	
			++i_row_sent;
		}

		// 2.3. Output the result
		output_result( one_sent, cout );
		++n;
	}

	return 0;

}


// Load models of the GENIA tagger 3.0.1
int genia_init( const string &genia_dir )
{
	init_morphdic( genia_dir );
	
	cerr << "Loading pos_models";
	for ( int i = 0; i < 16; i++ ) {
		char		buf[8];
		string		fn_model = genia_dir + "pos.model.bidir.";
		sprintf(buf, "%d", i);
		fn_model += buf;
		vme[i].load_from_file( fn_model.c_str() );
		cerr << ".";
	}
	cerr << "done." << endl;

	cerr << "Loading chunk_models";
	
	for ( int i = 0; i < 8; i +=2 ) {
		char		buf[8];
		string		fn_model = genia_dir + "chunk.model.bidir.";
		sprintf( buf, "%d", i );
		fn_model += buf;
		vme_chunking[i].load_from_file(fn_model.c_str());
		cerr << ".";
	}
	cerr << "done." << endl;

	return 0;
}


// Get a sentence into a 2D string vector container
int get_sent( istream &is, V2_STR &one_sent )
{
	int	n_lines = 0;
	string	line = "";
	V1_STR	one_row;

	one_sent.clear();

	while( getline( is, line ) ) {
		if( line.empty() )		// Stop when a blank line (the end of a sentence) appears
			break;

		tokenize( one_row, line, "\t" );
		one_sent.push_back( one_row ); 

		++n_lines;
	}

	return n_lines;
}


// Assemble a tokens into a string
string assemble_tok_sent( const V2_STR &one_sent )
{
	string	tok_sent = "";

	for( V2_STR::const_iterator i_row = one_sent.begin(); i_row != one_sent.end(); ++i_row ) {
		tok_sent += i_row->back();
		if( (i_row + 1) != one_sent.end() )
			tok_sent += " ";
	}

	return tok_sent;
}


// Tokenize a string with given delimiters
int tokenize( V1_STR &one_seg, string &one_line, const string &del )
{
	int	total_elem = 0;
	size_t	beg = 0, end = one_line.find(del, 0);

	one_seg.clear();

	while( beg < one_line.length() )
	{
		if( (end = one_line.find(del, beg)) == string::npos )
			end = one_line.length();

		one_seg.push_back( one_line.substr(beg, end - beg) );

		beg = end + 1;
		++total_elem;
	}

	return total_elem;
}


// Print results
void output_result( V2_STR &one_sent, ostream &os )
{
	for( V2_STR::iterator i_row = one_sent.begin(); i_row != one_sent.end(); ++i_row ) {
		for( V1_STR::iterator i_col = i_row->begin(); i_col != i_row->end(); ++i_col ) {
			os << *i_col;
			if( (i_col + 1) != i_row->end() )
				os << "\t";
		}
		os << endl;
	}
	os <<  endl;
}



