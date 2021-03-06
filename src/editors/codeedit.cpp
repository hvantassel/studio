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
#include <QtWidgets>
#include "editors/codeedit.h"
#include "studiosettings.h"
#include "search/searchdialog.h"
#include "exception.h"
#include "logger.h"
#include "syntax.h"
#include "keys.h"
#include "editorhelper.h"
#include "viewhelper.h"
#include "search/searchlocator.h"
#include "settingslocator.h"

namespace gams {
namespace studio {

inline const KeySeqList &hotkey(Hotkey _hotkey) { return Keys::instance().keySequence(_hotkey); }

CodeEdit::CodeEdit(QWidget *parent)
    : AbstractEdit(parent)
{
    mLineNumberArea = new LineNumberArea(this);
    mLineNumberArea->setMouseTracking(true);
    mBlinkBlockEdit.setInterval(500);
    mWordDelay.setSingleShot(true);
    mParenthesesDelay.setSingleShot(true);
    mSettings = SettingsLocator::settings();

    connect(&mBlinkBlockEdit, &QTimer::timeout, this, &CodeEdit::blockEditBlink);
    connect(&mWordDelay, &QTimer::timeout, this, &CodeEdit::updateExtraSelections);
    connect(&mParenthesesDelay, &QTimer::timeout, this, &CodeEdit::updateExtraSelections);
    connect(this, &CodeEdit::blockCountChanged, this, &CodeEdit::updateLineNumberAreaWidth);
    connect(this, &CodeEdit::updateRequest, this, &CodeEdit::updateLineNumberArea);
    connect(this, &CodeEdit::cursorPositionChanged, this, &CodeEdit::recalcExtraSelections);
    connect(this, &CodeEdit::textChanged, this, &CodeEdit::recalcExtraSelections);
    connect(this->verticalScrollBar(), &QScrollBar::actionTriggered, this, &CodeEdit::updateExtraSelections);
    connect(document(), &QTextDocument::undoCommandAdded, this, &CodeEdit::undoCommandAdded);

    setMouseTracking(true);
    viewport()->setMouseTracking(true);
}

CodeEdit::~CodeEdit()
{
    while (mBlockEditPos.size())
        delete mBlockEditPos.takeLast();
}

int CodeEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 0;

    if (showLineNr())
        space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    if (marks() && marks()->hasVisibleMarks()) {
        space += iconSize();
        mIconCols = 1;
    } else {
        mIconCols = 0;
    }

    return space;
}

int CodeEdit::iconSize()
{
    return fontMetrics().height()-3;
}

LineNumberArea* CodeEdit::lineNumberArea()
{
    return mLineNumberArea;
}

void CodeEdit::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    if (viewportMargins().left() != mLnAreaWidth) {
        mLineNumberArea->repaint();
        mLnAreaWidth = viewportMargins().left();
    }
}

void CodeEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        mLineNumberArea->scroll(0, dy);
    } else {
        int top = rect.y();
        int bottom = top + rect.height();
        // NOTE! major performance issue on calling :blockBoundingGeometry()
        mLineNumberArea->update(0, top, mLineNumberArea->width(), bottom-top);
    }

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEdit::blockEditBlink()
{
    if (mBlockEdit) mBlockEdit->refreshCursors();
}

void CodeEdit::checkBlockInsertion()
{
    bool extraJoin = mBlockEditInsText.isNull();
    QTextCursor cur = textCursor();
    bool validText = (mBlockEditRealPos != cur.position());
    if (validText) {
        cur.setPosition(mBlockEditRealPos, QTextCursor::KeepAnchor);
        mBlockEditInsText = cur.selectedText();
        cur.removeSelectedText();
        if (!extraJoin) cur.endEditBlock();
        mBlockEdit->replaceBlockText(mBlockEditInsText);
        mBlockEditInsText = "";
    }
    if (!validText || extraJoin) {
        cur.endEditBlock();
    }
    mBlockEditRealPos = -1;
}

void CodeEdit::undoCommandAdded()
{
    while (document()->availableUndoSteps()-1 < mBlockEditPos.size())
        delete mBlockEditPos.takeLast();
    while (document()->availableUndoSteps() > mBlockEditPos.size()) {
        BlockEditPos *bPos = nullptr;
        if (mBlockEdit) bPos = new BlockEditPos(mBlockEdit->startLine(), mBlockEdit->currentLine(), mBlockEdit->column());
        mBlockEditPos.append(bPos);
    }
}

void CodeEdit::extendedRedo()
{
    if (mBlockEdit) endBlockEdit();
    redo();
    updateBlockEditPos();
}

void CodeEdit::extendedUndo()
{
    if (mBlockEdit) endBlockEdit();
    undo();
    updateBlockEditPos();
}

void CodeEdit::convertToLower()
{
    if (isReadOnly()) return;

    textCursor().insertText(textCursor().selectedText().toLower());
    if (mBlockEdit) {
        QStringList lowerLines = mBlockEdit->blockText().toLower()
                                 .split("\n", QString::SplitBehavior::SkipEmptyParts);
        mBlockEdit->replaceBlockText(lowerLines);
    }
}

void CodeEdit::convertToUpper()
{
    if (isReadOnly()) return;

    textCursor().insertText(textCursor().selectedText().toUpper());
    if (mBlockEdit) {
        QStringList lowerLines = mBlockEdit->blockText().toUpper()
                                 .split("\n", QString::SplitBehavior::SkipEmptyParts);
        mBlockEdit->replaceBlockText(lowerLines);
    }
}

void CodeEdit::updateBlockEditPos()
{
    if (document()->availableUndoSteps() <= 0 || document()->availableUndoSteps() > mBlockEditPos.size())
        return;
//    debugUndoStack(mBlockEditPos, document()->availableUndoSteps()-1);
    BlockEditPos * bPos = mBlockEditPos.at(document()->availableUndoSteps()-1);
    if (mBlockEdit) endBlockEdit();
    if (bPos && !mBlockEdit && mAllowBlockEdit) {
        startBlockEdit(bPos->startLine, bPos->column);
        mBlockEdit->selectTo(bPos->currentLine, bPos->column);
    }
}

QString CodeEdit::wordUnderCursor() const
{
    return mWordUnderCursor;
}

bool CodeEdit::hasSelection() const
{
    return textCursor().hasSelection();
}

void CodeEdit::disconnectTimers()
{
    AbstractEdit::disconnectTimers();
    disconnect(&mWordDelay, &QTimer::timeout, this, &CodeEdit::updateExtraSelections);
    disconnect(&mParenthesesDelay, &QTimer::timeout, this, &CodeEdit::updateExtraSelections);
}

void CodeEdit::clearSelection()
{
    if (isReadOnly()) return;
    if (mBlockEdit && !mBlockEdit->blockText().isEmpty()) {
        mBlockEdit->replaceBlockText(QStringList()<<QString());
    } else {
        textCursor().clearSelection();
    }
}

void CodeEdit::cutSelection()
{
    if (mBlockEdit && !mBlockEdit->blockText().isEmpty()) {
        mBlockEdit->selectionToClipboard();
        mBlockEdit->replaceBlockText(QStringList()<<QString());
    } else {
        cut();
    }
}

void CodeEdit::copySelection()
{
    if (mBlockEdit && !mBlockEdit->blockText().isEmpty()) {
        mBlockEdit->selectionToClipboard();
    } else {
        copy();
    }
}

void CodeEdit::selectAllText()
{
    selectAll();
}

void CodeEdit::pasteClipboard()
{
    bool isBlock;
    QStringList texts = clipboard(&isBlock);
    if (!mBlockEdit) {
        if (isBlock && mAllowBlockEdit) {
            QTextCursor c = textCursor();
            if (c.hasSelection()) c.removeSelectedText();
            startBlockEdit(c.blockNumber(), c.columnNumber());
            mBlockEdit->replaceBlockText(texts);
        } else {
            paste();
        }
    } else {
        mBlockEdit->replaceBlockText(texts);
    }
}

void CodeEdit::resizeEvent(QResizeEvent *e)
{
    AbstractEdit::resizeEvent(e);

    QRect cr = contentsRect();
    mLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    updateLineNumberAreaWidth();
    updateExtraSelections();
}

