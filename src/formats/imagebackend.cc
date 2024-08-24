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

/// A private hierarchy for reading the different image formats
class ImageReadingJob {
protected:
  const QImage * image;
public:

  ImageReadingJob(const QImage * img) :
    image(img) {
  };
  
  ~ImageReadingJob() {
  };

  /// Returns true if it can only return monochrome data
  virtual bool monochrome() const = 0;

  /// Reads a scan line into monochrome data
  virtual Vector readMonochromeScanLine(int i) const = 0;

  /// Reads a scan line into three color data
  virtual QList<Vector> readColorScanLine(int i) const {
    throw RuntimeError("Cannot read color images");
  };

  static ImageReadingJob * readerForImage(const QImage * image);

  /// Returns the data as monochrome
  DataSet * readMonochrome() const {
    QList<Vector> rv;
    Vector perp;
    for(int i = 0; i < image->height(); i++) {
      rv += readMonochromeScanLine(i);
      perp << i;
    }
    if(rv.size() == 0)
      throw RuntimeError("Couldn't read any scan lines");
    Vector idx = rv.first();
    for(int i = 0; i < idx.size(); i++)
      idx[i] = i;
    rv.insert(0, idx);
    DataSet * ds = new DataSet(rv);
    ds->setPerpendicularCoordinates(perp);

    return ds;
  };
};

//////////////////////////////

class Grayscale8RJ : public ImageReadingJob {
public:

  Grayscale8RJ(const QImage * image) : ImageReadingJob(image) {
  };

  virtual bool monochrome() const override {
    return true;
  };

  virtual Vector readMonochromeScanLine(int i) const {
    int sz = image->width();
    Vector rv(image->width(), 0);
    const uchar * sl = image->constScanLine(i);
    for(int i = 0; i < sz; i++)
      rv[i] = sl[i];
    return rv;
  };

};

//////////////////////////////

class Grayscale16RJ : public ImageReadingJob {
public:

  Grayscale16RJ(const QImage * image) : ImageReadingJob(image) {
  };

  virtual bool monochrome() const override {
    return true;
  };

  virtual Vector readMonochromeScanLine(int i) const {
    int sz = image->width();
    Vector rv(image->width(), 0);
    // This is unclear in the documentation, but it looks like the
    // endianess is that of the host, so this cast works.
    const quint16 * sl = reinterpret_cast<const quint16*>(image->constScanLine(i));
    
    for(int i = 0; i < sz; i++)
      rv[i] = sl[i];
    return rv;
  };

};


//////////////////////////////

ImageReadingJob * ImageReadingJob::readerForImage(const QImage * image)
{
  switch(image->format()) {
  case QImage::Format_Alpha8:
  case QImage::Format_Grayscale8:
    return new Grayscale8RJ(image);
  case QImage::Format_Grayscale16:
    return new Grayscale16RJ(image);
  default:
    return NULL;
  };
  return NULL;
}

//////////////////////////////////////////////////////////////////////

/// @todo This should join headers.hh
/// Or should it ?
#include <QImageReader>

static ArgumentList 
imageLoadOptions(QList<Argument *>() 
                 << new IntegerArgument("frame",
                                        "frame",
                                        "reads the given frame (-1 for all)")
                );



/// A class that reads image files
class ImageBackend : public DataBackend {
protected:
  
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const override {
    // We always decline loading automatically a file, since the
    // penalty can be huge for stray image files that are likely to be
    // irrelevant.
    return 0;
  };

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const override {
    QImageReader r(stream);

    QList<DataSet *> rv;

    int count = r.imageCount();
    int frame = 0;
    updateFromOptions(opts, "frame", frame);

    auto readFrame = [&rv, this, &r, &fileName, &frame, &count]() -> void {
      QImage img = r.read();
      if(img.isNull())
        throw RuntimeError("Failed to load '%1' as an image: %2").
          arg(fileName).arg(r.errorString());

      std::unique_ptr<ImageReadingJob> job(ImageReadingJob::readerForImage(&img));
      if(! job)
        throw RuntimeError("Could not find a reader to decode image "
                           "'%1' with format %2").
          arg(fileName).arg(img.format());

      DataSet * ds = job->readMonochrome();
      ds->name = QDir::cleanPath(fileName);
      if(frame >= 0 && count > 0) {
        ds->setMetaData("frame", frame);
        ds->name += QString("@%1").arg(frame);
      }
      rv << ds;
    };

    if(frame < 0) {
      for(int i = 0; i < count; i++) {
        r.jumpToImage(i);
        frame = i;
        readFrame();
      }
    }
    else {
      if(frame > 0)
        r.jumpToImage(frame);
      readFrame();
    }
    return rv;
  };

  virtual ArgumentList loadOptions() const override {
    return ::imageLoadOptions;
    
  };


public:
  ImageBackend() : DataBackend("image", "Image files",
                               "image files as xyz data") { 
  };
};

static ImageBackend pb;
