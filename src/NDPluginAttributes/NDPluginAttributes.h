#ifndef NDPluginAttributes_H
#define NDPluginAttributes_H

#include <epicsTypes.h>
#include "PVAttribute.h"
//#include <asynStandardInterfaces.h>
#include <vector>
#include <string>

#include "NDPluginDriver.h"

/* PV Names */
#define NDPluginAttributesPV1String		"PV1"		/* (r/w) A PV attribute name */
#define NDPluginAttributesPV2String		"PV2"		/* (r/w)  */
#define NDPluginAttributesPV3String		"PV3"		/* (r/w)  */
#define NDPluginAttributesPV4String		"PV4"		/* (r/w)  */
#define NDPluginAttributesPV5String		"PV5"		/* (r/w)   */
#define NDPluginAttributesPV6String		"PV6"		/* (r/w)   */
#define NDPluginAttributesPV7String		"PV7"		/* (r/w)   */
#define NDPluginAttributesPV8String		"PV8"		/* (r/w)  */
#define NDPluginAttributesPV9String		"PV9"		/* (r/w)   */
#define NDPluginAttributesPV10String		"PV10"		/* (r/w)   */
#define NDPluginAttributesPV11String		"PV11"		/* (r/w)   */
#define NDPluginAttributesPV12String		"PV12"		/* (r/w)   */
#define NDPluginAttributesPV13String		"PV13"		/* (r/w)   */
#define NDPluginAttributesPV14String		"PV14"		/* (r/w)   */
#define NDPluginAttributesPV15String		"PV15"		/* (r/w)   */
#define NDPluginAttributesPV16String		"PV16"		/* (r/w)   */
#define NDPluginAttributesNoteString		"NOTE"		/* (r/w)   */

/** Performs macro substitution on an attribute file then adds the attributes to the current NDArray
  */
class NDPluginAttributes : public NDPluginDriver {
public:
    NDPluginAttributes(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
	~NDPluginAttributes();
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeOctet (asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    
protected:
    int NDPluginAttributesComputeStatistics;
    #define FIRST_NDPLUGIN_ATTRIBUTE_APPENDER_PARAM NDPluginAttributesPV1
    /* File and Macro Strings */
    int NDPluginAttributesPV1;
    int NDPluginAttributesPV2;
    int NDPluginAttributesPV3;
    int NDPluginAttributesPV4;
    int NDPluginAttributesPV5;
    int NDPluginAttributesPV6;
    int NDPluginAttributesPV7;
    int NDPluginAttributesPV8;
    int NDPluginAttributesPV9;
    int NDPluginAttributesPV10;
    int NDPluginAttributesPV11;
    int NDPluginAttributesPV12;
    int NDPluginAttributesPV13;
    int NDPluginAttributesPV14;
    int NDPluginAttributesPV15;
    int NDPluginAttributesPV16;
    int NDPluginAttributesNote;
	#define LAST_NDPLUGIN_ATTRIBUTE_APPENDER_PARAM NDPluginAttributesNote
                                
private:

	std::vector<std::string> _pv_names;
	std::vector<PVAttribute*> _pv_attributes;
};
#define NUM_NDPLUGIN_ATTRIBUTE_APPENDER_PARAMS (&LAST_NDPLUGIN_ATTRIBUTE_APPENDER_PARAM - &FIRST_NDPLUGIN_ATTRIBUTE_APPENDER_PARAM + 1)
    
#endif
