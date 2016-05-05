#ifndef __MODEL_H__
#define __MODEL_H__

#include <cstddef>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

// local includes
#include <marisa.h>
#include <paradigm.h>

using namespace std;

//------------------------------------------------------------------------------

typedef pair<string, string> StringPair;
typedef vector<string> StringVector;
typedef unsigned char uchar;
typedef unsigned int uint;

//------------------------------------------------------------------------------

class Model {
public:
	Model() { Reset(); }
	virtual ~Model() {}

	void Reset();
	bool Initialize( const string& dictionaryDirectory, ostream& errorStream );
	bool IsInitialized() const { return isInitialized; }

	bool Load( const string& filename, ostream& out );
	bool Save( const string& filename, ostream& out ) const;
	bool Train( const string& filename, ostream& out );
	double Test( const string& filename, ostream& out ) const;
	void Print( ostream& out ) const;
	StringPair Predict( const string& prevTag, const string& curWord ) const;
	void GetTags( const string& word,
		vector<StringPair>& variants, vector<uint>& probs ) const;

private:
	Model( const Model& );
	Model& operator=( const Model& );

	bool isInitialized;

	struct pair_hash {
		size_t operator () ( const pair<string, string>& p ) const
		{
			return hash<string>()( p.first ) ^ hash<string>()( p.second );
		}
	};

	unordered_map<StringPair, uint, pair_hash> countTagsPair;
	uint countWords;

	vector<Paradigm> paradigms;
	StringVector prefixes;
	StringVector suffixes;
	StringVector tags;
	marisa::Trie words;
	marisa::Trie ends;

	void getNFandTags( const string& key, vector<StringPair>& variants ) const;
	void getTagsAndCount( const string& key,
		vector<StringPair>& variants, vector<uint>& probs ) const;
	void parseAgentKey( const marisa::Agent& agent, int& p, int& n ) const;
	bool loadSimpleFile( const string& filename, StringVector& data );
	bool loadParadigms( const string& filename );
};

//------------------------------------------------------------------------------

namespace Utf8 {

// return the text where all ascii and utf-8 russian letters are in uppercase
string ToUppercase( const string& text );

// return the text where all utf-8 Yo (\xD0\x81) was replaced by utf-8 Ye (\xD0\x95)
string ReplaceYoWithYe( const string& text );

// return count last utf8 symbols from the text
string Suffix( const string& text, size_t count );

// return true if word starts with punctuation mark
bool StartsWithPunctuationMark( const string& word );

// return true if word ends with punctuation mark
bool EndsWithPunctuationMark( const string& word );

// return true if word starts with digit
bool StartsWithDigit( const string& word );

// return true if word starts with latin letter
bool StartsWithLatinLetter( const string& word );

} // end of Utf8 namespace

//------------------------------------------------------------------------------

#endif
