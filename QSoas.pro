# -*- mode: conf; -*-
# QMake configuration file for QCam

TEMPLATE = app
CONFIG += precompile_header debug warn_on thread exception
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

# Todo, later: add Ruby classes.
# INCLUDEPATH += /usr/lib/ruby/1.8/x86_64-linux
# LIBS += -lruby1.8

# Input files
SOURCES += src/qmain.cc \
        src/mainwin.cc \
        src/command.cc \
        src/group.cc \
        src/argumentlist.cc \
        src/general-commands.cc \
        src/commandeffector.cc \
        src/commandwidget.cc \
        src/terminal.cc

HEADERS += src/headers.hh \
        src/mainwin.hh \
        src/command.hh \
        src/group.hh \
        src/argument.hh \
        src/argumentmarshaller.hh \
        src/utils.hh \
        src/argumentlist.hh \
        src/commandeffector.hh \
        src/commandeffector-templates.hh \
        src/possessive-containers.hh \
        src/commandwidget.hh \
        src/terminal.hh

