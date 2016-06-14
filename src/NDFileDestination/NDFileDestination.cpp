/*
 * NDFileDestination.cpp
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
#include "NDFileDestination.h"
#include "paramAttribute.h"
#include "NDPluginFile.h"
#include <epicsExport.h>

using namespace std;

static const char *driverName="NDFileDestination";

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Add the attributes to the NDArray
  * \param[in] pArray  The NDArray from the callback.
  */
void NDFileDestination::processCallbacks ( NDArray *pArray ) {

    const char* functionName = "processCallbacks";

	// set the filename, regardless of whether or not the image is being saved
	char filename_buffer[32];
	epicsTimeToStrftime( filename_buffer, 32, "%Y%m%d-%H%M%S.%06f", &(pArray->epicsTS) );
	pArray->pAttributeList->add ( "FilePluginFileName", "File Name", NDAttrString, &filename_buffer );
	this->setStringParam ( NDFileName, filename_buffer );

    /* Call the base class method */
    NDPluginFile::processCallbacks(pArray);

	callParamCallbacks();
	doCallbacksGenericPointer ( pArray, NDArrayData, 0 );
}


NDFileDestination::~NDFileDestination() {

}


asynStatus NDFileDestination::openFile ( const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray ) {

	return asynSuccess;
}


asynStatus NDFileDestination::readFile ( NDArray **pArray ) {

	return asynError;
}


asynStatus NDFileDestination::writeFile ( NDArray *pArray ) {

	// set the FILEPLUGIN_DESTINATION to all
	const int file_write = 1;
	pArray->pAttributeList->add ( FILEPLUGIN_WRITEFILE, "", NDAttrInt32, (void*)&file_write );

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "%s:writeFile(): Tagging an array for storage.\n", driverName );

	return asynSuccess;
}


asynStatus NDFileDestination::closeFile () {

	return asynSuccess;
}


/** Constructor for NDFileDestination; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDFileDestination::NDFileDestination(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
	/* Invoke the NDPluginDriver class constructor */
	: NDPluginFile (
		portName, queueSize, blockingCallbacks,
		NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_NDFileDestination_PARAMS,
		maxBuffers, maxMemory, asynGenericPointerMask, asynGenericPointerMask,
		0, 1, priority, stackSize ) {

	asynStatus status;
	//const char *functionName = "NDFileDestination";
    
	/* Set the plugin type string */
	setStringParam ( NDPluginDriverPluginType, "NDFileDestination" );

	/* Try to connect to the array port */
	status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDFileDestinationConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDFileDestination *pPlugin =
        new NDFileDestination(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDFileDestinationConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileDestinationConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDFileDestinationRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileDestinationRegister);
}
