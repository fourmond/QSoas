/**
    \file headers.hh precompiled headers for QSoas
    Copyright 2011 by Vincent Fourmond

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

#include <QApplication>
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

#include <QSplitter>

#include <QListWidget>
#include <QComboBox>

#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDateEdit>
#include <QTimeEdit>

// Dialogs and the like
#include <QFileDialog>
#include <QDialog>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>

#include <QScrollBar>
#include <QSpinBox>

// The graphics view !
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsItem>
#include <QPainterPath>

// A rich text viewer (and editor?).
// (not used)
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

// Templates
#include <QHash>
#include <QMultiHash>
#include <QList>

// We want to be able to select the locale for parsing floating-point
// values.
#include <QLocale>
#include <QKeyEvent>

// We do C++ sometimes
#include <stdexcept>

#endif
