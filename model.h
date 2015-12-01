#ifndef __MODEL_H__
#define __MODEL_H__

#include <QTextStream>
#include <QHash>
#include <QtGlobal>
#include <QVector>
#include "paradigm.h"
#include <dawgdic/dictionary.h>

typedef QPair<QString, QString> StringPair;
typedef unsigned char uchar;
typedef unsigned int uint;

class Model {
    public:
        Model(const char *dictdir);
        virtual ~Model();
        bool train(const QString &filename, QTextStream &out);
        double test(const QString &filename, QTextStream &out);
        void print(QTextStream &out);
        StringPair predict(const QString &prevTag, const QString &curWord);
        QList<StringPair> getTags(const QString &word, QList<ulong> &probs);
        bool save(const QString &filename, QTextStream &out);
        bool load(const QString &filename, QTextStream &out);

    private:
        QHash<StringPair, ulong> countTagsPair;
        ulong countWords;

        Paradigm *paradigms;
        char **prefixes;
        char **suffixes;
        char **tags;
        dawgdic::Dictionary words;
        dawgdic::Dictionary ends;

        QList<StringPair> getNFandTags(const QString& key) const;
        QVector<QPair<QString, uint> > getTagsAndCount(const QString& key) const;
};

#endif
