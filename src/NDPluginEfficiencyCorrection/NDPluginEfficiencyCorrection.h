#ifndef NDPluginEfficiencyCorrection_H
#define NDPluginEfficiencyCorrection_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>
#ifdef USE_OPENCV
 #include "opencv2/core/core.hpp"
#endif

#include <fstream>
#include <tuple>
#include <vector>
#include <string>

#include "ViewScreenConfiguredNDPlugin.h"

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
#define NDPluginEfficiencyCorrectionCurrentTargetNumberString	"CURRENT_TARGET_NUMBER"
#define NDPluginEfficiencyCorrectionCurrentIrisDiameterString	"CURRENT_IRIS_DIAMETER"
#define NDPluginEfficiencyCorrectionCurrentBeamEnergyString		"CURRENT_BEAM_ENERGY"

/** Perform a magnification correction on NDArrays.   */
class NDPluginEfficiencyCorrection : public ViewScreenConfiguredNDPlugin {
public:
    NDPluginEfficiencyCorrection(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

    /* These methods are unique to this class */

protected:

	#define FIRST_NDPluginEfficiencyCorrection_PARAM NDPluginEfficiencyCorrectionCurrentTargetNumber
	int NDPluginEfficiencyCorrectionCurrentTargetNumber;
	int NDPluginEfficiencyCorrectionCurrentIrisDiameter;
	int NDPluginEfficiencyCorrectionCurrentBeamEnergy;
	#define LAST_NDPluginEfficiencyCorrection_PARAM NDPluginEfficiencyCorrectionCurrentBeamEnergy

private:

	typedef std::vector<std::vector<float>> grid_type;

	struct grid_slice {
		grid_type data;
		float iris_diameter;
		float roi_width_stride; // the positive distance between two x-coordinates
		float roi_height_stride; // positive positive distance between two y-coordinates
		float roi_xi; // the x-value of the left-most row
		float roi_yf; // the y-value of the upper-most row
	};

	struct efficiency_correction_table_parameters_t {

		double beam_energy;
		double iris_diameter;
		std::string target_material;
		std::string target_light_distribution;

		efficiency_correction_table_parameters_t (): beam_energy ( 0. ), iris_diameter ( 0. ), target_material ( "" ), target_light_distribution ( "" ) {};
		bool operator== (const efficiency_correction_table_parameters_t &rhs ) const {

			return (beam_energy == rhs.beam_energy) && (iris_diameter == rhs.iris_diameter) && (target_material == rhs.target_material) && (target_light_distribution == rhs.target_light_distribution);
		};

		bool operator!= (const efficiency_correction_table_parameters_t &rhs ) const {

			return (beam_energy != rhs.beam_energy) || (iris_diameter != rhs.iris_diameter) || (target_material != rhs.target_material) || (target_light_distribution != rhs.target_light_distribution);
		};
	};

	/** The following functions are for loading and reading the efficiency maps **/
	template <typename T> bool read_parameter ( std::ifstream &stream, T &mapread, std::string &parameter_name );
	template <typename T, typename T_param> bool read_vectorparameter ( std::ifstream &stream, T &mapread, std::string &parameter_name );
	template <typename T> bool check_parameters ( T &mapcheck );
	template< typename T> bool check_vector_parameters ( T &mapcheck );

	/** Calculates the number of rays which were emitted by the foil and collected within a specific distance of the brightest pixel on the ccd **/
	float count_collected_rays ( std::vector <std::tuple<size_t, size_t, float> > &psfdata, const size_t nearest_max_points, const float nearest_max_radius );

	/** Transfers the efficiency map from the file to the grid_slice structure **/
	ConfigurationStatus_t load_efficiency_map ( std::string filename, grid_slice &slice, const ViewScreenConfiguredNDPlugin::TargetInfo );

	/** Loads all of the efficiency maps from directory into the grid **/
	ConfigurationStatus_t load_efficiency_maps ( std::vector<grid_slice> &grid, std::string directory, const ViewScreenConfiguredNDPlugin::TargetInfo );

	/** Performs a two dimensional bilinear interpolation of the grid_slice data
	 */
	float interpolate_grid_slice ( const grid_slice &slice, const float x, const float y );

	/** Creates the efficiency table using grid data and view screen calibration
	 */
	#ifdef USE_OPENCV
	bool create_efficiency_correction_table ( const std::vector<grid_slice> &grid, cv::Mat &table );
	#else
	bool create_efficiency_correction_table ( const efficiency_correction_table_parameters_t parameters, std::vector<float> &table );
	#endif

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
	std::map<std::pair<std::string,TargetInfo>,cv::Mat> efficiency_correction_tables;
	cv::Mat current_efficiency_correction_table;
	#else
	std::map<TargetInfo,std::vector<grid_slice>> efficiency_grids;
	std::vector<float> efficiency_correction_table;
	efficiency_correction_table_parameters_t efficiency_correction_table_parameters;
	#endif

	//std::vector <grid_slice> grid;

	/** Calibration parameters **/
};

#define NUM_NDPluginEfficiencyCorrection_PARAMS (&LAST_NDPluginEfficiencyCorrection_PARAM - &FIRST_NDPluginEfficiencyCorrection_PARAM + 1)

#endif // NDPluginEfficiencyCorrection