void CodeEdit::keyPressEvent(QKeyEvent* e)
{
    if (!mBlockEdit && mAllowBlockEdit && e == Hotkey::BlockEditStart) {
        QTextCursor c = textCursor();
        QTextCursor anc = c;
        anc.setPosition(c.anchor());
        startBlockEdit(anc.blockNumber(), anc.columnNumber());
    }
    e->ignore();
    if (mBlockEdit) {
        if (e == Hotkey::SelectAll) {
            endBlockEdit();
        } else if (e->key() == Hotkey::NewLine || e == Hotkey::BlockEditEnd) {
            endBlockEdit();
            return;
        } else {
            mBlockEdit->keyPressEvent(e);
            return;
        }
    }
    QTextCursor cur = textCursor();
    if (e == Hotkey::MatchParentheses || e == Hotkey::SelectParentheses) {
        ParenthesesMatch pm = matchParentheses();
        bool sel = (e == Hotkey::SelectParentheses);
        QTextCursor::MoveMode mm = sel ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        if (pm.match >= 0) {
            if (sel) cur.clearSelection();
            if (cur.position() != pm.pos) cur.movePosition(QTextCursor::Left);
            cur.setPosition(pm.match+1, mm);
            e->accept();
        }
    } else if (e == Hotkey::MoveToEndOfLine) {
        QTextCursor::MoveMode mm = (e->modifiers() & Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        cur.movePosition(QTextCursor::EndOfLine, mm);
        e->accept();
    } else if (e == Hotkey::MoveToStartOfLine) {
        QTextBlock block = cur.block();
        QTextCursor::MoveMode mm = QTextCursor::MoveAnchor;

        if (e->modifiers() & Qt::ShiftModifier)
            mm = QTextCursor::KeepAnchor;

        QRegularExpression leadingSpaces("^(\\s*)");
        QRegularExpressionMatch lsMatch = leadingSpaces.match(block.text());

        if (cur.positionInBlock()==0 || lsMatch.capturedLength(1) < cur.positionInBlock())
            cur.setPosition(block.position() + lsMatch.capturedLength(1), mm);
        else cur.setPosition(block.position(), mm);
        e->accept();
    } else if (e == Hotkey::MoveCharGroupRight || e == Hotkey::SelectCharGroupRight) {
        QTextCursor::MoveMode mm = (e == Hotkey::SelectCharGroupRight) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        int p = cur.positionInBlock();
        EditorHelper::nextWord(0, p, cur.block().text());
        if (p >= cur.block().length()) {
            QTextBlock block = cur.block().next();
            if (block.isValid()) cur.setPosition(block.position(), mm);
            else cur.movePosition(QTextCursor::EndOfBlock, mm);
        } else {
            cur.setPosition(cur.block().position() + p, mm);
        }
        e->accept();
    } else if (e == Hotkey::MoveCharGroupLeft || e == Hotkey::SelectCharGroupLeft) {
        QTextCursor::MoveMode mm = (e == Hotkey::SelectCharGroupLeft) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        int p = cur.positionInBlock();
        if (p == 0) {
            QTextBlock block = cur.block().previous();
            if (block.isValid()) cur.setPosition(block.position()+block.length()-1, mm);
        } else {
            EditorHelper::prevWord(0, p, cur.block().text());
            cur.setPosition(cur.block().position() + p, mm);
        }
        e->accept();
    }
    if (e->isAccepted()) {
        setTextCursor(cur);
        return;
    }

    if (!isReadOnly()) {
        if (e->key() == Hotkey::NewLine) {
            QTextCursor cursor = textCursor();
            int pos = cursor.positionInBlock();
            cursor.beginEditBlock();
            QString leadingText = cursor.block().text().left(pos).trimmed();
            cursor.insertText("\n");
            if (!leadingText.isEmpty()) {
                if (cursor.block().previous().isValid())
                    truncate(cursor.block().previous());
                adjustIndent(cursor);
            }
            cursor.endEditBlock();
            setTextCursor(cursor);
            e->accept();
            return;
        }
        if (e == Hotkey::Indent) {
            indent(mSettings->tabSize());
            e->accept();
            return;
        }
        if (e == Hotkey::Outdent) {
            indent(-mSettings->tabSize());
            e->accept();
            return;
        }
        if (mSettings->autoIndent() && e->key() == Qt::Key_Backspace) {
            int pos = textCursor().positionInBlock();

            QString line = textCursor().block().text();
            QRegularExpression regex("^(\\s+)");
            QRegularExpressionMatch match = regex.match(line);
            bool allWhitespace = match.hasMatch();

            if (allWhitespace && !textCursor().hasSelection() && match.capturedLength() == pos) {
                indent(-mSettings->tabSize());
                e->accept();
                return;
            }
        }
    }

    if (e == Hotkey::SearchFindPrev)
        emit searchFindPrevPressed();
    else if (e == Hotkey::SearchFindNext)
        emit searchFindNextPressed();

    // smart typing:
    if (SettingsLocator::settings()->autoCloseBraces() && !isReadOnly())  {
        QSet<int> moveKeys;
        moveKeys << Qt::Key_Home << Qt::Key_End << Qt::Key_Down << Qt::Key_Up
                 << Qt::Key_Left << Qt::Key_Right << Qt::Key_PageUp << Qt::Key_PageDown;
        // deactivate when manual cursor movement was detected
        if (moveKeys.contains(e->key())) mSmartType = false;

        int index = mOpening.indexOf(e->text());
        int indexClosing = mClosing.indexOf(e->text());

        // exclude modifier combinations
        if (e->text().isEmpty()) {
            index = -1;
            indexClosing = -1;
        }

        // surround selected text with characters
        if ((index != -1) && (textCursor().hasSelection())) {
            QTextCursor tc(textCursor());
            QString selection(tc.selectedText());
            selection = mOpening.at(index) + selection + mClosing.at(index);
            tc.insertText(selection);
            setTextCursor(tc);
            return;

        // jump over closing character if already in place
        } else if (mSmartType && indexClosing != -1 &&
                   mClosing.indexOf(document()->characterAt(textCursor().position())) == indexClosing) {
            QTextCursor tc = textCursor();
            tc.movePosition(QTextCursor::NextCharacter);
            setTextCursor(tc);
            e->accept();
            return;

        // insert closing characters
        } else if (index != -1 && allowClosing(index)) {
            mSmartType = true;
            QTextCursor tc = textCursor();
            tc.insertText(e->text());
            tc.insertText(mClosing.at(index));
            tc.movePosition(QTextCursor::PreviousCharacter);
            setTextCursor(tc);
            e->accept();
            return;
        }

        if (mSmartType && e->key() == Qt::Key_Backspace) {

            QChar a = document()->characterAt(textCursor().position()-1);
            QChar b = document()->characterAt(textCursor().position());

            // ( is opening char )       && (char before and after cursor are identical)
            if (mOpening.indexOf(a) != -1 && (mOpening.indexOf(a) ==  mClosing.indexOf(b))) {
                textCursor().deleteChar();
                textCursor().deletePreviousChar();
                e->accept();

                // check cursor surrounding characters again
                a = document()->characterAt(textCursor().position()-1);
                b = document()->characterAt(textCursor().position());

                // keep smarttype on conditionally; e.g. for ((|)); qt creator does this, too
                mSmartType = (mOpening.indexOf(a) == mClosing.indexOf(b));
                return;
            }
        }
    }

    AbstractEdit::keyPressEvent(e);
}

bool CodeEdit::allowClosing(int chIndex)
{
    QString allowingChars(",;){}] ");
    bool nextAllows = allowingChars.indexOf(document()->characterAt(textCursor().position())) != -1;
    bool nextLinebreak = textCursor().positionInBlock() == textCursor().block().length()-1;

    // insert closing if next char permits or is end of line
    bool allowAutoClose =  nextAllows || nextLinebreak;

    // if char before and after the cursor are a matching pair: allow
    QChar prior = document()->characterAt(textCursor().position() - 1);
    bool matchingPairExisting = mOpening.indexOf(prior) == mClosing.indexOf(document()->characterAt(textCursor().position()));

    // next is allowed char && if brackets are there and matching && no quotes after letters or numbers
    return allowAutoClose && matchingPairExisting && (!prior.isLetterOrNumber() || chIndex < 3);
}

void CodeEdit::keyReleaseEvent(QKeyEvent* e)
{
    // return pressed: ignore here
    if (!isReadOnly() && e->key() == Hotkey::NewLine) {
        e->accept();
        return;
    }
    AbstractEdit::keyReleaseEvent(e);
}

void CodeEdit::adjustIndent(QTextCursor cursor)
{
    if (!mSettings->autoIndent()) return;

    QRegularExpression rex("^(\\s*).*$");
    QRegularExpressionMatch match = rex.match(cursor.block().text());
    if (match.hasMatch()) {
        QTextBlock prev = cursor.block().previous();
        QRegularExpression pRex("^((\\s+)|([^\\s]+))");
        QRegularExpressionMatch pMatch = pRex.match(prev.text());
        while (true) {
            if (pMatch.hasMatch()) {
                if (pMatch.capturedLength(2) < 1)
                    break;
                QString spaces = pMatch.captured(2);
                cursor.setPosition(cursor.position() + match.capturedLength(1), QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                cursor.insertText(spaces);
                setTextCursor(cursor);
                break;
            }
            prev = prev.previous();
            if (!prev.isValid() || prev.blockNumber() < cursor.blockNumber()-50)
                break;
            pMatch = pRex.match(prev.text());
        }
    }
}


void CodeEdit::truncate(QTextBlock block)
{
    QRegularExpression pRex("(^.*[^\\s]|^)(\\s+)$");
    QRegularExpressionMatch match = pRex.match(block.text());
    if (match.hasMatch()) {
        QTextCursor cursor(block);
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.setPosition(cursor.position() - match.capturedLength(2), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
}

int CodeEdit::textCursorColumn(QPoint mousePos)
{
    QTextCursor cursor = cursorForPosition(mousePos);
    int col = cursor.columnNumber();
    int addX = mousePos.x()-cursorRect(cursor).right();
    if (addX > 0) {
        QFontMetrics metric(font());
        col += addX / metric.width(' ');
    }
    return col;
}

void CodeEdit::mousePressEvent(QMouseEvent* e)
{
    mSmartType = false; // exit on mouse navigation
    setContextMenuPolicy(Qt::DefaultContextMenu);
    if (e->modifiers() == (Qt::AltModifier | Qt::ShiftModifier) && mAllowBlockEdit)
        setContextMenuPolicy(Qt::PreventContextMenu);
    if (e->modifiers() & Qt::AltModifier && mAllowBlockEdit) {
        QTextCursor cursor = cursorForPosition(e->pos());
        QTextCursor anchor = textCursor();
        anchor.setPosition(anchor.anchor());
        if (e->modifiers() == Qt::AltModifier) {
            if (mBlockEdit) endBlockEdit();
            startBlockEdit(cursor.blockNumber(), textCursorColumn(e->pos()));
        } else if (e->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
            if (mBlockEdit) mBlockEdit->selectTo(cursor.blockNumber(), textCursorColumn(e->pos()));
            else startBlockEdit(anchor.blockNumber(), anchor.columnNumber());
        }
        if (mBlockEdit) {
            mBlockEdit->selectTo(cursor.blockNumber(), textCursorColumn(e->pos()));
            emit cursorPositionChanged();
        }
    } else {
        if (mBlockEdit) {
            if (e->modifiers() || e->buttons() != Qt::RightButton) {
                endBlockEdit(false);
                AbstractEdit::mousePressEvent(e);
            } else if (e->button() == Qt::RightButton) {
                QTextCursor mouseTC = cursorForPosition(e->pos());
                if (mouseTC.blockNumber() < qMin(mBlockEdit->startLine(), mBlockEdit->currentLine())
                        || mouseTC.blockNumber() > qMax(mBlockEdit->startLine(), mBlockEdit->currentLine())) {
                    endBlockEdit(false);
                    setTextCursor(mouseTC);
                }
            } else {
                AbstractEdit::mousePressEvent(e);
            }
        } else
            AbstractEdit::mousePressEvent(e);
    }
}

void CodeEdit::mouseMoveEvent(QMouseEvent* e)
{
    if (mBlockEdit) {
        if ((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::AltModifier)) {
            mBlockEdit->selectTo(cursorForPosition(e->pos()).blockNumber(), textCursorColumn(e->pos()));
        }
    } else {
        AbstractEdit::mouseMoveEvent(e);
    }
    Qt::CursorShape shape = Qt::ArrowCursor;
    if (!marksAtMouse().isEmpty()) marksAtMouse().first()->cursorShape(&shape, true);
    lineNumberArea()->setCursor(shape);
}

void CodeEdit::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        const int delta = e->delta();
        if (delta < 0) {
            int pix = fontInfo().pixelSize();
            zoomOut();
            if (pix == fontInfo().pixelSize() && fontInfo().pointSize() > 1) zoomIn();
        } else if (delta > 0) {
            int pix = fontInfo().pixelSize();
            zoomIn();
            if (pix == fontInfo().pixelSize()) zoomOut();
        }
        updateTabSize();
        return;
    }
    AbstractEdit::wheelEvent(e);
}

void CodeEdit::paintEvent(QPaintEvent* e)
{
    int cw = mBlockEdit ? 0 : 2;
    if (cursorWidth()!=cw) setCursorWidth(cw);
    AbstractEdit::paintEvent(e);
    if (mBlockEdit) {
        mBlockEdit->paintEvent(e);
    }
}

void CodeEdit::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu *menu = createStandardContextMenu();
    bool hasBlockSelection = mBlockEdit && !mBlockEdit->blockText().isEmpty();
    QAction *lastAct = nullptr;
    for (int i = menu->actions().count()-1; i >= 0; --i) {
        QAction *act = menu->actions().at(i);
        if (act->objectName() == "select-all") {
            if (mBlockEdit) act->setEnabled(false);
            menu->removeAction(act);
            act->disconnect();
            connect(act, &QAction::triggered, this, &CodeEdit::selectAllText);
            menu->insertAction(lastAct, act);
        } else if (act->objectName() == "edit-paste" && act->isEnabled()) {
            menu->removeAction(act);
            act->disconnect();
            connect(act, &QAction::triggered, this, &CodeEdit::pasteClipboard);
            menu->insertAction(lastAct, act);
        } else if (hasBlockSelection) {
            if (act->objectName() == "edit-cut") {
                menu->removeAction(act);
                act->disconnect();
                act->setEnabled(true);
                connect(act, &QAction::triggered, this, &CodeEdit::cutSelection);
                menu->insertAction(lastAct, act);
            } else if (act->objectName() == "edit-copy") {
                menu->removeAction(act);
                act->disconnect();
                act->setEnabled(true);
                connect(act, &QAction::triggered, this, &CodeEdit::copySelection);
                menu->insertAction(lastAct, act);
            } else if (act->objectName() == "edit-delete") {
                menu->removeAction(act);
                act->disconnect();
                act->setEnabled(true);
                connect(act, &QAction::triggered, this, &CodeEdit::clearSelection);
                menu->insertAction(lastAct, act);
            }
        }
        lastAct = act;
    }
    if (!isReadOnly()) {
        QMenu *submenu = menu->addMenu(tr("Advanced"));
        QList<QAction*> ret;
        emit requestAdvancedActions(&ret);
        submenu->addActions(ret);
        emit cloneBookmarkMenu(menu);
    }
    menu->exec(e->globalPos());
    delete menu;
}

void CodeEdit::marksChanged(const QSet<int> dirtyLines)
{
    AbstractEdit::marksChanged(dirtyLines);
    bool doPaint = dirtyLines.isEmpty() || dirtyLines.size() > 5;
    if (!doPaint) {
        int firstLine = topVisibleLine();
        for (const int &line: dirtyLines) {
            if (line >= firstLine && line <= firstLine+100) {
                doPaint = true;
                break;
            }
        }
    }
    if (doPaint) {
        updateLineNumberAreaWidth();
        mLineNumberArea->repaint();
    }
}

void CodeEdit::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls()) {
        e->ignore(); // paste to parent widget
    } else {
        AbstractEdit::dragEnterEvent(e);
    }
}

