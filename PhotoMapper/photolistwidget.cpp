#include "photolistwidget.h"
#include <QContextMenuEvent>

PhotoListWidget::PhotoListWidget(QWidget *parent):
    QTableWidget(parent) {
    removeAction = contextMenu.addAction(QString("Remove selected"));
    zoomAction = contextMenu.addAction(QString("Zoom map to selected"));
    connect(&contextMenu,SIGNAL(triggered(QAction*)),
            this,SLOT(menuTriggered(QAction*)));
}

void PhotoListWidget::contextMenuEvent(QContextMenuEvent *e) {
    contextMenu.popup(e->globalPos());
}

void PhotoListWidget::menuTriggered(QAction *action) {
  if (action == removeAction) {
    emit removeSelected();
  }
  if (action == zoomAction) {
    emit zoomToSelected();
  }
}
