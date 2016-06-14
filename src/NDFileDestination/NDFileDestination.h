#ifndef NDFileDestination_H
#define NDFileDestination_H

#include <epicsTypes.h>

#include "NDPluginFile.h"

/** Sets the FILEPLUGIN_WRITEFILE attribute of an NDArray. This ensures that when multiple views of the same frame are kept in sync when the operator uses the save feature while the camera is acquiring data
 */

class NDFileDestination : public NDPluginFile {
public:
    NDFileDestination (
		const char *portName, int queueSize, int blockingCallbacks,
		const char *NDArrayPort, int NDArrayAddr,
		int maxBuffers, size_t maxMemory,
		int priority, int stackSize );

	~NDFileDestination();

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
	virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
	virtual asynStatus readFile(NDArray **pArray);
	virtual asynStatus writeFile(NDArray *pArray);
	virtual asynStatus closeFile();
    
protected:
    #define FIRST_NDPLUGIN_NDFileDestination_PARAM NDFileDestinationDummyParameter
	int NDFileDestinationDummyParameter; // keep this here in case parameters are added in the future
	#define LAST_NDPLUGIN_NDFileDestination_PARAM NDFileDestinationDummyParameter
                                
private:

};
#define NUM_NDPLUGIN_NDFileDestination_PARAMS (&LAST_NDPLUGIN_NDFileDestination_PARAM - &FIRST_NDPLUGIN_NDFileDestination_PARAM + 1)
    
#endif
