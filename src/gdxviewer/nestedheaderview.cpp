#include "nestedheaderview.h"

#include <QTableView>
#include <QScrollBar>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QMap>

namespace gams {
namespace studio {
namespace gdxviewer {

NestedHeaderView::NestedHeaderView(Qt::Orientation orientation, QWidget *parent)
    :QHeaderView(orientation, parent)
{
    setAcceptDrops(true);
}

NestedHeaderView::~NestedHeaderView()
{

}

void NestedHeaderView::setModel(QAbstractItemModel *model)
{
    QHeaderView::setModel(model);
    bindScrollMechanism();
}

void NestedHeaderView::reset()
{
    if (this->model() && orientation() == Qt::Vertical) {
        int dimension = dim();
        vhSectionWidth.clear();
        vhSectionWidth.resize(dimension);

        QFont fnt = font();
        fnt.setBold(true);

        QFontMetrics fm(fnt);

        for (int i=0; i<dimension; i++) {
            QVector<QList<QString>> labelsInRows = sym()->labelsInRows();
            for (QString label : labelsInRows.at(i))
                vhSectionWidth.replace(i, qMax(vhSectionWidth.at(i), fm.width(label)));
        }

        QStyleOptionHeader opt;
        initStyleOption(&opt);
        //TODO CW: The size is not completely correct. We need to adjust the width using the styles margins/paddings, etc
        int borderWidth = 10;
        for (int i=0; i<dimension; i++)
            vhSectionWidth.replace(i, vhSectionWidth.at(i) + borderWidth);
    }
    QHeaderView::reset();
}

int NestedHeaderView::dim() const
{
    if (orientation() == Qt::Vertical && sym()->needDummyRow())
        return 1;
    else if (orientation() == Qt::Horizontal && sym()->needDummyColumn())
        return 1;
    else if (orientation() == Qt::Vertical)
        return sym()->dim() - sym()->tvColDim();
    else {
        int d = sym()->tvColDim();
        if (sym()->type() == GMS_DT_VAR || sym()->type() == GMS_DT_EQU)
            return d+1;
        return d;
    }
}

void NestedHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    if (!rect.isValid())
        return;
    QStyleOptionHeader opt;
    initStyleOption(&opt);

    opt.rect = rect;
    opt.section = logicalIndex;

    QStringList labelCurSection = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toStringList();
    QStringList labelPrevSection;

    // first section needs always show all labels
    if (logicalIndex > 0 && sectionViewportPosition(logicalIndex) !=0) {
        int prevIndex = logicalIndex -1;
        while (prevIndex >0 && ((QTableView*)this->parent())->isColumnHidden(prevIndex)) //find the preceding column that is not hidden
            prevIndex--;
        labelPrevSection = model()->headerData(prevIndex, orientation(), Qt::DisplayRole).toStringList();
    }
    else {
        for(int i=0; i<dim(); i++)
            labelPrevSection << "";
    }
    QPointF oldBO = painter->brushOrigin();

    int lastRowWidth = 0;
    int lastHeight = 0;

