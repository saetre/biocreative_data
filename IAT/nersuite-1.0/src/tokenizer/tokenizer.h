//
// A sentence tokenizer class with a user-defined 
// delimiters
//
//

#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>


using namespace std;


typedef		vector< string >	V1_STR;
typedef		vector< V1_STR >	V2_STR;


class TOKENIZER {
private:
	int spliter( const string &trimmed_sent, V2_STR &data );
	void mark_pos( const string &raw_sent, V2_STR &data, size_t init_offset = 0 );

	string trim_ws( const string &raw_sent );
	size_t find_token_end( const string &trimmed_sent, const size_t beg );

public:
	int tokenize( const string &raw_sent, V2_STR &data, size_t init_offset = 0 );

};


