#ifndef _TAG_DIALOG_H
#define _TAG_DIALOG_H

#include "ui_tag_dialog.h"

class tag_dialog: public QDialog, private Ui::tag_dialog {
  Q_OBJECT
  public:
    enum ModificationState{
	  SAVE_EXPOSURE_DATE,
	  SAVE_MODIFICATION_DATE,
	  CHANGE_MODIFICATION_DATE
	} ;
	
    tag_dialog(QWidget *parent=NULL);
    ModificationState getModificationState();
};

#endif
