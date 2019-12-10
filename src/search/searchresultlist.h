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
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SEARCHRESULTLIST_H
#define SEARCHRESULTLIST_H

#include <QAbstractTableModel>
#include <QRegularExpression>
#include "result.h"

namespace gams {
namespace studio {
namespace search {

class SearchResultList : public QAbstractTableModel
{
    Q_OBJECT
public:
    SearchResultList(QRegularExpression regex);
    virtual ~SearchResultList() override;
    QList<Result> resultsAsList() const;
    QMultiHash<QString, QList<Result> > resultHash() const;
    void addResult(int lineNr, int colNr, int length, QString fileLoc, QString context = "");
    QList<Result> filteredResultList(QString fileLocation);
    QRegularExpression searchRegex();
    void setSearchRegex(QRegularExpression searchRegex);
    int size();
    Result at(int index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QRegularExpression mSearchRegex;
    int mSize = 0;
    QHash<QString, QList<Result>> mResultHash;

};

}
}
}
#endif // SEARCHRESULTLIST_H
