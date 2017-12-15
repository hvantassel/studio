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
#ifndef TEXTMARK_H
#define TEXTMARK_H

#include <QtWidgets>

namespace gams {
namespace studio {

class FileContext;

class TextMark
{
public:
    enum Type {none, error, link, bookmark, all};

    explicit TextMark(TextMark::Type tmType);
    void setPosition(FileContext* fileContext, int line, int column, int size = 0);
    void updateCursor();
    void jumpToRefMark(bool focus = true);
    void jumpToMark(bool focus = true);
    void setRefMark(TextMark* refMark);
    inline bool isErrorRef() {return mReference && mReference->type() == error;}

    void showToolTip();

    int value() const;
    void setValue(int value);

    QIcon icon();
    inline Type type() const {return mType;}
    Qt::CursorShape& cursorShape(Qt::CursorShape* shape, bool inIconRegion = false);
    inline bool isValid() {return mFileContext && (mLine>=0) && (mColumn>=0);}
    inline bool isValidLink(bool inIconRegion = false)
    { return mReference && ((mType == error && inIconRegion) || mType == link); }
    inline QTextBlock textBlock();
    QTextCursor textCursor() const;
    inline int in(int pos, int len) {
        if (mCursor.isNull()) return -2;
        return (mCursor.position() < pos) ? -1 : (mCursor.position() >= pos+len) ? 1 : 0;
    }

    inline int line() const {return (mCursor.isNull()) ? mLine : mCursor.block().blockNumber();}
    inline int column() const {return mColumn;}
    inline int size() const {return mSize;}
    inline bool inColumn(int col) const {return !mSize || (col >= mColumn && col < (mColumn+mSize));}
    inline int position() const {return (mCursor.isNull()) ? -1 : mCursor.position();}
    inline int blockStart() const {return (mCursor.isNull()) ? -1 : mCursor.selectionStart()-mCursor.block().position();}
    inline int blockEnd() const {return (mCursor.isNull()) ? -1 : mCursor.selectionEnd()-mCursor.block().position();}
    void rehighlight();
    void modified();

    QString dump();

private:
    FileContext* mFileContext = nullptr;
    Type mType = none;
    int mLine = -1;
    int mColumn = 0;
    int mSize = 0;
    int mValue = 0;
    QTextCursor mCursor;
    Qt::CursorShape *mCursorShape = nullptr;
    TextMark* mReference = nullptr;
};

} // namespace studio
} // namespace gams

#endif // TEXTMARK_H
