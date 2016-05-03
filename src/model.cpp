#include "model.h"
#include <QString>
#include <QTextCodec>
#include <QByteArray>
#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QList>
#include <QHash>
#include <QDebug>
#include <stdio.h>
#include <fstream>
#include <iostream>

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
	QFile fin( filename.c_str() );

    if (!fin.open(QIODevice::ReadOnly | QIODevice::Text)) {
        out << "ERROR: input file not found" << endl;
        return false;
    }

    QString prevTag = "NONE";

    QTextStream sfin(&fin);
    sfin.setCodec("UTF-8");
    while (!sfin.atEnd()) {
        QString line = sfin.readLine();
        if (line == "----------") {
            prevTag = "NONE";
            continue;
        }

        QStringList words = line.split(" ");
        ++countWords;
		++countTagsPair[StringPair(prevTag.toStdString(), words[2].toStdString())];
        prevTag = words[2];
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
	QList<uint> probs;
	QList<StringPair> variants = GetTags( curWord, probs );
    uint maxVariant = 0;
    if (variants[maxVariant].second == "PNKT" || variants[maxVariant].second == "NUMB" ||
            variants[maxVariant].second == "LATN" || variants[maxVariant].second == "UNKN") {
        return variants[maxVariant];
    }
    Q_ASSERT(probs.size() == variants.size());
    for (int i = 0; i < variants.size(); ++i) {
		if (probs[i] * countTagsPair[StringPair(prevTag.c_str(), variants[i].second)] > probs[maxVariant] * countTagsPair[StringPair(prevTag.c_str(), variants[maxVariant].second)]) {
            maxVariant = i;
        }
    }
    return variants[maxVariant];
}

QList<StringPair> Model::GetTags( const string& word, QList<uint> &probs )
{
    QList<StringPair> result;
    result.clear();
    probs.clear();
    QChar yo = QString::fromUtf8("Ё")[0];
    QChar ye = QString::fromUtf8("Е")[0];
	//result = getNFandTags(QString(word.c_str()).toUpper().replace(yo, ye, Qt::CaseInsensitive).toStdString());
	result = getNFandTags( word );
	if (result.size() > 0) {
        for (QList<StringPair>::iterator i = result.begin(); i != result.end(); ++i) {
            probs.append(1);
        }
        return result;
    }
	if (QChar(word[0]).isPunct()) {
		result.append( StringPair( word, "PNCT" ) );
		probs.append( 1 );
	} else if (QChar(word[0]).isNumber()) {
		result.append( StringPair( word, "NUMB" ) );
		probs.append( 1 );
	} else if (QChar(word[0]).toLatin1() != 0) {
		result.append( StringPair( word, "LATN" ) );
		probs.append( 1 );
    } else {
		vector< pair<string, uint> > predictedTags = getTagsAndCount(QString(word.c_str()).toUpper().right(3).toStdString());
		if( predictedTags.size() == 0 ) {
			result.append( StringPair( word, "UNKN" ) );
			probs.append( 1 );
        }
		for( auto i = predictedTags.cbegin(); i != predictedTags.cend(); ++i ) {
			result.append( StringPair( word, i->first ) );
			probs.append( i->second );
        }
    }
    return result;
}

QList<StringPair> Model::getNFandTags( const string& key ) const
{
	string normalForm = key;
    string hkey = normalForm + " ";
    QList<StringPair> res;
    marisa::Agent agent;
    agent.set_query(hkey.c_str());
    while(words.predictive_search(agent)){
		normalForm = key;
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
        QString nf = QString::fromUtf8(normalForm.c_str());
        QString t = QString::fromUtf8(tags[paradigms[p].getTags(n)].c_str());
		res.append( make_pair( nf.toStdString(), t.toStdString() ) );
    }
    return res;
}

vector< pair<string, uint> > Model::getTagsAndCount( const string& key ) const
{
	QByteArray temp = QString( key.c_str() ).toUtf8();
    string hkey = temp.data();
    hkey += " ";
	vector< pair<string, uint> > res;
    marisa::Agent agent;
    agent.set_query(hkey.c_str());
    while(words.predictive_search(agent)){
        char *buf = new char[agent.key().length() + 1];
        char *b = new char[agent.key().length() + 1];
        int p;
        uint n;
        strncpy(buf, agent.key().ptr(), agent.key().length());
        buf[agent.key().length()] = '\0';
        sscanf(buf, "%s %d %u", b, &p, &n);
        delete [] b;
        delete [] buf;
        QString t = QString::fromUtf8(tags[p].c_str());
		res.push_back( make_pair( t.toStdString(), n ) );
    }
    return res;
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
