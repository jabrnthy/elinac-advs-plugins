TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDPluginAttributes.dbd
DB += NDAttributes.template

USR_INCLUDES += -I $(AREADETECTOR)
USR_INCLUDES += -I $(ASYN)

LIB_LIBS += NDPlugin ADBase asyn Com

#SHRLIB_VERSION = 2
LIBRARY_IOC = NDPluginAttributes
LIB_SRCS += NDPluginAttributes.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

