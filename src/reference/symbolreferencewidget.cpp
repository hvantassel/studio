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
#include "symbolreferencewidget.h"
#include "ui_symbolreferencewidget.h"

namespace gams {
namespace studio {
namespace reference {

SymbolReferenceWidget::SymbolReferenceWidget(Reference* ref, SymbolDataType::SymbolType type, ReferenceViewer *parent) :
    ui(new Ui::SymbolReferenceWidget),
    mReference(ref),
    mType(type),
    mReferenceViewer(parent)
{
    ui->setupUi(this);

    mSymbolTableModel = new SymbolTableModel(mReference, mType, this);
    ui->symbolView->setModel( mSymbolTableModel );

    ui->symbolView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->symbolView->setSelectionMode(QAbstractItemView::SingleSelection);
    if (mType == SymbolDataType::SymbolType::FileUsed)
        ui->symbolView->sortByColumn(0, Qt::AscendingOrder);
    else
        ui->symbolView->sortByColumn(1, Qt::AscendingOrder);
    ui->symbolView->setSortingEnabled(true);
    ui->symbolView->resizeColumnsToContents();
    ui->symbolView->setAlternatingRowColors(true);

    ui->symbolView->horizontalHeader()->setStretchLastSection(true);
    ui->symbolView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->symbolView->verticalHeader()->setDefaultSectionSize(int(ui->symbolView->fontMetrics().height()*1.4));

    connect(mSymbolTableModel, &SymbolTableModel::symbolSelectionToBeUpdated, this, &SymbolReferenceWidget::updateSymbolSelection);
    connect(ui->symbolView, &QAbstractItemView::doubleClicked, this, &SymbolReferenceWidget::jumpToFile);
    connect(ui->symbolView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SymbolReferenceWidget::updateSelectedSymbol);
    connect(ui->symbolSearchLineEdit, &QLineEdit::textChanged, mSymbolTableModel, &SymbolTableModel::setFilterPattern);
    connect(ui->allColumnToggleSearch, &QCheckBox::toggled, mSymbolTableModel, &SymbolTableModel::toggleSearchColumns);

    mReferenceTreeModel =  new ReferenceTreeModel(mReference, this);
    ui->referenceView->setModel( mReferenceTreeModel );

    ui->referenceView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->referenceView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->referenceView->setItemsExpandable(true);
    ui->referenceView->expandAll();
    ui->referenceView->resizeColumnToContents(0);
    ui->referenceView->resizeColumnToContents(1);
    ui->referenceView->setAlternatingRowColors(true);
    ui->referenceView->setColumnHidden(3, true);

    connect(ui->referenceView, &QAbstractItemView::doubleClicked, this, &SymbolReferenceWidget::jumpToReferenceItem);
    connect( mReferenceTreeModel, &ReferenceTreeModel::modelReset, this, &SymbolReferenceWidget::expandResetModel);
}

SymbolReferenceWidget::~SymbolReferenceWidget()
{
    delete ui;
    delete mSymbolTableModel;
    delete mReferenceTreeModel;
}

void SymbolReferenceWidget::selectSearchField()
{
    ui->symbolSearchLineEdit->setFocus();
}

void SymbolReferenceWidget::updateSelectedSymbol(QItemSelection selected, QItemSelection deselected)
{
    Q_UNUSED(deselected);
    if (selected.indexes().size() > 0) {
        if (mType == SymbolDataType::FileUsed) {
            mCurrentSymbolID = -1;
            QModelIndex idx = mSymbolTableModel->index(selected.indexes().at(0).row(), SymbolTableModel::COLUMN_FILEUSED_NAME);
            mCurrentSymbolSelection = mSymbolTableModel->data( idx ).toString();
            mReferenceTreeModel->resetModel();
        } else {
            QModelIndex idx = mSymbolTableModel->index(selected.indexes().at(0).row(), SymbolTableModel::COLUMN_SYMBOL_ID);
            mCurrentSymbolID = mSymbolTableModel->data( idx ).toInt();
            idx = mSymbolTableModel->index(selected.indexes().at(0).row(), SymbolTableModel::COLUMN_SYMBOL_NAME);
            mCurrentSymbolSelection = mSymbolTableModel->data( idx ).toString();
            mReferenceTreeModel->updateSelectedSymbol( mCurrentSymbolSelection );
        }

    }
}

void SymbolReferenceWidget::expandResetModel()
{
    ui->referenceView->expandAll();
    ui->referenceView->resizeColumnToContents(0);
    ui->referenceView->resizeColumnToContents(1);
}

void SymbolReferenceWidget::resetModel()
{
//    qDebug() << "symrefwidget resetModel::" << mCurrentSymbolID << ":" << mCurrentSymbolSelection;
    mSymbolTableModel->resetModel();
    mReferenceTreeModel->resetModel();

    if (mCurrentSymbolSelection.isEmpty() && mCurrentSymbolID==-1)
        return;

    QModelIndex idx = (mType==SymbolDataType::FileUsed ?
        mSymbolTableModel->index(0, SymbolTableModel::COLUMN_FILEUSED_NAME):
        mSymbolTableModel->index(0, SymbolTableModel::COLUMN_SYMBOL_NAME) );
    QModelIndexList items = mSymbolTableModel->match(idx, Qt::DisplayRole,
                                                     mCurrentSymbolSelection, 1,
                                                     Qt::MatchExactly);
//    ui->symbolView->selectionModel()->clearSelection();
    int i = 0;
    for(QModelIndex itemIdx : items) {
//        qDebug() << i++ << ":match:(" << itemIdx.row() << "," << itemIdx.column() << "):"
//                 << mSymbolTableModel->data(itemIdx).toString();
//        if (QString::localeAwareCompare(mSymbolTableModel->data(itemIdx).toString(), mCurrentSymbolSelection)!=0)
//            continue;
        ui->symbolView->selectionModel()->select(
                    QItemSelection(
                        mSymbolTableModel->index(itemIdx.row(),0),
                        mSymbolTableModel->index(itemIdx.row(),mSymbolTableModel->columnCount()- 1)),
                    QItemSelectionModel::ClearAndSelect);
    }
}

void SymbolReferenceWidget::jumpToFile(const QModelIndex &index)
{
    if (mType == SymbolDataType::FileUsed) {
        ReferenceItem item(-1, ReferenceDataType::Unknown, ui->symbolView->model()->data(index.sibling(index.row(), 0)).toString(), 0, 0);
        emit mReferenceViewer->jumpTo( item );
    }
}

void SymbolReferenceWidget::jumpToReferenceItem(const QModelIndex &index)
{
    QModelIndex  parentIndex =  ui->referenceView->model()->parent(index);
    if (parentIndex.row() >= 0) {
        QVariant location = ui->referenceView->model()->data(index.sibling(index.row(), 0), Qt::UserRole);
        QVariant lineNumber = ui->referenceView->model()->data(index.sibling(index.row(), 1), Qt::UserRole);
        QVariant colNumber = ui->referenceView->model()->data(index.sibling(index.row(), 2), Qt::UserRole);
        QVariant typeName = ui->referenceView->model()->data(index.sibling(index.row(), 3), Qt::UserRole);
        ReferenceItem item(-1, ReferenceDataType::typeFrom(typeName.toString()), location.toString(), lineNumber.toInt(), colNumber.toInt());
        emit mReferenceViewer->jumpTo( item );
    }
}

void SymbolReferenceWidget::updateSymbolSelection()
{
    int updatedSelectedRow = mSymbolTableModel->getSortedIndexOf( mCurrentSymbolSelection );
    ui->symbolView->selectionModel()->clearCurrentIndex();
    ui->symbolView->selectionModel()->clearSelection();
    if (updatedSelectedRow >= 0 && updatedSelectedRow < ui->symbolView->model()->rowCount()) {
        QModelIndex topLeftIdx = mSymbolTableModel->index( updatedSelectedRow, 0 );
        QModelIndex bottomRightIdx = mSymbolTableModel->index( updatedSelectedRow, mSymbolTableModel->columnCount()-1 );
        ui->symbolView->selectionModel()->select( QItemSelection(topLeftIdx, bottomRightIdx), QItemSelectionModel::Select);
    } else {
         mReferenceTreeModel->resetModel();
    }
}

} // namespace reference
} // namespace studio
} // namespace gams
