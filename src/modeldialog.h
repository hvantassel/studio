/*
 * This file is part of the GAMS Studio project.
 *
 * Copyright (c) 2017 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017 GAMS Development Corp. <support@gams.com>
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
#ifndef MODELDIALOG_H
#define MODELDIALOG_H

#include "ui_modeldialog.h"

namespace gams {
namespace studio {

class ModelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModelDialog(QWidget *parent = 0);

private slots:
    void on_lineEdit_textChanged(const QString &arg1);

private:
    Ui::ModelDialog ui;
    bool populateTable(QTableWidget *tw, QString glbFile);

    QList<QPair<QTableWidget*, QString>> libraryList;
};

}
}

#endif // MODELDIALOG_H
