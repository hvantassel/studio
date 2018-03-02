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
#ifndef GAMSINFO_H
#define GAMSINFO_H

#include <QString>

namespace gams {
namespace studio {

class GAMSPaths
{
public:
    ///
    /// \brief Get GAMS system directory.
    /// \return Returns the GAMS system directory.
    /// \remark If GAMS Studio is part of the GAMS distribution a relateive
    ///         path based on the executable location is returned;
    ///         otherwise the PATH environment variable used to find GAMS.
    ///
    static QString systemDir();

    static QString defaultWorkingDir();

    static QString userDocumentsDir();

    static QString userModelLibraryDir();

private:
    GAMSPaths();
};

}
}

#endif // GAMSINFO_H