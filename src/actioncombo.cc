/*
  actioncombo.cc: implementation of the ActionCombo class
  Copyright 2011 by Vincent Fourmond

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <headers.hh>
#include <actioncombo.hh>

ActionCombo::ActionCombo(const QString & title)
{
  addItem(title);
  connect(this, SIGNAL(activated(int)), SLOT(elementSelected(int)));
}

void ActionCombo::addAction(const QString & name, QObject * receiver, 
                            const char * slot, const QKeySequence & shortCut)
{
  QString str = name;
  if(! shortCut.isEmpty())
    str += "   (" + shortCut.toString() + ")";
  addItem(str);
  QAction * ac = createAction(name, receiver, slot, shortCut, this);
  actions << ac;
  QWidget::addAction(ac);
}

void ActionCombo::elementSelected(int idx)
{
  if(idx > 0) {
    actions[idx-1]->trigger();
    setCurrentIndex(0);         // Reset to the first, non-active,
                                // element
  }
}

QAction * ActionCombo::createAction(const QString & name, 
                                    QObject * receiver, 
                                    const char * slot, 
                                    const QKeySequence & shortCut,
                                    QObject * parent)
{
  QAction * ac = new QAction(name, parent);
  receiver->connect(ac, SIGNAL(triggered()), slot);
  if(! shortCut.isEmpty()) {
    ac->setShortcut(shortCut);
    ac->setShortcutContext(Qt::WindowShortcut);
  }
  return ac;
}
