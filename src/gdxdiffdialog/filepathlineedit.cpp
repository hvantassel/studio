#include "filepathlineedit.h"
#include <QMimeData>

namespace gams {
namespace studio {
namespace gdxdiffdialog {

FilePathLineEdit::FilePathLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    this->setAcceptDrops(true);
}

void FilePathLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1 && event->mimeData()->urls().at(0).toLocalFile().endsWith(".gdx")) {
        event->acceptProposedAction();
    } else
        event->ignore();
}

void FilePathLineEdit::dropEvent(QDropEvent *event)
{
    QString localFile = event->mimeData()->urls().at(0).toLocalFile();
    setText(localFile);
}

} // namespace gdxdiffdialog
} // namespace studio
} // namespace gams
