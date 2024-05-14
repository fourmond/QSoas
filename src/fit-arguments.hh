/**
   \file fit-arguments.hh
   New argument types specific to fits
   Copyright 2024 by CNRS/AMU

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
#ifndef __FIT_ARGUMENTS_HH
#define __FIT_ARGUMENTS_HH

#include <argument.hh>

class DataSet;

/// An argument that represents a list of trajectories
class TrajectoriesArgument : public Argument {
public:

  TrajectoriesArgument(const char * cn, const char * pn,
                       const char * d = "", bool def = false);

  /// Returns a wrapped FitTrajectories
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  
  virtual void concatenateArguments(ArgumentMarshaller * a,
                                    const ArgumentMarshaller * b) const override;
  virtual QStringList proposeCompletion(const QString & starter) const override ;

  virtual QString typeName() const override;
  virtual QString typeDescription() const override;
  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;
  
  virtual QStringList toString(const ArgumentMarshaller * arg) const override;
  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;
};

#endif
