include("binaural-sdr.version.pri")

TARGET = binaural-sdr
TEMPLATE = app
message(Building: = $${TARGET} - $${VERSION})

DEPENDPATH += .
INCLUDEPATH += . ../3rdparty/SoftFM

QT += widgets
CONFIG += c++11 thread release
LIBS += -lrtlsdr -lusb-1.0 -lasound

HEADERS += ../3rdparty/SoftFM/AudioOutput.h ../3rdparty/SoftFM/Filter.h \
../3rdparty/SoftFM/FmDecode.h ../3rdparty/SoftFM/RtlSdrSource.h ../3rdparty/SoftFM/SoftFM.h
SOURCES += ../3rdparty/SoftFM/AudioOutput.cc ../3rdparty/SoftFM/Filter.cc \
../3rdparty/SoftFM/FmDecode.cc ../3rdparty/SoftFM/RtlSdrSource.cc

HEADERS += \
        app/DualwordApp.h \
	app/Receiver.h \
	app/global.h \
	gui/MainWindow.h \
	gui/LEDButton.h

SOURCES += \
	app/main.cpp \
	app/DualwordApp.cpp \
	app/Receiver.cpp \
	gui/MainWindow.cpp


FORMS += \
	gui/MainWindow.ui

OBJECTS_DIR = .build/obj
MOC_DIR     = .build/moc
RCC_DIR     = .build/rcc
UI_DIR      = .build/ui
