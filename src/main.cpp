#include <cstdio>
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>

// local includes
#include <model.h>

// temporary includes
#include <QDir>
#include <QTextStream>
#include <QFile>
#include <QXmlStreamReader>

using namespace std;

bool printMorph( const QString& f1, const QString& f2, Model& m )
{
    QFile xmlInput(f1);
    QFile outFile(f2);
    int j = 1;
    int s = 0;
    bool fSnt = true;

    if (!xmlInput.open(QIODevice::ReadOnly | QIODevice::Text)) {
		cerr << "Can't open input file" << endl;
        return false;
    }
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		cerr << "Can't open output file" << endl;
        return false;
    }
    QTextStream outF(&outFile);
    outF.setCodec("UTF-8");
    QXmlStreamReader xmlReader(&xmlInput);
    QXmlStreamReader::TokenType token;
    QString prevTag = "NONE";
    QString id;
    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        token = xmlReader.readNext();
        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }
        if (token == QXmlStreamReader::StartElement) {
            if (xmlReader.name() == "se") {
                j = 1;
                ++s;
                if (!fSnt) {
                    outF << endl;
                } else {
                    fSnt = false;
                }
                prevTag = "NONE";
                id = "0";
            }
            if (xmlReader.name() == "w") {
                id = xmlReader.attributes().value("id").toString();
                token = xmlReader.readNext();
                QString word = xmlReader.text().toString();
                if (!word[0].isPunct() && word[word.size() - 1].isPunct()) {
                    word.chop(1);
                }

                StringPair predicted = m.predict(prevTag, word);
                prevTag = predicted.second;
                token = xmlReader.readNext();
                if (xmlReader.name() != "rel") {
                    return false;
                }
                QString idHead = xmlReader.attributes().value("id_head").toString();
                QString type = xmlReader.attributes().value("type").toString();

                QStringList lineParts;
                lineParts = predicted.second.split(",");
                QString Pos = lineParts[0];
                QString tags = "";
                for (int i = 1; i < lineParts.size(); ++i) {
                    tags += lineParts[i];
                    tags += "|";
                }
                if (tags.size() == 0) {
                    tags = "_";
                } else {
                    tags.chop(1);
                }
                if (idHead.size() == 0) idHead = "0";
                if (type.size() == 0) type = "punct";
                outF << id << "\t"
                        << word << "\t"
                        << predicted.first << "\t"
                        << Pos << "\t"
                        << Pos << "\t"
                        << tags << "\t"
                        << idHead << "\t"
                        << type << "\t" << "_" << "\t" << "_" << endl;
                ++j;
            }
        }

    }
    outFile.close();
    return true;
}

//------------------------------------------------------------------------------

// If filePath already is an absolute path just return it
string AbsoluteFilePath( const string& filePath )
{
	return QDir::current().absoluteFilePath( filePath.c_str() ).toStdString();
}

//------------------------------------------------------------------------------

const char* const TemporaryMorphFilename = "temporary_morph_file";
const char* const ModelSubdirectoryName = "dict";

bool SaveMorph( const string& modelFilename,
	const string& textFilename, const string& morphFilename )
{
	QTextStream out( stderr );
	out.setCodec( "UTF-8" );
	Model m( ModelSubdirectoryName );
	m.load( modelFilename.c_str(), out );
	return printMorph( textFilename.c_str(), morphFilename.c_str(), m );
}

//------------------------------------------------------------------------------

// argv: train.txt newMorphModel.txt
bool MorphTrain( const char* argv[] )
{
	QTextStream out( stderr );
	out.setCodec( "UTF-8" );
	Model m( ModelSubdirectoryName );
	m.train( argv[0], out );
	m.Save( argv[1], cerr );
	return true;
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
		<< '"' << argv[4] << '"' // path to turbo parser
		<< " --train"
		<< " --file_test=\"" << AbsoluteFilePath( TemporaryMorphFilename ) << '"'
		<< " --file_model=\"" << AbsoluteFilePath( argv[2] ) << '"';

	// run turboparser
	cout << "Running '" << argv[4] <<"'" << endl;
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
		<< " --file_test=\"" << AbsoluteFilePath( TemporaryMorphFilename ) << '"'
		<< " --file_model=\"" << AbsoluteFilePath( argv[2] ) << '"'
		<< " --file_prediction=\"" + AbsoluteFilePath( argv[3] ) << '"';

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
	bool success = false;
	if( argc >= 2 ) {
		const string firstArgument( argv[1] );

		// skip first two arguments
		argc -= 2;
		argv += 2;

		int i = 0;
		while( StartupModes[i].FirstArgument != nullptr ) {
			if( firstArgument == StartupModes[i].FirstArgument ) {
				if( argc == StartupModes[i].NumberOfArguments ) {
					success = StartupModes[i].StartupFunction( argv );
				}
				break;
			}
			i++;
		}
	}

	if( !success ) {
		PrintUsage();
	}
	return success;
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
