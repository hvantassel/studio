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
#include <QtDebug>
#include "solveroptiondefinitionmodel.h"

namespace gams {
namespace studio {
namespace option {

SolverOptionDefinitionModel::SolverOptionDefinitionModel(Option *data, int optionGroup, QObject *parent):
    OptionDefinitionModel (data, optionGroup, parent)
{

}

QStringList SolverOptionDefinitionModel::mimeTypes() const
{
    QStringList types;
    types << "application/vnd.solver-opt.text";
    return types;
}

QMimeData *SolverOptionDefinitionModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;
qDebug() << "create mimeData => ...";
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex &index : indexes) {
        if (index.isValid()) {
            if (index.column()>0) {
                continue;
            }

            QString text;
            OptionDefinitionItem *childItem = static_cast<OptionDefinitionItem*>(index.internalPointer());
            OptionDefinitionItem *parentItem = childItem->parentItem();

            if (parentItem == rootItem) {
                QModelIndex defValueIndex = index.sibling(index.row(), OptionDefinitionModel::COLUMN_DEF_VALUE);
                text = QString("%1=%2").arg(data(index, Qt::DisplayRole).toString()).arg(data(defValueIndex, Qt::DisplayRole).toString());
            } else {
                text = QString("%1=%2").arg(parentItem->data(index.column()).toString()).arg(data(index, Qt::DisplayRole).toString());
            }
            qDebug() << "create mimeData => " << text;
            stream << text;
        }
    }

    mimeData->setData("application/vnd.solver-opt.text", encodedData);
    qDebug() << "create mimeData => " << mimeData->formats().at(0);
    return mimeData;
}

void SolverOptionDefinitionModel::modifyOptionDefinitionItem(const SolverOptionItem* optionItem)
{
    QModelIndexList indices = match(index(0, OptionDefinitionModel::COLUMN_ENTRY_NUMBER),
                                             Qt::DisplayRole,
                                             QString::number(optionItem->optionId) , 1);
    beginResetModel();
    for(QModelIndex idx : indices) {
        QModelIndex node = index(idx.row(), OptionDefinitionModel::COLUMN_ENTRY_NUMBER);

        OptionDefinitionItem* nodeItem = static_cast<OptionDefinitionItem*>(node.internalPointer());
        OptionDefinitionItem *parentItem = nodeItem->parentItem();
        if (parentItem == rootItem) {
            OptionDefinition optdef = mOption->getOptionDefinition(nodeItem->data(OptionDefinitionModel::COLUMN_OPTION_NAME).toString());
            optdef.modified = !optionItem->disabled;
            setData(node, optdef.modified ? Qt::CheckState(Qt::Checked) : Qt::CheckState(Qt::Unchecked), Qt::CheckStateRole );
        }
    }
    endResetModel();
}

void SolverOptionDefinitionModel::modifyOptionDefinition(const QList<SolverOptionItem *> &optionItems)
{
    QMap<int, int> modifiedOption;
    for(int i = 0; i<optionItems.size(); ++i) {
        if (optionItems.at(i)->optionId != -1)
            modifiedOption[optionItems.at(i)->optionId] = i;
    }

    QList<int> ids = modifiedOption.keys();
    beginResetModel();
    for(int i=0; i<rowCount(); ++i)  {
        QModelIndex node = index(i, OptionDefinitionModel::COLUMN_ENTRY_NUMBER);

        OptionDefinitionItem* item = static_cast<OptionDefinitionItem*>(node.internalPointer());
        OptionDefinitionItem *parentItem = item->parentItem();
        if (parentItem == rootItem) {
            OptionDefinition optdef = mOption->getOptionDefinition(item->data(OptionDefinitionModel::COLUMN_OPTION_NAME).toString());
            if (ids.contains(optdef.number))
                optdef.modified = true;
            else
                optdef.modified = false;
            setData(node, optdef.modified ? Qt::CheckState(Qt::Checked) : Qt::CheckState(Qt::Unchecked), Qt::CheckStateRole );
        }
    }
    endResetModel();
}


} // namespace option
} // namespace studio
} // namespace gams
