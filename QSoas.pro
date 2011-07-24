# -*- mode: conf; -*-
# QMake configuration file for QCam

TEMPLATE = app
CONFIG += precompile_header debug warn_on thread
INCLUDEPATH += . src

DEPENDPATH += src

VERSION = 0.0

# QT += xml network

PRECOMPILED_HEADER = src/headers.hh

# Use a build/ directory for building
MOC_DIR = build
OBJECTS_DIR = build

# Really, this should be the default, since it means segfault in the
# best case
QMAKE_CXXFLAGS += -Werror=return-type 

# Handling of Ruby sources, but rather bad for now
# Bad for now
# INCLUDEPATH += /usr/lib/ruby/1.8/x86_64-linux
# LIBS += -lruby1.8

# Input files
SOURCES += src/qmain.cc

HEADERS += src/headers.hh

