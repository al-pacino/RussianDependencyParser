#include <cstdio>
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>

// local includes
#include <model.h>
#include <tinyxml2.h>

using namespace std;

//------------------------------------------------------------------------------

const char* const TemporaryMorphFilename = "temporary_morph_file";
const char* const ModelSubdirectoryName = "dict";

//------------------------------------------------------------------------------

void SplitTags( const string& tags, string& firstTag, string& restTags )
{
	size_t pos = tags.find( ',' );
	if( pos == string::npos ) {
		firstTag = tags;
		restTags.clear();
	} else {
		firstTag = tags.substr( 0, pos );
		restTags = tags.substr( pos + 1 );
		// replace all , with | in restTags
		pos = 0;
		while( ( pos = restTags.find( ',', pos ) ) != string::npos ) {
			restTags[pos] = '|';
		}
	}
	if( restTags.empty() ) {
		restTags = "_";
	}
	assert( !firstTag.empty() );
}

bool WriteSentence( const tinyxml2::XMLElement* seElem,
	ostream& out, const Model& model )
{
	using namespace tinyxml2;

	assert( seElem != nullptr );
	assert( string( seElem->Name() ) == "se" );

	string prevTag = "NONE";
	for( const tinyxml2::XMLElement* wElem = seElem->FirstChildElement();
		wElem != nullptr; wElem = wElem->NextSiblingElement() )
	{
		if( string( wElem->Name() ) != "w" ) {
			continue;
		}
		const tinyxml2::XMLElement* relElem = wElem->FirstChildElement();
		if( relElem == nullptr || string( relElem->Name() ) != "rel" ) {
			return false;
		}

		unsigned int id;
		if( wElem->QueryUnsignedAttribute( "id", &id ) != XML_NO_ERROR ) {
			return false;
		}

		string word( wElem->GetText() == nullptr ? "_" : wElem->GetText() );
		if( !Utf8::StartsWithPunctuationMark( word )
			&& Utf8::EndsWithPunctuationMark( word ) )
		{
			word.pop_back();
		}

		const char* idHead = relElem->Attribute( "id_head" );
		if( idHead == nullptr || *idHead == '\0' ) {
			idHead = "0";
		}
		const char* type = relElem->Attribute( "type" );
		if( type == nullptr || *type == '\0' ) {
			type = "punct";
		}

		StringPair predicted = model.Predict( prevTag, word );
		prevTag = predicted.second;

		string firstTag;
		string restTags;
		SplitTags( predicted.second, firstTag, restTags );

		out << id << "\t" << word << "\t" << predicted.first << "\t"
			<< firstTag << "\t" << firstTag << "\t" << restTags
			<< "\t" << idHead << "\t" << type << "\t_\t_" << endl;
	}

	return true;
}

bool WriteMorph( const string& xmlFilename,
	ostream& out, const Model& model )
{
	using namespace tinyxml2;

	XMLDocument doc;
	if( doc.LoadFile( xmlFilename.c_str() ) != XML_NO_ERROR ) {
		return false;
	}

	const tinyxml2::XMLElement* sentencesElem = doc.RootElement();
	if( string( sentencesElem->Name() ) != "sentences" ) {
		return false;
	}

	for( const tinyxml2::XMLElement* pElem = sentencesElem->FirstChildElement();
		pElem != nullptr; pElem = pElem->NextSiblingElement() )
	{
		if( string( pElem->Name() ) != "p" ) {
			return false;
		}

		for( const tinyxml2::XMLElement* seElem = pElem->FirstChildElement();
			seElem != nullptr; seElem = seElem->NextSiblingElement() )
		{
			if( string( seElem->Name() ) != "se" ) {
				return false;
			}

			if( !WriteSentence( seElem, out, model ) ) {
				return false;
			}
			out << endl;
		}
	}

	return true;
}

