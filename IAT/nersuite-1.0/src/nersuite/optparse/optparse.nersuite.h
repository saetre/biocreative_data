/**
* ner_option class parses an input parameters (1~n-th command input parameters)
*
* @param	-m (or --model)	a model file name
* @param	-d (or --dictionary)	an external dictionary file name (CDB++ compiled dictionary)
* @param	-f (or --format)	input file format ("standoff" or "none")
* @return		the number of consumed command line parameters 
*
* Modified for NERsuite at 13/07/10
*/

#ifndef		_OPTPARSE_FOR_NERSUITE_
#define		_OPTPARSE_FOR_NERSUITE_
	

#include <iostream>
#include <stdlib.h>
#include "optparse.h"


using namespace std;


class nersuite_optparse : public optparse {
public:

	string	fn_model;
	double	sigma;
	bool	is_standoff;

	nersuite_optparse(void) {
		fn_model = "";
		sigma = 1.0;
		is_standoff = false;
	}

	BEGIN_OPTION_MAP_INLINE()
		ON_OPTION_WITH_ARG(SHORTOPT('m') || LONGOPT("model"))
			fn_model = arg;

		ON_OPTION_WITH_ARG(SHORTOPT('s') || LONGOPT("sigma"))
			sigma = atof( arg );

		ON_OPTION_WITH_ARG(SHORTOPT('f') || LONGOPT("format"))
			string	format = arg;
			if( format == "standoff" ) {
				is_standoff = true;
			}else if( format == "conll" ) {
				is_standoff = false;
			}else {
				throw unrecognized_option( format.c_str() );
			}
	END_OPTION_MAP()
};

#endif

#ifdef _TEST_OPTPARSE_NER_VER1

int main(int argc, char *argv[]) {
    
	try {
        nersuite_optparse opt;

        int argused = opt.parse(&argv[1], argc-1); // Skip argv[0].

		cout << "used argv: " << argused << endl;
		cout << "Model name: " << opt.fn_model << endl;
		cout << "Dictionary name: " << opt.fn_dic << endl;
		cout << "Input file format: " << (opt.is_standoff ? "true" : "false") << endl;

	} catch (const optparse::unrecognized_option& e) {
		cout << "unrecognized option: " << e.what() << endl;
		return 1;
	} catch (const optparse::invalid_value& e) {
		cout << "invalid value: " << e.what() << endl;
		return 1;
	}

	return 0;
}

#endif


