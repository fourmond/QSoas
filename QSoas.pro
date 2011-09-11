# -*- mode: conf; -*-
# QMake configuration file for QCam

TEMPLATE = app
CONFIG += precompile_header debug warn_on thread exception
INCLUDEPATH += . src

DEPENDPATH += src

# For faster rendering of the antialiased curves
QT += opengl                    # Doesn't work that well, actually
QT -= webkit                    # We really don't need webkit, for now ?

VERSION = 0.0

DEFINES += SOAS_VERSION=\'\"$$VERSION\"\'

PRECOMPILED_HEADER = src/headers.hh

# Use a build/ directory for building
MOC_DIR = build
OBJECTS_DIR = build

# Really, this should be the default, since it means segfault in the
# best case (excepted when a function only exits via an exception)
QMAKE_CXXFLAGS += -Werror=return-type 

# Generate doxygen documentation
doc.commands = doxygen 

QMAKE_EXTRA_TARGETS += doc

# You can specify the full path of ruby on the command-line using:
# qmake RUBY=/usr/local/ruby1.8/bin/ruby
isEmpty(RUBY):RUBY = ruby


# Ruby detection/installation
RUBY_LIB_ARG = $$system($$RUBY ./get-ruby-config.rb libarg)
RUBY_INCLUDE_DIRS = $$system($$RUBY ./get-ruby-config.rb includedir)

isEmpty(RUBY_LIB_ARG) {
  error("Could not find ruby, make sure $$RUBY is in the PATH !")
}

message("Ruby: using $$RUBY, found library: $$RUBY_LIB_ARG and includes at $$RUBY_INCLUDE_DIRS")

INCLUDEPATH += $$RUBY_INCLUDE_DIRS
LIBS += $$RUBY_LIB_ARG


# GSL
LIBS += -lgsl -lgslcblas -lm

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
        src/common-fits.cc \
        src/fitdialog.cc \
        src/curvevector.cc \
        src/settings.cc \
        src/pointpicker.cc \
        src/spline.cc \
        src/actioncombo.cc \
        src/flowinggridlayout.cc \
        src/fitparameters.cc

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
        src/fitparameters.hh
