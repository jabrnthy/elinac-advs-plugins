TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDPluginOverlayExtra.dbd
DB += NDOverlayExtra.template

USR_INCLUDES += -I /usr/include/synapps5 -I/usr/include/ImageMagick
USR_CPPFLAGS += $(shell GraphicsMagick++-config --cppflags)

LIB_LIBS += NDPlugin ADBase asyn Com
GMLIBS = $(shell GraphicsMagick++-config --libs)
LIB_SYS_LIBS += $(patsubst -l%,%,$(shell pkg-config GraphicsMagick++ --libs))

LIBRARY_IOC = NDPluginOverlayExtra
LIB_SRCS += NDPluginOverlayExtra.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

