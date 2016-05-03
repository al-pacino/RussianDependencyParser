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

// temporary includes
#include <QTextStream>
#include <QHash>
#include <QtGlobal>
#include <QVector>

using namespace std;

typedef pair<string, string> StringPair;
typedef unsigned char uchar;
typedef unsigned int uint;

class Model {
public:
	Model(const char *dictdir);
	virtual ~Model();

	bool Save( const string& filename, ostream& out ) const;
	bool Load( const string& filename, ostream& out );
	bool Train( const string& filename, ostream& out );
	double Test( const string& filename, ostream& out );
	void Print( ostream& out ) const;
	StringPair Predict( const string& prevTag, const string& curWord );
	QList<StringPair> GetTags( const string& word, QList<uint> &probs );

private:
	struct pair_hash {
		size_t operator () ( const pair<string, string>& p ) const
		{
			return hash<string>()( p.first ) ^ hash<string>()( p.second );
		}
	};

	unordered_map<StringPair, uint, pair_hash> countTagsPair;
	uint countWords;

	Paradigm *paradigms;
	string *prefixes;
	string *suffixes;
	string *tags;
	marisa::Trie words;
	marisa::Trie ends;

	QList<StringPair> getNFandTags( const string& key ) const;
	vector< pair<string, uint> > getTagsAndCount( const string& key ) const;
};

#endif
