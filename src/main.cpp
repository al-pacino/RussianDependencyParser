#include "model.h"
#include <QTextStream>
#include <QFile>
#include <QXmlStreamReader>
#include <QList>
#include <QStringList>
#include <QProcess>
#include <QDir>

#include <cassert>
#include <string>
#include <iostream>

using namespace std;

bool
printMorph(const QString& f1, const QString& f2, Model& m, QTextStream& out) {
    QFile xmlInput(f1);
    QFile outFile(f2);
    int j = 1;
    int s = 0;
    bool fSnt = true;

    if (!xmlInput.open(QIODevice::ReadOnly | QIODevice::Text)) {
        out << "Can't open input file";
        return false;
    }
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        out << "Can't open output file";
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

bool MorphTrain( const char* argv[] )
{
	QTextStream out(stderr);
	out.setCodec("UTF-8");
	Model m("dict");
	m.train(argv[0], out);
	m.save(argv[1], out);
	return true;
}

//------------------------------------------------------------------------------

bool MorphMark( const char* argv[] )
{
	QTextStream out(stderr);
	out.setCodec("UTF-8");
	Model m("dict");
	m.load(argv[2], out);
	return ( printMorph(argv[0], argv[1], m, out) );
}

//------------------------------------------------------------------------------

bool SyntTrain( const char* argv[] )
{
	QTextStream out(stderr);
	out.setCodec("UTF-8");
	Model m("dict");
	m.load(argv[1], out);
	if( !printMorph(argv[0], "tmpFile", m, out) ) {
		return false;
	}
	QProcess turboParser;
	turboParser.setProcessChannelMode(QProcess::MergedChannels);
	/*turboParser.start(argv[3],
			QStringList() << "--train" << "--file_train=" + dir.absoluteFilePath("tmpFile") << "--file_model=" + dir.absoluteFilePath(argv[2]));
	turboParser.waitForFinished(-1);
	QFile::remove("tmpFile");*/
	return true;
}

//------------------------------------------------------------------------------

bool SyntMark( const char* argv[] )
{
	QTextStream out(stderr);
	out.setCodec("UTF-8");
	out << "Loading model" << endl;
	Model m("dict");
	m.load(argv[1], out);
	out << "Generating tmp file" << endl;
	if( !printMorph( argv[0], "tmpFile", m, out ) ) {
		return false;
	}
	out << "Running TurboParser" << endl;
	QProcess turboParser;
	turboParser.setProcessChannelMode(QProcess::MergedChannels);
/*	turboParser.start(argv[4],
			QStringList() << "--test" << "--evaluate" << "--file_test=" + dir.absoluteFilePath("tmpFile")
					<< "--file_model=" + dir.absoluteFilePath(argv[2]) << "--file_prediction=" + dir.absoluteFilePath(argv[3]));
	turboParser.waitForFinished(-1);
	QFile::remove("tmpFile");*/
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
		// skip first two arguments
		argc -= 2;
		argv += 2;

		const string firstArgument( argv[1] );
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
