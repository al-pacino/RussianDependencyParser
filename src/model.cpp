#include <cstdio>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

// temporary includes
#include <QString>

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
			tagPair.first = "NONE";
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
	ifstream file( filename );

	if( !file.good() ) {
		out << "Error: Input file '" << filename << "' was not found." << endl;
		return false;
	}
	
	uint countAll = 0;
	uint countWrong = 0;
	StringPair tagPair( "NONE", "" );
	string zeroWord;
	string secondWord;
	string line;
	while( file.good() ) {
		getline( file, line );
		if( line.empty() || line == "----------" ) {
			tagPair.first = "NONE";
			continue;
		}

		istringstream lineStream( line );
		lineStream >> zeroWord >> secondWord >> secondWord;

		if( lineStream.fail() ) {
			file.setstate( ios::failbit );
			break;
		}

		tagPair.second = Predict( tagPair.first, zeroWord ).second;
		if( tagPair.second != "PNKT"
			&& tagPair.second != "NUMB"
			&& tagPair.second != "LATN"
			&& tagPair.second != "UNKN")
		{
			if( secondWord == "UNKN" ) {
				continue;
			}
			countAll++;
			if( secondWord != tagPair.second ) {
				out << zeroWord << " : " << secondWord
					<< " != " << tagPair.second << endl;
				countWrong++;
			}
		}
		swap( tagPair.first, tagPair.second );
	}

	if( file.fail() ) {
		out << "Error: Input file '" << filename << "' is corrupted." << endl;
		return false;
	}

	out << static_cast<double>( countAll - countWrong ) / countAll << endl;
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

// return the text where all ascii and utf-8 russian letters are in uppercase
static string utf8makeUppercase( const string& text )
{
	string result( text );
	for( size_t pos = 0; pos < result.length(); pos++ ) {
		unsigned char c = result[pos];
		if( c < 128 ) {
			result[pos] = ::toupper( result[pos] );
		} else if( c == 0xD0 ) {
			unsigned char nc = result[pos + 1];
			if( nc >= 0xB0 && nc <= 0xBF ) {
				result[pos + 1] = static_cast<char>( nc - 32 );
			}
		} else if( c == 0xD1 ) {
			unsigned char nc = result[pos + 1];
			if( nc >= 0x80 && nc <= 0x8F ) {
				result[pos] = '\xD0';
				result[pos + 1] = static_cast<char>( nc + 32 );
			} else if( nc == 0x91 ) {
				result[pos] = '\xD0';
				result[pos + 1] = '\x81';
			}
		}
	}
	return result;
}

// return the text where all utf-8 Yo (\xD0\x81) was replaced by utf-8 Ye (\xD0\x95)
static string utf8replaceYoWithYe( const string& text )
{
	string result( text );
	size_t d0pos = 0;
	while( ( d0pos = result.find( '\xD0', d0pos ) ) != string::npos ) {
		d0pos++;
		if( result[d0pos] == '\x81' ) {
			result[d0pos] = '\x95';
		}
	}
	return result;
}

// return count last utf8 symbols from the text
static string utf8suffix( const string& text, size_t count )
{
	size_t offset = text.length();
	while( offset > 0 && count > 0 ) {
		offset--;
		if( ( static_cast<unsigned char>( text[offset] ) & 0xC0 ) != 0x80 ) {
			count--;
		}
	}
	return text.substr( offset );
}

void Model::GetTags( const string& word,
	vector<StringPair>& variants, vector<uint>& probs ) const
{
	variants.clear();
	probs.clear();

	const string uppercaseWord = utf8makeUppercase( word );
	getNFandTags( utf8replaceYoWithYe( uppercaseWord ), variants );

	if( variants.empty() ) {
		QChar firstChar = QString::fromStdString( word )[0];
		if (firstChar.isPunct()) {
			variants.push_back( StringPair( word, "PNCT" ) );
		} else if (firstChar.isNumber()) {
			variants.push_back( StringPair( word, "NUMB" ) );
		} else if (firstChar.toLatin1() != 0) {
			variants.push_back( StringPair( word, "LATN" ) );
		} else {
			getTagsAndCount( utf8suffix( uppercaseWord, 3 ), variants, probs );

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
