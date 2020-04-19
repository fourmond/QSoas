# -*- mode: conf; -*-
# QMake configuration file for QSoas

TEMPLATE = app

CONFIG += debug warn_on thread exception

contains(QT_MAJOR_VERSION, 4) {
  error("QSoas no longer supports Qt4")
}


INCLUDEPATH += . src
TARGET = QSoas

DEPENDPATH += src

# For faster rendering of the antialiased curves
QT += opengl                    # Doesn't work that well, actually
QT -= webkit                    # We really don't need webkit, for now ?

QT += printsupport


# We sometimes need the network
QT += network

# We need SVG support for icons
QT += svg

# And now the shiny help system
QT += help

# We want to build a console application on win32 as we don't have a
# proper winmain for windows only applications.
win32:CONFIG += console


# The version should be provided on the command-line.
isEmpty(VERSION) {
  VERSION = 2.2
}

# We attempt to make a universal binary on mac
isEmpty(UNIVERSAL) {
} else:macx {
  CONFIG += x86_64 x86
}                

# Let's optimize !
QMAKE_CXXFLAGS += -O2


# We want icons

macx:ICON = QSoas.icns
win32:RC_FILE = QSoas-icon.rc

# Compile with static libgcc in win32, but only in Qt4 builds
# -> just static libgcc results in not catching exceptions...
# -> one needs to also link stdc++ statically.
contains(QT_MAJOR_VERSION, 4) {
  win32:QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

DEFINES += SOAS_VERSION=\'\"$$VERSION\"\'

# This is a very crude test, but as this is a completely non-vital
# part of the work, we can afford not detecting this feature
!win32:exists(/usr/include/execinfo.h) {
  DEFINES += HAS_EXECINFO                               
  message("Found /usr/include/execinfo.h, so we should have stack traces")
}
else {
  message("No stack traces, but that doesn't matter anyway")
}

# We use a precompiled header everywhere but on macosX (it fails !)
unix:!macx {
  CONFIG += precompile_header
  PRECOMPILED_HEADER = src/headers.hh
}


# Do not use compressed debug symbols, more complicated than anything else
never {
  QMAKE_CXXFLAGS += -gz=none
  QMAKE_CFLAGS += -gz=none
}

# Use a build/ directory for building

MOC_DIR = build
OBJECTS_DIR = build

######################################################################
# WARNINGS THAT SHOULD BE ERRORS

# Really, this should be the default, since it means segfault in the
# best case (excepted when a function only exits via an exception)
QMAKE_CXXFLAGS += -Werror=return-type

# Linux-only flags
unix:!macx {
  # This is a real error. It should not be a warning
  QMAKE_CXXFLAGS += -Werror=delete-incomplete


  # Useful warnings
  QMAKE_CXXFLAGS += -Werror=misleading-indentation
  # QMAKE_CXXFLAGS += -Wsuggest-override
}



unix {
  QMAKE_CXXFLAGS += -Winit-self -Werror=init-self 
}


# Use C++11 everywhere !
QMAKE_CXXFLAGS += -std=c++11 -DMRB_ENABLE_DEBUG_HOOK

# We need to use libc++ on macos to have the correct C++ 11 header
# files
macx:QMAKE_CXXFLAGS += -stdlib=libc++



QMAKE_CFLAGS += -O3

# Generate doxygen documentation
doc.commands = doxygen 
QMAKE_EXTRA_TARGETS += doc


# Activate the gcc address sanitizer by passing a CONFIG+=sanitizer
# argument to qmake
sanitizer {
  message("Activating the address sanitizer code")
  QMAKE_CXXFLAGS += -fno-omit-frame-pointer -fsanitize=address
  LIBS += -fsanitize=address
  TARGET = $$join(TARGET,,,-snt)
}


# This activates garbage collection at link time. This is not a good
# idea for making a production executable, but rather as a tool for
# finding unused code.
#
# make 2> gc and then:
# cat gc | grep text. | sed 's/text./text /' | sort | c++filt | less
gc {
  message("Activating the GC code in order to find unused functions")
  QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
  LIBS += -Wl,--gc-sections -Wl,--print-gc-sections
  TARGET = $$join(TARGET,,,-gc)
}




# You can specify the full path of ruby on the command-line using:
# qmake RUBY=/usr/local/ruby1.8/bin/ruby
isEmpty(RUBY):RUBY = ruby


# # Ruby detection/installation
# RUBY_LIB_ARG = $$system($$RUBY ./get-ruby-config.rb libarg)
# RUBY_INCLUDE_DIRS = $$system($$RUBY ./get-ruby-config.rb includedir)

# RUBY_LIB_DIR = $$system($$RUBY ./get-ruby-config.rb libdir)

# isEmpty(RUBY_LIB_ARG) {
#   error("Could not find ruby, make sure $$RUBY is in the PATH !")
# }

# RUBY_VERSION = $$system($$RUBY ./get-ruby-config.rb version)
# RUBY_COMPATIBILITY = $$system($$RUBY ./get-ruby-config.rb compatible)

# isEmpty(RUBY_COMPATIBILITY) {
#   error("$$RUBY (version $$RUBY_VERSION) is not compatible with QSoas, try building with version between 1.9.3 and the 2.2 series. This is possible by running, for instance, qmake RUBY=ruby2.1")
# }



# message("Ruby: using $$RUBY, found library: $$RUBY_LIB_ARG and includes at $$RUBY_INCLUDE_DIRS")

# Here, we prepare the build information, using the only script
# language we're guaranteed to have:

system($$RUBY build-info.rb)
HEADERS += src/build.hh

build-info.commands = $$RUBY build-info.rb

QMAKE_EXTRA_TARGETS += build-info


# INCLUDEPATH += $$RUBY_INCLUDE_DIRS
# LIBS += $$RUBY_LIB_ARG

# win32:LIBS += -L$$RUBY_LIB_DIR

RESOURCES += qsoas.qrc

######################################################################
## GSL: we may have to build against non-standard gsl locations:
! isEmpty(GSL_DIR) {
  # We add the directory to both the include path and the lib path:
  message("Building against GSL in directory: $$GSL_DIR")
  exists($${GSL_DIR}/include) {
    INCLUDEPATH += $${GSL_DIR}/include
  } else {
    INCLUDEPATH += $$GSL_DIR
  }
  exists($${GSL_DIR}/lib) {
    LIBS += -L$${GSL_DIR}/lib
  } else {
    LIBS += -L$$GSL_DIR
  }
}
LIBS += -lgsl -lgslcblas -lm

######################################################################
# Mruby: offer the possibility to build against non-standard locations
! isEmpty(MRUBY_DIR) {
  # We add the directory to both the include path and the lib path:
  message("Building against MRUBY in directory: $$MRUBY_DIR")
  exists($${MRUBY_DIR}/build/host-debug/lib) {
    LIBS += -L$$MRUBY_DIR/build/host-debug/lib
  } else {
    LIBS += -L$$MRUBY_DIR/lib
  }
  INCLUDEPATH += $$MRUBY_DIR/include

# IMPORTANT NOTE: we need a recent version on mruby,
# https://github.com/mruby/mruby/commit/7450a774a5f796f7e9d312ba9c9690097f4aa309,
# seems to do the trick.
}

LIBS += -lmruby
######################################################################



FULL_VERSION=$$VERSION

win32 {
# Simplify the version scheme for win32, that doesn't like it so much...
  VERSION=$$system($$RUBY strip-win-version.rb $$VERSION)
  message("Mangled version to $$VERSION")
  system($$RUBY prepare-wix-input.rb $$FULL_VERSION $$VERSION)
  LIBS += -lpsapi
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
        src/fitworkspace.cc \
        src/fitdata.cc \
        src/exceptions.cc \
        src/file-arguments.cc \
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
        src/pointtracker.cc \
        src/misc-fits.cc \
        src/custom-fits.cc \
        src/derivativefit.cc \
        src/formattedstring.cc \
        src/expression.cc \
        src/fitparameter.cc \
        src/fitengine.cc \
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
        src/valuehash.cc \
        src/curve-effectors.cc \
        src/stylegenerator.cc \
        src/widgets.cc \
        src/fittrajectory.cc \
        src/baselinehandler.cc \
        src/regex.cc \
        src/checkablewidget.cc \
        src/datasetoptions.cc \
        src/pointiterator.cc \
        src/dataseteditor.cc \
        src/statistics.cc \
        src/solver.cc \
        src/rubysolver.cc \
        src/msolver.cc \
        src/integrator.cc \
        src/gslintegrator.cc \
        src/distribution-fits.cc \
        src/metadataprovider.cc \
        src/parametersviewer.cc \
        src/curvebrowser.cc \
        src/lineedit.cc \
        src/linereader.cc \
        src/hook.cc \
        src/nupwidget.cc \
        src/timedependentparameter.cc \
        src/timedependentparameters.cc \
        src/odefit.cc \
        src/commandlineparser.cc \
        src/valuehasheditor.cc \
        src/multiintegrator.cc \
        src/multiintegrators.cc \
        src/distribfit.cc \
        src/parametersitemmodel.cc \
        src/parametersspreadsheet.cc \
        src/abdmatrix.cc \
        src/box.cc \
        src/functions.cc \
        src/cachedfunction.cc \
        src/datasetexpression.cc \
        src/modifiedfit.cc \
        src/wave-fits.cc \
        src/peaks-commands.cc \
        src/datastackhelper.cc \
        src/icons.cc \
        src/curvepoints.cc \
        src/xyiterable.cc \
        src/tuneabledatadisplay.cc \
        src/boundingbox.cc \
        src/sparsejacobian.cc \
        src/sparsecovariance.cc \
        src/onetimewarnings.cc \
        src/fitparametersfile.cc \
        src/ruby-interface.cc \
        src/linearkineticsystem.cc \
        src/commandcontext.cc \
        src/fit-commands.cc \
        src/fwexpression.cc \
        src/fittrajectories.cc \
        src/parameterspaceexplorer.cc \
        src/fittrajectorydisplay.cc \
        src/ruby-distribution.cc \
        src/gauss-kronrod.cc \
        src/particleswarm.cc \
        src/filelock.cc \
        src/printpreviewhelper.cc \
        src/utils-osspec.cc \
        src/graphicoutput.cc \
        src/argumentsdialog.cc \
        src/filepromptwidget.cc \
        src/datasetlist.cc

#        src/conditionsprovider.cc \

# Fit engines, grouped in an easy way to disable them:
# (its missing ODRPACK, though)

SOURCES += src/qsoasfitengine.cc \
        src/gslfitengine.cc \
        src/simplexfitengine.cc \
        src/gslsimplexfitengine.cc \
        src/multifitengine.cc


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
        src/fitworkspace.hh \
        src/fitdata.hh \
        src/exceptions.hh \
        src/file-arguments.hh \
        src/fitparametereditor.hh  \
        src/bijection.hh \
        src/parametersdialog.hh \
        src/bsplines.hh \
        src/fft.hh \
        src/peaks.hh \
        src/datasetbrowser.hh \
        src/pointtracker.hh \
        src/derivativefit.hh \
        src/formattedstring.hh \
        src/expression.hh \
        src/fitparameter.hh \
        src/fitengine.hh \
        src/combinedfit.hh \
        src/gslfunction.hh \
        src/odesolver.hh \
        src/rubyodesolver.hh \
        src/kineticsystem.hh \
        src/kineticsystemevolver.hh \
        src/guarded.hh \
        src/graphicssettings.hh \
        src/credits.hh \
        src/eventhandler.hh \
        src/valuehash.hh \
        src/curve-effectors.hh \
        src/idioms.hh \
        src/stylegenerator.hh \
        src/widgets.hh \
        src/fittrajectory.hh \
        src/baselinehandler.hh \
        src/regex.hh \
        src/checkablewidget.hh \
        src/datasetoptions.hh \
        src/argument-templates.hh \
        src/pointiterator.hh \
        src/dataseteditor.hh \
        src/statistics.hh \
        src/solver.hh \
        src/rubysolver.hh \
        src/msolver.hh \
        src/integrator.hh \
        src/namedinstance.hh \
        src/factory.hh \
        src/factoryargument.hh \
        src/metadataprovider.hh \
        src/parametersviewer.hh \
        src/curvebrowser.hh \
        src/lineedit.hh \
        src/linereader.hh \
        src/hook.hh \
        src/nupwidget.hh \
        src/timedependentparameter.hh \
        src/timedependentparameters.hh \
        src/odefit.hh \
        src/textbackend.hh \
        src/commandlineparser.hh \
        src/valuehasheditor.hh \
        src/multiintegrator.hh \
        src/distribfit.hh \
        src/parametersitemmodel.hh \
        src/parametersspreadsheet.hh \
        src/abdmatrix.hh \
        src/box.hh \
        src/functions.hh \
        src/cachedfunction.hh \
        src/datasetexpression.hh \
        src/datastackhelper.hh \
        src/icons.hh \
        src/curvepoints.hh \
        src/xyiterable.hh \
        src/tuneabledatadisplay.hh \
        src/boundingbox.hh \
        src/gsl-types.hh \
        src/sparsejacobian.hh \
        src/sparsecovariance.hh \
        src/onetimewarnings.hh \
        src/fitparametersfile.hh \
        src/linearkineticsystem.hh \
        src/commandcontext.hh \
        src/fwexpression.hh \
        src/fittrajectories.hh \
        src/parameterspaceexplorer.hh \
        src/fittrajectorydisplay.hh \
        src/filelock.hh \
        src/printpreviewhelper.hh \
        src/graphicoutput.hh \
        src/argumentsdialog.hh \
        src/filepromptwidget.hh \
        src/namedinstanceargument.hh \
        src/datasetlist.hh

# mruby
HEADERS += src/mruby.hh
SOURCES += src/mruby.cc \
           src/ruby-regexp.cc


# complex numbers
SOURCES += src/ruby-complex.cc


# These are for JSON meta-data files
HEADERS += src/metadatafile.hh
SOURCES += src/valuehash-json.cc \
        src/metadatafile.cc \
        src/formats/qsmprovider.cc

# Help browser:
HEADERS += src/helpbrowser.hh
SOURCES += src/helpbrowser.cc


######################################################################
# Sources for the parameter space explorers
SOURCES += src/parameterspaceexplorers.cc
                

######################################################################
# Sources of file-format specific code
SOURCES += src/formats/gpesprovider.cc \
        src/formats/chi.cc \
        src/formats/eclab.cc \
        src/formats/parametersbackend.cc

# Using signals on platforms that support them
unix|macx {
  SOURCES += src/signals.cc
}

######################################################################
# We link with the converted ODRPACK library
message("Using odrpack")

SOURCES += odrpack/odrpackfitengine.cc \        
        odrpack/d_lpkbls.c \
        odrpack/d_odr.c \
        odrpack/d_mprec.c \
        odrpack/f2c_remains.c

