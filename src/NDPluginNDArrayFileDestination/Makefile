TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDPluginNDArrayFileDestination.dbd
DB += NDArrayFileDestination.template

USR_INCLUDES += -I $(AREADETECTOR)

LIB_LIBS += NDPlugin ADBase asyn Com

LIBRARY_IOC = NDPluginNDArrayFileDestination
LIB_SRCS += NDPluginNDArrayFileDestination.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

