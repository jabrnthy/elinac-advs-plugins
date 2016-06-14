#ifndef NDPluginViewScreenConfiguration_H
#define NDPluginViewScreenConfiguration_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>
#ifdef USE_OPENCV
 #include "opencv2/core/core.hpp"
#else
 #include <vector>
#endif
#include <array>
#include <tuple>

#include "ViewScreenConfiguredNDPlugin.h"

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
#define NDPluginViewScreenConfigurationTarget0MaterialString	"TARGET0_MATERIAL"
#define NDPluginViewScreenConfigurationTarget1MaterialString	"TARGET1_MATERIAL"
#define NDPluginViewScreenConfigurationTarget2MaterialString	"TARGET2_MATERIAL"

#define NDPluginViewScreenConfigurationTarget0LightDistributionString	"TARGET0_LIGHT_DISTRIBUTION"
#define NDPluginViewScreenConfigurationTarget1LightDistributionString	"TARGET1_LIGHT_DISTRIBUTION"
#define NDPluginViewScreenConfigurationTarget2LightDistributionString	"TARGET2_LIGHT_DISTRIBUTION"

#define NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryString	"TARGET0_EFFICIENCY_MAP_DIRECTORY"
#define NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryString	"TARGET1_EFFICIENCY_MAP_DIRECTORY"
#define NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryString	"TARGET2_EFFICIENCY_MAP_DIRECTORY"

#define NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryExistsString	"TARGET0_EFFICIENCY_MAP_DIRECTORY_EXISTS"
#define NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryExistsString	"TARGET1_EFFICIENCY_MAP_DIRECTORY_EXISTS"
#define NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryExistsString	"TARGET2_EFFICIENCY_MAP_DIRECTORY_EXISTS"

#define NDPluginViewScreenConfigurationEfficiencyMapIncludeRadiusString	"EFFICIENCY_MAP_INCLUDE_RADIUS"
#define NDPluginViewScreenConfigurationEfficiencyMapIncludeCountString	"EFFICIENCY_MAP_INCLUDE_COUNT"

#define NDPluginViewScreenConfigurationConfigurationFileDirectoryString	"CONFIGURATION_FILE_DIRECTORY"

#define NDPluginViewScreenConfigurationBeamspaceStartXString			"BEAMSPACE_START_X"
#define NDPluginViewScreenConfigurationBeamspaceEndXString				"BEAMSPACE_END_X"
#define NDPluginViewScreenConfigurationBeamspaceStartYString			"BEAMSPACE_START_Y"
#define NDPluginViewScreenConfigurationBeamspaceEndYString				"BEAMSPACE_END_Y"

#define NDPluginViewScreenConfigurationInputImageWidthString			"INPUT_IMAGE_WIDTH"
#define NDPluginViewScreenConfigurationInputImageHeightString			"INPUT_IMAGE_HEIGHT"
#define NDPluginViewScreenConfigurationOutputImageWidthString			"OUTPUT_IMAGE_WIDTH"
#define NDPluginViewScreenConfigurationOutputImageHeightString			"OUTPUT_IMAGE_HEIGHT"


/** Export the view screen configuration parameters to EPICS.   */
class NDPluginViewScreenConfiguration : public ViewScreenConfiguredNDPlugin {
public:

    NDPluginViewScreenConfiguration(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

    /* These methods are unique to this class */

protected:

	#define FIRST_NDPluginViewScreenConfiguration_PARAM NDPluginViewScreenConfigurationTarget0Material
	int NDPluginViewScreenConfigurationTarget0Material;
	int NDPluginViewScreenConfigurationTarget1Material;
	int NDPluginViewScreenConfigurationTarget2Material;

	int NDPluginViewScreenConfigurationTarget0LightDistribution;
	int NDPluginViewScreenConfigurationTarget1LightDistribution;
	int NDPluginViewScreenConfigurationTarget2LightDistribution;

	int NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectory;
	int NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectory;
	int NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectory;

	int NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryExists;
	int NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryExists;
	int NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryExists;

	int NDPluginViewScreenConfigurationEfficiencyMapIncludeRadius;
	int NDPluginViewScreenConfigurationEfficiencyMapIncludeCount;

	int NDPluginViewScreenConfigurationConfigurationFileDirectory;

	int NDPluginViewScreenConfigurationBeamspaceStartX;
	int NDPluginViewScreenConfigurationBeamspaceEndX;
	int NDPluginViewScreenConfigurationBeamspaceStartY;
	int NDPluginViewScreenConfigurationBeamspaceEndY;

	int NDPluginViewScreenConfigurationInputImageWidth;
	int NDPluginViewScreenConfigurationInputImageHeight;
	int NDPluginViewScreenConfigurationOutputImageWidth;
	int NDPluginViewScreenConfigurationOutputImageHeight;
	#define LAST_NDPluginViewScreenConfiguration_PARAM NDPluginViewScreenConfigurationOutputImageHeight

private:

	/** ViewScreenConfiguredNDPlugin::configuration_change_callback
	 *	This function is called when the configuration changes
	 */
	virtual ConfigurationStatus_t configuration_change_callback ();

	/** Report the compatibility of the input array and correction table
	 */
	bool preprocess_check ( NDArray *pArray );
};
#define NUM_NDPluginViewScreenConfiguration_PARAMS (&LAST_NDPluginViewScreenConfiguration_PARAM - &FIRST_NDPluginViewScreenConfiguration_PARAM + 1)

#endif // NDPluginViewScreenConfiguration
