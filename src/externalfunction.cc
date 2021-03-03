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

ExternalFunction::~ExternalFunction()
{
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
    char buffer[20000];
    while(true) {
      qint64 read = process->readLine(buffer, sizeof(buffer));
      if(read > 0) {
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

  ~JSONBasedFunction() {
    if(process) {
      process->closeWriteChannel();
      // Wait 10 ms before closing
      process->waitForFinished(10);
      delete process;
    }
  };

  Vector compute(const Vector & xv, const double * params) override {
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
    process->waitForStarted();
    process->write(doc.toJson(QJsonDocument::Compact) + "\n");
    process->waitForBytesWritten();

    // Now read the response

    QByteArray resp;
    while(true) {
      // if(process->atEnd())
      //   throw RuntimeError("Premature end of subprocess %1 %2").
      //     arg(process->program()).arg(process->arguments().join(" "));
      QByteArray ln = readLine();
      if(ln.startsWith("// -> DONE"))
        break;
      if(ln.size() == 0) {
        if(process->state() == QProcess::NotRunning)
            throw RuntimeError("Premature end of subprocess %1 %2").
              arg(process->program()).arg(process->arguments().join(" "));
        process->waitForReadyRead();
      }
      else 
        resp += ln;
    }
    QJsonParseError err;
    doc = QJsonDocument::fromJson(resp, &err);
    if(doc.isNull()) {
      throw RuntimeError("Failed to parse the JSON response:");
    }

    if(! doc.isObject())
      throw RuntimeError("The JSON data should be an object");
    obj = doc.object();
    if(obj.contains("error"))
      throw RuntimeError("Error executing function: %1").
        arg(obj["error"].toString());
    if(! obj.contains("values"))
      throw RuntimeError("Subprocess failed to return values");
    vals = obj["values"].toArray();
    

    Vector rv;
    for(const QJsonValue & v : vals)
      rv << v.toDouble();
    return rv;
  };

  
  
};


//////////////////////////////////////////////////////////////////////

#include <file.hh>

#include <QTemporaryFile>

/// This class implements an ExternalFunction communicating to a
/// python process.
class PythonFunction : public JSONBasedFunction {
protected:

  /// The name of the function
  QString function;

  /// The parameters
  QStringList parameterList;

  QHash<QString, double> defaults;


  /// The name of the file
  QString fileName;

  // The code for the function
  QString code;

  /// The path to the python interpreter
  QString python;

  /// Reads the file, parses the argument list and generates the code
  /// necessary.
  ///
  /// @todo Watch out for change in directory...
  void generateCode() {
    File src(fileName, File::TextRead);
    QTextStream in(src);
    QString s = in.readAll();

    // Look for all functions

    QRegExp fundef("def\\s+(\\w+)\\s*\\(([^:]+)\\)\\s*:");
    int pos = 0;
    QString args;
    while(true) {
      pos = fundef.indexIn(s, pos);
      if(pos >= 0) {
        if(fundef.cap(1) == function) {
          args = fundef.cap(2);
          break;
        }
        pos += fundef.matchedLength();
      }
      else
        break;
    }

    if(pos < 0)
      throw RuntimeError("Could not find function '%1'").arg(function);

    QStringList lst = args.split(QRegExp("\\s*,\\s*"));
    if(lst.size() < 1)
      throw RuntimeError("Found less than one argument for function: '%1' -- argument list is: '%2'").
        arg(function).arg(args);
    lst.takeFirst();
    parameterList.clear();
    for(const QString & s : lst) {
      QStringList l = s.split(QRegExp("\\s*=\\s*"));
      if(l[0].startsWith("**"))
        break;
      if(l.size() > 1) {
        bool ok = false;
        double v = l[1].toDouble(&ok);
        if(! ok)
          break;
        defaults[l.first()] = v;
      }
      parameterList << l.first();
    }
    code = s;
    code += QString("\nfunc = %1\n").arg(function);
  };
  

  
  virtual void spawn() {
    if(code.isEmpty())
      generateCode();
    if(process)
      throw InternalError("Trying to create a second process");
    /// @todo cd to the correct directory
    // We create a temporary file

    File wrp(":resources/python-wrapper.py", File::TextRead);
    QTextStream in(wrp);
    QString s = in.readAll();
    s.replace("## CODE", code);

    /// @todo Move to File ?
    QTemporaryFile file;
    file.setAutoRemove(false); // Accumulation of temporary files ?
    file.setFileTemplate("python-temporary-code-XXXXXX.py");
    file.open();
    {
      QTextStream out(&file);
      out << s;
    }
    file.close();

    

    process = new QProcess();
    process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process->start(python,
                   QStringList() << file.fileName(),
                   QIODevice::ReadWrite|QIODevice::Unbuffered);
    
  };

public:

  PythonFunction(const QString & file, const QString & fn,
                 const QString & py = QString()) :
    function(fn), fileName(file), python(py.isEmpty() ?
                                         QString("python3") : py)
  {
  };

  /// The list of parameters.
  QStringList parameters() const override {
    if(code.isEmpty()) {
      /// @todo Const cast...
      const_cast<PythonFunction*>(this)->generateCode();
    }
    return parameterList;
  };

  QHash<QString, double> defaultValues() const override {
    return defaults;
  };
};

//////////////////////////////////////////////////////////////////////

ExternalFunction * ExternalFunction::pythonFunction(const QString & file,
                                                    const QString & function,
                                                    const QString & interpreter)
{
  std::unique_ptr<ExternalFunction>
    fn(new PythonFunction(file, function, interpreter));
  /// Ensure opening woks
  fn->parameters();

  return fn.release();
}


//////////////////////////////////////////////////////////////////////

#include <commandlineparser.hh>

static CommandLineOption v("--pyfunc", [](const QStringList & args) {
    {
      QTextStream o(stdout);
      if(args.size() < 4)
        throw RuntimeError("Need at least 4 arguments");
      ExternalFunction * func = ExternalFunction::pythonFunction(args[0],
                                                                 args[1]);

      o << "Function: " << args[1] << " -- parameters:\n"
        << func->parameters().join(", ") << endl;
      double xv = args[2].toDouble();
      Vector v;
      for(int i = 3; i < args.size(); i++)
        v << args[i].toDouble();
      Vector x;
      x << xv;
      x = func->compute(x, v.data());
      o << "Function: " << args[1] << " -> " << x[0] << endl;
      delete func;
    }
    ::exit(0);
  }, -3, "display QSoas version");

