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
#include "gotodialog.h"
#include "ui_gotodialog.h"

#include <QIntValidator>
#include <QPainter>

namespace gams {
namespace studio {

GoToDialog::GoToDialog(QWidget *parent, int maxLines, bool wait)
    : QDialog(parent),
      ui(new Ui::GoToDialog),
      mMaxLines(qAbs(maxLines)),
      mWait(wait)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    ui->lineEdit->setPlaceholderText(QString::number(maxLines));
    int min = parent->fontMetrics().width(QString::number(maxLines)+"0");
    ui->lineEdit->setMinimumWidth(min);
    connect(ui->lineEdit, &QLineEdit::editingFinished, this, &GoToDialog::on_goToButton_clicked);
}

GoToDialog::~GoToDialog()
{
    delete ui;
}

int GoToDialog::lineNumber() const
{
    return mLineNumber;
}

void GoToDialog::on_goToButton_clicked()
{
    mLineNumber = (ui->lineEdit->text().toInt())-1;
    if (!mWait && mLineNumber > mMaxLines)
        ui->lineEdit->setText(QString::number(mMaxLines));
    if (mLineNumber <= mMaxLines)
        accept();
    mWait = false;
}

}
}
