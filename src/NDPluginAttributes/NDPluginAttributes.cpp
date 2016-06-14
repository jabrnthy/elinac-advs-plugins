/*
 * NDPluginAttributes.cpp
 *
 * Adds the user-defined PVs as NDArray attributes
 * Author: Jason Abernathy
 *
 * Created March 17, 2012
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// for debugging
#include <iostream>
#include <string>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "NDPluginAttributes.h"
#include "PVAttribute.h"
#include "paramAttribute.h"
#include <epicsExport.h>

using namespace std;

static const char *driverName="NDPluginAttributes";

/** Called when asyn clients call pasynOctet->write().
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDPluginAttributes::writeOctet(asynUser *pasynUser, const char *value,
                                   size_t nChars, size_t *nActual)
{
	int addr=0;
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *functionName = "writeOctet";

	if ( (function >= NDPluginAttributesPV1) && (function <= NDPluginAttributesPV16) ) {

		/* writeOctet is occasionally being passed "\0" with a length of 1. This case should be checked for */
		string temp( value, nChars );
		*nActual = temp.length();
		if ( strlen( temp.c_str() ) == 0 ) {
			temp = string("");
		}
		_pv_names[function - NDPluginAttributesPV1] = temp;
	}

	return asynNDArrayDriver::writeOctet(pasynUser, value, nChars, nActual);	
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Add the attributes to the NDArray
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginAttributes::processCallbacks(NDArray *pArray)
{
    const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

	/* Add the attributes to the array */
	for( vector<string>::const_iterator pv_name = _pv_names.begin(); pv_name != _pv_names.end(); pv_name++ ) {

		if( pv_name->empty() ) continue;

		PVAttribute *pv_attribute = new PVAttribute( pv_name->c_str(), "", pv_name->c_str(), DBR_NATIVE );
		pArray->pAttributeList->add(pv_attribute);
	}

	/* Add the note */
	char buffer[64];
	getStringParam( NDPluginAttributesNote, 64, buffer );
	pArray->pAttributeList->add( "Note", "", NDAttrString, (void*)buffer );

	/*paramAttribute *note = new paramAttribute( "Note", "", NDPluginAttributesNoteString, 0, this, "STRING" );
	if ( note ) {

		note->updateValue();
		pArray->pAttributeList->add( note );	
		char buffer[64];
		string notestring( buffer, 64 );
		getStringParam( NDPluginAttributesNote, 64, buffer );
	}*/
	
	
	doCallbacksGenericPointer(pArray, NDArrayData, 0);
}


NDPluginAttributes::~NDPluginAttributes() {

}


/** Constructor for NDPluginAttributes; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginAttributes::NDPluginAttributes(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_ATTRIBUTE_APPENDER_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    asynStatus status;
    //const char *functionName = "NDPluginAttributes";
    
    /* File and Macro Strings */
	createParam( NDPluginAttributesPV1String, asynParamOctet, &NDPluginAttributesPV1 );
	createParam( NDPluginAttributesPV2String, asynParamOctet, &NDPluginAttributesPV2 );
	createParam( NDPluginAttributesPV3String, asynParamOctet, &NDPluginAttributesPV3 );
	createParam( NDPluginAttributesPV4String, asynParamOctet, &NDPluginAttributesPV4 );
	createParam( NDPluginAttributesPV5String, asynParamOctet, &NDPluginAttributesPV5 );
	createParam( NDPluginAttributesPV6String, asynParamOctet, &NDPluginAttributesPV6 );
	createParam( NDPluginAttributesPV7String, asynParamOctet, &NDPluginAttributesPV7 );
	createParam( NDPluginAttributesPV8String, asynParamOctet, &NDPluginAttributesPV8 );
	createParam( NDPluginAttributesPV9String, asynParamOctet, &NDPluginAttributesPV9 );
	createParam( NDPluginAttributesPV10String, asynParamOctet, &NDPluginAttributesPV10 );
	createParam( NDPluginAttributesPV11String, asynParamOctet, &NDPluginAttributesPV11 );
	createParam( NDPluginAttributesPV12String, asynParamOctet, &NDPluginAttributesPV12 );
	createParam( NDPluginAttributesPV13String, asynParamOctet, &NDPluginAttributesPV13 );
	createParam( NDPluginAttributesPV14String, asynParamOctet, &NDPluginAttributesPV14 );
	createParam( NDPluginAttributesPV15String, asynParamOctet, &NDPluginAttributesPV15 );
	createParam( NDPluginAttributesPV16String, asynParamOctet, &NDPluginAttributesPV16 );
	createParam( NDPluginAttributesNoteString, asynParamOctet, &NDPluginAttributesNote );
	    
	/* Resize the string vectors */
	_pv_names.resize(16);
    
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginAttributes");

	/* Initialize the status of each attribute file */
	setStringParam( NDPluginAttributesPV1, "" );
	setStringParam( NDPluginAttributesPV2, "" );
	setStringParam( NDPluginAttributesPV3, "" );
	setStringParam( NDPluginAttributesPV4, "" );
	setStringParam( NDPluginAttributesPV5, "" );
	setStringParam( NDPluginAttributesPV6, "" );
	setStringParam( NDPluginAttributesPV7, "" );
	setStringParam( NDPluginAttributesPV8, "" );
	setStringParam( NDPluginAttributesPV9, "" );
	setStringParam( NDPluginAttributesPV10, "" );
	setStringParam( NDPluginAttributesPV11, "" );
	setStringParam( NDPluginAttributesPV12, "" );
	setStringParam( NDPluginAttributesPV13, "" );
	setStringParam( NDPluginAttributesPV14, "" );
	setStringParam( NDPluginAttributesPV15, "" );
	setStringParam( NDPluginAttributesPV16, "" );
	setStringParam( NDPluginAttributesNote, "" );

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDAttributesConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginAttributes *pPlugin =
        new NDPluginAttributes(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDAttributesConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDAttributesConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDAttributesRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDAttributesRegister);
}
