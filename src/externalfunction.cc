/*
  externalfunction.cc: implementation of the ExternalFunction class
  Copyright 2021 by CNRS/AMU

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
#include <externalfunction.hh>
#include <exceptions.hh>



QHash<QString, double> ExternalFunction::defaultValues() const
{
  QHash<QString, double> rv;
  return rv;
}


//////////////////////////////////////////////////////////////////////


/// This class implements an ExternalFunction using a client/server
/// model ith simple JSON exchange.
///
/// The client sends a JSON dict with an array of X values and an
/// array for the parameters (same size as what is returned by
/// parameters()). The server replies with a JSON array of the
/// computed values.
///
/// In both directions, the communication is finished by sending a
/// '// -> DONE' string terminated with a newline.
///
/// The object sent contains two elements: "parameters" and "xvalues"
///
/// @todo Implement timeout.
class JSONBasedFunction : public ExternalFunction {
protected:

  QProcess * process;

  /// This function starts the process, or restarts it in case it died
  /// unexpectedly.
  virtual void spawn() = 0;

  /// Reads a line from the subprocess
  QByteArray readLine() {
    QByteArray rv;
    char buffer[1024];
    while(true) {
      qint64 read = process->readLine(buffer, sizeof(buffer));
      if (read != -1) {
        rv.append(buffer, read);
        if(buffer[read-1] == '\n')
          break;
      }
      else
        break;
    }
    return rv;
  };


public:
  JSONBasedFunction() : process(NULL) {
  };

  Vector compute(const Vector & xv, const double * params) {
    if(! process)
      spawn();
    if(! process)
      throw InternalError("Process creation failed for some reason");

    QJsonObject obj;
    QJsonArray vals, vals2;
    for(double x : xv)
      vals << x;
    obj["xvalues"] = vals;
    int sz = parameters().size();
    for(int i = 0; i < sz; i++)
      vals2 << params[i];
    obj["parameters"] = vals2;
    QJsonDocument doc(obj);
    process->write(doc.toJson(QJsonDocument::Compact));

    // Now read the response

    QByteArray resp;
    while(true) {
      QByteArray ln = readLine();
      if(ln.startsWith("// -> DONE"))
        break;
      ln  += resp;
    }
    QJsonParseError err;
    doc = QJsonDocument::fromJson(resp, &err);
    if(doc.isNull()) {
      throw RuntimeError("Failed to parse the JSON in the meta-data file");
    }

    if(! doc.isArray())
      throw RuntimeError("The JSON data should be an array");
    vals = doc.array();

    Vector rv;
    for(const QJsonValue & v : vals)
      rv << v.toDouble();
    return rv;
  };

  
  
};


//////////////////////////////////////////////////////////////////////

#include <file.hh>


/// This class implements an ExternalFunction communicating to a
/// python process.
class PythonFunction : public JSONBasedFunction {
protected:

  QString function;

  // The code for the function
  QString code;
  

  
  virtual void spawn() {
    /// @todo cd to the correct directory
    // We create a temporary file
    File fl("temporary-python-code.py", File::TextOverwrite);
    QTextStream out(fl);
    /// ...
    out << code;
    
  };

  

public:
};
