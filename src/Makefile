TOP = ../

ifdef EPICS_HOST_ARCH

	include $(TOP)/configure/CONFIG

	DIRS += ViewScreenConfiguredNDPlugin
	DIRS += NDPluginAxesOverlay
	DIRS += NDPluginViewScreenConfiguration
	DIRS += NDPluginOverlayExtra
	DIRS += NDPluginGeometricTransform
	DIRS += NDPluginFileXMLAttributes
	DIRS += NDPluginAttributes
	DIRS += NDFileDestination
	DIRS += NDPluginMagnificationCorrection
	DIRS += NDPluginEfficiencyCorrection
	DIRS += NDPluginBeamStats

	include $(TOP)/configure/RULES_TOP

else
   ifneq ($(wildcard $(TOP)/config)x,x)
     # New Makefile.Host config file location
     include $(TOP)/config/CONFIG_EXTENSIONS
     include $(TOP)/config/RULES_ARCHS
   else
     # Old Makefile.Unix config file location
     EPICS=../../..
     include $(EPICS)/config/CONFIG_EXTENSIONS
     include $(EPICS)/config/RULES_ARCHS
   endif
endif

