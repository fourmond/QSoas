/*
  credits.cc: keeping track of who to thank really
  Copyright 2013, 2015 by CNRS/AMU

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
#include <credits.hh>

#include <terminal.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>
#include <command.hh>

QList<Credits *> * Credits::currentCredits = NULL;

void Credits::registerSelf()
{
  if(! currentCredits)
    currentCredits = new QList<Credits *>;
  (*currentCredits) << this;
}

Credits::Credits(const QString & n, const QStringList & a, 
                 const QStringList & u, const QString & d,
                 Credits::Kind k, const QString & f) :
  name(n), authors(a),
  urls(u), notice(d), kind(k), fileName(f)
{
  registerSelf();
}

QString Credits::text(bool full) const
{
  QString c("%1:\nAuthors: %2\n%3\nRefs: %4\n");
  QString nt;
  if(full) {
    QFile f(fileName);
    f.open(QIODevice::ReadOnly);
    nt = f.readAll();
  }
  else {
    nt = notice;
  }
  return c.arg(name, authors.join(", "), nt, urls.join(", "));
}

void Credits::displayCredits(bool full)
{
  if(! currentCredits)
    return;                     // Nothing to do

  for(int i = 0; i < currentCredits->size(); i++) {
    Credits * c = currentCredits->value(i);
    Terminal::out << c->text(full) << endl;
  }
  if(! full) {
    Terminal::out << "To obtain the full text of the licenses, use the /full=true option" << endl;
  }
}


void Credits::displayStartupMessage()
{
  QStringList prjs;
  if(currentCredits) {
    for(int i = 0; i < currentCredits->size(); i++) {
      Credits * c = currentCredits->value(i);
      if(c->kind == Projects)
        prjs << c->name;
    }
  }
  Terminal::out << "Copyright 2011 by Vincent Fourmond\n"
                << "          2012-2015 by CNRS/AMU\n\n"
                << "Based on Christophe Leger's original Soas\n\n"
                << "This program is free software, released under the terms of "
                << "the GNU general public license (see http://www.gnu.org/copyleft/gpl.html)\n\n"
                << "QSoas includes or links to code from the following projects: "
                << prjs.join(", ")
                << "\nFor more information, run the command 'credits'\n\n"
                << "Thanks for thanking !\nPlease refer to Fourmond, Anal Chem, "
                << "2016, DOI: 10.1021/acs.analchem.6b00224\n"
                << endl;
}

Credits qsoas("QSoas itself", 
              QStringList() << "Vincent Fourmond" << "Christophe Leger", 
              QStringList() << "http://bip.cnrs-mrs.fr/bip06/software.html"
              << "http://www.gnu.org/licenses/gpl.html",
              "QSoas is free software; you can redistribute it and/or modify "
              "it under the terms of the GNU General Public License as published by "
              "the Free Software Foundation; either version 2 of the License, or "
              "(at your option) any later version.\n\n"
              "This program is distributed in the hope that it will be useful, "
              "but WITHOUT ANY WARRANTY; without even the implied warranty of "
              "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
              "GNU General Public License for more details.",
              Credits::QSoas,
              ":/licenses/GPL-2.txt");

Credits ruby("mruby", 
             QStringList() << "mrubyby developers",
             QStringList() << "http://mruby.org",
             "mruby is copyrighted free software released under the terms of the 'MIT' license",
             Credits::Projects,
             ":/licenses/mruby.txt");

Credits gsl("GSL", 
            QStringList() << "Brian Gough and others",
            QStringList() << "http://www.gnu.org/software/gsl/",
            "The GNU Scientific Library is free software; "
            "you can redistribute it and/or modify it under the terms "
            "of the GNU General Public License as published by the "
            "Free Software Foundation; either version 3 of the License, "
            "or (at your option) any later version.\n\n"
            "This program is distributed in the hope that it will be "
            "useful, but WITHOUT ANY WARRANTY; without even the implied "
            "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR "
            "PURPOSE.  See the GNU General Public License for more details.",
            Credits::Projects,
            ":/licenses/GPL-3.txt");

Credits qt("Qt4", 
           QStringList() 
           << "Nokia Corporation and/or its subsidiary(-ies)"
           << "Trolltech ASA",
           QStringList() << "http://qt-project.org/",
           "Commercial Usage\n"
           "Licensees holding valid Qt Commercial licenses may use this file in accordance with the Qt Commercial License Agreement provided with the Software or, alternatively, in accordance with the terms contained in a written agreement between you and Nokia.\n\n"
           "GNU Lesser General Public License Usage\n"
           "Alternatively, this file may be used under the terms of the GNU Lesser General Public License version 2.1 as published by the Free Software Foundation and appearing in the file LICENSE.LGPL included in the packaging of this file.  Please review the following information to ensure the GNU Lesser General Public License version 2.1 requirements will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.\n\n"
           "In addition, as a special exception, Nokia gives you certain additional rights. These rights are described in the Nokia Qt LGPL Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this package.\n\n"
           "GNU General Public License Usage\n"
           "Alternatively, this file may be used under the terms of the GNU General Public License version 3.0 as published by the Free Software Foundation and appearing in the file LICENSE.GPL included in the packaging of this file.  Please review the following information to ensure the GNU General Public License version 3.0 requirements will be met: http://www.gnu.org/copyleft/gpl.html.",
           Credits::Projects,
           ":/licenses/LGPL-2.1.txt");


//////////////////////////////////////////////////////////////////////

static void creditsCommand(const QString &, const CommandOptions &opts)
{
  bool full = false;
  updateFromOptions(opts, "full", full);
  Credits::displayCredits(full);
}

ArgumentList crOpts(QList<Argument*>() 
                    << new BoolArgument("full", 
                                        "Full text",
                                        "Full text of the licenses"));
                    

static Command 
credits("credits", // command name
     effector(creditsCommand), // action
     "help",  // group name
     NULL, // arguments
     &crOpts, // options
     "Credits",
     "Display credits");
