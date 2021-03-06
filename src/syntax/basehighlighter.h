/*
 * This file is part of the GAMS Studio project.
 *
 * Copyright (c) 2017-2019 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2019 GAMS Development Corp. <support@gams.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef BASEHIGHLIGHTER_H
#define BASEHIGHLIGHTER_H

#include <QObject>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextObject>
#include <QVector>
#include <QTime>

namespace gams {
namespace studio {
namespace syntax {

class BaseHighlighter : public QObject
{
    Q_OBJECT
public:
    explicit BaseHighlighter(QObject *parent = nullptr);
    explicit BaseHighlighter(QTextDocument *parent = nullptr);
    virtual ~BaseHighlighter();
    void abortHighlighting();

    void setDocument(QTextDocument *doc, bool wipe = false);
    QTextDocument *document() const;

public slots:
    void rehighlight();
    void rehighlightBlock(const QTextBlock &startBlock);

private slots:
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void blockCountChanged(int newBlockCount);
    void processDirtyParts();

protected:
    virtual void highlightBlock(const QString &text) = 0;
    void setFormat(int start, int count, const QTextCharFormat &format);
    QTextCharFormat format(int pos) const;

    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int newState);

    void setCurrentBlockUserData(QTextBlockUserData *data);
    QTextBlockUserData *currentBlockUserData() const;

    QTextBlock currentBlock() const;

private:
    void reformatCurrentBlock();
    void applyFormatChanges();
    QTextBlock nextDirty();
    void setDirty(QTextBlock fromBlock, QTextBlock toBlock);
    void setClean(QTextBlock block);
    inline int dirtyIndex(int blockNr) {
        for (int i = 0; i < mDirtyBlocks.size(); ++i) {
            if (mDirtyBlocks.at(i).first > blockNr || mDirtyBlocks.at(i).second > blockNr)
                return i;
        }
        return mDirtyBlocks.size();
    }
    inline QString debugDirty() {
        QString s;
        for (int i = 0; i < mDirtyBlocks.size() ; ++i) {
            s.append(QString::number(mDirtyBlocks.at(i).first)+"-"+QString::number(mDirtyBlocks.at(i).second)+" ");
        }
        return s;
    }

private:
    class Interval : public QPair<int,int>  {
        void set(const QTextBlock &block, QTextBlock &bVal, int &val) {
            if (!block.isValid()) {
                bVal = QTextBlock();
                val = 0;
            } else {
                bVal = block;
                val = block.blockNumber();
            }
        }
    public:
        Interval(QTextBlock firstBlock = QTextBlock(), QTextBlock secondBlock = QTextBlock());
        virtual ~Interval() {}
        void setFirst(QTextBlock block = QTextBlock()) { set(block, bFirst, first); }
        void setSecond(QTextBlock block = QTextBlock()) { set(block, bSecond, second); }
        bool isValid() const { return !bFirst.isValid() || !bSecond.isValid() || second < first; }
        bool updateFromBlocks();
        bool extendOverlap(const Interval &other);
        Interval setSplit(QTextBlock &block);
        QTextBlock bFirst;  // backup for changes
        QTextBlock bSecond; // backup for changes
    };

//    class BInterval : public QPair<QTextBlock, QTextBlock>  {
//    public:
//        BInterval(QTextBlock first = QTextBlock(), QTextBlock second = QTextBlock())
//            : QPair<QTextBlock, QTextBlock>(qMin(first, second), qMax(first, second)) {}
//        bool isEmpty() const { return !first.isValid() || !second.isValid() || second < first; }
//        bool extendOverlap(const BInterval &other);
//        int iFirst() const {return first.isValid() ? first.blockNumber() : -1;}
//        int iSecond() const {return second.isValid() ? second.blockNumber() : -1;}
//        BInterval subtractOverlap(const BInterval &other);
//        virtual ~BInterval() {}
//    };

    QTime mTime;
    bool mAborted = false;
    QTextDocument *mDoc = nullptr;
    int mBlockCount = 1;
    QTextBlock mCurrentBlock;
    QVector<Interval> mDirtyBlocks;             // disjoint regions of dirty blocks
    QVector<QTextCharFormat> mFormatChanges;

};

} // namespace syntax
} // namespace studio
} // namespace gams

#endif // BASEHIGHLIGHTER_H
