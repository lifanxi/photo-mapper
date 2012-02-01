/*

    PhotoMapper - Tag Photos with GPS data
    Copyright (C) 2007 Christoffer Hallqvist, COPIKS

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    www.copiks.se
*/

#include "tag_dialog.h"

tag_dialog::tag_dialog(QWidget *parent) {
  setupUi(this);
}

tag_dialog::ModificationState tag_dialog::getModificationState() {
  if (preserveRadioButton->isChecked()) return SAVE_MODIFICATION_DATE;
  if (exposureRadioButton->isChecked()) return SAVE_EXPOSURE_DATE;
  if (noneRadioButton->isChecked()) return CHANGE_MODIFICATION_DATE;
  return SAVE_MODIFICATION_DATE; // Shouldn't get here
}
