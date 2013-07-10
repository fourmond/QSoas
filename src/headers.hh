/**
    \file headers.hh shared headers for QSoas (for use with precompiled headers)
    Copyright 2011, 2013 by Vincent Fourmond

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
#include <QThread>
#include <QTest>                // For waiting...
#include <QSettings>

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

// Keyboard short cuts
#include <QShortcut>

// Dialogs and the like
#include <QFileDialog>
#include <QDialog>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QToolTip>

#include <QScrollBar>
#include <QSpinBox>
// #include <QDoubleSpinBox>

#include <QPainterPath>
#include <QPainter>

// Printing
#include <QPrinter>
#include <QPrintDialog>

// Signals with sender !
#include <QSignalMapper>

// To have openGL rendering
#include <QGLWidget>

// A rich text viewer (and editor?).
#include <QTextEdit>

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


// Binary serialization
#include <QDataStream>

// Templates
#include <QHash>
#include <QMultiHash>
#include <QList>
#include <QVarLengthArray>
#include <QCache>

// Explicit sharing
#include <QSharedData>
#include <QExplicitlySharedDataPointer>

// We want to be able to select the locale for parsing floating-point
// values.
#include <QLocale>
#include <QKeyEvent>

// We do C++ sometimes
#include <stdexcept>
#include <functional>

#endif

// Now, C inclues

// ... and Ruby !
#include <ruby.h>

// Ruby pollutes the name space, which is quite a pain
#undef truncate


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

#endif
