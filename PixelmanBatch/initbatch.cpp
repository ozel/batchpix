#include <string>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <vector>

#include <stdlib.h>     /* atof */

#include "batchpixel.h"

using namespace std;

#define __max_length 2024

void ExtractInfo(string, double &, double &, double &, string &, bool &, bool );
string GetRidOfPrefixSpaces(string s1);

void ExtractInfo(string data, double & min, double & max, double & step, string & theinfo, bool & isrange, bool isstring) {

	size_t pos = data.find( '-' );
	if( pos != string::npos ) { // A range

		isrange = true;
		char temp[__max_length];
		char temp2[__max_length];

		// first number
		data.copy(temp, pos, 0);	// copy the interesting part
		// erase
		data.erase(0, pos+1);		// erase from string
		temp[pos+1] = '\0';			// terminate correctly
		min = atof( temp ); 		// atof

		// second number
		pos = data.find( ':' );
		data.copy(temp2, pos, 0);	// copy the interesting part
		// erase
		data.erase(0, pos+1);		// erase from string
		temp2[pos+1] = '\0';		// terminate correctly
		max = atof( temp2 ); 		// atof

		// step
		step = atof( data.c_str() ); 		// atof

	} else {
		if ( ! isstring ) min = atof( data.c_str() );
		else theinfo = data;
		isrange = false;
	}

}

bool AllParameters::ParseAndModifyParameter(char * oneline) {

	string * toparse = new string(oneline);
	vector<BatchParCommon * >::iterator itr = m_parameters.begin();
	BatchParCommon * p;
	BatchParameter<int> * p1 = 0x0; BatchParameter<double> * p2 = 0x0; BatchParameter<string> * p3 = 0x0;
	size_t pos;
	double min=0., max=0., step=0.;
	string theinfo;
	bool isrange = false, isstring = false;
	bool foundtag = false;

	for ( ; itr != m_parameters.end() ; itr++ ) {

		p1 = 0x0; p2 = 0x0; p3 = 0x0;

		p = (*itr);

		if ( (*itr)->GetTypeName() == string( typeid(int).name() ) ) 	p1 = (BatchParameter<int> *) (p);
		if ( (*itr)->GetTypeName() == string( typeid(double).name() ) ) p2 = (BatchParameter<double> *) (p);
		if ( (*itr)->GetTypeName() == string( typeid(string).name() ) ) p3 = (BatchParameter<string> *) (p);

		// check par name
		string parname = p->GetParName();
		// Isolate the part before the "|" and check if the parameter is there
		pos = toparse->find( '|' );
		string parcheck = toparse->substr(0, pos);
		// now check
		pos = parcheck.find( parname );
		// See if this is the exact string
		if( (*toparse)[ pos + parname.length() ] != char(' ') && (*toparse)[ pos + parname.length() ] != char('|') ) {
			pos = string::npos; // not found
		}

		if ( pos != string::npos ) {

			// Extract info
			pos = toparse->find( '|' );
			toparse->erase(0, pos+1);

			// See if this is a string
			if( typeid(string).name() == p->GetTypeName() ) isstring = true;
			else isstring = false;

			// This function returns --> min, max, step, theinfo, isrange
			ExtractInfo(*toparse, min, max, step, theinfo, isrange, isstring);

			// Kepp track of those which are defined in range (calls for a Scan)
			if ( isrange ) {
				m_parametersRange.push_back( p );
				m_nparsScan++;
			} else {
				m_parametersFix.push_back( p );
			}

			// Enter info
			if ( p1 ) {				// int

				p1->SetMin(min);
				if (isrange) {
					p1->SetMax(max);
					p1->SetStep(step);
				} else {
					p1->SetMax(min);
					p1->SetStep(1); // just to stop the loop artificially
				}
				p1->SetIsRange(isrange);

			} else if ( p2 ) {		// double

				p2->SetMin(min);
				if (isrange) {
					p2->SetMax(max);
					p2->SetStep(step);
				} else {
					p2->SetMax(min);
					p2->SetStep(1); // just to stop the loop artificially
				}
				p2->SetIsRange(isrange);

			} else if ( p3 ) {		// string

				// get rid of any space at the beginning of the string
				theinfo = GetRidOfPrefixSpaces(theinfo);

				p3->SetMin(theinfo);
				p3->SetMax("");
				p3->SetStep("");
				p3->SetIsRange(isrange);

			}

			foundtag = true;
		}


		if ( foundtag ) break;
	}

	delete toparse;

	return foundtag;
}

string GetRidOfPrefixSpaces(string s1) {

	size_t len = s1.length();
	size_t pos = 0;
	for ( ; pos < len ; pos++ ) {
		if ( s1[pos] != ' ' ) break;
	}
	if ( pos > 0 ) { // cut
		s1.erase(0, pos);
	}

	return s1;
}

