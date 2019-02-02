#ifndef NESTEDHEADERVIEW_H
#define NESTEDHEADERVIEW_H

#include "gdxsymbol.h"

#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>

namespace gams {
namespace studio {
namespace gdxviewer {

class NestedHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    NestedHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~NestedHeaderView() override;
    void setModel(QAbstractItemModel *model) override;

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize sectionSizeFromContents(int logicalIndex) const override;

private:
    int pointToDimension(QPoint p);
    void bindScrollMechanism();

    GdxSymbol* sym() const;
    int dim() const;
    QPoint mMousePos = QPoint(-1,-1);
    QPoint mDragStartPosition;
};

} // namespace gdxviewer
} // namespace studio
} // namespace gams

#endif // NESTEDHEADERVIEW_H
