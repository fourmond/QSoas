/*
  binary.cc: loading and displaying of binary data files
  Copyright 2014 by CNRS/AMU

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
#include <namedinstance.hh>

#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>
#include <soas.hh>
#include <utils.hh>

#include <datastack.hh>

#include <dataset.hh>
#include <vector.hh>

#include <cstdint>
#include <factoryargument.hh>


/// This is the base class for all binary data types understood.
///
/// It is a pure abstract class
class BinaryData : public NamedInstance<BinaryData> {
public:

  /// The name of the type
  QString name;

  /// Parses the value at \a data as a double
  virtual double parseAt(const char * data) const = 0;

  /// Returns the size of the value at *data.
  virtual size_t sizeAt(const char * data) const = 0;
  
  /// Parse the data at the pointer as a double and returns the 
  virtual double parseAsDouble(const char *& data) const {
    double d = parseAt(data);
    data += sizeAt(data);
    return d;
  };

  BinaryData(const QString & n)  : name(n) {
    registerInstance(this);
  };

};


/// Template-based implementation that is used for all machine
/// types. Care should be taken with endianness (at least when trying
/// to make something portable) !
template <typename T> class NativeData : public BinaryData {
public:
  NativeData(const QString & n) : BinaryData(n) {;};
  
  virtual double parseAt(const char * data) const {
    const T * ptr = reinterpret_cast<const T*>(data);
    return (double) *ptr;
  };

  virtual size_t sizeAt(const char *) const {
    return sizeof(T);
  };
};

static NativeData<uint8_t>  d1("nu8");
static NativeData<uint16_t> d2("nu16");
static NativeData<uint32_t> d3("nu32");
static NativeData<uint64_t> d4("nu64");
static NativeData<int8_t>   d5("ni8");
static NativeData<int16_t>  d6("ni16");
static NativeData<int32_t>  d7("ni32");
static NativeData<int64_t>  d8("ni64");
static NativeData<float>    d9("nf");
static NativeData<double>   d10("nd");


//////////////////////////////////////////////////////////////////////////

// The command "per se"
  
static void binaryReadCommand(const QString &, QString fileName,
                          BinaryData * type,
                          const CommandOptions & opts)
{
  QByteArray data;
  {
    QFile file(fileName);
    Utils::open(&file, QIODevice::ReadOnly);
    data = file.readAll();
  }
  
  int offset = 0;
  int skipped = 0;
  bool show = false;

  Vector nx, ny;

  int nbread = 0;
  int number = 0;
  int delta = 0;

  updateFromOptions(opts, "offset", offset);
  updateFromOptions(opts, "skip", skipped);
  updateFromOptions(opts, "delta", delta);
  updateFromOptions(opts, "number", number);
  updateFromOptions(opts, "show", show);

  int cur = offset;
  
  int tot = data.size();
  const char * dt = data.constData();
  while(cur < tot) {
    int sz = type->sizeAt(dt + cur);
    if(sz + cur > tot)
      break;
    
    double val = type->parseAt(dt + cur);
    cur += sz + delta;
    if(skipped > 0) {
      skipped --;
      continue;
    }
    nx << nx.size();
    ny << val;
    nbread++;
    if(number > 0 && nx.size() >= number)
      break;
  }
  if(show) {
    for(int i = 0; i < ny.size(); i++)
      Terminal::out << "value[" << i << "] = " << ny[i] << endl;
  }
  else {
    DataSet * nds = new DataSet(nx, ny);
    nds->name = fileName;
    soas().pushDataSet(nds);
  }
}

static ArgumentList 
brA(QList<Argument *>() 
    << new FileArgument("file", 
                        "File",
                        "File to read data from")
    << new FactoryArgument<BinaryData>("type", 
                                       "Type",
                                       "Type of the data to read")
    );

static ArgumentList 
brO(QList<Argument *>() 
    << new IntegerArgument("offset", 
                           "Offset",
                           "Start to read from offset")
    << new IntegerArgument("delta", 
                           "Delta",
                           "Skip that many bytes between each read")
    << new IntegerArgument("skip", 
                           "Skip",
                           "Skip that many records before starting to read")
    << new IntegerArgument("number", 
                           "Number",
                           "Read only that many records")
    << new BoolArgument("show", 
                        "Show only",
                        "Show only the values, do not make a dataset")
    );



static Command 
sa("binary-read", // command name
   effector(binaryReadCommand), // action
   "buffer",  // group name
   &brA, // arguments
   &brO, // options
   "Binary read",
   "Reads and possibly load binary data",
   "...");
