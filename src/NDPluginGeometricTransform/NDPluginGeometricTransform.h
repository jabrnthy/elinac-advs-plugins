#ifndef NDPluginGeometricTransform_H
#define NDPluginGeometricTransform_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>
#include <vector>
#include <array>
#include <tuple>

// GPC
extern "C" {
	#include "gpc.h"
}

#include "NDPluginDriver.h"

/** Map parameter enums to strings that will be used to set up EPICS databases
  */

/** Perform a geometric correction on NDArrays.   */
class NDPluginGeometricTransform : public ViewScreenConfiguredNDPlugin {
public:
    NDPluginGeometricTransform(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

    /* These methods are unique to this class */

protected:
    #define FIRST_GEOMTRANSFORM_PARAM NDPluginGeometricTransformNextOrder
    #define LAST_GEOMTRANSFORM_PARAM NDPluginGeometricTransformCalibrate

private:

	/** ViewScreenConfiguredNDPlugin::configuration_change_callback
	 *	This function is called when the configuration changes
	 */
	virtual ConfigurationStatus_t configuration_change_callback();

	/** Report the compatibility of the input array and correction table
	 */
	bool preprocess_check ( NDArray *pArray );

	/** NDPluginGeometricTransform::calculate_gpc_polygon_area
 	* This function calculates the area contained within the first contour of a gpc_polygon
	 */
	double calculate_gpc_polygon_area(gpc_polygon &polygon);

	/** Produces the offsets which create a convex polynomial in input image-space
	 */
	std::array< std::tuple<double,double>,4 > produce_corner_offsets();
	
	/** Performs a geometric transformation to produce the output image
	 * \param[in] pArrayIn the input array
	*/
	template <typename epicsType> asynStatus transform_array ( NDArray *pArrayIn, NDArray &pArrayOut );

	// the geometric correction table
	std::vector< std::array< std::tuple<unsigned int, float>, 8 > > geometric_correction_table;

	// the total area of the input image in beam coordinates
	double _total_input_area;
};
/*#define NUM_GEOMTRANSFORM_PARAMS (&LAST_GEOMTRANSFORM_PARAM - &FIRST_GEOMTRANSFORM_PARAM + 1)*/
#define NUM_GEOMTRANSFORM_PARAMS 0

#endif
