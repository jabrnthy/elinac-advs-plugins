#include <iocsh.h>
#include <epicsExport.h>

#include <string>

#include "NDPluginViewScreenConfiguration.h"

/** Configuration command */
extern "C" int NDViewScreenConfigurationConfigure (
	const char *portName, int queueSize, int blockingCallbacks,
	const char *NDArrayPort, int NDArrayAddr,
	int maxBuffers, size_t maxMemory,
	int priority, int stackSize ) {

    new NDPluginViewScreenConfiguration ( portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                        maxBuffers, maxMemory, priority, stackSize);

    return ( asynSuccess );
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

static const iocshFuncDef initFuncDef = { "NDViewScreenConfigurationConfigure", 9, initArgs };

static void initCallFunc ( const iocshArgBuf *args )
{
    NDViewScreenConfigurationConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival, args[7].ival, args[8].ival);
}


extern "C" void NDViewScreenConfigurationRegister ( void )
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar ( NDViewScreenConfigurationRegister );
}
