# -*- mode: conf; -*-
# QMake configuration file for QSoas

TEMPLATE = app

CONFIG += debug warn_on thread exception 
INCLUDEPATH += . src
TARGET = QSoas

DEPENDPATH += src

# For faster rendering of the antialiased curves
QT += opengl                    # Doesn't work that well, actually
QT -= webkit                    # We really don't need webkit, for now ?
QT += testlib                   # For QTest::qSleep


# The version should be provided on the command-line.
isEmpty(VERSION) {
  VERSION = 0.0
}

isEmpty(UNIVERSAL) {
} else:macx {
  CONFIG += x86_64 x86
}                

# We want icons

macx:ICON = QSoas.icns
win32:RC_FILE = QSoas-icon.rc


DEFINES += SOAS_VERSION=\'\"$$VERSION\"\'

# This is a very crude test, but as this is a completely non-vital
# part of the work, we can afford not detecting this feature
exists(/usr/include/execinfo.h) {
  DEFINES += HAS_EXECINFO                               
  message("Found /usr/include/execinfo.h, so we should have stack traces")
}
else {
  message("No stack traces, but that doesn't matter anyway")
}

# We use a precompiled header everywhere but on macosX (it fails !)
! macx {
  CONFIG += precompile_header
  PRECOMPILED_HEADER = src/headers.hh
}


# Use a build/ directory for building

MOC_DIR = build
OBJECTS_DIR = build

# Really, this should be the default, since it means segfault in the
# best case (excepted when a function only exits via an exception)
QMAKE_CXXFLAGS += -Werror=return-type 

# Let's try building C++11 (or almost, with lambdas at least !)
# compiler_version = $$system($$QMAKE_CXX -v)
# @todo Try detecting the mingw version properly !
win32|exists(/usr/bin/gcc-4.6) {
  message("Old compiler, using the -std=c++0x flag for C++ 11")
  QMAKE_CXXFLAGS += -std=c++0x  #We use an old version of gcc on win !
} else {
  message("Using the -std=c++11 flag")
  QMAKE_CXXFLAGS += -std=c++11
}


QMAKE_CFLAGS += -O3

# Generate doxygen documentation
doc.commands = doxygen 
QMAKE_EXTRA_TARGETS += doc




# You can specify the full path of ruby on the command-line using:
# qmake RUBY=/usr/local/ruby1.8/bin/ruby
isEmpty(RUBY):RUBY = ruby


# Ruby detection/installation
RUBY_LIB_ARG = $$system($$RUBY ./get-ruby-config.rb libarg)
RUBY_INCLUDE_DIRS = $$system($$RUBY ./get-ruby-config.rb includedir)

RUBY_LIB_DIR = $$system($$RUBY ./get-ruby-config.rb libdir)

isEmpty(RUBY_LIB_ARG) {
  error("Could not find ruby, make sure $$RUBY is in the PATH !")
}

message("Ruby: using $$RUBY, found library: $$RUBY_LIB_ARG and includes at $$RUBY_INCLUDE_DIRS")

# Here, we prepare the build information, using the only script
# language we're guaranteed to have:

system($$RUBY build-info.rb)
        
HEADERS += src/build.hh

INCLUDEPATH += $$RUBY_INCLUDE_DIRS
LIBS += $$RUBY_LIB_ARG

win32:LIBS += -L$$RUBY_LIB_DIR

RESOURCES += qsoas.qrc

# GSL: we may have to build against non-standard gsl locations:
! isEmpty(GSL_DIR) {
  # We add the directory to both the include path and the lib path:
  LIBS += -L$$GSL_DIR
  INCLUDEPATH += $$GSL_DIR
}
LIBS += -lgsl -lgslcblas -lm

win32 {
# Simplify the version scheme for win32, that doesn't like it so much...
  VERSION=$$system($$RUBY strip-win-version.rb $$VERSION)
  message("Mangled version to $$VERSION")
  system($$RUBY prepare-nsis-include.rb)
}
                 

