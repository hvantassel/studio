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
#include <QDir>
#include "file.h"
#include "gamsprocess.h"
#include "lxiviewer.h"
#include "lxiparser.h"
#include "lxitreemodel.h"
#include "lxitreeitem.h"
#include "editors/textview.h"
#include "exception.h"
#include "ui_lxiviewer.h"
#include "file/projectgroupnode.h"

namespace gams {
namespace studio {
namespace lxiviewer {

LxiViewer::LxiViewer(TextView *textView, const QString &lstFile, QWidget *parent):
    QWidget(parent),
    ui(new Ui::LxiViewer),
    mTextView(textView)
{
    ui->setupUi(this);

    ui->splitter->addWidget(mTextView);

    QFileInfo info(lstFile);
    mLxiFile = info.path() + "/" + info.baseName() + ".lxi";

    loadLxi();
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 3);
    setFocusProxy(ui->lxiTreeView);

    connect(ui->lxiTreeView, &QTreeView::activated, this, &LxiViewer::jumpToLine);
    connect(ui->lxiTreeView, &QTreeView::doubleClicked, this, &LxiViewer::jumpToLine);
    connect(mTextView, &TextView::selectionChanged, this, &LxiViewer::jumpToTreeItem);
}

LxiViewer::~LxiViewer()
{
    LxiTreeModel* oldModel = static_cast<LxiTreeModel*>(ui->lxiTreeView->model());
    if (oldModel)
        delete oldModel;
    delete ui;
}

TextView *LxiViewer::textView() const
{
    return mTextView;
}

void LxiViewer::loadLxi()
{
    if (QFileInfo(mLxiFile).exists() && QFileInfo(mLxiFile).size() > 0) {
        ui->splitter->widget(0)->show();
        LxiTreeModel* model = LxiParser::parseFile(mLxiFile);
        LxiTreeModel* oldModel = static_cast<LxiTreeModel*>(ui->lxiTreeView->model());
        ui->lxiTreeView->setModel(model);
        if (oldModel)
            delete oldModel;
    }
    else
        ui->splitter->widget(0)->hide();
}

void LxiViewer::jumpToTreeItem()
{
    if (ui->splitter->widget(0)->isHidden())
        return;

    int lineNr  = mTextView->position().y();
    if (lineNr < 0) return; // negative lineNr is estimated

    LxiTreeModel* lxiTreeModel = static_cast<LxiTreeModel*>(ui->lxiTreeView->model());
    if (!lxiTreeModel) return;
    int itemIdx = 0;

    if (lineNr >= lxiTreeModel->lineNrs().first()) {
        itemIdx=1;
        while (lxiTreeModel->lineNrs().size()>itemIdx && lineNr >= lxiTreeModel->lineNrs()[itemIdx])
            itemIdx++;
        itemIdx--;
    }

    LxiTreeItem* treeItem = lxiTreeModel->treeItems()[itemIdx];
    if (!ui->lxiTreeView->isExpanded(treeItem->parentItem()->modelIndex()))
        ui->lxiTreeView->expand(treeItem->parentItem()->modelIndex());
    ui->lxiTreeView->selectionModel()->select(treeItem->modelIndex(), QItemSelectionModel::SelectCurrent);
    ui->lxiTreeView->scrollTo(treeItem->modelIndex());
}

void LxiViewer::jumpToLine(const QModelIndex &modelIndex)
{
    LxiTreeItem* selectedItem = static_cast<LxiTreeItem*>(modelIndex.internalPointer());
    int lineNr = selectedItem->lineNr();

    //jump to first child for virtual nodes
    if (lineNr == -1) {
        if (!ui->lxiTreeView->isExpanded(modelIndex)) {
            QModelIndex mi = modelIndex.model()->index(0, 0, modelIndex);
            selectedItem = static_cast<LxiTreeItem*>(mi.internalPointer());
            lineNr = selectedItem->lineNr();
        }
        else
            return;
    }
    disconnect(mTextView, &TextView::selectionChanged, this, &LxiViewer::jumpToTreeItem);
    mTextView->jumpTo(lineNr-1, 0);
    connect(mTextView, &TextView::selectionChanged, this, &LxiViewer::jumpToTreeItem);
}

} // namespace lxiviewer
} // namespace studio
} // namespace gams
