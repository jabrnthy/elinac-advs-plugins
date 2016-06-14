/* NDFileXMLAttribute.cpp
 * Writes NDArrays to JPEG files.
 *
 * Mark Rivers
 * May 9, 2009
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <epicsStdio.h>
#include <iocsh.h>

#include "NDPluginFile.h"
#include "NDFileXMLAttributes.h"
#include <epicsExport.h>

using namespace std;

static const char *driverName = "NDFileXMLAttribute";
#define MAX_ATTRIBUTE_STRING_SIZE 256

/** Opens a JPEG file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
asynStatus NDFileXMLAttribute::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    static const char *functionName = "openFile";
    NDAttribute *pAttribute;

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    //if (openMode & NDFileModeAppend) return(asynError);

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

	/* Open the file */
	ios_base::openmode mode = fstream::out;
	if ( openMode & NDFileModeAppend ) mode |= fstream::app;
	fs.open( fileName, mode );

   /* Create the file. */
	if ( !fs ) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s:%s error opening file %s\n",
        driverName, functionName, fileName);
        return(asynError);
    }

    return(asynSuccess);
}

/** Write the attributes from this NDArray into the XML file
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus NDFileXMLAttribute::writeFile(NDArray *pArray)
{
    static const char *functionName = "writeFile";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s:s: %d, %d\n", 
              driverName, functionName, pArray->dims[0].size, pArray->dims[1].size);
              
	/* Write the header */
	fs << "<?xml version=\"1.0\"?>" << endl << "<attributes>" << endl;
	
	NDAttrDataType_t type;
	size_t size;

	/* Get the list of attributes to write */
	NDAttributeList *pList = pArray->pAttributeList;
	NDAttribute *pAttribute = pList->next(0);
	while( pAttribute ) {

		pAttribute->getValueInfo( &type, &size );

		fs << "\t<attribute name=\"" << pAttribute->getName() << "\">";
		switch( type ) {

			case NDAttrInt8:
			case NDAttrInt16:
			case NDAttrInt32: {
				int value = 0;
				pAttribute->getValue( type, (void*)(&value), sizeof(int) );
				fs << value;
				break;
			}
			case NDAttrUInt8:
			case NDAttrUInt16:
			case NDAttrUInt32: {
				unsigned int value = 0;
				pAttribute->getValue( type, (void*)(&value), sizeof(unsigned int) );
				fs << value;
				break;
			}
			case NDAttrFloat32: {
				float value = 0;
				pAttribute->getValue( type, (void*)(&value), sizeof(float) );
				fs << value;
				break;
			}
			case NDAttrFloat64: {
				double value = 0;
				pAttribute->getValue( type, (void*)(&value), sizeof(double) );
				fs << value;
				break;
			}
			case NDAttrString: {
				// force the null-termination of the buffer
				char buffer[128];
				pAttribute->getValue( type, (void*)(&buffer), 128 );
				fs << string( (char*)buffer );
				break;
			}
		}
		fs << "</attribute>" << endl;
		
		pAttribute = pList->next(pAttribute);
	}

	fs << "</attributes>" << endl;
    return(asynSuccess);
}

/** Reads single NDArray from a JPEG file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus NDFileXMLAttribute::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


/** Closes the JPEG file. */
asynStatus NDFileXMLAttribute::closeFile()
{
    //static const char *functionName = "closeFile";
	fs.close();

    return asynSuccess;
}

/** Constructor for NDFileXMLAttribute; all parameters are simply passed to NDPluginFile::NDPluginFile.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDFileXMLAttribute::NDFileXMLAttribute(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_XML_PARAMS,
                   1, -1, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //const char *functionName = "NDFileXMLAttribute";


    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDFileXMLAttribute");
    this->supportsMultipleArrays = 0;
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileXMLAttributeConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    NDFileXMLAttribute *pPlugin =
        new NDFileXMLAttribute(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                       priority, stackSize);
    pPlugin = NULL;  /* This is just to eliminate compiler warning about unused variables/objects */
    return(asynSuccess);
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDFileXMLAttributeConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileXMLAttributeConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileXMLAttributeRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileXMLAttributeRegister);
}
