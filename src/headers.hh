/**
    \file headers.hh shared headers for QSoas (for use with precompiled headers)
    Copyright 2011 by Vincent Fourmond
              2012, 2013, 2014 by CNRS/AMU

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
#ifndef __HEADERS_HH
#define __HEADERS_HH

/// @todo There are probably a great number of useless headers
/// here. Sort them out.

#if defined __cplusplus 
#include <QApplication>
#include <QClipboard>
#include <QSettings>

// Thread-related headers
#include <QThread>
#include <QThreadStorage>
#include <QWaitCondition>
#include <QMutex>
#include <QMutexLocker>
#include <QReadWriteLock>

// Main GUI Classse
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QTabBar>
#include <QTabWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QSplitter>
#include <QListWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QTableWidget>

#include <QStackedLayout>


// Keyboard short cuts
#include <QShortcut>

// Dialogs and the like
#include <QFileDialog>
#include <QDialog>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QToolTip>
#include <QDialogButtonBox>

#include <QScrollBar>
#include <QSpinBox>
// #include <QDoubleSpinBox>

#include <QPainterPath>
#include <QPainter>

// Printing
#include <QPrinter>
#include <QPrintDialog>

// SVG export
#include <QSvgGenerator>

// Signals with sender !
#include <QSignalMapper>

// To have openGL rendering
#include <QGLWidget>

// A rich text viewer (and editor?).
#include <QTextEdit>
#include <QTextCursor>

// Desktop stuff
#include <QUrl>
#include <QDesktopServices>

// Non-GUI objects
#include <QDate>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QPointer>
#include <QTemporaryFile>
#include <QTimer>
#include <QTextCodec>


// Binary serialization
#include <QDataStream>

// Templates
#include <QHash>
#include <QMultiHash>
#include <QList>
#include <QVarLengthArray>
#include <QCache>
#include <QQueue>

// Explicit sharing
#include <QSharedData>
#include <QExplicitlySharedDataPointer>

// We want to be able to select the locale for parsing floating-point
// values.
#include <QLocale>
#include <QKeyEvent>


// MVC classes
#include <QAbstractTableModel>
#include <QHeaderView>

// Plugins
#include <QLibrary>

// Networking ?
#include <QTcpSocket>
#include <QTcpServer>

// We do C++ sometimes
#include <stdexcept>
#include <functional>
#include <memory>

#include <cmath>

#include <mruby.h>

#endif


// The gsl
#ifndef Q_WS_WIN
// We don't use inlined functions on windows, I can't get them to work
// for now !
#define HAVE_INLINE             // Using gsl inline functions
#endif
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_roots.h>
#include <gsl/gsl_multiroots.h>

#endif
