/*
  argumentsdialog.cc: implementation of arguments/option prompting
  Copyright 2019 by CNRS/AMU

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
#include <argumentsdialog.hh>
#include <argument.hh>
#include <argumentlist.hh>
#include <command.hh>

#include <debug.hh>

ArgumentEditor::ArgumentEditor(const Argument * arg, bool opt) :
  argName(NULL), optName(NULL), isOption(opt)
{
  if(isOption) {
    optName = new QCheckBox("/" + arg->argumentName());
  }
  else {
    argName = new QLabel(arg->argumentName());
  }

  editor = arg->createEditor();
}

ArgumentEditor::~ArgumentEditor()
{
}

void ArgumentEditor::addToGrid(QGridLayout * target, int row)
{
  if(isOption)
    target->addWidget(optName, row, 0);
  else
    target->addWidget(argName, row, 0);
  target->addWidget(editor, row, 1);
}



//////////////////////////////////////////////////////////////////////


ArgumentsDialog::ArgumentsDialog(const Command * cmd) : QDialog(),
                                                        command(cmd)
{
  QVBoxLayout * global = new QVBoxLayout(this);

  global->addWidget(new QLabel(QString("Preparing to launch command %1").arg(command->commandName())));

  global->addSpacing(10);

  const ArgumentList * args = command->commandArguments();
  if(args && args->size() > 0) {
    global->addWidget(new QLabel("<b>Arguments:</b>"));
    QGridLayout * grid = new QGridLayout;

    for(const Argument * arg : *args) {
      ArgumentEditor * ed = new ArgumentEditor(arg, false);
      ed->addToGrid(grid, arguments.size());
      arguments << ed;
    }
    global->addLayout(grid);
  }

  args = command->commandOptions();
  if(args && args->size() > 0) {
    global->addWidget(new QLabel("<b>Options:</b>"));
    QGridLayout * grid = new QGridLayout;

    for(const Argument * arg : *args) {
      ArgumentEditor * ed = new ArgumentEditor(arg, true);
      ed->addToGrid(grid, options.size());
      options << ed;
    }
    global->addLayout(grid);
  }

  QDialogButtonBox * buttons =
    new QDialogButtonBox(QDialogButtonBox::Ok |
                         QDialogButtonBox::Cancel,
                         Qt::Horizontal);
  global->addWidget(buttons);
}

ArgumentsDialog::~ArgumentsDialog()
{
}

bool ArgumentsDialog::doFullPrompt(const Command * cmd,
                                   CommandArguments * args,
                                   CommandOptions * opts)
{

  ArgumentsDialog dlg(cmd);
  dlg.exec();

  return false;                 // always cancelled ?
}