    if(orientation() == Qt::Vertical) {
        for(int i=0; i<dim(); i++) {
            QStyle::State state = QStyle::State_None;
            if (isEnabled())
                state |= QStyle::State_Enabled;
            if (window()->isActiveWindow())
                state |= QStyle::State_Active;
            int rowWidth = vhSectionWidth.at(i);

            if (labelPrevSection[i] != labelCurSection[i])
                opt.text = labelCurSection[i];
            else
                opt.text = "";
            opt.rect.setLeft(opt.rect.left()+ lastRowWidth);\
            lastRowWidth = rowWidth;
            opt.rect.setWidth(rowWidth);

            if (opt.rect.contains(mMousePos))
                state |= QStyle::State_MouseOver;
            opt.state = state;
            opt.textAlignment = Qt::AlignLeft | Qt::AlignVCenter;
            style()->drawControl(QStyle::CE_Header, &opt, painter, this);
            if (dimIdxEnd == i) {
                painter->restore();
                painter->drawLine(opt.rect.left(), opt.rect.top(), opt.rect.left(), opt.rect.bottom());
                painter->save();
            } else if (dimIdxEnd-1 == i && dimIdxEnd == dim()) {
                painter->restore();
                painter->drawLine(opt.rect.right(), opt.rect.top(), opt.rect.right(), opt.rect.bottom());
                painter->save();
            }
        }
    } else {
        for(int i=0; i<dim(); i++) {
            QStyle::State state = QStyle::State_None;
            if (isEnabled())
                state |= QStyle::State_Enabled;
            if (window()->isActiveWindow())
                state |= QStyle::State_Active;

            if (labelPrevSection[i] != labelCurSection[i])
                opt.text = labelCurSection[i];
            else
                opt.text = "";
            opt.rect.setTop(opt.rect.top()+ lastHeight);
            lastHeight = QHeaderView::sectionSizeFromContents(logicalIndex).height();

            opt.rect.setHeight(QHeaderView::sectionSizeFromContents(logicalIndex).height());
            if (opt.rect.contains(mMousePos))
                state |= QStyle::State_MouseOver;
            opt.state = state;
            style()->drawControl(QStyle::CE_Header, &opt, painter, this);
            if (dimIdxEnd == i) {
                painter->restore();
                painter->drawLine(opt.rect.left(), opt.rect.top(), opt.rect.right(), opt.rect.top());
                painter->save();
            } else if (dimIdxEnd-1 == i && dimIdxEnd == dim()) {
                painter->restore();
                if (sym()->type() == GMS_DT_VAR || sym()->type() == GMS_DT_EQU)
                    painter->drawLine(opt.rect.left(), opt.rect.top(), opt.rect.right(), opt.rect.top());
                else
                    painter->drawLine(opt.rect.left(), opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
                painter->save();
            }
        }
    }
    painter->setBrushOrigin(oldBO);
    painter->restore();
}

void NestedHeaderView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mDragStartPosition = event->pos();
        dimIdxStart = pointToDimension(event->pos());
    }
    QHeaderView::mousePressEvent(event);
}

void NestedHeaderView::mouseMoveEvent(QMouseEvent *event)
{
    QHeaderView::mouseMoveEvent(event);
    mMousePos = event->pos();
    if ((event->buttons() & Qt::LeftButton) && (mMousePos - mDragStartPosition).manhattanLength() > QApplication::startDragDistance()) {
        // do not allow to drag a dummy row/column
        if (orientation() == Qt::Vertical && pointToDimension(mDragStartPosition) == 0 && sym()->needDummyRow())
            return;
        if (orientation() == Qt::Horizontal && pointToDimension(mDragStartPosition) == 0 && sym()->needDummyColumn())
            return;
        //do not allow to drag the value column (lavel, marginal,...) of variables and equations
        if (orientation() == Qt::Horizontal && (sym()->type() == GMS_DT_EQU || sym()->type() == GMS_DT_VAR) && pointToDimension(mDragStartPosition)==dim()-1)
            return;
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        if (orientation() == Qt::Vertical)
            mimeData->setData("GDXDRAGDROP/COL", QByteArray::number(pointToDimension(mDragStartPosition)));
        else
            mimeData->setData("GDXDRAGDROP/ROW", QByteArray::number(pointToDimension(mDragStartPosition)));
        drag->setMimeData(mimeData);
        drag->exec();
    }

    if(orientation() == Qt::Vertical)
        headerDataChanged(orientation(),0, qMax(0,model()->rowCount()-1));
    else
        headerDataChanged(orientation(),0, qMax(0,model()->columnCount()-1));
}

void NestedHeaderView::dragEnterEvent(QDragEnterEvent *event)
{
    decideAcceptDragEvent(event);
    event->acceptProposedAction();
}

void NestedHeaderView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("GDXDRAGDROP/ROW")) {
        dimIdxStart = event->mimeData()->data("GDXDRAGDROP/ROW").toInt();
        dragOrientationStart = Qt::Horizontal;
    }
    else {
        dimIdxStart = event->mimeData()->data("GDXDRAGDROP/COL").toInt();
        dragOrientationStart = Qt::Vertical;
    }
    dimIdxEnd = pointToDropDimension(event->pos());
    dragOrientationEnd = orientation();
    decideAcceptDragEvent(event);
    viewport()->update();
}

void NestedHeaderView::decideAcceptDragEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("GDXDRAGDROP/COL") || event->mimeData()->hasFormat("GDXDRAGDROP/ROW"))
        event->acceptProposedAction();
    else
        event->ignore();
}

int NestedHeaderView::toGlobalDim(int localDim, int orientation)
{
    if (orientation == Qt::Horizontal)
        return localDim + sym()->dim()-sym()->tvColDim();
    else
        return localDim;

}

