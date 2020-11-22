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
#include <helpbrowser.hh>

ArgumentEditor::ArgumentEditor(const Argument * arg, bool opt) :
  argument(arg),
  isOption(opt),
  argName(NULL),
  optName(NULL)
{
  editor = arg->createEditor();

  if(isOption) {
    optName = new QCheckBox("/" + arg->argumentName());
    connect(optName, SIGNAL(clicked(bool)), SLOT(enable(bool)));
    optName->setChecked(false);
    enable(false);
  }
  else {
    argName = new QLabel(arg->argumentName());
  }

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

void ArgumentEditor::enable(bool enabled)
{
  editor->setEnabled(enabled);
}

bool ArgumentEditor::isPresent() const
{
  if(! isOption)
    return true;
  return optName->isChecked();
}


void ArgumentEditor::setValue(const ArgumentMarshaller * arg)
{
  argument->setEditorValue(editor, arg);
}

ArgumentMarshaller * ArgumentEditor::getValue() const
{
  return argument->getEditorValue(editor);
}

QString ArgumentEditor::argumentName() const
{
  return argument->argumentName();
}

//////////////////////////////////////////////////////////////////////

ArgumentsWidget::ArgumentsWidget(const ArgumentList & args, bool opt,
                                 QWidget * parent) :
  QWidget(parent), arguments(args), optional(opt)
{
  QGridLayout * grid = new QGridLayout(this);
  
  for(int i = 0; i < arguments.size(); i++) {
    const Argument * arg = arguments[i];
    ArgumentEditor * ed = new ArgumentEditor(arg, optional);
    ed->addToGrid(grid, editors.size());
    editors << ed;
  }
}

ArgumentsWidget::~ArgumentsWidget()
{
}

void ArgumentsWidget::setOptions(CommandOptions & opts) const
{
  for(const ArgumentEditor * ed : editors) {
    QString n = ed->argumentName();
    if(opts.contains(n))
      delete opts.take(n);
    if(ed->isPresent())
      opts[n] = ed->getValue();
  }
}

CommandArguments ArgumentsWidget::asArguments() const
{
  CommandArguments rv;
  for(const ArgumentEditor * ed : editors)
    rv << ed->getValue();
  return rv;
}

void ArgumentsWidget::setFromOptions(const CommandOptions & opts)
{
  for(ArgumentEditor * ed : editors) {
    QString n = ed->argumentName();
    if(opts.contains(n))
      ed->setValue(opts[n]);
  }
}


//////////////////////////////////////////////////////////////////////


ArgumentsDialog::ArgumentsDialog(const Command * cmd) :
  QDialog(),
  command(cmd),
  arguments(NULL),
  options(NULL)
{
  QVBoxLayout * global = new QVBoxLayout(this);

  global->addWidget(new QLabel(QString("Preparing to launch command %1").arg(command->commandName())));

  global->addSpacing(10);

  const ArgumentList * args = command->commandArguments();
  if(args && args->size() > 0) {
    global->addWidget(new QLabel("<b>Arguments:</b>"));
    arguments = new ArgumentsWidget(*args, false);
    global->addWidget(arguments);
  }

  args = command->commandOptions();
  if(args && args->size() > 0) {
    global->addWidget(new QLabel("<b>Options:</b>"));
    options = new ArgumentsWidget(*args, true);
    global->addWidget(options);
  }

  QDialogButtonBox * buttons =
    new QDialogButtonBox(QDialogButtonBox::Ok |
                         QDialogButtonBox::Cancel |
                         QDialogButtonBox::Help,
                         Qt::Horizontal);
  global->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), SLOT(reject()));
  connect(buttons, SIGNAL(helpRequested()), SLOT(showHelp()));
}

ArgumentsDialog::~ArgumentsDialog()
{
}

bool ArgumentsDialog::doFullPrompt(const Command * cmd,
                                   CommandArguments * args,
                                   CommandOptions * opts)
{

  ArgumentsDialog dlg(cmd);
  // For now not setting from arguments...
  if(dlg.exec() == QDialog::Accepted) {
    dlg.retrieveArgumentsAndOptions(args, opts);
    return true;
  }
  else
    return false;
}

void ArgumentsDialog::retrieveArgumentsAndOptions(CommandArguments * args,
                                                  CommandOptions * opts) const
{
  // First empty the arguments
  for(ArgumentMarshaller * val : *args)
    delete val;
  args->clear();

  for(ArgumentMarshaller * val : opts->values())
    delete val;

  opts->clear();

  if(arguments) 
    *args = arguments->asArguments();

  if(options)
    options->setOptions(*opts);
}

void ArgumentsDialog::showHelp()
{
  HelpBrowser::browseCommand(command);
}
