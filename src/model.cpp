#include <cstdio>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

// local includes
#include <model.h>

//------------------------------------------------------------------------------

void Model::Reset()
{
	isInitialized = false;
	countTagsPair.clear();
	countWords = 0;
	paradigms.clear();
	prefixes.clear();
	suffixes.clear();
	tags.clear();
	words.clear();
	ends.clear();
}

bool Model::Initialize( const string& dictionaryDirectory, ostream& errorStream )
{
	Reset();

	try {
		words.load( ( dictionaryDirectory + "/words.trie" ).c_str() );
		ends.load( ( dictionaryDirectory + "/ends.trie" ).c_str() );
	} catch( std::exception& e ) {
		errorStream << "Error: " << e.what() << "." << endl;
		return false;
	}

	if( !loadSimpleFile( dictionaryDirectory + "/prefixes", prefixes ) ) {
		errorStream << "Error: Dictionary file '" << dictionaryDirectory
			<< "/prefixes" << "' is corrupted." << endl;
		return false;
	}

	if( !loadSimpleFile( dictionaryDirectory + "/suffixes", suffixes ) ) {
		errorStream << "Error: Dictionary file '" << dictionaryDirectory
			<< "/suffixes" << "' is corrupted." << endl;
		return false;
	}

	if( !loadSimpleFile( dictionaryDirectory + "/tags", tags ) ) {
		errorStream << "Error: Dictionary file '" << dictionaryDirectory
			<< "/tags" << "' is corrupted." << endl;
		return false;
	}

	if( !loadParadigms( dictionaryDirectory + "/paradigms" ) ) {
		errorStream << "Error: Dictionary file '" << dictionaryDirectory
			<< "/paradigms" << "' is corrupted." << endl;
		return false;
	}

	return true;
}

bool Model::loadSimpleFile( const string& filename, StringVector& data )
{
	data.clear();
	ifstream file( filename );
	if( file.good() ) {
		size_t size;
		file >> size;
		file.ignore( 10, '\n' );
		if( !file.fail() ) {
			data.resize( size );
			for( size_t i = 0; !file.fail() && i < size; i++ ) {
				getline( file, data[i] );
				if( !data[i].empty() && data[i].back() == '\r' ) {
					data[i].pop_back();
				}
			}
		}
	}
	return !file.fail();
}

bool Model::loadParadigms( const string& filename )
{
	paradigms.clear();
	ifstream file( filename );
	if( file.good() ) {
		size_t size;
		file >> size;
		if( !file.fail() ) {
			paradigms.resize( size );
			for( size_t i = 0; !file.fail() && i < size; i++ ) {
				paradigms[i].load( file );
			}
		}
	}
	return !file.fail();
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

double Model::Test( const string& filename, ostream& out ) const
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

StringPair Model::Predict( const string& prevTag, const string& curWord ) const
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
		auto p = countTagsPair.find( tmp );
		if( p == countTagsPair.cend() ) {
			continue;
		}
		const uint current = probs[i] * p->second;
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

	const string uppercaseWord = Utf8::ToUppercase( word );
	getNFandTags( Utf8::ReplaceYoWithYe( uppercaseWord ), variants );

	if( variants.empty() ) {
		if( Utf8::StartsWithPunctuationMark( word ) ) {
			variants.push_back( StringPair( word, "PNCT" ) );
		} else if( Utf8::StartsWithDigit( word ) ) {
			variants.push_back( StringPair( word, "NUMB" ) );
		} else if( Utf8::StartsWithLatinLetter( word ) ) {
			variants.push_back( StringPair( word, "LATN" ) );
		} else {
			getTagsAndCount( Utf8::Suffix( uppercaseWord, 3 ), variants, probs );

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
		size_t p, n;
		parseAgentKey( agent, p, n );
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
		size_t p, n;
		parseAgentKey( agent, p, n );
		variants.push_back( StringPair( "", tags[p] ) );
		probs.push_back( n );
	}
}

void Model::parseAgentKey( const marisa::Agent& agent, size_t& p, size_t& n ) const
{
	string key( agent.key().ptr(), agent.key().ptr() + agent.key().length() );
	istringstream iss( key );
	string ignored;
	iss >> ignored >> p >> n;
	assert( !iss.fail() );
}

//------------------------------------------------------------------------------

namespace Utf8 {

// return the text where all ascii and utf-8 russian letters are in uppercase
string ToUppercase( const string& text )
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
string ReplaceYoWithYe( const string& text )
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
string Suffix( const string& text, size_t count )
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

// return true if word starts with punctuation mark
bool StartsWithPunctuationMark( const string& word )
{
	static const string puncts( "!\"'(),-.:;?[]" );
	return ( !word.empty() && puncts.find( word.front() ) != string::npos );
}

// return true if word ends with punctuation mark
bool EndsWithPunctuationMark( const string& word )
{
	static const string puncts( "!\"'(),-.:;?[]" );
	return ( !word.empty() && puncts.find( word.back() ) != string::npos );
}

// return true if word starts with digit
bool StartsWithDigit( const string& word )
{
	return ( !word.empty() && word[0] >= '0' && word[0] <= '9' );
}

// return true if word starts with latin letter
bool StartsWithLatinLetter( const string& word )
{
	return ( !word.empty()
		&& ( word[0] >= 'a' && word[0] <= 'z'
			|| word[0] >= 'A' && word[0] <= 'Z' ) );
}

} // end of Utf8 namespace
