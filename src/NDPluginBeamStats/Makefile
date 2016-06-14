TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDPluginBeamStats.dbd
DB += NDBeamStats.template

USR_CXXFLAGS += --std=c++0x

LIB_LIBS += NDPlugin

SHRLIB_VERSION = 1
LIBRARY_IOC = NDPluginBeamStats
LIB_SRCS += NDPluginBeamStats.cpp NDPluginBeamStatsIOCShell.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

