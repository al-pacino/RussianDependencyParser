#include <cstdio>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

// temporary includes
#include <QString>
#include <QFile>
#include <QTextStream>

// local includes
#include <model.h>

Model::Model(const char *dictdir) 
{
    countTagsPair.clear();

    string dir = dictdir;
    string path = dir + "/words.trie";
    words.load(path.c_str());

    path = dir + "/ends.trie";
    ends.load(path.c_str());

    int temp = 0;
    string str;
    path = dir + "/prefixes";
    ifstream ifs(path);
    ifs>>temp;
    ifs.ignore(10, '\n');
    prefixes = new string[temp];
    for(int i = 0; i < temp; i++)
        getline(ifs, prefixes[i]);
    ifs.close();

    path = dir + "/suffixes";
    ifs.open(path);
    ifs>>temp;
    ifs.ignore(10, '\n');
    suffixes = new string[temp];
    for(int i = 0; i < temp; i++)
        getline(ifs, suffixes[i]);
    ifs.close();

    path = dir + "/tags";
    ifs.open(path);
    ifs>>temp;
    ifs.ignore(10, '\n');
    tags = new string[temp];
    for(int i = 0; i < temp; i++)
        getline(ifs, tags[i]);
    ifs.close();

    path = dir + "/paradigms";
    ifs.open(path);
    ifs>>temp;
    paradigms = new Paradigm[temp];
    for(int i = 0; i < temp; i++)
        paradigms[i].load(ifs);
    ifs.close();
}

bool Model::Save( const string& filename, ostream& out ) const
{
	ofstream model( filename, ios::out | ios::trunc );

	if( model.good() ) {
		for( auto i = countTagsPair.cbegin(); i != countTagsPair.cend(); ++i ) {
			model << i->first.first << " " << i->first.second
				<< " " << i->second << " ";
		}
		model << "----------";
	}

	if( !model.good() ) {
		out << "Error: Cannot open or write model file '" << filename << "'." << endl;
		return false;
	}
	return true;
}

bool Model::Load( const string& filename, ostream& out )
{
	ifstream model( filename );

	if( !model.good() ) {
		out << "Error: Model file '" << filename << "' was not found." << endl;
		return false;
	}

	countTagsPair.clear();
	while( model.good() ) {
		string first;
		string second;
		unsigned int value;
		model >> first >> second >> value;

		if( model.good() ) {
			countTagsPair.insert( make_pair( make_pair( first, second ), value ) );
		} else if( model.eof() && second.empty() && first == "----------" ) {
			model.clear(); // reset state to good
			break;
		}
	}

	if( !model.good() ) {
		out << "Error: Model file '" << filename << "' is corrupted." << endl;
		return false;
	}
	return true;
}

bool Model::Train( const string& filename, ostream& out )
{
	ifstream file( filename );

	if( !file.good() ) {
		out << "Error: Input file '" << filename << "' was not found." << endl;
		return false;
	}

	StringPair tagPair( "NONE", "" );
	string line;
	while( file.good() ) {
		getline( file, line );
		if( line.empty() || line == "----------" ) {
			continue;
		}

		istringstream lineStream( line );
		lineStream >> tagPair.second >> tagPair.second >> tagPair.second;

		if( lineStream.fail() ) {
			file.setstate( ios::failbit );
			break;
		}

		countWords++;
		auto i = countTagsPair.insert( make_pair( tagPair, 0 ) );
		i.first->second++;
		swap( tagPair.first, tagPair.second );
	}

	if( file.fail() ) {
		out << "Error: Input file '" << filename << "' is corrupted." << endl;
		return false;
	}
	return true;
}

double Model::Test( const string& filename, ostream& out )
{
	QFile fin( filename.c_str() );

    if (!fin.open(QIODevice::ReadOnly | QIODevice::Text)) {
        out << "ERROR: input file not found" << endl;
        return false;
    }

    QString curTag, prevTag = "NONE";
	uint countAll = 0, countWrong = 0;

    QTextStream sfin(&fin);
    sfin.setCodec("UTF-8");
    while (!sfin.atEnd()) {
        QString line = sfin.readLine();
        if (line == "----------") {
            prevTag = "NONE";
            continue;
        }

        QStringList words = line.split(" ");
		curTag = Predict(prevTag.toStdString(), words[0].toStdString()).second.c_str();
        if (curTag != "PNKT" && curTag != "NUMB" && curTag != "LATN" && curTag != "UNKN") {
            if (words[2] == "UNKN")
                continue;
            ++countAll;
            if (words[2] != "UNKN" && words[2] != curTag) {
				out << words[0].toStdString() << " : " << words[2].toStdString() << " != " << curTag.toStdString() << endl;
                ++countWrong;
            }
        }
        prevTag = curTag;
    }
    out << (countAll - countWrong) / (1. * countAll) << endl;
    return true;
}

