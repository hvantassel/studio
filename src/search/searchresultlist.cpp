/*
 * This file is part of the GAMS Studio project.
 *
 * Copyright (c) 2017-2018 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2018 GAMS Development Corp. <support@gams.com>
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
#include "searchresultlist.h"

namespace gams {
namespace studio {

SearchResultList::SearchResultList()
{
}

SearchResultList::SearchResultList(SearchResultList &searchResultList)
    : QAbstractTableModel(searchResultList.parent()),
      mSearchTerm(searchResultList.searchTerm()),
      mResultList(searchResultList.resultList())
{
}

SearchResultList::SearchResultList(const QString &searchTerm, QObject *parent) :
     QAbstractTableModel(parent), mSearchTerm(searchTerm)
{
    useRegex(false); // set default
}

SearchResultList::~SearchResultList()
{
}

QList<Result> SearchResultList::resultList()
{
    return mResultList;
}

void SearchResultList::addResult(int lineNr, int colNr, QString fileLoc, QString context)
{
    mResultList.append(Result(lineNr, colNr, fileLoc, context));
}

void SearchResultList::addResultList(QList<Result> resList)
{
    mResultList.append(resList);
}

QList<Result> SearchResultList::filteredResultList(QString fileLocation)
{
    // TODO(rogo): put something here
    return QList<Result>();
}

QString SearchResultList::searchTerm() const
{
    return mSearchTerm;
}

void SearchResultList::useRegex(bool regex)
{
    mIsRegex = regex;
}

int SearchResultList::size()
{
    return mResultList.size();
}

void SearchResultList::clear()
{
    mResultList.clear();
}

int SearchResultList::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return mResultList.size();
}

int SearchResultList::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 3;
}

QVariant SearchResultList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        int row = index.row();

        Result item = mResultList.at(row);

        switch(index.column())
        {
        case 0: return item.filepath();
        case 1: return item.lineNr();
        case 2: return item.context();
        }
    }
    return QVariant();
}

QVariant SearchResultList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Filename");
            case 1:
                return QString("LineNr");
            case 2:
                return QString("Context");
            }
        }
    }
    return QVariant();
}

void SearchResultList::setSearchTerm(const QString &searchTerm)
{
    mSearchTerm = searchTerm;
}

bool SearchResultList::isRegex() const
{
    return mIsRegex;
}

}
}

