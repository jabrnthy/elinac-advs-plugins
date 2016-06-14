#***************************************************
# Make top level for a Support Library
# $Revision: 1.1.1.1 $
# $Date: 2014/02/04 02:05:04 $
#***************************************************
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *configure*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *src*))
include $(TOP)/configure/RULES_DIRS

