TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES
DB += ViewScreenConfiguredNDPlugin.template
DBD += ViewScreenConfiguredNDPlugin.dbd

USR_CXXFLAGS += --std=c++0x
USR_INCLUDES += -I $(ASYN)
USR_INCLUDES += -I $(ADCORE)

#LIB_SYS_LIBS += gsl gslcblas opencv_gpu opencv_contrib opencv_legacy opencv_objdetect opencv_calib3d opencv_features2d opencv_video opencv_highgui opencv_ml opencv_imgproc opencv_flann opencv_core

#DB += ViewScreenConfiguredNDPlugin.template

LIB_SYS_LIBS += gsl gslcblas
LIB_LIBS += asyn

LIBRARY_IOC += ViewScreenConfiguredNDPlugin
LIB_SRCS += ViewScreenConfiguredNDPlugin.cpp ViewScreenConfiguredNDPluginIOCShell.cpp tinyxml2.cpp

INC += ViewScreenConfiguredNDPlugin.h
INC += tinyxml2.h

#PROD += NDViewScreenConfiguredDriverTest
#PROD_SRCS += NDViewScreenConfiguredDriverTest.cpp
#PROD_LIBS += asyn NDViewScreenConfiguredDriver
#PROD_SYS_LIBS += opencv_core

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