# Input files
SOURCES += src/qmain.cc \
        src/mainwin.cc \
        src/command.cc \
        src/group.cc \
        src/argumentlist.cc \
        src/general-commands.cc \
        src/commandeffector.cc \
        src/commandwidget.cc \
        src/terminal.cc \
        src/general-arguments.cc \
        src/argument.cc \
        src/vector.cc \
        src/dataset.cc \
        src/commandprompt.cc \
        src/utils.cc \
        src/databackend.cc \
        src/textbackend.cc \
        src/datastack.cc \
        src/datastack-commands.cc \
        src/curveitem.cc \
        src/curveview.cc \
        src/soas.cc \
        src/curveeventloop.cc \
        src/curvedataset.cc \
        src/curveitems.cc \
        src/curvepanel.cc \
        src/debug.cc \
        src/dataset-commands.cc \
        src/curvemarker.cc \
        src/help-commands.cc \
        src/data-processing-commands.cc \
        src/outfile.cc \
        src/ruby.cc \
        src/ruby-commands.cc \
        src/fit.cc \
        src/perdatasetfit.cc \
        src/exponential-fits.cc \
        src/fitdialog.cc \
        src/curvevector.cc \
        src/settings.cc \
        src/pointpicker.cc \
        src/spline.cc \
        src/actioncombo.cc \
        src/flowinggridlayout.cc \
        src/fitparameters.cc \
        src/fitdata.cc \
        src/inactivation-fits.cc \
        src/exceptions.cc \
        src/file-arguments.cc \
        src/linearkineticsystem.cc \
        src/simulation-commands.cc \
        src/fitparametereditor.cc \
        src/bijection.cc \
        src/bijections.cc \
        src/parametersdialog.cc \
        src/echem-fits.cc \
        src/bsplines.cc \
        src/fft.cc \
        src/peaks.cc \
        src/datasetbrowser.cc \
        src/ducksim.cc \
        src/pointtracker.cc \
        src/misc-fits.cc \
        src/custom-fits.cc \
        src/derivativefit.cc \
        src/formattedstring.cc \
        src/linearwave.cc \
        src/linearwavefit.cc \
        src/expression.cc \
        src/fitparameter.cc \
        src/fitengine.cc \
        src/fittrajectorydisplay.cc \
        src/gslfitengine.cc \
        src/combinedfit.cc \
        src/gslfunction.cc \
        src/odesolver.cc \
        src/rubyodesolver.cc \
        src/kineticsystem.cc \
        src/kineticsystemevolver.cc \
        src/graphicssettings.cc \
        src/backtrace.cc \
        src/alias.cc \
        src/credits.cc \
        src/eventhandler.cc \
        src/valuehash.cc


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
        src/terminal.hh \
        src/general-arguments.hh \
        src/vector.hh \
        src/dataset.hh \
        src/commandprompt.hh \
        src/databackend.hh \
        src/textbackend.hh \
        src/datastack.hh \
        src/curveitem.hh \
        src/curveview.hh \
        src/soas.hh \
        src/curveeventloop.hh  \
        src/curvedataset.hh \
        src/curveitems.hh \
        src/curvepanel.hh \
        src/debug.hh \
        src/curvemarker.hh \
        src/outfile.hh \
        src/ruby.hh \
        src/ruby-templates.hh \
        src/fit.hh \
        src/perdatasetfit.hh \
        src/fitdialog.hh \
        src/curvevector.hh  \
        src/settings.hh \
        src/settings-templates.hh \
        src/pointpicker.hh  \
        src/spline.hh \
        src/actioncombo.hh \
        src/flowinggridlayout.hh \
        src/fitparameters.hh \
        src/fitdata.hh \
        src/exceptions.hh \
        src/file-arguments.hh \
        src/linearkineticsystem.hh \
        src/fitparametereditor.hh  \
        src/bijection.hh \
        src/parametersdialog.hh \
        src/bsplines.hh \
        src/fft.hh \
        src/peaks.hh \
        src/datasetbrowser.hh \
        src/ducksim.hh \
        src/pointtracker.hh \
        src/derivativefit.hh \
        src/formattedstring.hh \
        src/linearwave.hh \
        src/expression.hh \
        src/fitparameter.hh \
        src/fitengine.hh \
        src/fittrajectorydisplay.hh \
        src/combinedfit.hh \
        src/gslfunction.hh \
        src/odesolver.hh \
        src/rubyodesolver.hh \
        src/kineticsystem.hh \
        src/kineticsystemevolver.hh \
        src/guarded.hh \
        src/graphicssettings.hh \
        src/credits.hh \
        src/temporarychange.hh \
        src/eventhandler.hh \
        src/valuehash.hh


# We link with the converted ODRPACK library
message("Using odrpack")

SOURCES += odrpack/odrpackfitengine.cc \        
        odrpack/d_lpkbls.new.c \
        odrpack/d_odr.new.c \
        odrpack/d_mprec.new.c \
        odrpack/f2c_remains.c