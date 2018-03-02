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
#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <QWidget>
#include "mainwindow.h"

namespace Ui {
class WelcomePage;
}

namespace gams {
namespace studio {

class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(HistoryData *history, MainWindow *parent = 0);
    void historyChanged(HistoryData *history);
    ~WelcomePage();

private:
    Ui::WelcomePage *ui;
    QList<QLabel*> mFileHistory;
    MainWindow *mMain;

signals:
    void linkActivated(const QString &link);
    void relayActionWp(QString action);
    void relayModLibLoad(QString lib);

public slots:
    void on_relayAction(QString action);
    void on_relayModLibLoad(QString lib);


    // QWidget interface
protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
};

}
}

#endif // WELCOMEPAGE_H