void CodeEdit::duplicateLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(cursor.block().text()+'\n');
    cursor.endEditBlock();
}

void CodeEdit::removeLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

void CodeEdit::commentLine()
{
    QTextCursor cursor = textCursor();
    if (mBlockEdit) {
        QTextBlock startBlock = cursor.document()->findBlockByNumber(mBlockEdit->startLine());
        QTextBlock endBlock = cursor.document()->findBlockByNumber(mBlockEdit->currentLine());
        int columnFrom = mBlockEdit->colFrom();
        int columnTo = mBlockEdit->colTo();
        cursor.setPosition(startBlock.position() + mBlockEdit->colFrom());
        cursor.setPosition(textCursor().block().position() + mBlockEdit->colTo(), QTextCursor::KeepAnchor);

        endBlockEdit();
        applyLineComment(cursor, qMin(startBlock, endBlock), qMax(startBlock.blockNumber(), endBlock.blockNumber()));
        setTextCursor(cursor);
        startBlockEdit(startBlock.blockNumber(), columnTo);
        mBlockEdit->selectTo(endBlock.blockNumber(), columnFrom);
    } else {
        QTextBlock startBlock = cursor.document()->findBlock(qMin(cursor.position(), cursor.anchor()));
        int lastBlockNr = cursor.document()->findBlock(qMax(cursor.position(), cursor.anchor())).blockNumber();
        applyLineComment(cursor, startBlock, lastBlockNr);
        setTextCursor(cursor);
    }

    recalcExtraSelections();
}

