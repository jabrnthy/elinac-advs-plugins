/*
 * NDFileXMLAttribute.h
 * Writes NDArray attributes to an XML file
 * Jason Abernathy
 * Mar 2, 2012
 */

#ifndef NDFileXMLAttributes_H
#define NDFileXMLAttributes_H

#include <iostream>
#include <fstream>

/** Writes the attributes from an NDArray into an xml file
  */
class NDFileXMLAttribute : public NDPluginFile {
public:
    NDFileXMLAttribute(const char *portName, int queueSize, int blockingCallbacks,
               const char *NDArrayPort, int NDArrayAddr,
               int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

private:
    NDColorMode_t colorMode;
    std::fstream fs;
};
//#define NUM_NDFILE_XML_PARAMS (&LAST_NDFILE_XML_PARAM - &FIRST_NDFILE_XML_PARAM + 1)
#define NUM_NDFILE_XML_PARAMS 0

#endif