bool ParseConfFile (string fn, AllParameters * pars) {

	std::filebuf fb;
	fb.open (fn.c_str(), std::ios::in);

	if( ! fb.is_open() ) {
		cout << "[ERROR] Couldn't open config file " << fn.c_str() << endl;
		return false;
	} else {
		cout << "Openning config file " << fn.c_str() << endl;
	}

	char temp[__max_length];

	if ( fb.is_open() ) {

		std::istream is(&fb);
		while ( is.good() ) {

			// read a line
			is.getline(temp, __max_length);

			// check end of file
			if ( is.eof() ) {
				fb.close();
				break;
			}

			// Analyze the line and modify
			if ( ! pars->ParseAndModifyParameter(temp) ) { // if something goes wrong with the line
				cout << "[ERROR] the line \"" << temp << "\" couldn't be parsed.  Fall back to default parameters." << endl;
			}

		}

		fb.close();
	}

	return true;
}
///////////////////////////////////////////////////////////////
// Class
// BatchParCommon --> Wrapper for BatchParameter family
BatchParCommon::BatchParCommon(string parname) {

	m_name = parname;

}


///////////////////////////////////////////////////////////////
// Class
// template <class T> class BatchParameter
template <class T>
BatchParameter<T>::BatchParameter (string parname, T initval) : BatchParCommon (parname) {

	m_isRange = false; 								// to be determined later during parsing

	// Set default values
	m_min = initval; m_max = initval; m_step = initval;

	// Set a few things in the parent
	SetTypeName( typeid(initval).name() );

}

template <class T>
BatchParameter<T>::~BatchParameter () {

}

template class BatchParameter<int>;
template class BatchParameter<double>;
template class BatchParameter<string>;

///////////////////////////////////////////////////////////////
// Class
// template <class T> AllParameters
AllParameters::AllParameters () {
	m_nparsScan = 0;
	m_npars = 0;
}

AllParameters::~AllParameters () {

}

BatchParCommon * AllParameters::GetParByName(string parname) {

	vector<BatchParCommon * >::iterator itr = m_parameters.begin();
	BatchParCommon * p;

	for ( ; itr != m_parameters.end() ; itr++ ) {

		p = (*itr);

		if( p->GetParName() == parname ) return p;

	}

	return 0x0; // not found
}

template <typename T> void AllParameters::PushParameter (string parname, T initval) {

	// Create the parameter
	BatchParameter<typeof(initval)> * onepar  = new BatchParameter<typeof(initval)>( parname, initval );
	// And push it back in the vector by casting to its parent
	m_parameters.push_back( static_cast<BatchParCommon *>(onepar) );
	m_npars++;

}

void AllParameters::DumpAllParameters() {

	vector<BatchParCommon * >::iterator itr = m_parameters.begin();
	BatchParCommon * p;
	BatchParameter<int> * p1 = 0x0; BatchParameter<double> * p2 = 0x0; BatchParameter<string> * p3 = 0x0;

	cout << "-- User parameters list ---------------- " << endl;

	for ( ; itr != m_parameters.end() ; itr++ ) {

		p1 = 0x0; p2 = 0x0; p3 = 0x0;

		p = (*itr);

		if ( (*itr)->GetTypeName() == string( typeid(int).name() ) ) 	p1 = (BatchParameter<int> *) (p);
		if ( (*itr)->GetTypeName() == string( typeid(double).name() ) ) p2 = (BatchParameter<double> *) (p);
		if ( (*itr)->GetTypeName() == string( typeid(string).name() ) ) p3 = (BatchParameter<string> *) (p);

		cout << "[" << p->GetParName() << "] (" << p->GetTypeName() << ")" << "\t| ";

		if ( p1 ) {				// int

			if ( p1->IsRange() ) {
				cout << p1->GetMin() << " - " << p1->GetMax() << " : " << p1->GetStep();
			} else {
				cout << p1->GetMin();
			}

		} else if ( p2 ) {		// double

			if ( p2->IsRange() ) {
				cout << p2->GetMin() << " - " << p2->GetMax() << " : " << p2->GetStep();
			} else {
				cout << p2->GetMin();
			}

		} else if ( p3 ) {		// string

			if ( p3->IsRange() ) {
				cout << p3->GetMin() << " - " << p3->GetMax() << " : " << p3->GetStep();
			} else {
				cout << p3->GetMin();
			}
		}

		cout << endl;
	}
	cout << "-- Loop info --------------------------- " << endl;
	cout << "Fixed parameters = " << m_npars - m_nparsScan << " | Scan parameters = " << m_nparsScan << endl;
	cout << "---------------------------------------- " << endl;

}

template void AllParameters::PushParameter(string, int);
template void AllParameters::PushParameter(string, double);
template void AllParameters::PushParameter(string, string);

