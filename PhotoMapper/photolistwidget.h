#ifndef PHOTOLISTWIDGET_H
#define PHOTOLISTWIDGET_H

#include <QTableWidget>
#include <QMenu>

class PhotoListWidget: public QTableWidget {
  Q_OBJECT
  public:
    PhotoListWidget(QWidget *parent=0);
  public slots:
    void menuTriggered(QAction*);
  protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);
  signals:
    void removeSelected();
    void zoomToSelected();
  private:
    QMenu contextMenu;
    QAction *removeAction;
    QAction *zoomAction;
};

#endif // PHOTOLISTWIDGET_H
