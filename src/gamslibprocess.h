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
#ifndef GAMSLIBPROCESS_H
#define GAMSLIBPROCESS_H

#include "abstractprocess.h"

namespace gams {
namespace studio {

class GAMSLibProcess
        : public AbstractProcess
{
    Q_OBJECT

public:
    GAMSLibProcess(QObject *parent = Q_NULLPTR);

    QString app() override;

    QString nativeAppPath() override;

    void setTargetDir(const QString &targetDir);
    QString targetDir() const;

    void setModelNumber(int modelNumber);
    int modelNumber() const;

    void setModelName(const QString &modelName);
    QString modelName() const;

    void execute() override;

    void setGlbFile(const QString &glbFile);

private:
    QString mApp = "gamslib";
    QString mTargetDir;
    int mModelNumber = -1;
    QString mModelName;
    QString mGlbFile;
};

} // namespace studio
} // namespace gams

#endif // GAMSLIBPROCESS_H