void NestedHeaderView::dropEvent(QDropEvent *event)
{
    if (dragOrientationEnd == Qt::Horizontal)
        dimIdxEnd += sym()->dim()-sym()->tvColDim();
    if (dragOrientationStart == Qt::Horizontal)
        dimIdxStart += sym()->dim()-sym()->tvColDim();

    if (dimIdxStart == dimIdxEnd && dragOrientationStart == dragOrientationEnd) { //nothing happens
        event->accept();
        dimIdxEnd = -1;
        dimIdxStart = -1;
        return;
    }

    //if (orientation() == Qt::Horizontal && sym()->needDummyColumn())
    //    dimIdxEnd--;

    int newColDim = sym()->tvColDim();
    if (dragOrientationStart != dragOrientationEnd) {
        if (dragOrientationStart == Qt::Vertical)
            newColDim++;
        else
            newColDim--;
    }
    QVector<int> tvDims = sym()->tvDimOrder();

    if (dimIdxStart < dimIdxEnd)
        dimIdxEnd--;
    tvDims.move(dimIdxStart, dimIdxEnd);

    sym()->setTableView(newColDim, tvDims);
    event->accept();

    (static_cast<QTableView*>(this->parent()))->horizontalHeader()->geometriesChanged();
    (static_cast<QTableView*>(this->parent()))->verticalHeader()->geometriesChanged();

    dimIdxEnd = -1;
    dimIdxStart = -1;
}

void NestedHeaderView::dragLeaveEvent(QDragLeaveEvent *event)
{
    QHeaderView::dragLeaveEvent(event);
    dimIdxEnd = -1;
    viewport()->update();
}

void NestedHeaderView::leaveEvent(QEvent *event)
{
    QHeaderView::leaveEvent(event);
    mMousePos = QPoint(-1,-1);
}

int NestedHeaderView::pointToDimension(QPoint p)
{
    if (orientation() == Qt::Vertical) {
        int totWidth = 0;
        for(int i=0; i<dim(); i++) {
            totWidth += vhSectionWidth.at(i);
            if (p.x() < totWidth)
                return i;
        }
    } else {
        int sectionHeight = QHeaderView::sectionSizeFromContents(0).height();
        int totHeight = 0;
        for(int i=0; i<dim(); i++) {
            totHeight += sectionHeight;
            if (p.y() < totHeight)
                return i;
        }
    }
    return 0; //is never reached
}

int NestedHeaderView::pointToDropDimension(QPoint p)
{
    int globalStart = toGlobalDim(dimIdxStart, dragOrientationStart);
    int globalEnd   = toGlobalDim(pointToDimension(p), dragOrientationEnd);

    if ((sym()->type() == GMS_DT_EQU || sym()->type() == GMS_DT_VAR) && globalEnd == sym()->dim()) {
        if (globalStart+1 == globalEnd)
            return dimIdxStart;
        else
            return dim()-1;
    }

    //special behavior for switching adjacent dimensions when start and end orientation is the same
    if (dragOrientationStart == dragOrientationEnd) {
        if (globalStart == globalEnd)
            return dimIdxStart;
        else if (globalStart+1 == globalEnd)
            return dimIdxStart+2;
        else if (globalStart-1 == globalEnd)
            return dimIdxStart-1;
    }
    if (dragOrientationEnd == Qt::Vertical) {
        if (sym()->needDummyRow())
            return 0;
        int totWidth = 0;
        for(int i=0; i<dim(); i++) {
            int curWidth = vhSectionWidth.at(i);
            totWidth += curWidth;
            if (p.x() < totWidth) {
                int relX = p.x() - totWidth + curWidth;
                if (relX > curWidth/2)
                    i++;
                return i;
            }
        }
    } else {
        if (sym()->needDummyColumn())
            return 0;
        int sectionHeight = QHeaderView::sectionSizeFromContents(0).height();
        int totHeight = 0;
        for(int i=0; i<dim(); i++) {
            totHeight += sectionHeight;
            if (p.y() < totHeight) {
                int relY = p.y() - totHeight + sectionHeight;
                if (relY > sectionHeight/2)
                    i++;
                return i;
            }
        }
    }
    return 0; //is never reached
}

void NestedHeaderView::bindScrollMechanism()
{
    // need to update the first visible sections when scrolling in order to trigger the repaint for showing all labels for the first section
    if (orientation() == Qt::Vertical)
        connect((static_cast<QTableView*>(parent()))->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() { model()->headerDataChanged(this->orientation(), 0, 2); });
    else
        connect((static_cast<QTableView*>(parent()))->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { model()->headerDataChanged(this->orientation(), 0, 2); });
}

TableViewModel *NestedHeaderView::sym() const
{
    return static_cast<TableViewModel*>(model());
}

QSize NestedHeaderView::sectionSizeFromContents(int logicalIndex) const
{
    if (orientation() == Qt::Vertical) {
        QSize s(0,sectionSize(logicalIndex));
        int totWidth = 0;
        for (int i=0; i<dim(); i++)
            totWidth += vhSectionWidth.at(i);
        s.setWidth(totWidth);
        return s;
    } else {
        QSize s = QHeaderView::sectionSizeFromContents(logicalIndex);
        s.setHeight(s.height()*dim());
        return s;
    }
}


} // namespace gdxviewer
} // namespace studio
} // namespace gams
