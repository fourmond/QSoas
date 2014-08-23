/*
  credits.cc: keeping track of who to thank really
  Copyright 2013 by CNRS/AMU

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
#include <commandeffector-templates.hh>
#include <command.hh>

QList<Credits *> * Credits::currentCredits = NULL;

void Credits::registerSelf()
{
  if(! currentCredits)
    currentCredits = new QList<Credits *>;
  (*currentCredits) << this;
}

Credits::Credits(const QString & n, const QString & a, 
                 const QString & u, const QString & d) :
  name(n), authors(QStringList() << a),
  urls(QStringList() << u), notice(d) 
{
  registerSelf();
}

Credits::Credits(const QString & n, const QStringList & a, 
                 const QStringList & u, const QString & d) :
  name(n), authors(a),
  urls(u), notice(d)
{
  registerSelf();
}

void Credits::displayCredits()
{
  if(! currentCredits)
    return;                     // Nothing to do

  for(int i = 0; i < currentCredits->size(); i++) {
    Credits * c = currentCredits->value(i);
    Terminal::out << "\n" << c->name << "\n"
                  << "Authors:\t" << c->authors.join("\n\t") << "\n"
                  << c->notice << "\n"
                  << "More info:\t"<< c->urls.join("\n\t") << endl;
  }
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
              "GNU General Public License for more details.");
              

//////////////////////////////////////////////////////////////////////

static void creditsCommand(const QString &)
{
  Credits::displayCredits();
}

static Command 
quit("credits", // command name
     optionLessEffector(creditsCommand), // action
     "file",  // group name
     NULL, // arguments
     NULL, // options
     "Credits",
     "Display credits");