bool SaveMorph( const string& modelFilename,
	const string& textFilename, const string& morphFilename )
{
	Model model;
	if( !model.Initialize( ModelSubdirectoryName, cerr ) ) {
		return false;
	}
	if( !model.Load( modelFilename, cerr ) ) {
		return false;
	}

	ofstream morphStream( morphFilename, ios::out | ios::trunc );
	if( morphStream.good()
		&& !WriteMorph( textFilename, morphStream, model ) )
	{
		cerr << "Error: Wrong text file '"
			<< textFilename << "'." << endl;
		return false;
	}

	if( !morphStream.good() ) {
		cerr << "Error: Cannot open or write morph file '"
			<< morphFilename << "'." << endl;
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------

// argv: train.txt newMorphModel.txt
bool MorphTrain( const char* argv[] )
{
	Model model;
	return ( model.Initialize( ModelSubdirectoryName, cerr )
		&& model.Train( argv[0], cerr )
		&& model.Save( argv[1], cerr ) );
}

//------------------------------------------------------------------------------

// argv: input.txt output.txt MorphModel.txt
bool MorphMark( const char* argv[] )
{
	return SaveMorph( argv[2], argv[0], argv[1] );
}

//------------------------------------------------------------------------------

// argv: input.txt MorphModel.txt SyntModel.txt TURBO_PARSER
bool SyntTrain( const char* argv[] )
{
	if( !SaveMorph( argv[1], argv[0], TemporaryMorphFilename ) ) {
		return false;
	}

	ostringstream arguments;
	arguments
		<< '"' << argv[3] << '"' // path to turbo parser
		<< " --train"
		<< " --file_test=\"" << TemporaryMorphFilename << '"'
		<< " --file_model=\"" << argv[2] << '"';

	// run turboparser
	cout << "Running '" << argv[3] <<"'" << endl;
	system( arguments.str().c_str() );

	// delete TemporaryMorphFilename
	remove( TemporaryMorphFilename );

	return true;
}

//------------------------------------------------------------------------------

// argv: input.txt MorphModel.txt SyntModel.txt output.txt TURBO_PARSER
bool SyntMark( const char* argv[] )
{
	if( !SaveMorph( argv[1], argv[0], TemporaryMorphFilename ) ) {
		return false;
	}

	ostringstream arguments;
	arguments
		<< '"' << argv[4] << '"' // path to turbo parser
		<< " --test"
		<< " --evaluate"
		<< " --file_test=\"" << TemporaryMorphFilename << '"'
		<< " --file_model=\"" << argv[2] << '"'
		<< " --file_prediction=\"" << argv[3] << '"';

	// run turboparser
	cout << "Running '" << argv[4] <<"'" << endl;
	system( arguments.str().c_str() );

	// delete TemporaryMorphFilename
	remove( TemporaryMorphFilename );

	return true;
}

//------------------------------------------------------------------------------

typedef bool ( *StartupFunctionPtr )( const char* argv[] );

struct StartupMode {
	const char* FirstArgument;
	int NumberOfArguments;
	StartupFunctionPtr StartupFunction;
	const char* HelpString;
};

const StartupMode StartupModes[] = {
	{ "--morphtrain", 2, MorphTrain,
	  "--morphtrain train.txt newMorphModel.txt" },

	{ "--morphmark", 3, MorphMark,
	  "--morphmark input.txt output.txt MorphModel.txt" },

	{ "--synttrain", 4, SyntTrain,
	  "--synttrain input.txt MorphModel.txt SyntModel.txt TURBO_PARSER" },

	{ "--syntmark", 5, SyntMark,
	  "--syntmark input.txt MorphModel.txt "
	  "SyntModel.txt output.txt TURBO_PARSER" },

	{ nullptr, -1, nullptr, nullptr }
};

//------------------------------------------------------------------------------

void PrintUsage( const char* programName = 0 )
{
	cerr << "Usage: " << ( programName != 0 ? programName : "" ) << endl
		<< endl;
	for( int i = 0; StartupModes[i].FirstArgument != nullptr; i++ ) {
		assert( StartupModes[i].HelpString != nullptr );
		if( i > 0 ) {
			cerr << "OR" << endl;
		}
		cerr << "  " << StartupModes[i].HelpString << endl;
	}
	cerr << endl
		<< "Where TURBO_PARSER is a path to the turbo parser program file."
		<< endl << endl;
}

//------------------------------------------------------------------------------

bool TryRun( int argc, const char* argv[] )
{
	int startupMode = -1;
	if( argc >= 2 ) {
		const string firstArgument( argv[1] );

		// skip first two arguments
		argc -= 2;
		argv += 2;

		for( int i = 0; StartupModes[i].FirstArgument != nullptr; i++ ) {
			if( firstArgument == StartupModes[i].FirstArgument ) {
				if( argc == StartupModes[i].NumberOfArguments ) {
					startupMode = i;
				}
				break;
			}
		}
	}

	if( startupMode != -1 ) {
		bool success = StartupModes[startupMode].StartupFunction( argv );

		if( success ) {
			cout << endl << "Done!" << endl;
		} else {
			cerr << endl << "Failed!" << endl;
		}

		return success;
	}

	PrintUsage();
	return false;
}

//------------------------------------------------------------------------------

int main( int argc, const char* argv[] )
{
	if( !TryRun( argc, argv ) ) {
		// an error ocurred
		return 1;
	}
	return 0;
}
