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
#ifndef DISTRIBUTIONVALIDATOR_H
#define DISTRIBUTIONVALIDATOR_H

#include <QThread>

class QString;

namespace gams {
namespace studio {
namespace support {

class DistributionValidator
    : public QThread
{
    Q_OBJECT

public:
    DistributionValidator(QObject *parent = Q_NULLPTR);

    void run() override;

private:
    ///
    /// \brief Verify the GAMS Distribution bitness on Windows.
    /// \remark On Windows we support both 32 Bit and 64 Bit.
    ///
    void checkBitness();

    ///
    /// \brief Check the GAMS Distribution to GAMS Studio compatibility.
    ///
    void checkCompatibility();
};

}
}
}

#endif // DISTRIBUTIONVALIDATOR_H