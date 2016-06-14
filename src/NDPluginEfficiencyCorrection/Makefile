TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES

DBD += NDPluginEfficiencyCorrection.dbd
DB += NDEfficiencyCorrection.template

USR_CXXFLAGS += --std=c++0x -rdynamic

#USR_INCLUDES += -I /usr/include/opencv 

LIB_LIBS += ViewScreenConfiguredNDPlugin ADBase asyn Com

#LIB_SYS_LIBS += gsl gslcblas opencv_gpu opencv_contrib opencv_legacy opencv_objdetect opencv_calib3d opencv_features2d opencv_video opencv_highgui opencv_ml opencv_imgproc opencv_flann opencv_core

##LIB_SYS_LIBS += gsl gslcblas opencv_core
## Email from Jason 05/23/2014:
## p.s. you'll need to remove the opencv_core library dependency from NDPluginMagnificationCorrection/Makefile and NDPluginEfficiencyCorrection/Makefile 
LIB_SYS_LIBS += gsl gslcblas

LIBRARY_IOC = NDPluginEfficiencyCorrection
LIB_SRCS += NDPluginEfficiencyCorrection.cpp NDPluginEfficiencyCorrectionIOCShell.cpp

#PROD = NDPluginEfficiencyCorrection

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


#LDFLAGS=-Wl,-rpath,opencv-2.3.1/obj-x86_64-linux-gnu/lib/
#LDLIBS=-Lopencv-2.3.1/obj-x86_64-linux-gnu/lib/ -lopencv_gpu -lopencv_contrib -lopencv_legacy -lopencv_objdetect -lopencv_calib3d -lopencv_features2d -lopencv_video -lopencv_highgui -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core

#EfficiencyCorrection: NDPluginEfficiencyCorrection.o tinyxml2.o
#	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)
