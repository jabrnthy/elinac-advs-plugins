/*
 * NDPluginNDArrayFileDestination.cpp
 *
 * Adds the user-defined PVs as NDArray attributes
 * Author: Jason Abernathy
 *
 * Created March 17, 2012
 */

// for debugging
#include <iostream>
#include <string>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "NDPluginNDArrayFileDestination.h"
#include "paramAttribute.h"
#include <epicsExport.h>

using namespace std;

static const char *driverName="NDPluginNDArrayFileDestination";

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including NDReadFile, NDWriteFile and NDFileCapture.
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter. 
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginNDArrayFileDestination::writeInt32(asynUser *pasynUser, epicsInt32 value) {

	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char* functionName = "writeInt32";

	/* Set the parameter in the parameter library. */
	status = (asynStatus) setIntegerParam(function, value);

	if ( function == NDWriteFile ) {

		if ( value == 1 ) {

			// queue one array for storage
			setIntegerParam( NDFileNumCapture, 1 );
			setIntegerParam( NDFileNumCaptured, 0 );
			/* Set the flag back to 0, since this could be a busy record */
			setIntegerParam( NDWriteFile, 0 );
		}
	}
	else if ( function == NDReadFile ) {

		if ( value == 1 ) {

			asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: ERROR, NDReadFile not implemented for NDPluginNDArrayFileDestination",	driverName, functionName);
			setIntegerParam( NDReadFile, 0 );
		}
	}
	else if ( function == NDFileCapture ) {

		if ( value == 1 ) {

			setIntegerParam( NDFileNumCaptured, 0 );
		}
	}
	else  {

		return NDPluginDriver::writeInt32( pasynUser, value );
	}

	callParamCallbacks();

	return status;
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Add the attributes to the NDArray
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginNDArrayFileDestination::processCallbacks(NDArray *pArray)
{
    const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

	// determine if we need to set the FILEPLUGIN_DESTINATION for this array
	int capturing = 0;
	int num_capture = 0;
	int num_captured = 0;

	getIntegerParam( NDFileNumCapture, &num_capture );
	getIntegerParam( NDFileNumCaptured, &num_captured );

	if ( (num_capture <= 0) || (num_capture == num_captured) ) {

		pArray->pAttributeList->add( FILEPLUGIN_DESTINATION, "", NDAttrString, (void*)"none" );
		doCallbacksGenericPointer( pArray, NDArrayData, 0 );
		return;
	}

	// set the FILEPLUGIN_DESTINATION to all
	pArray->pAttributeList->add( FILEPLUGIN_DESTINATION, "", NDAttrString, (void*)"all" );
	//cout << "Set FILEPLUGIN_DESTINATION to all: " << num_captured+1 << " of " << num_capture << endl;

	setIntegerParam( NDFileNumCaptured, num_captured+1 );

	if ( (num_captured+1) >= num_capture ) {

		getIntegerParam( NDFileCapture, &capturing );

		if ( capturing ) {

			setIntegerParam( NDFileCapture, 0 );
		}
	}

	callParamCallbacks();
	doCallbacksGenericPointer(pArray, NDArrayData, 0);
}


NDPluginNDArrayFileDestination::~NDPluginNDArrayFileDestination() {

}


/** Constructor for NDPluginNDArrayFileDestination; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginNDArrayFileDestination::NDPluginNDArrayFileDestination(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
	/* Invoke the NDPluginDriver class constructor */
	: NDPluginDriver(portName, queueSize, blockingCallbacks, 
                     NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_NDARRAYFILEDESTINATION_PARAMS, maxBuffers, maxMemory, 
                     asynInt32Mask | asynGenericPointerMask, asynInt32Mask | asynGenericPointerMask,
                     0, 1, priority, stackSize)
{
    asynStatus status;
    //const char *functionName = "NDPluginNDArrayFileDestination";
    
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginNDArrayFileDestination");

	/* Set the initial number of Arrays to capture to zero */
	setIntegerParam(NDFileNumCaptured, 0);

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDArrayFileDestinationConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginNDArrayFileDestination *pPlugin =
        new NDPluginNDArrayFileDestination(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                        maxBuffers, maxMemory, priority, stackSize);
    pPlugin = NULL;  /* This is just to eliminate compiler warning about unused variables/objects */
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDArrayFileDestinationConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDArrayFileDestinationConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDArrayFileDestinationRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDArrayFileDestinationRegister);
}
