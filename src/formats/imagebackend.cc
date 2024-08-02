/**
   @file imagebackend.cc: backend loads image files
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
#include <databackend.hh>
#include <dataset.hh>

#include <argumentlist.hh>
#include <argumentmarshaller.hh>
#include <general-arguments.hh>

#include <utils.hh>
#include <exceptions.hh>

#include <fileinfo.hh>

/// A private hierarchy for reading image
class ImageReadingJob {
protected:
  QImage * image;
public:
  ~ImageReadingJob() {
  };

  virtual void initialize(QImage * img) {
    image = img;
    // Children should perform appropriate initialization
  };

  /// Returns true if it can only return monochrome data
  virtual bool monochrome() const = 0;

  /// Reads a scan line into monochrome data
  virtual Vector readMonochromeScanLine(int i) const = 0;

  /// Reads a scan line into three color data
  virtual QList<Vector> readColorScanLine(int i) const {
    throw RuntimeError("Cannot read color images");
  };
};

class Grayscale8RJ : public ImageReadingJob {

  virtual bool monochrome() const override {
    return true;
  };

  virtual Vector readMonochromeScanLine(int i) const {
    return Vector();
  };

};


/// A class that reads image files
class ImageBackend : public DataBackend {
protected:
  
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const override {
    // We always decline loading automatically a file
    return 0;
  };

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const override {
    QList<DataSet *> rv;
    return rv;
  };

public:
  ImageBackend() : DataBackend("image", "Image files",
                               "image files as xyz data") { 
  };
};

static ImageBackend pb;
