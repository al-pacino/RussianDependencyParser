#ifndef __MODEL_H__
#define __MODEL_H__

#include <string>
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

typedef QPair<QString, QString> StringPair;
typedef unsigned char uchar;
typedef unsigned int uint;

class Model {
    public:
        Model(const char *dictdir);
        virtual ~Model();
		bool Train( const string& filename, ostream& out );
		double Test( const string& filename, ostream& out );
		void Print( ostream& out );
        StringPair predict(const QString &prevTag, const QString &curWord);
        QList<StringPair> getTags(const QString &word, QList<ulong> &probs);
		bool Save( const string& filename, ostream& out );
		bool Load( const string& filename, ostream& out );

    private:
        QHash<StringPair, ulong> countTagsPair;
        ulong countWords;

        Paradigm *paradigms;
        string *prefixes;
        string *suffixes;
        string *tags;
        marisa::Trie words;
        marisa::Trie ends;

        QList<StringPair> getNFandTags(const QString& key) const;
        QVector<QPair<QString, uint> > getTagsAndCount(const QString& key) const;
};

#endif
