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
#ifndef SYNTAXIDENTIFIER_H
#define SYNTAXIDENTIFIER_H

#include "syntaxformats.h"

namespace gams {
namespace studio {
namespace syntax {

class SyntaxIdentifier : public SyntaxAbstract
{
public:
    SyntaxIdentifier(SyntaxKind kind);
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
private:
    int identChar(const QChar &c) const;
};

class SyntaxIdentifierDim : public SyntaxAbstract
{
    QChar mDelimiterIn;
    QChar mDelimiterOut;
    bool mTable;
public:
    SyntaxIdentifierDim(SyntaxKind kind);
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
    int maxNesting() override { return 1; }
};

class SyntaxIdentifierDimEnd : public SyntaxAbstract
{
    QChar mDelimiter;
    bool mTable;
public:
    SyntaxIdentifierDimEnd(SyntaxKind kind);
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
};

class SyntaxIdentDescript : public SyntaxAbstract
{
    bool mTable;
public:
    SyntaxIdentDescript(SyntaxKind kind);
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
};

class SyntaxIdentAssign : public SyntaxAbstract
{
    QChar mDelimiter;
public:
    SyntaxIdentAssign(SyntaxKind kind);
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
};

class AssignmentLabel: public SyntaxAbstract
{
public:
    AssignmentLabel();
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
};

class AssignmentValue: public SyntaxAbstract
{
public:
    AssignmentValue();
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
};

class SyntaxTableAssign : public SyntaxAbstract
{
public:
    SyntaxTableAssign(SyntaxKind kind);
    SyntaxBlock find(const SyntaxKind entryKind, const QString &line, int index) override;
    SyntaxBlock validTail(const QString &line, int index, bool &hasContent) override;
};


} // namespace syntax
} // namespace studio
} // namespace gams

#endif // SYNTAXIDENTIFIER_H
