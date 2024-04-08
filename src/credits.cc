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

#include <file.hh>

QList<Credits *> * Credits::currentCredits = NULL;

void Credits::registerSelf()
{
  if(! currentCredits)
    currentCredits = new QList<Credits *>;
  (*currentCredits) << this;
}

Credits::Credits(const QString & n, const QStringList & a, 
                 const QStringList & u, const QString & d,
                 const QString & w,
                 Credits::Kind k, const QString & f,
                 const QString & ex) :
  name(n), authors(a),
  urls(u), notice(d), what(w), kind(k), fileName(f), extra(ex)
{
  registerSelf();
}

Credits::Credits(const QString & cite,
          const QString & w,
          const QString & doi) :
  name(cite), what(w), kind(Credits::Paper)
{
  urls << doi;
  registerSelf();
}

QString Credits::text(bool full) const
{
  
  QString fmt;
  switch(kind) {
  case QSoas:
    fmt = "%1%5:\nAuthors: %2\n%3\n%6\nRefs: %4\n";
    break;
  case Projects:
    fmt = "%1 -- %5\nAuthors: %2\n%3\nRefs: %4\n";
    break;
  case Paper:
    if(urls.size() > 0 && urls[0].startsWith("10.10"))
      fmt = "%1 -- %5, DOI: %4%2%3";
    else
      fmt = "%1 -- %5, URL: %4%2%3";
  }
  
  QString nt;
  if(full && (kind != Paper))
    nt = File::readFile(fileName);
  else {
    nt = notice;
  }
  return fmt.arg(name, authors.join(", "), nt, urls.join(", "), what, extra);
}


QString Credits::docText() const
{
  QStringList linkList;
  for(const QString & lnk : urls) {
    if(lnk.startsWith("10.10")) {
      linkList << QString("[DOI: %1](https://doi.org/%2)").
        arg(lnk).arg(lnk);
    }
    else {
      linkList << QString("[`%1`](%2)").
        arg(lnk).arg(lnk);
    }
  }
  switch(kind) {
  case QSoas:
  case Projects: 
    return QString("%1 -- %2: Authors: %3\n%5See also: %4\n").
      arg(name).arg(what).arg(authors.join(", ")).
      arg(linkList.join(", "), extra);
  case Paper:
    return QString("%1 -- %2: %4\n").
      arg(name).arg(what).
      arg(linkList.join(", "));
  }
  return "";
}

QString Credits::docString()
{
  if(! currentCredits)
    return QString();
  QList<Credits*> lst = (*currentCredits);
  std::sort(lst.begin(), lst.end(), [](const Credits * a,
                                       const Credits * b) -> bool {
    if(a->kind != b->kind)
      return a->kind < b->kind;
    return a->name < b->name;
  });

  QString rv;
  for(const Credits * c : lst)
    rv += QString(" * %1\n").arg(c->docText());
  return rv;
}



QString Credits::creditString(bool full)
{
  QString rv;
  if(! currentCredits)
    return rv;                     // Nothing to do

  QTextStream out(&rv);
  for(int i = 0; i < currentCredits->size(); i++) {
    Credits * c = currentCredits->value(i);
    if(i > 0)
      out << "----------------------------------------------------------------------\n";
    out << c->text(full) << endl;
  }
  return rv;
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
                << "          2012-2023 by CNRS/AMU\n\n"
                << "Based on Christophe Leger's original SOAS\n\n"
                << "This program is free software, released under the terms of "
                << "the GNU general public license (see http://www.gnu.org/copyleft/gpl.html)\n\n"
                << "QSoas includes or links to code from the following projects: "
                << prjs.join(", ")
                << "\nFor more information, run the command 'credits'\n\n"
                << "Thanks for thanking !\nPlease refer to Fourmond, Anal Chem, "
                << "2016, DOI: 10.1021/acs.analchem.6b00224\n"
                << endl;
}

// 
Credits qsoas("QSoas itself", 
              QStringList() << "Vincent Fourmond" << "Christophe Leger", 
              QStringList() << "http://qsoas.org"
              << "http://www.gnu.org/licenses/gpl.html",
              "QSoas is free software; you can redistribute it and/or modify "
              "it under the terms of the GNU General Public License as published by "
              "the Free Software Foundation; either version 2 of the License, or "
              "(at your option) any later version.\n\n"
              "This program is distributed in the hope that it will be useful, "
              "but WITHOUT ANY WARRANTY; without even the implied warranty of "
              "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
              "GNU General Public License for more details."
              ,
              "", 
              Credits::QSoas,
              ":/licenses/GPL-2.txt",
              "QSoas was developed based on the ideas of Christophe Leger in the original QSoas, and using thanks to the bug reports and/or suggestions of many enthusiastic users, including, but not limited to, in random order: "
              "Christina Felbek, "
              "Christophe Léger, "
              "Matteo Sensi, "
              "Marta Meneghello, "
              "Meriem Merrouch, "
              "Asmaa Hadj-Ahmed, "
              "Ana Rita Oliveira, "
              "Nazua Costa, "
              "Christophe Orain, "
              "Melisa del Barrio, "
              "Carole Baffert, "
              "Pierre Ceccaldi, "
              "Patrick Bertrand, "
              "Andrea Fasano, "
              "Anna Aldinio-Colbachini, "
              "Laura Opdam"
              "\n");

Credits ruby("mruby", 
             QStringList() << "mruby developers",
             QStringList() << "http://mruby.org",
             "mruby is copyrighted free software released under the terms of the 'MIT' license",
             "embedded ruby interpreter, for formulas and scripting",
             Credits::Projects,
             ":/licenses/mruby.txt");

Credits gsl("GSL -- including derived implementations of "
            "Gauss-Kronrod integrators",
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
            "general-purpose scientific library, used for a large part of the computations",
            Credits::Projects,
            ":/licenses/GPL-3.txt");

Credits qt("Qt5", 
           QStringList() 
           << "The Qt company",
           QStringList() << "https://www.qt.io/",
           "Commercial License Usage\n"
           "Licensees holding valid commercial Qt licenses may use this file in" 
           "accordance with the commercial license agreement provided with the "
           "Software or, alternatively, in accordance with the terms contained in "
           "a written agreement between you and The Qt Company. For licensing terms "
           "and conditions see https://www.qt.io/terms-conditions. For further "
           "information use the contact form at https://www.qt.io/contact-us."
           "\n"
           "GNU General Public License Usage\n"
           "Alternatively, this file may be used under the terms of the GNU "
           "General Public License version 3 as published by the Free Software "
           "Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT "
           "included in the packaging of this file. Please review the following "
           "information to ensure the GNU General Public License requirements will "
           "be met: https://www.gnu.org/licenses/gpl-3.0.html.",
           "cross-platform graphical user interface",
           Credits::Projects,
           ":/licenses/LICENSE.GPL3-EXCEPT");


//////////////////////////////////////////////////////////////////////

static void creditsCommand(const QString &, const CommandOptions &opts)
{
  bool full = false;
  updateFromOptions(opts, "full", full);
  Terminal::out << Credits::creditString(full);
  if(! full) {
    Terminal::out << "To obtain the full text of the licenses, "
                  << "use the /full=true option" << endl;
  }
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

#include <commandlineparser.hh>

static CommandLineOption hlp("--credits", [](const QStringList & args) {
  {
    QTextStream o(stdout);
    o << Credits::creditString(false);
  }
  ::exit(0);
 }, 0, "prints out the credits of QSoas");
