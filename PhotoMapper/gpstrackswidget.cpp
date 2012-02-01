#include "gpstrackswidget.h"
#include <stdio.h>
#include <QContextMenuEvent>

GpsTracksWidget::GpsTracksWidget(QWidget *parent):
  QTreeWidget(parent),
  nSelected(0),
  removeAction(0) {
  selectionList = new QList<int>();
  removeAction = contextMenu.addAction(QString("Remove tracks"));
  supressEmits = false;
  connect(&contextMenu,SIGNAL(triggered(QAction*)),
          this,SLOT(menuTriggered(QAction*)));  
  connect(this,SIGNAL(itemSelectionChanged()),
          this,SLOT(selectionChanged()));
}

void GpsTracksWidget::contextMenuEvent(QContextMenuEvent *e) {
 contextMenu.popup(e->globalPos());
}

void GpsTracksWidget::menuTriggered(QAction *action) {
  if (action->text() == "Remove tracks") {
    emit removeSelected();
  }
}

// Method called by application which makes sure emits are not made when user
// has not invoked selection.
void GpsTracksWidget::applicationSelect(QTreeWidgetItem *selItem, bool selected) {
  supressEmits = true;
  selItem->setSelected(selected);
  supressEmits = false;
}


void GpsTracksWidget::selectionChanged() {
  QList<int> oldSelection = *selectionList;
  nSelected = selectedItems().count();
  selectionList->clear();

  QTreeWidgetItem *rootItem = invisibleRootItem();
  for (int i=0; i<rootItem->childCount(); i++) {
    if (rootItem->child(i)->isSelected()) selectionList->append(i);
  }

  QList<int> deletedItems;
  QList<int> addedItems;

  for (int i=0; i<oldSelection.count(); i++) {
    if (selectionList->indexOf(oldSelection.at(i)) == -1) {
        deletedItems.append(oldSelection.at(i));
    }
  }

  for (int i=0; i<selectionList->count(); i++) {
    if (oldSelection.indexOf(selectionList->at(i)) == -1) {
      addedItems.append(selectionList->at(i));
    }
  }

  if (!supressEmits) {
    emit(selectionListChanged(selectionList));
    emit(selectionAdded(addedItems));
    emit(selectionDeleted(deletedItems));
  }
  
  if (nSelected == 0) {
    removeAction->setEnabled(false); 
  } else {
    removeAction->setEnabled(true);
  }
}