bool CodeEdit::hasLineComment(QTextBlock startBlock, int lastBlockNr) {
    bool hasComment = true;
    for (QTextBlock block = startBlock; block.blockNumber() <= lastBlockNr; block = block.next()) {
        if (!block.isValid())
            break;
        if (!block.text().startsWith('*'))
            hasComment = false;
    }
    return hasComment;
}

void CodeEdit::applyLineComment(QTextCursor cursor, QTextBlock startBlock, int lastBlockNr)
{
    bool hasComment = hasLineComment(startBlock, lastBlockNr);
    cursor.beginEditBlock();
    QTextCursor anchor = cursor;
    anchor.setPosition(anchor.anchor());
    for (QTextBlock block = startBlock; block.blockNumber() <= lastBlockNr; block = block.next()) {
        if (!block.isValid()) break;

        cursor.setPosition(block.position());
        if (hasComment)
            cursor.deleteChar();
        else
            cursor.insertText("*");
    }
    cursor.setPosition(anchor.position());
    cursor.setPosition(textCursor().position(), QTextCursor::KeepAnchor);
    cursor.endEditBlock();
}


int CodeEdit::minIndentCount(int fromLine, int toLine)
{
    QTextCursor cursor = textCursor();
    QTextBlock block = (fromLine < 0) ? document()->findBlock(cursor.anchor()) : document()->findBlockByNumber(fromLine);
    QTextBlock last = (toLine < 0) ? document()->findBlock(cursor.position()) : document()->findBlockByNumber(toLine);
    if (block.blockNumber() > last.blockNumber()) qSwap(block, last);
    int res = block.text().length();
    QRegularExpression rex("^(\\s*)");
    while (true) {
        QRegularExpressionMatch match = rex.match(block.text());
        if (res > match.capturedLength(1)) res = match.capturedLength(1);
        if (block == last) break;
        block = block.next();
    }
    return res;
}

int CodeEdit::indent(int size, int fromLine, int toLine)
{
    if (!size) return 0;
    QTextCursor savePos;
    QTextCursor saveAnc;
    // determine affected lines
    bool force = true;
    if (fromLine < 0 || toLine < 0) {
        if (fromLine >= 0) toLine = fromLine;
        else if (toLine >= 0) fromLine = toLine;
        else if (mBlockEdit) {
            force = false;
            fromLine = mBlockEdit->startLine();
            toLine = mBlockEdit->currentLine();
        } else {
            force = textCursor().hasSelection();
            fromLine = document()->findBlock(textCursor().anchor()).blockNumber();
            toLine = textCursor().block().blockNumber();
        }
    }
    if (force) {
        savePos = textCursor();
        saveAnc = textCursor();
        saveAnc.setPosition(saveAnc.anchor());
    }
    if (fromLine > toLine) qSwap(fromLine, toLine);

    // smallest indent of affected lines
    QTextBlock block = document()->findBlockByNumber(fromLine);
    int minIndentPos = block.length();
    while (block.isValid() && block.blockNumber() <= toLine) {
        QString text = block.text();
        int w = 0;
        while (w < text.length() && text.at(w).isSpace()) w++;
        if (w <= text.length() && w <= minIndentPos) minIndentPos = w;
        block = block.next();
    }

    // adjust justInsert if current position is beyond minIndentPos
    if (!force) {
        if (mBlockEdit)
            force = (mBlockEdit->colFrom() != mBlockEdit->colTo() || mBlockEdit->colFrom() <= minIndentPos);
        else {
            force = (textCursor().positionInBlock() <= minIndentPos);
        }
    }
    // determine insertPos
    int insertPos = mBlockEdit ? mBlockEdit->colFrom() : textCursor().positionInBlock();
    if (force) insertPos = minIndentPos;
    if (size < 0 && insertPos == 0) return 0;

    // check if all lines contain enough spaces to remove them
    if (size < 0 && !force && insertPos+size >= 0) {
        bool canRemoveSpaces = true;
        block = document()->findBlockByNumber(fromLine);
        while (block.isValid() && block.blockNumber() <= toLine && canRemoveSpaces) {
            QString text = block.text();
            int w = insertPos + size;
            while (w < text.length() && w < insertPos) {
                if (!text.at(w).isSpace()) canRemoveSpaces = false;
                w++;
            }
            block = block.next();
        }
        if (!canRemoveSpaces) return 0;
    }

    // store current blockEdit mode
    bool inBlockEdit = mBlockEdit;
    QPoint beFrom;
    QPoint beTo;
    if (mBlockEdit) {
        beFrom = QPoint(mBlockEdit->colTo(), mBlockEdit->startLine());
        beTo = QPoint(mBlockEdit->colFrom(), mBlockEdit->currentLine());
        endBlockEdit();
    }

    // perform deletion
    int charCount;
    if (size < 0) charCount = ((insertPos-1) % qAbs(size)) + 1;
    else charCount = size - insertPos % size;
    QString insText(charCount, ' ');
    block = document()->findBlockByNumber(fromLine);
    QTextCursor editCursor = textCursor();
    editCursor.beginEditBlock();
    while (block.isValid() && block.blockNumber() <= toLine) {
        editCursor.setPosition(block.position());
        if (size < 0) {
            if (insertPos - charCount < block.length()) {
                editCursor.setPosition(block.position() + insertPos - charCount);
                int tempCount = qMin(charCount, block.length() - (insertPos - charCount));
                editCursor.setPosition(editCursor.position() + tempCount, QTextCursor::KeepAnchor);
                editCursor.removeSelectedText();
            }
        } else {
            if (insertPos < block.length()) {
                editCursor.setPosition(block.position() + insertPos);
                editCursor.insertText(insText);
            }
        }
        block = block.next();
    }
    editCursor.endEditBlock();
    // restore normal selection
    if (!savePos.isNull()) {
        editCursor.setPosition(saveAnc.position());
        editCursor.setPosition(savePos.position(), QTextCursor::KeepAnchor);
    }
    setTextCursor(editCursor);

    if (inBlockEdit) {
        int add = (size > 0) ? charCount : -charCount;
        startBlockEdit(beFrom.y(), qMax(beFrom.x() + add, 0));
        mBlockEdit->selectTo(beTo.y(), qMax(beTo.x() + add, 0));
    }
    return charCount;
}

void CodeEdit::startBlockEdit(int blockNr, int colNr)
{
    if (!mAllowBlockEdit) return;
    if (mBlockEdit) endBlockEdit();
    bool overwrite = overwriteMode();
    if (overwrite) setOverwriteMode(false);
    mBlockEdit = new BlockEdit(this, blockNr, colNr);
    mBlockEdit->setOverwriteMode(overwrite);
    mBlockEdit->startCursorTimer();
    updateLineNumberAreaWidth();
}

void CodeEdit::endBlockEdit(bool adjustCursor)
{
    mBlockEdit->stopCursorTimer();
    if (adjustCursor) mBlockEdit->adjustCursor();
    bool overwrite = mBlockEdit->overwriteMode();
    delete mBlockEdit;
    mBlockEdit = nullptr;
    setOverwriteMode(overwrite);
}

void dumpClipboard()
{
    QClipboard *clip = QGuiApplication::clipboard();
    const QMimeData * mime = clip->mimeData();
    DEB() << "----------------- Clipboard --------------------" ;
    DEB() << "  C " << clip->ownsClipboard()<< "  F " << clip->ownsFindBuffer()<< "  S " << clip->ownsSelection();
    for (QString fmt: mime->formats()) {
        DEB() << "   -- " << fmt << "   L:" << mime->data(fmt).length()
              << "\n" << mime->data(fmt)
              << "\n" << QString(mime->data(fmt).toHex(' '));
    }
}

