//
// A sentence tokenizer class with a user-defined 
// delimiters
//
//

#include "tokenizer.h"

using namespace std;


int TOKENIZER::tokenize( const string &raw_sent, V2_STR &data, const size_t init_offset ) 
{
	string	trimmed_sent = trim_ws( raw_sent );		

	if( trimmed_sent == "") {
		return 0;
	}else {
		int	n_tokens = spliter( trimmed_sent, data );
		mark_pos( raw_sent, data, init_offset );

		return	n_tokens;
	}
}


/*
*
* Private functions
*
*/

int TOKENIZER::spliter( const string &trimmed_sent, V2_STR &data )
{
	size_t	beg = 0, end = find_token_end( trimmed_sent, 0 ), n_tokens = 0;

	V1_STR	one_row;
	one_row.push_back( "token" );		

	while( end != string::npos ) {
		string	token = trimmed_sent.substr( beg, end - beg );

		if( token != " " ) {				// Put all tokens except a space token into the 'data' container
			one_row.back() = token;
			data.push_back( one_row );
			++n_tokens;
		}

		beg = end;
		end = find_token_end( trimmed_sent, beg );
	}

	return n_tokens;
}

void TOKENIZER::mark_pos( const string &raw_sent, V2_STR &data, const size_t init_offset ) 
{
	size_t	beg = 0, end = 0;
	char	chr_pos[128];

	for( V2_STR::iterator i_row = data.begin(); i_row != data.end(); ++i_row ) {
		beg = init_offset + raw_sent.find_first_of( *(i_row->end() - 1), end);
		end = beg + (i_row->end() - 1)->length();
		
		sprintf(chr_pos, "%d", beg);
		i_row->insert( i_row->end() - 1, chr_pos );

		sprintf(chr_pos, "%d", end);
		i_row->insert( i_row->end() - 1, chr_pos );
	}	
}


// Trim left and right hand side spaces, and squeeze all continuous spaces into a single space within a sentence
string TOKENIZER::trim_ws( const string &raw_sent )
{
	string	trimmed_sent = "";
	size_t	beg = 0, end = raw_sent.length() - 1;

	while( raw_sent[ beg ] == ' ' )			// 1. Trim LHS spaces
		++beg;
	
	while( raw_sent[ end ] == ' ' )			// 2. Trim RHS spaces
		--end;

	trimmed_sent += raw_sent[ beg ];		// 3. Squeeze continuous spaces inside a sentence into a single space
	for( int i = beg; i < end; ++i )
		if( (raw_sent[ i ] != ' ') || (raw_sent[ i + 1 ] != ' ') )
			trimmed_sent += raw_sent[ i + 1 ];
	
	return trimmed_sent;
}

// Find the past the end position for a next token
size_t TOKENIZER::find_token_end( const string &trimmed_sent, const size_t beg )
{
	size_t	max = trimmed_sent.length();

	if( beg == max ) {										// Case 1. all characters are processed
		return string::npos;
	}else if( ( trimmed_sent[ beg ] == ' ' ) || ( isalnum( trimmed_sent[ beg ] ) == 0 ) ) {		// Case 2. the current character is a delimiter character
		return beg + 1;
	}else {												// Case 3. find the position of a next delimiter character
		for( size_t i = beg; i < max; ++i )
			if( ( trimmed_sent[ i ] == ' ' ) || ( isalnum( trimmed_sent[ i ] ) == 0 ) )
				return i;
		return max;
	}
}


