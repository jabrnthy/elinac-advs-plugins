TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDFileDestination.dbd
DB += NDFileDestination.template

#USR_INCLUDES += -I /usr/include/synapps5

LIB_LIBS += NDPlugin ADBase asyn Com

LIBRARY_IOC = NDFileDestination
LIB_SRCS += NDFileDestination.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

