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
#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QtWidgets>

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

namespace gams {
namespace studio {

class TextMark;
class StudioSettings;
class LineNumberArea;
class SearchWidget;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(StudioSettings *settings, QWidget *parent = 0);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    int iconSize();
    LineNumberArea* lineNumberArea();
    QMimeData* createMimeDataFromSelection() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

signals:
    void updateBlockSelection();
    void requestMarkHash(QHash<int, TextMark*>* marks);
    void requestMarksEmpty(bool* marksEmpty);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    friend class BlockEdit;
    void adjustIndent(QTextCursor cursor);
    void truncate(QTextBlock block);
    void duplicateLine();

    void startBlockEdit(int blockNr, int colNr);
    void endBlockEdit();

private:
    LineNumberArea *mLineNumberArea;
    int mCurrentCol;
    StudioSettings *mSettings;
    QPoint mDragStart;

private:
    class BlockEdit
    {
    public:
        BlockEdit(CodeEditor* edit, int blockNr, int colNr);
        virtual ~BlockEdit();
        void keyPressEvent(QKeyEvent *e);
        void keyReleaseEvent(QKeyEvent *e);
        void mouseMoveEvent(QMouseEvent *e);
        void mousePressEvent(QMouseEvent *e);
        void mouseReleaseEvent(QMouseEvent *e);
        int hasBlock(int blockNr) { return mLines.contains(blockNr); }
        int colFrom() { return 0; }
        int colTo() { return 0; }

    private:
    private:
        CodeEditor* mEdit;
        QVector<int> mLines;
        int mColumn = 0;
        int mSize = 0;
    };
    BlockEdit* mBlockEdit = nullptr;

};



class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor) {
        mCodeEditor = editor;
    }

    QSize sizeHint() const override {
        return QSize(mCodeEditor->lineNumberAreaWidth(), 0);
    }
    QHash<int, QIcon> &icons() {
        return mIcons;
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        mCodeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *mCodeEditor;
    QHash<int, QIcon> mIcons;

};


} // namespace studio
} // namespace gams

#endif // CODEEDITOR_H
