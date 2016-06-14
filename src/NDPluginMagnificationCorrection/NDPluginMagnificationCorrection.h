#ifndef NDPluginMagnificationCorrection_H
#define NDPluginMagnificationCorrection_H

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

/** Perform a magnification correction on NDArrays.   */
class NDPluginMagnificationCorrection : public ViewScreenConfiguredNDPlugin {
public:
    NDPluginMagnificationCorrection(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

    /* These methods are unique to this class */

protected:

private:

	/** ViewScreenConfiguredNDPlugin::configuration_change_callback
	 *	This function is called when the configuration changes
	 */
	virtual ConfigurationStatus_t configuration_change_callback ();

	/** Report the compatibility of the input array and correction table
	 */
	bool preprocess_check ( NDArray *pArray );

	/** Carry out the calibration procedure
	*/
	asynStatus calibrate ();

	#ifdef USE_OPENCV
	cv::Mat magnification_correction_table;
	#else
	std::vector<float> magnification_correction_table;
	#endif
	

	/** Calibration parameters **/
};
/*#define NUM_NDPluginMagnificationCorrection_PARAMS (&LAST_NDPluginMagnificationCorrection_PARAM - &FIRST_NDPluginMagnificationCorrection_PARAM + 1)*/
#define NUM_NDPluginMagnificationCorrection_PARAMS 0

#endif // NDPluginMagnificationCorrection
