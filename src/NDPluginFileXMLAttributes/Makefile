TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDFileXMLAttributes.dbd
#DB += NDGeometricTransform.template

#USR_INCLUDES += -I /usr/include/synapps5
USR_INCLUDES += -I $(AREADETECTOR)

LIB_LIBS += NDPlugin ADBase asyn Com

SHRLIB_VERSION = 2
LIBRARY_IOC = NDPluginXMLAttributes
LIB_SRCS += NDFileXMLAttributes.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