void Model::Print( ostream& out ) const
{
	unordered_map<string, uint> count;
	for( auto i = countTagsPair.cbegin(); i != countTagsPair.cend(); ++i ) {
		out << i->first.first << " before " << i->first.second
			<< " : " << i->second << endl;

		auto p = count.insert( make_pair( i->first.first, 0 ) );
		p.first->second += i->second;
	}

	for( auto i = count.cbegin(); i != count.cend(); ++i ) {
		out << i->first << " " << i->second << endl;
	}
}

StringPair Model::Predict( const string& prevTag, const string& curWord )
{
	vector<uint> probs;
	vector<StringPair> variants;
	GetTags( curWord, variants, probs );
	assert( probs.size() == variants.size() );

	if( variants.front().second == "PNKT"
		|| variants.front().second == "NUMB"
		|| variants.front().second == "LATN"
		|| variants.front().second == "UNKN" )
	{
		return variants.front();
	}

	uint best = 0;
	size_t bestIndex = 0;

	StringPair tmp( prevTag, "" );
	for( size_t i = 0; i < variants.size(); i++ ) {
		tmp.second = variants[i].second;
		const uint current = probs[i] * countTagsPair[tmp];
		if( best < current ) {
			best = current;
			bestIndex = i;
		}
	}

	return variants[bestIndex];
}

void Model::GetTags( const string& word,
	vector<StringPair>& variants, vector<uint>& probs ) const
{
	variants.clear();
	probs.clear();
	QChar yo = QString::fromUtf8("Ё")[0];
	QChar ye = QString::fromUtf8("Е")[0];
	string _word = QString::fromStdString(word).toUpper().replace(yo, ye, Qt::CaseInsensitive).toStdString();

	getNFandTags( _word, variants );
	if( variants.empty() ) {
		QChar firstChar = QString::fromStdString( word )[0];
		if (firstChar.isPunct()) {
			variants.push_back( StringPair( word, "PNCT" ) );
		} else if (firstChar.isNumber()) {
			variants.push_back( StringPair( word, "NUMB" ) );
		} else if (firstChar.toLatin1() != 0) {
			variants.push_back( StringPair( word, "LATN" ) );
		} else {
			string _word = QString(word.c_str()).toUpper().right(3).toStdString();

			getTagsAndCount( _word, variants, probs );

			for( auto i = variants.begin(); i != variants.end(); ++i ) {
				i->first = word;
			}

			if( variants.empty() ) {
				variants.push_back( StringPair( word, "UNKN" ) );
			}
		}
	}

	if( probs.empty() ) {
		probs.resize( variants.size(), 1 );
	}
}

void Model::getNFandTags( const string& key, vector<StringPair>& variants ) const
{
	variants.clear();

	marisa::Agent agent;
	const string agentQuery = key + " "; // because agent don't copy query
	agent.set_query( agentQuery.data(), agentQuery.length() );

	while( words.predictive_search( agent ) ) {
		string normalForm = key;
		char *buf = new char[agent.key().length() + 1];
		char *b = new char[agent.key().length() + 1];
		int p;
		int n;
		strncpy(buf, agent.key().ptr(), agent.key().length());
		buf[agent.key().length()] = '\0';
		sscanf(buf, "%s %d %d", b, &p, &n);
		delete [] b;
		delete [] buf;
		string suffix = suffixes[paradigms[p].getSuffix(n)];
		normalForm = normalForm.substr(0, normalForm.size() - suffix.size());
		suffix = suffixes[paradigms[p].getSuffix(0)];
		normalForm += suffix;
		string prefix = prefixes[paradigms[p].getPrefix(n)];
		normalForm = normalForm.substr(prefix.size());
		prefix = prefixes[paradigms[p].getPrefix(0)];
		normalForm = prefix + normalForm;
		variants.push_back( StringPair( normalForm,
			tags[paradigms[p].getTags( n )] ) );
	}
}

void Model::getTagsAndCount( const string& key,
	vector<StringPair>& variants, vector<uint>& probs ) const
{
	variants.clear();
	probs.clear();

	marisa::Agent agent;
	const string agentQuery = key + " "; // because agent don't copy query
	agent.set_query( agentQuery.data(), agentQuery.length() );

	while( words.predictive_search( agent ) ) {
		char *buf = new char[agent.key().length() + 1];
		char *b = new char[agent.key().length() + 1];
		int p;
		uint n;
		strncpy(buf, agent.key().ptr(), agent.key().length());
		buf[agent.key().length()] = '\0';
		sscanf(buf, "%s %d %u", b, &p, &n);
		delete [] b;
		delete [] buf;
		variants.push_back( StringPair( "", tags[p] ) );
		probs.push_back( n );
	}
}

Model::~Model()
{
    countTagsPair.clear();
    words.clear();
    ends.clear();
    delete [] paradigms;
    delete [] tags;
    delete [] prefixes;
    delete [] suffixes;
}
