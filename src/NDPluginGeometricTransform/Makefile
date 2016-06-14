TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDPluginGeometricTransform.dbd
DB += NDGeometricTransform.template

USR_CXXFLAGS += --std=c++0x

USR_INCLUDES += -I $(AREADETECTOR)

LIB_LIBS += NDPlugin ADBase asyn Com

LIB_SYS_LIBS += gsl gslcblas

LIBRARY_IOC = NDPluginGeometricTransform
LIB_SRCS += gpc.c NDPluginGeometricTransform.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

