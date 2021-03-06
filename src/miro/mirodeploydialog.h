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
#ifndef MIRODEPLOYDIALOG_H
#define MIRODEPLOYDIALOG_H

#include <QDialog>

#include "mirodeployprocess.h"
#include "mirocommon.h"

namespace gams {
namespace studio {
namespace miro {

namespace Ui {
class MiroDeployDialog;
}

class MiroDeployDialog : public QDialog
{
    Q_OBJECT

public:
    MiroDeployDialog(QWidget *parent = nullptr);

    bool baseMode() const;
    bool hypercubeMode() const;
    MiroTargetEnvironment targetEnvironment();

    void setDefaults();

    void setModelAssemblyFile(const QString &file) {
        mModelAssemblyFile = file;
    }

signals:
    void testDeploy(bool testDeploy, MiroDeployMode mode);

private slots:
    void on_testBaseButton_clicked();
    void on_testHcubeButton_clicked();
    void on_deployButton_clicked();

    void updateTestDeployButtons();

private:
    bool showMessageBox();

private:
    Ui::MiroDeployDialog *ui;
    QString mModelAssemblyFile;
};

}
}
}

#endif // MIRODEPLOYDIALOG_H
