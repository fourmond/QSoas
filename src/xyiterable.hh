/**
   \file xyiterable.hh
   The XYIterable class hierarchy of reusable iterators over XY data
   Copyright 2017 by CNRS/AMU

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
#ifndef __XYITERABLE_HH
#define __XYITERABLE_HH

class DataSet;
class Vector;

/// XYIterable is a hiearchy of iterators giving X,Y data (possibly
/// with errors). They are reusable, which means that, after calling
/// reset(), you can iterate over the whole set of data again.
///
/// They can be used to iterate over:
/// @li DataSet, using the XYIDataset class
/// @li series of gsl_vector pointers, using XYIGSLVectors
///
///
/// XYIterable are iterated using either next() or nextWithErrors()
///
/// The data optionally has a display name, dataName().
///
/// Several utility inspection functions are available:
class XYIterable {
public:

  virtual ~XYIterable() {;};

  /// Iterates over the data, storing the data into @a x, @a y
  ///
  /// Returns false when no next points exists, in which case the
  /// pointed-to-values are not relevant.
  virtual bool next(double * x, double * y) = 0;

  /// Same thing, but iterates with errors.
  ///
  /// By default, 0 is written to err
  virtual bool nextWithErrors(double * x, double * y, double * err);

  /// Resets the iterator, will restart enumerating points.
  ///
  /// The function can be use whatever the iteration state of the
  /// XYIterable.
  virtual void reset() = 0;

  /// Returns a name for the object we're iterating over.
  virtual QString dataName() const;

  /// Returns the bounding box of the data (not including error bars).
  /// 
  /// Does a reset()
  virtual QRectF boundingBox();

  /// Returns a newly-created dataset populated with the X,Y values. 
  DataSet * makeDataSet(bool errors = true);

};

/// Iterates over a DataSet
///
/// @warning This class will not take into account changes in the
/// number of columns after creation.
class XYIDataSet : public XYIterable {
  const DataSet * dataset;

  int cur;

  bool hasErrors;

public:

  explicit XYIDataSet(const DataSet * dataset);
  
  virtual bool next(double * x, double * y) override;
  virtual void reset() override;
  virtual QString dataName() const override;
  virtual bool nextWithErrors(double * x, double * y, double * err) override;
};

/// Iterates over a two GSL vectors
class XYIGSLVectors : public XYIterable {
protected:
  const gsl_vector * xv;
  const gsl_vector * yv;
  const gsl_vector * ev;

  size_t cur;

public:

  /// A simple enough to edit name
  QString name;

  XYIGSLVectors(const gsl_vector * xv, const gsl_vector * yv,
                const gsl_vector * ev = NULL);

  virtual bool next(double * x, double * y) override;
  virtual void reset() override;
  virtual QString dataName() const override;
  virtual bool nextWithErrors(double * x, double * y, double * err) override;
};



#endif