QStringList CodeEdit::clipboard(bool *isBlock)
{
//    dumpClipboard();
    QString mimes = "|" + QGuiApplication::clipboard()->mimeData()->formats().join("|") + "|";
    bool asBlock = (mimes.indexOf("MSDEVColumnSelect") >= 0) || (mimes.indexOf("Borland IDE Block Type") >= 0);
    QStringList texts = QGuiApplication::clipboard()->mimeData()->text().split("\n");
    if (texts.last().isEmpty()) texts.removeLast();
    if (!asBlock || texts.count() <= 1) {
        if (isBlock) *isBlock = false;
        texts = QStringList() << QGuiApplication::clipboard()->mimeData()->text();
    }
    if (isBlock && (asBlock || mBlockEdit))
        *isBlock = true;

    return texts;
}

CodeEdit::CharType CodeEdit::charType(QChar c)
{
    switch (c.category()) {
    case QChar::Number_DecimalDigit:
        return CharType::Number;
    case QChar::Separator_Space:
    case QChar::Separator_Line:
    case QChar::Separator_Paragraph:
        return CharType::Seperator;
    case QChar::Letter_Uppercase:
        return CharType::LetterUCase;
    case QChar::Letter_Lowercase:
        return CharType::LetterLCase;
    case QChar::Punctuation_Dash:
    case QChar::Punctuation_Open:
    case QChar::Punctuation_Close:
    case QChar::Punctuation_InitialQuote:
    case QChar::Punctuation_FinalQuote:
    case QChar::Punctuation_Other:
    case QChar::Symbol_Math:
    case QChar::Symbol_Currency:
    case QChar::Symbol_Modifier:
    case QChar::Symbol_Other:
        return CharType::Punctuation;
    default:
        break;
    }
    return CharType::Other;
}

void CodeEdit::updateTabSize()
{
    QFontMetrics metric(font());
    setTabStopDistance(mSettings->tabSize() * metric.width(' '));
}

int CodeEdit::findAlphaNum(const QString &text, int start, bool back)
{
    QChar c = ' ';
    int pos = (back && start == text.length()) ? start-1 : start;
    while (pos >= 0 && pos < text.length()) {
        c = text.at(pos);
        if (!c.isLetterOrNumber() && c != '_' && (pos != start || !back)) break;
        pos = pos + (back?-1:1);
    }
    pos = pos - (back?-1:1);
    if (pos == start) {
        c = (pos >= 0 && pos < text.length()) ? text.at(pos) : ' ';
        if (!c.isLetterOrNumber() && c != '_') return -1;
    }
    if (pos >= 0 && pos < text.length()) { // must not start with number
        c = text.at(pos);
        if (!c.isLetterOrNumber() && c != '_') return -1;
    }
    return pos;
}

void CodeEdit::rawKeyPressEvent(QKeyEvent *e)
{
    AbstractEdit::keyPressEvent(e);
}

AbstractEdit::EditorType CodeEdit::type()
{
    return EditorType::CodeEdit;
}

void CodeEdit::wordInfo(QTextCursor cursor, QString &word, int &intKind)
{
    QString text = cursor.block().text();
    int start = cursor.positionInBlock();
    int from = findAlphaNum(text, start, true);
    int to = findAlphaNum(text, from, false);
    if (from >= 0 && from <= to) {
        word = text.mid(from, to-from+1);
        start = from + cursor.block().position();
        emit requestSyntaxKind(start+1, intKind);
//        cursor.setPosition(start+1);
//        intState = cursor.charFormat().property(QTextFormat::UserProperty).toInt();
    } else {
        word = "";
        intKind = 0;
    }
}

void CodeEdit::getPositionAndAnchor(QPoint &pos, QPoint &anchor)
{
    if (mBlockEdit) {
        pos = QPoint(mBlockEdit->colFrom()+1, mBlockEdit->currentLine()+1);
        anchor = QPoint(mBlockEdit->colTo()+1, mBlockEdit->startLine()+1);
    } else {
        QTextCursor cursor = textCursor();
        pos = QPoint(cursor.positionInBlock()+1, cursor.blockNumber()+1);
        if (cursor.hasSelection()) {
            cursor.setPosition(cursor.anchor());
            anchor = QPoint(cursor.positionInBlock()+1, cursor.blockNumber()+1);
        }
    }
}

ParenthesesMatch CodeEdit::matchParentheses()
{
    static QString parentheses("{[(/E}])\\e");
    static int pSplit = parentheses.length()/2;
    QTextBlock block = textCursor().block();
    if (!block.userData()) return ParenthesesMatch();
//    int state = block.userState();
    QVector<ParenthesesPos> parList = static_cast<BlockData*>(block.userData())->parentheses();
    int pos = textCursor().positionInBlock();
    int start = -1;
    for (int i = parList.count()-1; i >= 0; --i) {
        if (parList.at(i).relPos == pos || parList.at(i).relPos == pos-1) {
            start = i;
        }
    }
    if (start < 0) return ParenthesesMatch();
    // prepare matching search
    int ci = parentheses.indexOf(parList.at(start).character);
    bool back = ci >= pSplit;
    ci = ci % pSplit;
    ParenthesesMatch result(block.position() + parList.at(start).relPos);
    QStringRef parEnter = parentheses.midRef(back ? pSplit : 0, pSplit);
    QStringRef parLeave = parentheses.midRef(back ? 0 : pSplit, pSplit);
    QVector<QChar> parStack;
    parStack << parLeave.at(ci);
    int pi = start;
    while (block.isValid()) {
        // get next parentheses entry
        if (back ? --pi < 0 : ++pi >= parList.count()) {
            bool isEmpty = true;
            while (block.isValid() && isEmpty) {
                block = back ? block.previous() : block.next();
                if (block.isValid() && block.userData()) {
                    parList = static_cast<BlockData*>(block.userData())->parentheses();
                    if (!parList.isEmpty()) isEmpty = false;
                }
            }
            if (isEmpty) continue;
            parList = static_cast<BlockData*>(block.userData())->parentheses();
            pi = back ? parList.count()-1 : 0;
        }

        int i = parEnter.indexOf(parList.at(pi).character);
        if (i < 0) {
            // Only last stacked character is valid
            if (parList.at(pi).character == parStack.last()) {
                parStack.removeLast();
                if (parStack.isEmpty()) {
                    if (parentheses.at(ci) == 'E') return ParenthesesMatch(); // only mark embedded on mismatch
                    result.valid = true;
                    result.match = block.position() + parList.at(pi).relPos;
                    return result;
                }
            } else {
                // Mark bad parentheses
                parStack.clear();
                result.match = block.position() + parList.at(pi).relPos;
                return result;
            }
        } else {
            // Stack new character
            parStack << parLeave.at(i);
        }

    }
    return ParenthesesMatch();
}

void CodeEdit::setOverwriteMode(bool overwrite)
{
    if (mBlockEdit) mBlockEdit->setOverwriteMode(overwrite);
    else AbstractEdit::setOverwriteMode(overwrite);
}

bool CodeEdit::overwriteMode() const
{
    if (mBlockEdit) return mBlockEdit->overwriteMode();
    return AbstractEdit::overwriteMode();
}

inline int CodeEdit::assignmentKind(int p)
{
    int preKind = 0;
    int postKind = 0;
    emit requestSyntaxKind(p-1, preKind);
    emit requestSyntaxKind(p+1, postKind);
    if (postKind == static_cast<int>(syntax::SyntaxKind::IdentifierAssignment)) return 1;
    if (preKind == static_cast<int>(syntax::SyntaxKind::IdentifierAssignment)) return -1;
    if (preKind == static_cast<int>(syntax::SyntaxKind::IdentifierAssignmentEnd)) return -1;
    return 0;
}

void CodeEdit::recalcWordUnderCursor()
{
    mWordUnderCursor = "";
    QTextEdit::ExtraSelection selection;
    selection.cursor = textCursor();
    QString text = selection.cursor.block().text();
    int start = qMin(selection.cursor.position(), selection.cursor.anchor()) - selection.cursor.block().position();
    int from = findAlphaNum(text, start, true);
    int to = findAlphaNum(text, from, false);
    if (from >= 0 && from <= to) {
        if (!textCursor().hasSelection() || text.mid(from, to-from+1) == textCursor().selectedText())
            mWordUnderCursor = text.mid(from, to-from+1);
    }
}

