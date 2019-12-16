/*
  formattedstring.cc: implementation of the string formats
  Copyright 2012, 2014 by CNRS/AMU

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
#include <formattedstring.hh>

#include <utils.hh>

/// Represents plain text
class FSPlainText : public FormattedString {
protected:
  QString text;

public:
  FSPlainText(const QString & s) : FormattedString(), 
                                   text(s) {;};
  
  virtual QString toText() const override {
    return text;
  };

  virtual QString toHTML() const override {
    return Utils::escapeHTML(text);
  };

  virtual QString toLaTeX() const override {
    return text;                /// @todo Quote !
  };
};

/// Represents text with a given format
class FSFormattedText : public FormattedString {
public:
  typedef enum {
    Strong, 
    Emph
  } Format;

protected:
  QString text;

  Format format;

public:

  
  FSFormattedText(const QString & s, Format fmt) : 
    FormattedString(), 
    text(s), format(fmt) {;};
  
  virtual QString toText() const override {
    return text;
  };

  virtual QString toHTML() const override {
    return Utils::escapeHTML(text);    /// @todo Quote + markup
  };

  virtual QString toLaTeX() const override {
    return text;                /// @todo Quote + markup
  };
};

// class FSEquation : public FormattedString {
// protected:
//   /// The equation, in LaTeX format
//   QString equation;
// public:

//   FSEquation(const QString & eq) : FormattedString(), equation(eq) {
//     ;
//   };

//   virtual QString toText() const {
//     return text;    
//   };

//   virtual QString toHTML() const {
//     return Qt::escape(text);    /// @todo try to use images ?
//   };
// };

FormattedString::FormattedString() 
{
}

FormattedString::~FormattedString() 
{
}

QList<QExplicitlySharedDataPointer<FormattedString> > 
FormattedString::parseString(const QString & str)
{
  // For now, nothing really sexy:
  QList<QExplicitlySharedDataPointer<FormattedString> > ret;
  ret << QExplicitlySharedDataPointer<FormattedString>(new FSPlainText(str));
  return ret;
}



QString FormattedString::toText() const
{
  QString str;
  for(int i = 0; i < nodes.size(); i++)
    str += nodes[i]->toText();
  return str;
}


QString FormattedString::toHTML() const
{
  QString str;
  for(int i = 0; i < nodes.size(); i++)
    str += nodes[i]->toHTML();
  return str;
}


QString FormattedString::toLaTeX() const
{
  QString str;
  for(int i = 0; i < nodes.size(); i++)
    str += nodes[i]->toLaTeX();
  return str;
}
