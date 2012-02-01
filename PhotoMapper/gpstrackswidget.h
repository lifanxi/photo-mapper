#ifndef GPSTRACKSWIDGET_H
#define GPSTRACKSWIDGET_H

#include <QTreeWidget>
#include <QMenu>

class GpsTracksWidget: public QTreeWidget {
  Q_OBJECT
  public: 
    GpsTracksWidget(QWidget *parent=0);
    void setItemsSelected(int); // Set number of selected items
    void applicationSelect(QTreeWidgetItem *selItem, bool selected);
  protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);
  public slots:
    void menuTriggered(QAction*);
  private slots:
    void selectionChanged();
  signals:
    void removeSelected();
    void selectionListChanged(QList<int>*);
    void selectionAdded(QList<int>);
    void selectionDeleted(QList<int>);
  private:
    QMenu contextMenu;
    int nSelected;
    QAction *removeAction;
    QList<int> *selectionList;
    bool supressEmits;
};

#endif