void CodeEdit::recalcExtraSelections()
{
    QList<QTextEdit::ExtraSelection> selections;
    mParenthesesMatch = ParenthesesMatch();
    if (!mBlockEdit) {
        extraSelCurrentLine(selections);
        recalcWordUnderCursor();
        mParenthesesDelay.start(100);
        int wordDelay = 10;
        if (mSettings->wordUnderCursor()) wordDelay = 500;
        mWordDelay.start(wordDelay);
    }
    extraSelBlockEdit(selections);
    setExtraSelections(selections);
}

void CodeEdit::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> selections;
    extraSelCurrentLine(selections);
    if (!mBlockEdit) {
        QString selectedText = textCursor().selectedText();
        QRegularExpression regexp = search::SearchLocator::searchDialog()->results()
                                    ? search::SearchLocator::searchDialog()->results()->searchRegex()
                                    : QRegularExpression();

        // word boundary (\b) only matches start-of-string when first character is \w
        // so \b will only be added when first character of selectedText is a \w
        // if first character is not \w  the whole string needs to be matched in order to deactivate HWUC
        QRegularExpression startsWithW("^\\w");
        if (startsWithW.match(selectedText).hasMatch())
            regexp.setPattern("\\b" + regexp.pattern());

        QRegularExpressionMatch match = regexp.match(selectedText);
        bool skipWordTimer = (sender() == &mParenthesesDelay
                              || sender() == this->verticalScrollBar()
                              || sender() == nullptr);

        //    (  not caused by parenthiesis matching                               ) OR has selection
        if ( (( !extraSelMatchParentheses(selections, sender() == &mParenthesesDelay) || hasSelection())
               // ( depending on settings: no selection necessary OR has selection )
               && (mSettings->wordUnderCursor() || hasSelection())
               // (  depending on settings: no selection necessary skip word-timer )
               && (mSettings->wordUnderCursor() || skipWordTimer))
             // AND deactivate when navigating search results
             && match.captured(0).isEmpty()) {
            extraSelCurrentWord(selections);
        }
    }
    extraSelMatches(selections);
    extraSelBlockEdit(selections);
    extraSelMarks(selections);
    setExtraSelections(selections);
}

void CodeEdit::extraSelBlockEdit(QList<QTextEdit::ExtraSelection>& selections)
{
    if (mBlockEdit) {
        selections.append(mBlockEdit->extraSelections());
    }
}

void CodeEdit::extraSelCurrentWord(QList<QTextEdit::ExtraSelection> &selections)
{
    if (!mWordUnderCursor.isEmpty()) {
        QTextBlock block = firstVisibleBlock();
        QRegularExpression rex(QString("(?i)(^|[^\\w]|-)(%1)($|[^\\w]|-)").arg(mWordUnderCursor));
        QRegularExpressionMatch match;
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        while (block.isValid() && top < viewport()->height()) {
            int i = 0;
            while (true) {
                i = block.text().indexOf(rex, i, &match);
                if (i < 0) break;
                QTextEdit::ExtraSelection selection;
                selection.cursor = textCursor();
                selection.cursor.setPosition(block.position()+match.capturedStart(2));
                selection.cursor.setPosition(block.position()+match.capturedEnd(2), QTextCursor::KeepAnchor);
                selection.format.setBackground(mSettings->colorScheme().value("Edit.currentWordBg", QColor(Qt::lightGray)));
                selections << selection;
                i += match.capturedLength(1) + match.capturedLength(2);
            }
            top += qRound(blockBoundingRect(block).height());
            block = block.next();
        }
    }
}

bool CodeEdit::extraSelMatchParentheses(QList<QTextEdit::ExtraSelection> &selections, bool first)
{
    if (!mParenthesesMatch.isValid())
        mParenthesesMatch = matchParentheses();

    if (!mParenthesesMatch.isValid()) return false;

    if (mParenthesesMatch.pos == mParenthesesMatch.match) mParenthesesMatch.valid = false;
    QColor fgColor = mParenthesesMatch.valid ? QColor(Qt::red) : QColor(Qt::black);
    QColor bgColor = mParenthesesMatch.valid ? QColor(Qt::green).lighter(170) : QColor(Qt::red).lighter(150);
    QColor bgBlinkColor = mParenthesesMatch.valid ? QColor(Qt::green).lighter(130) : QColor(Qt::red).lighter(115);
    QTextEdit::ExtraSelection selection;
    selection.cursor = textCursor();
    selection.cursor.setPosition(mParenthesesMatch.pos);
    selection.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    selection.format.setForeground(fgColor);
    selection.format.setBackground(bgColor);
    selections << selection;
    if (mParenthesesMatch.match >= 0) {
        selection.cursor = textCursor();
        selection.cursor.setPosition(mParenthesesMatch.match);
        selection.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        selection.format.setForeground(fgColor);
        selection.format.setBackground(first ? bgBlinkColor : bgColor);
        selections << selection;
    }
    return true;
}

void CodeEdit::extraSelMatches(QList<QTextEdit::ExtraSelection> &selections)
{
    search::SearchDialog *searchDialog = search::SearchLocator::searchDialog();
    if (!searchDialog || searchDialog->searchTerm().isEmpty()) return;

    search::SearchResultList* list = searchDialog->results();
    if (!list) return;

    if (list->filteredResultList(ViewHelper::location(this)).isEmpty()) return;

    QRegularExpression regEx = list->searchRegex();

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid() && top < viewport()->height()) {
        top += qRound(blockBoundingRect(block).height());

        QRegularExpressionMatchIterator i = regEx.globalMatch(block.text());
        while (i.hasNext()) {
            QRegularExpressionMatch m = i.next();
            QTextEdit::ExtraSelection selection;
            QTextCursor tc(document());
            tc.setPosition(block.position() + m.capturedStart(0));
            tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, m.capturedLength(0));
            selection.cursor = tc;
            selection.format.setBackground(mSettings->colorScheme().value("Edit.matchesBg", QColor(Qt::green).lighter(160)));
            selections << selection;
        }

        block = block.next();
    }
}

QPoint CodeEdit::toolTipPos(const QPoint &mousePos)
{
    QPoint pos = AbstractEdit::toolTipPos(mousePos);
    if (mousePos.x() < mLineNumberArea->width())
        pos.setX(mLineNumberArea->width()+6);
    else
        pos.setX(pos.x() + mLineNumberArea->width()+2);
    return pos;
}

QString CodeEdit::lineNrText(int blockNr)
{
    return QString::number(blockNr);
}

bool CodeEdit::showLineNr() const
{
    return mSettings->showLineNr();
}

void CodeEdit::setAllowBlockEdit(bool allow)
{
    mAllowBlockEdit = allow;
    if (mBlockEdit) endBlockEdit();
}

void CodeEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(mLineNumberArea);
    bool hasMarks = marks() && marks()->hasVisibleMarks();
    if (hasMarks && mIconCols == 0) QTimer::singleShot(0, this, &CodeEdit::updateLineNumberAreaWidth);
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());
    int markFrom = textCursor().blockNumber();
    int markTo = textCursor().blockNumber();
    if (markFrom > markTo) qSwap(markFrom, markTo);

    QRect paintRect(event->rect());
    painter.fillRect(paintRect, QColor(245,245,245));

    QRect markRect(paintRect.left(), top, paintRect.width(), static_cast<int>(blockBoundingRect(block).height())+1);
    while (block.isValid() && top <= paintRect.bottom()) {
        if (block.isVisible() && bottom >= paintRect.top()) {
            bool mark = mBlockEdit ? mBlockEdit->hasBlock(block.blockNumber())
                                   : blockNumber >= markFrom && blockNumber <= markTo;
            if (mark) {
                markRect.moveTop(top);
                markRect.setHeight(bottom-top);
                painter.fillRect(markRect, QColor(225,255,235));
            }

            if(showLineNr()) {
                QString number = lineNrText(blockNumber + 1);
                QFont f = font();
                f.setBold(mark);
                painter.setFont(f);
                painter.setPen(mark ? Qt::black : Qt::gray);
                int realtop = top; // (top+bottom-fontMetrics().height())/2;
                painter.drawText(0, realtop, mLineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
            }

            if (hasMarks && marks()->contains(absoluteBlockNr(blockNumber))) {
                int iTop = (2+top+bottom-iconSize())/2;
                painter.drawPixmap(1, iTop, marks()->value(absoluteBlockNr(blockNumber))->icon().pixmap(QSize(iconSize(),iconSize())));
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

CodeEdit::BlockEdit::BlockEdit(CodeEdit* edit, int blockNr, int colNr)
    : mEdit(edit)
{
    Q_ASSERT_X(edit, "BlockEdit constructor", "BlockEdit needs a valid editor");
    mStartLine = blockNr;
    mCurrentLine = blockNr;
    mColumn = colNr;
    setSize(0);
}

CodeEdit::BlockEdit::~BlockEdit()
{
    mSelections.clear();
    mEdit->updateExtraSelections();
}

void CodeEdit::BlockEdit::selectTo(int blockNr, int colNr)
{
    mCurrentLine = blockNr;
    setSize(colNr - mColumn);
    updateExtraSelections();
    startCursorTimer();
}

void CodeEdit::BlockEdit::selectToEnd()
{
    int end = 0;
    for (QTextBlock block = mEdit->document()->findBlockByNumber(qMin(mStartLine, mCurrentLine))
         ; block.blockNumber() <= qMax(mStartLine, mCurrentLine); block=block.next()) {
        if (end < block.length()-1) end = block.length()-1;
        if (!block.isValid()) break;
    }
    setSize(end - mColumn);
}

QString CodeEdit::BlockEdit::blockText()
{
    QString res = "";
    if (!mSize) return res;
    QTextBlock block = mEdit->document()->findBlockByNumber(qMin(mStartLine, mCurrentLine));
    int from = qMin(mColumn, mColumn+mSize);
    int to = qMax(mColumn, mColumn+mSize);

    if (qMin(mStartLine, mCurrentLine) == qMax(mStartLine, mCurrentLine)) {
        QString text = block.text();
        if (text.length()-1 < to) text.append(QString(qAbs(mSize), ' '));
        res.append(text.mid(from, to-from));
    } else {
        for (int i = qMin(mStartLine, mCurrentLine); i <= qMax(mStartLine, mCurrentLine); ++i) {
            QString text = block.text();
            if (text.length()-1 < to) text.append(QString(qAbs(mSize), ' '));
            res.append(text.mid(from, to-from)+"\n");
            block = block.next();
        }
    }

    return res;
}

void CodeEdit::BlockEdit::selectionToClipboard()
{
    QMimeData *mime = new QMimeData();
    mime->setText(blockText());
    mime->setData("application/x-qt-windows-mime;value=\"MSDEVColumnSelect\"", QByteArray());
    mime->setData("application/x-qt-windows-mime;value=\"Borland IDE Block Type\"", QByteArray(1,char(2)));
    QApplication::clipboard()->setMimeData(mime);
}

int CodeEdit::BlockEdit::currentLine() const
{
    return mCurrentLine;
}

int CodeEdit::BlockEdit::column() const
{
    return mColumn;
}

void CodeEdit::BlockEdit::setColumn(int column)
{
    mColumn = column;
}

void CodeEdit::BlockEdit::setOverwriteMode(bool overwrite)
{
    mOverwrite = overwrite;
}

bool CodeEdit::BlockEdit::overwriteMode() const
{
    return mOverwrite;
}

void CodeEdit::BlockEdit::setSize(int size)
{
    //const ensures the size is only changed HERE
    *(const_cast<int*>(&mSize)) = size;
    mLastCharType = CharType::None;
}

int CodeEdit::BlockEdit::startLine() const
{
    return mStartLine;
}

void CodeEdit::BlockEdit::keyPressEvent(QKeyEvent* e)
{
    QSet<int> moveKeys;
    moveKeys << Qt::Key_Home << Qt::Key_End << Qt::Key_Down << Qt::Key_Up << Qt::Key_Left << Qt::Key_Right
             << Qt::Key_PageUp << Qt::Key_PageDown;
    if (moveKeys.contains(e->key())) {
        if (e->key() == Qt::Key_Down && mCurrentLine < mEdit->document()->blockCount()-1) mCurrentLine++;
        if (e->key() == Qt::Key_Up && mCurrentLine > 0) mCurrentLine--;
        if (e->key() == Qt::Key_Home) setSize(-mColumn);
        if (e->key() == Qt::Key_End) selectToEnd();
        QTextBlock block = mEdit->document()->findBlockByNumber(mCurrentLine);
        if ((e->modifiers()&Qt::ControlModifier) != 0 && e->key() == Qt::Key_Right) {
            int size = mSize;
            EditorHelper::nextWord(mColumn, size, block.text());
            setSize(size);
        } else if (e->key() == Qt::Key_Right) setSize(mSize+1);
        if ((e->modifiers()&Qt::ControlModifier) != 0 && e->key() == Qt::Key_Left && mColumn+mSize > 0) {
            int size = mSize;
            EditorHelper::prevWord(mColumn, size, block.text());
            setSize(size);
        } else if (e->key() == Qt::Key_Left && mColumn+mSize > 0) setSize(mSize-1);
        QTextCursor cursor(block);
        if (block.length() > mColumn+mSize)
            cursor.setPosition(block.position()+mColumn+mSize);
        else
            cursor.setPosition(block.position()+block.length()-1);
        mEdit->setTextCursor(cursor);
        updateExtraSelections();
        emit mEdit->cursorPositionChanged();
    } else if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        if (!mSize && mColumn >= 0) {
            mLastCharType = CharType::None;
            setSize((e->key() == Qt::Key_Backspace) ? -1 : 1);
        }
        replaceBlockText("");
    } else if (e == Hotkey::Indent) {
        mEdit->indent(mEdit->mSettings->tabSize());
        return;
    } else if (e == Hotkey::Outdent) {
        mEdit->indent(-mEdit->mSettings->tabSize());
        return;
    } else if (e->text().length()) {
        mEdit->mBlockEditRealPos = mEdit->textCursor().position();
        QTextCursor cur = mEdit->textCursor();
        cur.joinPreviousEditBlock();
        mEdit->setTextCursor(cur);
        mEdit->rawKeyPressEvent(e);
        QTimer::singleShot(0, mEdit, &CodeEdit::checkBlockInsertion);
    }

    startCursorTimer();
}

void CodeEdit::BlockEdit::startCursorTimer()
{
    mEdit->mBlinkBlockEdit.start();
    mBlinkStateHidden = true;
    mEdit->lineNumberArea()->update(mEdit->lineNumberArea()->visibleRegion());
    refreshCursors();
}

void CodeEdit::BlockEdit::stopCursorTimer()
{
    mEdit->mBlinkBlockEdit.stop();
    mBlinkStateHidden = true;
    mEdit->lineNumberArea()->update(mEdit->lineNumberArea()->visibleRegion());
    refreshCursors();
}

void CodeEdit::BlockEdit::refreshCursors()
{
    mBlinkStateHidden = !mBlinkStateHidden;
    mEdit->viewport()->update(mEdit->viewport()->visibleRegion());
}

void CodeEdit::BlockEdit::paintEvent(QPaintEvent *e)
{
    QPainter painter(mEdit->viewport());
    QPointF offset(mEdit->contentOffset()); //
    QRect evRect = e->rect();
    bool editable = !mEdit->isReadOnly();
    painter.setClipRect(evRect);
    int cursorColumn = mColumn+mSize;
    QFontMetrics metric(mEdit->font());
    double spaceWidth = metric.width(QString(100,' ')) / 100.0;
    QTextBlock block = mEdit->firstVisibleBlock();
    QTextCursor cursor(block);
    cursor.setPosition(block.position()+block.length()-1);
//    qreal cursorOffset = 0; //mEdit->cursorRect(cursor).left()-block.layout()->minimumWidth();

    while (block.isValid()) {
        QRectF blockRect = mEdit->blockBoundingRect(block).translated(offset);
        if (!hasBlock(block.blockNumber()) || !block.isVisible()) {
            offset.ry() += blockRect.height();
            block = block.next();
            continue;
        }
        // draw extended extra-selection for lines past line-end
        int beyondEnd = qMax(mColumn, mColumn+mSize);
        if (beyondEnd >= block.length()) {
            cursor.setPosition(block.position()+block.length()-1);
            // we have to draw selection beyond the line-end
            int beyondStart = qMax(block.length()-1, qMin(mColumn, mColumn+mSize));
            QRectF selRect = mEdit->cursorRect(cursor);
//            qreal x = block.layout()->minimumWidth()+cursorOffset;
//            selRect.setLeft(x);
            if (block.length() <= beyondStart)
                selRect.translate(((beyondStart-block.length()+1) * spaceWidth), 0);
            selRect.setWidth((beyondEnd-beyondStart) * spaceWidth);
            QColor beColor = QColor(Qt::cyan).lighter(150);
            painter.fillRect(selRect, QBrush(beColor));
        }

        if (mBlinkStateHidden) {
            offset.ry() += blockRect.height();
            block = block.next();
            continue;
        }

        cursor.setPosition(block.position()+qMin(block.length()-1, cursorColumn));
        QRectF cursorRect = mEdit->cursorRect(cursor);
        if (block.length() <= cursorColumn) {
            cursorRect.translate((cursorColumn-block.length()+1) * spaceWidth, 0);
        }
        cursorRect.setWidth(mOverwrite ? spaceWidth : 2);

        if (cursorRect.bottom() >= evRect.top() && cursorRect.top() <= evRect.bottom()) {
            bool drawCursor = ((editable || (mEdit->textInteractionFlags() & Qt::TextSelectableByKeyboard)));
            if (drawCursor) {
                bool toggleAntialiasing = !(painter.renderHints() & QPainter::Antialiasing)
                                          && (painter.transform().type() > QTransform::TxTranslate);
                if (toggleAntialiasing)
                    painter.setRenderHint(QPainter::Antialiasing);
                QPainter::CompositionMode origCompositionMode = painter.compositionMode();
                if (painter.paintEngine()->hasFeature(QPaintEngine::RasterOpModes))
                    painter.setCompositionMode(QPainter::RasterOp_NotDestination);
                painter.fillRect(cursorRect, painter.pen().brush());
                painter.setCompositionMode(origCompositionMode);
                if (toggleAntialiasing)
                    painter.setRenderHint(QPainter::Antialiasing, false);

            }
        }

        offset.ry() += blockRect.height();
        if (offset.y() > mEdit->viewport()->rect().height())
            break;
        block = block.next();
    }

}

void CodeEdit::BlockEdit::updateExtraSelections()
{
    mSelections.clear();
    QTextCursor cursor(mEdit->document());
    for (int lineNr = qMin(mStartLine, mCurrentLine); lineNr <= qMax(mStartLine, mCurrentLine); ++lineNr) {
        QTextBlock block = mEdit->document()->findBlockByNumber(lineNr);
        QTextEdit::ExtraSelection select;
        QColor beColor = QColor(Qt::cyan).lighter(150);
        select.format.setBackground(beColor);

        int start = qMin(block.length()-1, qMin(mColumn, mColumn+mSize));
        int end = qMin(block.length()-1, qMax(mColumn, mColumn+mSize));
        cursor.setPosition(block.position()+start);
        if (end>start) cursor.setPosition(block.position()+end, QTextCursor::KeepAnchor);
        select.cursor = cursor;
        mSelections << select;
        if (cursor.blockNumber() == mCurrentLine) {
            QTextCursor c = mEdit->textCursor();
            c.setPosition(cursor.position());
            mEdit->setTextCursor(c);
        }
    }
    mEdit->updateExtraSelections();
}

void CodeEdit::BlockEdit::adjustCursor()
{
    QTextBlock block = mEdit->document()->findBlockByNumber(mCurrentLine);
    QTextCursor cursor(block);
    cursor.setPosition(block.position() + qMin(block.length()-1, mColumn+mSize));
    mEdit->setTextCursor(cursor);
}

void CodeEdit::BlockEdit::replaceBlockText(QString text)
{
    replaceBlockText(QStringList() << text);
}

void CodeEdit::BlockEdit::replaceBlockText(QStringList texts)
{
    if (mEdit->isReadOnly()) return;
    if (texts.isEmpty()) texts << "";
    CharType charType = texts.at(0).length()>0 ? mEdit->charType(texts.at(0).at(0)) : CharType::None;
    bool newUndoBlock = texts.count() > 1 || mLastCharType != charType || texts.at(0).length() != 1;
    // append empty lines if needed
    int missingLines = qMin(mStartLine, mCurrentLine) + texts.count() - mEdit->document()->lineCount();
    if (missingLines > 0) {
        QTextBlock block = mEdit->document()->lastBlock();
        QTextCursor cursor(block);
        cursor.movePosition(QTextCursor::End);
        cursor.beginEditBlock();
        cursor.insertText(QString(missingLines, '\n'));
        cursor.endEditBlock();
        newUndoBlock = false;
    }
    if (qAbs(mStartLine-mCurrentLine) < texts.count() - 1) {
        if (mStartLine > mCurrentLine) mStartLine = mCurrentLine + texts.count() - 1;
        else mCurrentLine = mStartLine + texts.count() - 1;
    }

    int i = (qAbs(mStartLine-mCurrentLine)) % texts.count();
    QTextBlock block = mEdit->document()->findBlockByNumber(qMax(mCurrentLine, mStartLine));
    int fromCol = qMin(mColumn, mColumn+mSize);
    int toCol = qMax(mColumn, mColumn+mSize);
    if (fromCol == toCol && mOverwrite) ++toCol;
    QTextCursor cursor = mEdit->textCursor();
    int maxLen = 0;
    for (const QString &s: texts) {
        if (maxLen < s.length()) maxLen = s.length();
    }

    if (newUndoBlock) cursor.beginEditBlock();
    else cursor.joinPreviousEditBlock();

    QChar ch(' ');
    while (block.blockNumber() >= qMin(mCurrentLine, mStartLine)) {
//        if (ch=='.') ch='0'; else ch=',';
        QString addText = texts.at(i);
        if (maxLen && addText.length() < maxLen)
            addText += QString(maxLen-addText.length(), ch);
        int offsetFromEnd = fromCol - block.length()+1;
        if (offsetFromEnd > 0 && !texts.at(i).isEmpty()) {
            // line ends before start of mark -> calc additional spaces
            cursor.setPosition(block.position()+block.length()-1);
            QString s(offsetFromEnd, ch);
            addText = s + addText;
        } else if (fromCol != toCol) {
            // block-edit contains marking -> remove to end of block/line
            int pos = block.position()+fromCol;
            int rmSize = block.position()+qMin(block.length()-1, toCol) - pos;
            cursor.setPosition(pos);
            if (rmSize > 0) {
                cursor.setPosition(cursor.position()+rmSize, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            }
        } else {
            cursor.setPosition(block.position() + qMin(block.length()-1, fromCol));
        }
        if (!addText.isEmpty()) cursor.insertText(addText);
        block = block.previous();
        i = (i>0) ? i-1 : texts.count()-1;
    }
    // unjoin the Block-Edit insertion from the may-follow normal insertion
    cursor.insertText(" ");
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    if (mSize < 0) mColumn += mSize;
    int insertWidth = -1;
    for (QString s: texts) {
        for (QStringRef ref: s.splitRef('\n')) {
            if (insertWidth < ref.length()) insertWidth = ref.length();
        }
    }
    mColumn += insertWidth;
    setSize(0);
    mLastCharType = charType;
    cursor.endEditBlock();
}

BlockData::~BlockData()
{ }

QChar BlockData::charForPos(int relPos)
{
    for (int i = mparentheses.count()-1; i >= 0; --i) {
        if (mparentheses.at(i).relPos == relPos || mparentheses.at(i).relPos-1 == relPos) {
            return mparentheses.at(i).character;
        }
    }
    return QChar();
}

QVector<ParenthesesPos> BlockData::parentheses() const
{
    return mparentheses;
}

void BlockData::setParentheses(const QVector<ParenthesesPos> &parentheses)
{
    mparentheses = parentheses;
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    pos.setX(pos.x()-width());
    QMouseEvent e(event->type(), pos, event->button(), event->buttons(), event->modifiers());
    mCodeEditor->mousePressEvent(&e);
}

void LineNumberArea::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    pos.setX(pos.x()-width());
    QMouseEvent e(event->type(), pos, event->button(), event->buttons(), event->modifiers());
    mCodeEditor->mouseMoveEvent(&e);
}

void LineNumberArea::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    pos.setX(pos.x()-width());
    QMouseEvent e(event->type(), pos, event->button(), event->buttons(), event->modifiers());
    mCodeEditor->mouseReleaseEvent(&e);
}

} // namespace studio
} // namespace gams
