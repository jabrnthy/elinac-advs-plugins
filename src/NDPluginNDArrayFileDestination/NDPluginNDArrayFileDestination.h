#ifndef NDPluginNDArrayFileDestination_H
#define NDPluginNDArrayFileDestination_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

#define FILEPLUGIN_DESTINATION "FilePluginDestination"

/** Sets the FILEPLUGIN_DESTINATION attribute of an NDArray. This ensures that when multiple views of the same frame are kept in sync when the operator uses the save feature while the camera is acquiring data
  */
class NDPluginNDArrayFileDestination : public NDPluginDriver {
public:
    NDPluginNDArrayFileDestination(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
	~NDPluginNDArrayFileDestination();
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    
protected:
    #define FIRST_NDPLUGIN_NDARRAYFILEDESTINATION_PARAM NDPluginNDArrayFileDestinationDummyParameter
	int NDPluginNDArrayFileDestinationDummyParameter; // keep this here in case parameters are added in the future
	#define LAST_NDPLUGIN_NDARRAYFILEDESTINATION_PARAM NDPluginNDArrayFileDestinationDummyParameter
                                
private:

};
#define NUM_NDPLUGIN_NDARRAYFILEDESTINATION_PARAMS (&LAST_NDPLUGIN_NDARRAYFILEDESTINATION_PARAM - &FIRST_NDPLUGIN_NDARRAYFILEDESTINATION_PARAM + 1)
    
#endif
