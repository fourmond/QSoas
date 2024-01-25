/*
  file.cc: implementation of the File class
  Copyright 2020 by CNRS/AMU

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
#include <file.hh>
#include <utils.hh>
#include <general-arguments.hh>

#ifdef HAS_LIBZIP
#include <zipfile.hh>
#endif

#include <QFile>
#include <QBuffer>

// For quoting !
#include <command.hh>

File::File(const QString & fn, OpenModes m,
           const CommandOptions & opts) : fileName(fn),
                                          originalFileName(fn),
                                          rotations(5),
                                          device(NULL), mode(m)
{
  // QTextStream o(stdout);
  // o << "Init, file name: " << fileName << ", mode = " << mode << endl;
  // o << " * " << opts.keys().join(", ") << endl;
  if(opts.contains("overwrite")) {
    bool ov = false;
    updateFromOptions(opts, "overwrite", ov);
    mode = (mode & ~OverwriteMask) | (ov ? AlwaysOverwrite : NeverOverwrite);
  }
  if(opts.contains("rotate")) {
    updateFromOptions(opts, "rotate", rotations);
    if(rotations > 0) {
      mode &= ~IOMask;
      mode |= RotateMode;
    }
    else {
      if((mode & IOMask) == RotateMode) {
        // Switch to append ?
        mode &= ~IOMask;
        mode |= AppendMode;
      }
    }
  }
  if(opts.contains("mkpath")) {
    bool mkpath = false;
    updateFromOptions(opts, "mkpath", mkpath);
    if(mkpath)
      mode |= MkPath;
  }
  // o << " -> mode = " << mode << endl;

  /// @todo Here, handle ExpandTilde
  if(mode & ExpandTilde)
    fileName = Utils::expandTilde(fileName);
}

/// @todo We could use std::unique_ptr for all of those to further
/// decrease the footprint at exit, but OTOH, the unique ptr must be
/// initialized and we're back to the original problem... Hooks ?
QStringList * File::filesRead = NULL;

QHash<QString, int> * File::filesWritten = NULL;

void File::trackFile(const QString & path, OpenModes m)
{
  if(! filesRead) {
    filesRead = new QStringList;
    filesWritten = new QHash<QString, int>;
  }
  if((m & IOMask) == ReadOnlyMode ) {
    *filesRead << path;
  }
  else {
    (*filesWritten)[path] = filesRead->size();
  }
}

/// Takes an absolute path. Returns a path relative to the given
/// directory, or an absolute path if the latter is shorter.
///
/// @todo Integrate into Utils ?
static QString simplifyPath(const QDir & ref, const QString & path)
{
  QString rv = ref.relativeFilePath(path);
  if(rv.size() < path)
    return rv;
  return path;
}

void File::writeDependencies(const QString & outputFile)
{
  if(! filesRead)
    return;                      // Nothing to do
  QDir cwd = QDir::current();
  File out(outputFile, TextOverwrite|MkPath);
  QString ot = out.info().absoluteFilePath();
  QTextStream o(out);
  for(const QString & n : filesWritten->keys()) {
    int nb = (*filesWritten)[n];
    QStringList deps = filesRead->mid(0, nb);
    for(int i = deps.size()-1; i >=0; i--) {
      if(deps[i].startsWith(":"))
        deps.takeAt(i);
    }
    if(n == ot)
      continue;                 // No need !
    for(QString & d : deps)
      d = Command::quoteString(d);

    QString n2 = simplifyPath(cwd, n);
    o << n2 << ": " << deps.join(" ") << "\n";
  }
}

void File::preOpen()
{
  QString ifn = inlineFileName(fileName);
  if(! ifn.isEmpty()) {
    if((mode & IOMask) != ReadOnlyMode)
      throw RuntimeError("Trying to write to inline file: %1").arg(fileName);
    return;
  }
  // QTextStream o(stdout);
  // o << "Preopen: " << mode << endl;
  if(mode & MkPath) {
    QDir::current().mkpath(info().path());
  }
  if((mode & IOMask) == RotateMode) {
    Utils::rotateFile(fileName, rotations);
  }
  if((mode & IOMask) == 0)
    throw InternalError("No IO mode specified: %1").arg(mode);

  // Check overwriting
  if((mode & IOMask) != ReadOnlyMode ) {
    switch(mode & OverwriteMask) {
    case PromptOverwrite:
    case NeverOverwrite:
      if(QFile::exists(fileName)) {
        if((mode & OverwriteMask) == PromptOverwrite) {
          QString s = QString("Overwrite file '%1' ?").
            arg(fileName);
          if(Utils::askConfirmation(s))
            break;
        }
        throw RuntimeError("Not overwriting file '%1'").arg(fileName);
      }
      break;
    case RequirePresent:
      if(! QFile::exists(fileName))
        throw RuntimeError("File '%1' should be present").arg(fileName);
    default:
      break;
    }
  }

  /// @todo Here, handle move-at-close
  actualName = fileName;
}

void File::open()
{
  if(device)
    throw InternalError("Trying to open an already opened File");
  preOpen();

  

  QIODevice::OpenMode m;
  if((mode & IOMask) == ReadOnlyMode)
    m = QIODevice::ReadOnly;
  else {
    m = QIODevice::WriteOnly;
    if((mode & IOMask) == AppendMode)
      m |= QIODevice::Append;

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    // only available for recent Qt.
    if((mode & OverwriteMask) == NeverOverwrite)
      m |= QIODevice::NewOnly;  // To avoid race conditions ?
#endif
  }
  if(mode & Text)
    m |= QIODevice::Text;

  QString ifn = inlineFileName(fileName);
  if(! ifn.isEmpty()) {
    InlineFile * fl = getInlineFile(ifn);
    if(! fl)
      throw RuntimeError("Inline file not found: '%1'").arg(fileName);
    QBuffer * buf = new QBuffer;
    /// @todo: decice between QByteArray and QString
    buf->setData(fl->contents.toUtf8());
    buf->open(m);
    device = buf;
    
    // We don't track internal files
    return;
  }

#ifdef Q_OS_WIN32
  // Emulation of /dev/null for input in Windows
  if(! fileName == "/dev/null") {
    QBuffer * buf = new QBuffer;
    buf->open(m);
    device = buf;
    // We're not going to track this too
    return;
  }
#endif

  
  // QTextStream o(stdout);
  // o << "Opening file: " << fileName << " with mode: "
  //   << m <<  " (internal mode: " << mode << ")" << endl;
  std::unique_ptr<QIODevice> f;
  QString arch;
#ifdef HAS_LIBZIP
  // Transparent opening of zip files
  if((mode & IOMask) == ReadOnlyMode) {
    arch = ZipFile::archiveForFile(actualName);
    if(! arch.isEmpty())
      f.reset(ZipFile::openFileInArchive(actualName));
  }
#endif
  if(! f)
    f.reset(new QFile(actualName));

  if(!f->open(m)) {
    QString error = f->errorString();
    QString mdStr = (m & QIODevice::ReadWrite) == QIODevice::ReadOnly ?
      "for reading" : "for writing";
    throw RuntimeError("Could not open file %1 %2: %3").
      arg(actualName).arg(mdStr).arg(error);
  }
  device = f.release();
  // Successful opening !

  trackFile(arch.isEmpty() ?
            info().absoluteFilePath() :
            QFileInfo(arch).absoluteFilePath(), mode);
}

void File::close()
{
  if(device) {
    delete device;
    /// @todo MoveAtClose !

    device = NULL;
  }
}

File::~File()
{
  close();
}

QIODevice * File::ioDevice()
{
  if(! device) {
    open();
    if(! device)
      throw InternalError("File opened but NULL nevertheless");
  }
  return device;
}

File::operator QIODevice*()
{
  return ioDevice();
}

QList<Argument *> File::fileOptions(Options options)
{
  QList<Argument *> rv;
  if(options & OverwriteOption)
    rv  << new BoolArgument("overwrite", 
                            "Overwrite",
                            "If true, overwrite without prompting");
  if(options & MkPathOption)
    rv << new BoolArgument("mkpath",
                           "Make path",
                           "If true, creates all necessary directories");

  if(options & RotationOption)
    rv << new IntegerArgument("rotate", 
                              "Rotate file",
                              "if not zero, performs a file rotation before writing");
  
  return rv;
}

FileInfo File::info() const
{
  return FileInfo(fileName);
}

QByteArray File::readFile(const QString & fileName, bool text)
{
  File f(fileName, BinaryRead|(text ? Text : Binary));
  return f.ioDevice()->readAll();
}

QString File::findFreeFile(const QString & base, bool random)
{
  QString last;
  int nb = -1;
  while(true) {
    if(random) {
      int tst = ::rand();
      if(tst == nb)
        continue;
      nb = tst;
    }
    else {
      ++nb;
    }
    QString file;
    file.asprintf(base.toUtf8(), nb);
    if(last == file)
      throw InternalError("Possible infinite loop");
    last = file;
    if(! QFile::exists(last))
      break;
  }
  return last;
}


QString File::checkOpen(const QString & fileName, const CommandOptions & opts,
                        OpenModes mode)
{
  File f(fileName, mode, opts);
  f.preOpen();
  trackFile(f.info().absoluteFilePath(), mode);
  return f.actualName;
}


QList<FileInfo> File::listDirectory(const QString & directory)
{
  QList<FileInfo> rv;
#ifdef HAS_LIBZIP
  QPair<QString, QString> pair = ZipFile::separatePath(directory);
  if(!pair.first.isEmpty()) {
    QStringList entries = ZipFile::listDirectory(pair.first, pair.second);
    for(const QString & s : entries)
      rv << FileInfo(pair.first, pair.second + "/" + s);
    return rv;
  }
#endif

  QDir dir(directory);
  QStringList entries = dir.entryList();
  for(const QString & s : entries)
    rv << FileInfo(directory + "/" + s);
  return rv;
}

QStringList File::globHelper(const QStringList & patterns,
                             const QString & base,
                             bool isDir)
{
  /// @todo Optimize when the first in the patterns is not a glob
  if(patterns.size() == 0)
    return QStringList();       // Nothing to do here

  const QString & p1 = patterns.first();

  if(! (p1.contains('[') || p1.contains('?') || p1.contains('*'))) {
    // Not a glob
    QString tgt = base + "/" +  p1;
    FileInfo info(tgt);
    if(patterns.size() == 1) {
      if(info.exists())
        return QStringList() << tgt;
      else
        return QStringList();
    }
    if(info.isDirLike())
      return globHelper(patterns.mid(1), tgt, isDir);
    else
      return QStringList();
  }
  
  Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#ifdef Q_OS_LINUX
  cs = Qt::CaseSensitive;
#endif
  bool sub = false;
  // QTextStream o(stdout);
  // o << "Glob: (" << base << "): " << patterns.join("/") << endl;
  if(patterns.first() == "**")
    sub = true;
  QRegExp pat(patterns.first(), cs, QRegExp::Wildcard);

  FileInfo bs(base);
  QString bs2 = base;
  if(!bs.isDir() && (bs.isDirLike()))
    bs2 += "/.";

  QList<FileInfo> cur = listDirectory(bs2);

  QStringList lst;

  for(const FileInfo & info : cur) {
    // o << " -> " << info.fileName() << endl;
    if(info.fileName() == "." || info.fileName() == "..")
      continue;
    if(sub) {
      if(info.fileName() == "." || info.fileName() == "..")
        continue;
      if(info.isDir()) {
        // Here, we do NOT descend into ZIP files
        /// @todo use QDir::join or somethinf ?
        QString nb = base + "/" + info.fileName();
        lst += globHelper(patterns, nb, isDir);
        lst += globHelper(patterns.mid(1), nb, isDir);
      }
      continue;
    }
    if(! pat.exactMatch(info.fileName()))
      continue;

    // o << "Found" << info.fileName() << endl;

    if(patterns.size() > 1) {
      if(info.isDirLike()) {
        // o << " -> is dir like" << endl;
        // here, we can descend into ZIP files
        QString nb = base + "/" + info.fileName();
        lst += globHelper(patterns.mid(1), nb, isDir);
      }
    }
    else {
      if((!isDir) || info.isDir()) {
        lst << base + "/" + info.fileName();
      }
    }
  }
  return lst;
}


QStringList File::glob(const QString & pattern, 
                       bool trim, bool isDir)
{
  QStringList pats = QDir::fromNativeSeparators(pattern).split("/");
  bool rewriteHome = false;
  QString hp;


  QRegExp winDrive("^[a-z]:$");
  QString base = ".";
  /// Expand home directory
  if(pats[0] == "~") {
    base = QDir::homePath() + "/";
    rewriteHome = true;
    hp = base;
    pats.takeFirst();
  }
  else {
    if(pats[0].isEmpty()) {
      base = "/";
      pats.takeFirst();
    }
    else if(winDrive.indexIn(pats[0]) == 0) {
      base = pats[0] + "/";
      pats.takeFirst();
    }
  }

  QStringList lst = globHelper(pats, base, isDir);

  if(lst.isEmpty() && ! trim)
    lst << pattern;

  // Sounds a little hackish, but should work anyway
  if(rewriteHome) {
    QRegExp rp("^" + QRegExp::escape(hp));
    lst.replaceInStrings(rp, "~/");
  }

  return lst;
}

File::InlineFile::InlineFile()
{
}

File::InlineFile::InlineFile(const QString & n, const QString & c,
                             const QDateTime & d) :
  name(n), date(d), contents(c)
{
}

QHash<QString, File::InlineFile> * File::inlineFiles = NULL;

File::InlineFile * File::getInlineFile(const QString & name)
{
  if(! inlineFiles)
    return NULL;
  if(! inlineFiles->contains(name))
    return NULL;
  return &(*inlineFiles)[name];
}

void File::createInlineFile(const QString & name,
                            const QString & contents)
{
  if(! inlineFiles)
    inlineFiles = new QHash<QString, File::InlineFile>;
  (*inlineFiles)[name] = InlineFile(name, contents,
                                    QDateTime::currentDateTime());
}

QStringList File::currentInlineFiles()
{
  if(! inlineFiles)
    return QStringList();
  return inlineFiles->keys();
}

QString File::inlineFileName(const QString & file)
{
  if(file.startsWith("inline:"))
    return file.mid(QString("inline:").size());
  return QString();
}


//////////////////////////////////////////////////////////////////////
#include <commandlineparser.hh>


/// This relies on the internal file pointers still being valid at
/// exit. Should be OK.
///
/// Even if there are std::unique_ptr up.
class WriteDeps {
public:
  QString file;

  ~WriteDeps() {
    if(! file.isEmpty())
      File::writeDependencies(file);
  };
  
};

static WriteDeps deps;

//////////////////////////////////////////////////////////////////////

static CommandLineOption
hlp("--write-deps",
    [](const QStringList & args) {
      deps.file = args[0];
    }, 1, "write the dependencies of created files to the given makefile");

