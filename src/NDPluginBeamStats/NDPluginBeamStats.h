#ifndef NDPluginBeamStats_H
#define NDPluginBeamStats_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "ViewScreenConfiguredNDPlugin.h"

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
#define NDPluginBeamStatsM00String				"M00"
#define NDPluginBeamStatsM10String				"M10"
#define NDPluginBeamStatsM01String				"M01"
#define NDPluginBeamStatsM20String				"M20"
#define NDPluginBeamStatsM11String				"M11"
#define NDPluginBeamStatsM02String				"M02"
#define NDPluginBeamStatsU00String				"U00"
#define NDPluginBeamStatsU10String				"U10"
#define NDPluginBeamStatsU01String				"U01"
#define NDPluginBeamStatsU20String				"U20"
#define NDPluginBeamStatsU11String				"U11"
#define NDPluginBeamStatsU02String				"U02"
#define NDPluginBeamStatsBeamCentroidXString	"CENTROIDX"
#define NDPluginBeamStatsBeamCentroidYString	"CENTROIDY"
#define NDPluginBeamStatsBeamStDevXString		"STDEVX"
#define NDPluginBeamStatsBeamStDevYString		"STDEVY"
#define NDPluginBeamStatsBeamCorrelationString	"CORRELATION"

/** Perform a magnification correction on NDArrays.   */
class NDPluginBeamStats : public ViewScreenConfiguredNDPlugin {
public:
    NDPluginBeamStats(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

    /* These methods are unique to this class */

protected:
	#define FIRST_NDPluginBeamStats_PARAM NDPluginBeamStatsM00
	int NDPluginBeamStatsM00;
	int NDPluginBeamStatsM10;
	int NDPluginBeamStatsM01;
	int NDPluginBeamStatsM20;
	int NDPluginBeamStatsM11;
	int NDPluginBeamStatsM02;
	int NDPluginBeamStatsU00;
	int NDPluginBeamStatsU10;
	int NDPluginBeamStatsU01;
	int NDPluginBeamStatsU20;
	int NDPluginBeamStatsU11;
	int NDPluginBeamStatsU02;
	int NDPluginBeamStatsBeamCentroidX;
	int NDPluginBeamStatsBeamCentroidY;
	int NDPluginBeamStatsBeamStDevX;
	int NDPluginBeamStatsBeamStDevY;
	int NDPluginBeamStatsBeamCorrelation;
	#define LAST_NDPluginBeamStats_PARAM NDPluginBeamStatsBeamCorrelation

private:

	/** ViewScreenConfiguredNDPlugin::configuration_change_callback
	 *	This function is called when the configuration changes
	 */
	virtual ConfigurationStatus_t configuration_change_callback ();

	/** Report the compatibility of the input array and correction table
	 */
	bool preprocess_check ( NDArray *pArray );

	/** Calculate the beam statistics
	*/
	template <typename dataType, typename accumulatorType> void calculate_beam_statistics ( NDArray *pArray );
	

	/** Calibration parameters **/
};
#define NUM_NDPluginBeamStats_PARAMS (&LAST_NDPluginBeamStats_PARAM - &FIRST_NDPluginBeamStats_PARAM + 1)

#endif // NDPluginBeamStats
