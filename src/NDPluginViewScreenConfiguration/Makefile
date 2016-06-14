TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDPluginViewScreenConfiguration.dbd
DB += NDViewScreenConfiguration.template

USR_CXXFLAGS += --std=c++0x -rdynamic

USR_INCLUDES += -I /usr/include/opencv 

LIB_LIBS += ViewScreenConfiguredNDPlugin ADBase asyn Com
LIB_LIBS += ViewScreenConfiguredNDPlugin Com


LIBRARY_IOC = NDPluginViewScreenConfiguration
LIB_SRCS += NDPluginViewScreenConfiguration.cpp NDPluginViewScreenConfigurationIOCShell.cpp

#PROD = NDPluginMagnificationCorrection

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


#LDFLAGS=-Wl,-rpath,opencv-2.3.1/obj-x86_64-linux-gnu/lib/
#LDLIBS=-Lopencv-2.3.1/obj-x86_64-linux-gnu/lib/ -lopencv_gpu -lopencv_contrib -lopencv_legacy -lopencv_objdetect -lopencv_calib3d -lopencv_features2d -lopencv_video -lopencv_highgui -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core

#MagnificationCorrection: NDPluginMagnificationCorrection.o tinyxml2.o
#	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)
