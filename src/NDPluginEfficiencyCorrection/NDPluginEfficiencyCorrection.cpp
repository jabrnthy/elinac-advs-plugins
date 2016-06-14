//#include <iostream>
#include <cmath>
#include <string>
#include <iterator>
//#include <vector>
//#include <array>
//#include <tuple>
//#include <sstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <numeric>
#include <dirent.h>

#include "NDPluginEfficiencyCorrection.h"

using namespace std;

#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
class third_tuple_element {
	public:
		bool operator () (tuple<size_t, size_t, float> a, tuple<size_t, size_t, float> b) {

			return get<2>(a) < get<2>(b);
		}
};

class by_closest_to {
	private:
		const size_t u, v;
	public:
		by_closest_to ( size_t u, size_t v ): u(u), v(v) {};
		bool operator () ( const tuple<size_t, size_t, float> &a, const tuple<size_t, size_t, float> &b ) {
			return (pow((int)get<0>(a)-(int)u,2)+pow((int)get<1>(a)-(int)v,2) < pow((int)get<0>(b)-(int)u,2)+pow((int)get<1>(b)-(int)v,2) );
		}
};

class within_distance_and_amount_limits {

	private:
		const size_t u, v;
		const float max_radius;
		size_t remaining;
	public:
		within_distance_and_amount_limits ( const size_t u, const size_t v, const float max_radius, const size_t amount ): u(u), v(v), max_radius(max_radius), remaining(amount) {};
		bool operator () ( const tuple<size_t, size_t, float> &t ) {

			if ( remaining == 0 ) return true;

			remaining--;
			const float distance = sqrt(pow((int)get<0>(t)-(int)u,2)+pow((int)get<1>(t)-(int)v,2) );
			return (distance > max_radius);
		}
};

class total_rays {

	public:
		double operator () ( const double total, const tuple<size_t, size_t, float> &t ) {

			return total + get<2>(t);
		}
};

int select_cform ( const dirent *d ) {

	if ( string::npos != string(d->d_name).find( "cform" ) ) {

		return true;
	}
	else {

		return false;
	}
};

#endif

static const char* pluginName = "NDPluginEfficiencyCorrection";

NDPluginEfficiencyCorrection::NDPluginEfficiencyCorrection ( const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory, int priority, int stackSize ):
	ViewScreenConfiguredNDPlugin (
		portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 1,
		NUM_NDPluginEfficiencyCorrection_PARAMS, maxBuffers, maxMemory,
		asynGenericPointerMask,
		asynGenericPointerMask, ASYN_CANBLOCK, 1, priority, stackSize ) {


	/* Create an empty magnification correction table */
	#ifdef USE_OPENCV
	//this->efficiency_correction_table.create ( 0, 0, cv::CV_32FC1 );
	#else
	this->efficiency_grids.clear();
	this->efficiency_correction_table.clear();
	#endif

	setStringParam  ( NDPluginDriverPluginType, "NDPluginEfficiencyCorrection" );

	createParam ( NDPluginEfficiencyCorrectionCurrentTargetNumberString, asynParamInt32, &this->NDPluginEfficiencyCorrectionCurrentTargetNumber );
	createParam ( NDPluginEfficiencyCorrectionCurrentIrisDiameterString, asynParamFloat64, &this->NDPluginEfficiencyCorrectionCurrentIrisDiameter );
	createParam ( NDPluginEfficiencyCorrectionCurrentBeamEnergyString, asynParamFloat64, &this->NDPluginEfficiencyCorrectionCurrentBeamEnergy );

	setIntegerParam ( this->NDPluginEfficiencyCorrectionCurrentTargetNumber, -1 );
	setDoubleParam  ( this->NDPluginEfficiencyCorrectionCurrentIrisDiameter, 0. );
	setDoubleParam  ( this->NDPluginEfficiencyCorrectionCurrentBeamEnergy, 0. );

	/* Try to connect to the NDArray port */
	this->connectToArrayPort();

	this->callParamCallbacks();
}


ViewScreenConfiguredNDPlugin::ConfigurationStatus_t NDPluginEfficiencyCorrection::configuration_change_callback ( ) {

	/* This function should be called from a locked state */

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::configuration_change_callback: Beginning calibration.\n", pluginName );

	const size_t image_width = this->get_output_image_width();
	const size_t image_height = this->get_output_image_height();

	if ( 0 == image_width || 0 == image_height ) {

		return ConfigurationStatusBadParameter;
	}
	
	size_t invalid_grids = 0;

	/* Load the efficiency maps */
	for ( size_t target_number = 0; target_number < this->get_maximum_target_count(); target_number++ ) {

		std::vector<grid_slice> &grid = this->efficiency_grids[this->get_target_info ( target_number )];
		const std::string directory = this->get_efficiency_map_directory ( target_number );

		if ( 0 == directory.compare ( "" ) ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Skipping target number %lu; undefined efficiency map directory.\n", pluginName, __func__, target_number );
			invalid_grids++;
			continue;
		}

		const ConfigurationStatus_t status = load_efficiency_maps ( grid, directory, this->get_target_info ( target_number ) );

		if ( !(status == ConfigurationStatusConfigured) || grid.size() == 0 ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Skipping target number %lu; unable to load the efficiency maps.\n", pluginName, __func__ , target_number );
			invalid_grids++;
			continue;
		}
	}

	if ( this->get_maximum_target_count() == invalid_grids ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Unable to load any efficiency maps.\n", pluginName, __func__ );
		return ConfigurationStatusUnconfigured;
	}

	return ConfigurationStatusConfigured;
}


/** Report the compatibility of the input array and correction table
  * \param[in] pArray  Pointer to the NDArray to check
  * @return true if the correction may be applied; false otherwise.
  */
bool NDPluginEfficiencyCorrection::preprocess_check ( NDArray *pArray ) {

	/* This function should be called while locked */

	/* Check the configuration status */
	if ( false == this->is_configured() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: View screen configuration not loaded.\n", pluginName, __func__ );
		// The plugin is uncalibrated; there is no point in performing further checks.
		return false;
	}

	bool perform_correction = true;

	/* First, check if we're operating on a supported NDArray. If we're not, then there is no point in checking the efficiency correction table. */
	/* Let's learn a little bit about this NDArray */
	NDArrayInfo_t ndarray_info;
	pArray->getInfo ( &ndarray_info );

	/* We will only operate on two dimensional arrays */
	if ( 2 != pArray->ndims ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Processing restricted to two-dimensional arrays.\n", pluginName );
		perform_correction = false;
	}

	/* We will only operate on greyscale images for now */
	if ( NDColorModeMono != ndarray_info.colorMode ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Plugin restricted to arrays with color mode NDColorModeMono.\n", pluginName );
		perform_correction = false;
	}

	if ( false == perform_correction ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Aborting; unsupported NDArray detected.", pluginName, __func__ );
		return false;
	}

	/* The NDArray is one which may be transformed. Let's check if the efficiency correction table is valid and compatible. */

	/* Grab the beam and optics parameters which affect the efficiency corrections. */
	int target_number = 0;
	if ( asynSuccess != this->getIntegerParam ( NDPluginEfficiencyCorrectionCurrentTargetNumber, &target_number ) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Unable to read the target number parameter from the parameter library.\n", pluginName, __func__ );
		perform_correction = false;
	}
	else if ( (0 > target_number) || (this->get_maximum_target_count() <= (size_t)target_number) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Target number is out of range; target_number=%i.\n", pluginName, __func__, target_number );
		perform_correction = false;
	}	

	/* Let's learn a little bit about the current target, if it's an OTR foil, then the efficiency corrections depend on energy */
	const TargetInfo current_target_info = this->get_target_info ( (size_t)target_number );

	double iris_diameter = 0.;
	if ( asynSuccess != this->getDoubleParam ( NDPluginEfficiencyCorrectionCurrentIrisDiameter, &iris_diameter ) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Unable to read the current iris diameter parameter from the parameter library.\n", pluginName, __func__ );
		perform_correction = false;
	}
	else if ( ( iris_diameter <= 5. ) || ( iris_diameter >= 50. ) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Iris diameter is not within design specifications; diameter=%f.\n", pluginName, __func__, iris_diameter );
		perform_correction = false;
	}	

	double beam_energy = 0.;
	if ( asynSuccess != this->getDoubleParam ( NDPluginEfficiencyCorrectionCurrentBeamEnergy, &beam_energy ) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Couldn't read the current beam energy parameter.\n", pluginName, __func__ );
		perform_correction = false;
	}
	else {

		if ( 0 == current_target_info.light_distribution.compare ( "otr" ) ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: FIXME:Efficiency corrections do not account for beam energy!!!; beam_energy=%f.\n", pluginName, __func__, beam_energy );
		}
		//perform_correction = false;
	}

	if ( false == perform_correction ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Aborting; unable to continue due to previous warnings.\n", pluginName, __func__ );
		return false;
	}

	/* We have a valid NDArray, efficiency correction parameters and target information. Let's ensure that the efficiency correction table matches the current machine state. */
	bool recreate_table = false;
		
	#ifdef USE_OPENCV
	const Size table_size = this->efficiency_correction_table.size();
	#else
	const size_t table_size = this->efficiency_correction_table.size();
	#endif

	/* Validate the correction table */
	#ifdef USE_OPENCV
	if ( (-1 == table_size.width) || (-1 == table_size.height) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Correction table dimensions (%ix%i) invalid.\n", pluginName, __func__, table_size.width, table_size.height );
		recreate_table = true;
	}
	#else
	if ( 0 == table_size ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Efficiency correction table is empty.\n", pluginName, __func__ );
		recreate_table = true;
	}
	#endif	

	/* Confirm that the dimensions of the input array and correction table are identical */
	// for now, we are only using 2-dimensional arrays so the Size() method is sufficient
	#ifdef USE_OPENCV
	if ( (size_t)table_size.width != ndarray_info.xSize || (size_t)table_size.height != ndarray_info.ySize ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Correction table dimensions (%dx%d) do not match image dimensions (%lux%lu).\n", pluginName, table_size.width, table_size.height, ndarray_info.xSize, ndarray_info.ySize );
		recreate_table = true;
	}
	#else
	if ( table_size != (size_t)(ndarray_info.xSize * ndarray_info.ySize) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Efficiency correction table size (%lu) does not match image dimensions (%lux%lu).\n", pluginName, table_size, ndarray_info.xSize, ndarray_info.ySize );
		recreate_table = true;
	}
	#endif

	efficiency_correction_table_parameters_t current_machine_parameters;
	current_machine_parameters.iris_diameter = iris_diameter;
	current_machine_parameters.beam_energy = beam_energy;
	current_machine_parameters.target_material = current_target_info.material;
	current_machine_parameters.target_light_distribution = current_target_info.light_distribution;

	if ( current_machine_parameters != this->efficiency_correction_table_parameters ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Current machine state and efficiency correction table parameters do not match.\n", pluginName, __func__ );
		recreate_table = true;
	}

	if ( true == recreate_table ) {

		const bool valid_table = this->create_efficiency_correction_table ( current_machine_parameters, this->efficiency_correction_table );
		if ( ! valid_table ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: Couldn't create the efficiency correction table.\n", pluginName, __func__ );
			perform_correction = false;
		}
		else {

			this->efficiency_correction_table_parameters = current_machine_parameters;
			asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s: Created an efficiency correction table; geometry: %s, light_distribution: %s., iris_diameter: %f\n", pluginName, __func__, this->get_geometry().c_str(), this->get_target_info( target_number ).light_distribution.c_str(), iris_diameter );
		}
	}

	return perform_correction;
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Corrects for a
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginEfficiencyCorrection::processCallbacks ( NDArray *pArray ) {

	/* This function is called with a lock and is unlocked afterward */

	assert ( NULL != pArray );

	/* Call the base class method */
	NDPluginDriver::processCallbacks ( pArray );

	const bool perform_correction = this->preprocess_check ( pArray );

	NDArray *pArrayOut = NULL;

	if ( perform_correction ) {

		/* Perform the processing with a floating point data type to reduce the accumulation of rounding errors within processing stages */
		if ( NDFloat32 != pArray->dataType ) {

			const int status = this->pNDArrayPool->convert ( pArray, &pArrayOut, NDFloat32 );

			if ( ND_SUCCESS != status ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::processCallbacks: Couldn't convert input array to an output array with data type NDFloat32.\n", pluginName );
			}
			else if ( NULL == pArrayOut ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::processCallbacks: NDArrayPool returned NULL output array upon conversion.\n", pluginName );
			}
		}
		else {

			// Make a copy of the input array, with the data
			pArrayOut = this->pNDArrayPool->copy ( pArray, pArrayOut, true );
		}

		if ( NULL == pArrayOut ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::processCallbacks: Unable to make a copy of the input array.\n", pluginName );
		}
		else {

			NDArrayInfo_t ndarray_info;
			const int status = pArrayOut->getInfo ( &ndarray_info );

			#ifdef USE_OPENCV
			Mat opencv_array ( ndarray_info.ySize, ndarray_info.xSize, CV_32FC1, pArrayOut->pData, sizeof(float) );
			multiply ( opencv_array, this->magnificiation_correction_table, opencv_array );
			#else
			float *pixel = (float*)pArrayOut->pData;
			for ( auto correction_factor = this->efficiency_correction_table.cbegin(); correction_factor != this->efficiency_correction_table.cend(); correction_factor++, pixel++ ) {

				(*pixel) *= *correction_factor;
			}
			#endif
		}
	}
	else {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:processCallbacks: Preprocessing check failed; efficiency corrections will not be applied to the input image.\n", pluginName );
	}

	if ( NULL != pArrayOut ) {

		this->unlock();
		doCallbacksGenericPointer ( pArrayOut, NDArrayData, 0 );
		this->lock();

		/* Release the last array */
		if ( NULL != this->pArrays[0] ) {

			this->pArrays[0]->release ();
		}
		this->pArrays[0] = pArrayOut;
	}
	else {

		//doCallbacksGenericPointer ( pArray, NDArrayData, 0 );
	}

	/* This isn't called by NDPluginDriver::processCallbacks; therefore, it needs to be done manually. */
	callParamCallbacks();
}


/** Counts the number of rays which were emitted by the foil and collected within a specific distance of the brightest pixel on the ccd **/
float NDPluginEfficiencyCorrection::count_collected_rays ( vector <tuple<size_t, size_t, float> > &psfdata, const size_t nearest_max_points, const float nearest_max_radius ) {

	// find the maximum value
	typedef tuple<size_t, size_t, float> tuple_type;
	auto maxpixel = max_element( psfdata.begin(), psfdata.end(),
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		third_tuple_element()
		#else
		[] (tuple_type &a, tuple_type &b) { return get<2>(a) < get<2>(b); }
		#endif
	);
	if ( maxpixel == psfdata.end() ) return 0.;

	const size_t u = get<0>(*maxpixel);
	const size_t v = get<1>(*maxpixel);

	/*cout << "Data: ";
	for_each( psfdata.begin(), psfdata.end(), [] (tuple_type &t) {

		cout << get<0>(t) << " " << get<1>(t) << " " << get<2>(t) << ", ";
	} );
	cout << endl;
	cout << "Max is " << get<0>(*maxpixel) << " " << get<1>(*maxpixel) << " " << get<2>(*maxpixel) << endl;*/

	sort( psfdata.begin(), psfdata.end(),
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		by_closest_to( u, v )
		#else
		[&u, &v] (const tuple_type &a,const tuple_type &b) { return (pow((int)get<0>(a)-(int)u,2)+pow((int)get<1>(a)-(int)v,2) < pow((int)get<0>(b)-(int)u,2)+pow((int)get<1>(b)-(int)v,2) ); }
		#endif
	);

	#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
	#else
	size_t remaining = nearest_max_points;
	#endif

	auto end = find_if( psfdata.begin(), psfdata.end(),
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		within_distance_and_amount_limits ( u, v, nearest_max_radius, nearest_max_points )
		#else
		[&u, &v, &remaining, &nearest_max_radius] (tuple_type &t) {

			if ( remaining == 0 ) return true;
			remaining--;
			const float distance = sqrt(pow((int)get<0>(t)-(int)u,2)+pow((int)get<1>(t)-(int)v,2) );
			//cout << "\t Remaining: " << remaining << ", Distance: " << distance << endl;
			return (distance > nearest_max_radius);
		} 
		#endif
	);

	const float total = accumulate( psfdata.begin(), end, 0.,
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		total_rays()
		#else
		[] (const float sum, const tuple_type &t) {

			return sum +get<2>(t);
		}
		#endif
	);

	/*cout << endl;
	cout << "Data: ";
	for_each( psfdata.begin(), psfdata.end(), [] (tuple_type &t) {

		cout << get<0>(t) << " " << get<1>(t) << " " << get<2>(t) << ", ";
	} );
	cout << endl;
	cout << "\tMax is " << get<0>(*maxpixel) << " " << get<1>(*maxpixel) << " " << get<2>(*maxpixel) << endl;
	cout << "Using these values: " << endl;
	for_each( psfdata.begin(), end, [] (tuple_type &t) {

		cout << get<0>(t) << " " << get<1>(t) << " " << get<2>(t) << ", ";
	} );
	cout << endl;*/

	//cout << "For a total of " << total << endl;

	return total;
}


/** Transfers the efficiency map from the file to the grid_slice structure **/
ViewScreenConfiguredNDPlugin::ConfigurationStatus_t NDPluginEfficiencyCorrection::load_efficiency_map ( string filename, grid_slice &slice, const ViewScreenConfiguredNDPlugin::TargetInfo target_info ) {

	typedef map <string, tuple<float*,bool> > float_parameter_map_type;
	float_parameter_map_type float_parameters;
	float_parameters[string("IrisDiameter")] = make_tuple( &slice.iris_diameter , false );
	float_parameters[string("ROIWidthStride")] = make_tuple( &slice.roi_width_stride , false );
	float_parameters[string("ROIHeightStride")] = make_tuple( &slice.roi_height_stride, false );

	int roi_width_nsamples, roi_height_nsamples;
	typedef map <string, tuple<int*,bool> > integer_parameter_map_type;
	integer_parameter_map_type integer_parameters;
	integer_parameters[string("ROIWidthNSamples")] = make_tuple( &roi_width_nsamples, false );
	integer_parameters[string("ROIHeightNSamples")] = make_tuple( &roi_height_nsamples, false );

	typedef map <string, tuple<string,vector<float>,bool> > floatvector_parameter_map_type;
	floatvector_parameter_map_type floatvector_parameters;
	floatvector_parameters[string("ROIXCoordinates")] = make_tuple( string("ROIXCoordinates"), vector<float>(), false );
	floatvector_parameters[string("ROIYCoordinates")] = make_tuple( string("ROIYCoordinates"), vector<float>(), false );

	ifstream mapfile( filename.c_str() );

	if( mapfile.fail() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: Unable to open efficiency map; filename=%s.\n", pluginName, __func__, filename.c_str() );
		return ConfigurationStatusXMLErrorFileNotFound;
	}
	
	string parameter_name;

	// load the parameters
	while ( mapfile >> parameter_name ) {

		if ( !read_parameter<integer_parameter_map_type>( mapfile, integer_parameters, parameter_name ) )
		if ( !read_parameter<float_parameter_map_type>( mapfile, float_parameters, parameter_name ) )
		if ( !read_vectorparameter<floatvector_parameter_map_type, float>( mapfile, floatvector_parameters, parameter_name ) ) {

			mapfile.ignore( 2048, '\n' );
			if ( parameter_name == "Data" ) {

				break;
			}
		}
	}

	// Ensure that the desired parameters are set
	if ( !check_parameters<integer_parameter_map_type>( integer_parameters ) ) {
		return ConfigurationStatusBadParameter;
	}
	if ( !check_parameters<float_parameter_map_type>( float_parameters ) ) {
		return ConfigurationStatusBadParameter;
	}
	if ( !check_vector_parameters<floatvector_parameter_map_type>( floatvector_parameters ) ) {
		return ConfigurationStatusBadParameter;
	}

	if ( (roi_height_nsamples <= 0) || (roi_width_nsamples <= 0) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:load_efficiency_map: Grid size (%ix%i) in efficiency map %s is invalid.\n", pluginName, roi_width_nsamples, roi_height_nsamples, filename.c_str() );
	}

	// set the xi and yf parameters
	vector<float> &xcoords = get<1>(floatvector_parameters[string("ROIXCoordinates")]);
	if ( xcoords.size() <= 0 ) {

		return ConfigurationStatusBadParameter;
	}
	else {

		slice.roi_xi = *xcoords.begin();
	}

	vector<float> &ycoords = get<1>(floatvector_parameters[string("ROIYCoordinates")]);
	if ( ycoords.size() <= 0 ) {

		return ConfigurationStatusBadParameter;
	}
	else {

		slice.roi_yf = *ycoords.begin();
	}

	// now load the data
	slice.data.clear();
	slice.data.resize( roi_height_nsamples, vector<float>(roi_width_nsamples) );

	//const size_t nearest_max_points = this->get_efficiency_map_pixel_include_count();
	//const float nearest_max_radius = this->get_efficiency_map_pixel_include_radius();	

	for ( size_t yi = 0; yi < (size_t)roi_height_nsamples; yi++ ) {

		for ( size_t xi = 0; xi < (size_t)roi_width_nsamples;  xi++ ) {

			//string line;
			//if ( getline( mapfile, line ).fail() ) {

				//asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:load_efficiency_map(filename=%s): Data format invalid.\n", pluginName, filename.c_str() );
				//return ConfigurationStatusBadParameter;
			//}

			//size_t u, v;
			//float e;	

			mapfile >> slice.data[yi][xi];
			//vector< tuple<size_t, size_t, float> > psfdata;i

			if ( !mapfile ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:load_efficiency_map(filename=%s): Data format invalid.\n", pluginName, filename.c_str() );
				return ConfigurationStatusBadParameter;
			}

			/*if ( slice.data[yi][xi] > 100 ) {

				cout << "Point: " << xi << "," << yi << " or " << xcoords[xi] << ", " << ycoords[yi] << endl;
			}*/
		}
	}

	if ( mapfile.is_open() ) {

		mapfile.close ();
	}
	return ConfigurationStatusConfigured;
}


/** Loads all of the efficiency maps from directory into a vector of grid_slices **/
ViewScreenConfiguredNDPlugin::ConfigurationStatus_t NDPluginEfficiencyCorrection::load_efficiency_maps ( std::vector<grid_slice> &grid, std::string directory, const ViewScreenConfiguredNDPlugin::TargetInfo target_info ) {

	struct dirent **eps = NULL;
	int n;

	#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
	#else
	auto findcform = [] (const struct dirent *d) { 

		string entryname(d->d_name);
		if ( entryname.find("cform") != string::npos ) {

			return 1;
		}
		return 0;
	 };
	#endif

	n = scandir ( directory.c_str(), &eps,
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		select_cform,
		#else
		findcform,
		#endif
		alphasort );

	bool valid = true;
	if (n >= 0) {

		grid.clear();
		grid.resize(n);
		for (int cnt = 0; cnt < n; ++cnt) {

			const string map_file_name = directory + string ( eps[cnt]->d_name );
			const ConfigurationStatus_t load_map_status = load_efficiency_map ( map_file_name, grid[cnt], target_info );

			if ( ConfigurationStatusConfigured != load_map_status ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s(directory=%s): Unable to load efficiency map; map_file_name=%s.\n", pluginName, __func__, directory.c_str(), map_file_name.c_str() );
				break;
			}
		}
	}
	else {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s(directory=%s): Unable to load efficiency map directory.\n", pluginName, __func__, directory.c_str() );
		return ConfigurationStatusBadParameter;
	}

	if ( eps ) {

		free( eps );
	}
	return ConfigurationStatusConfigured;
}


/** Performs a two dimensional bilinear interpolation of the grid_slice data **/
float NDPluginEfficiencyCorrection::interpolate_grid_slice ( const grid_slice &slice, const float x, const float y ) {

	/* bilinear interpolation */
	const float &ys = slice.roi_height_stride;
	const float &yf = slice.roi_yf;
	const int ny = -ceil((y-yf)/ys);
	const float yd = yf - ny*ys - y;	// yd = y2 - y

	//cout << "\t ys: " << ys << ",\tyf: " << yf << ",\tny: " << ny << ",\tyd: " << yd << ",\ty2: " << yf - ny*ys << ",\ty1: " << yf - (ny+1)*ys << endl;

	if ( ny < 0 || ny >= (int)(slice.data.size()-1) ) return 0.;

	const float &xs = slice.roi_width_stride;
	const float &xi = slice.roi_xi;
	const int nx = floor((x-xi)/xs);
	const float xd = x - xi - nx*xs;	// xd = x - x1

	//cout << "\t xs: " << xs << ",\txi: " << xi << ",\tnx: " << nx << ",\txd: " << xd << ",\tx1: " << xi + nx*xs << ",\tx2: " << xi + (nx+1)*xs << endl;

	if ( nx < 0 || nx >= (int)(slice.data[ny].size()-1) ) return 0.;

	const float fR1 = ((xs-xd)*slice.data[ny+1][nx] + xd*slice.data[ny+1][nx+1])/xs;
	const float fR2 = ((xs-xd)*slice.data[ny][nx] + xd*slice.data[ny][nx+1])/xs;
	const float fP = (yd*fR1 + (ys-yd)*fR2)/ys;

	//cout << "\tfQ11: " << slice.data[ny+1][nx] << ",\tfQ21: " << slice.data[ny+1][nx+1]
	//	<< ",\tfQ12: " << slice.data[ny][nx]   << ",\tfQ22: " << slice.data[ny][nx+1] << endl;
	//cout << "\tfR1: " << fR1 << ",\tfR2: " << fR2 << endl;
	return fP;
}


#ifdef USE_OPENCV
/** Creates the efficiency table using grid data and view screen calibration
 */
bool NDPluginEfficiencyCorrection::create_efficiency_correction_table ( const vector<grid_slice> &grid, const float iris_diameter, Mat &table ) {

	table.create ( ny, nx, CV_32FC1 );

	// pick the two grid slices which bracket the given iris diameter
	auto lowerbound = grid.rbegin();
	while ( lowerbound != grid.rend() ) {

		if (lowerbound->iris_diameter <= iris_diameter ) { break; }
		lowerbound++;
	}
	auto upperbound = grid.begin();
	while ( upperbound != grid.end() ) {

		if (upperbound->iris_diameter >= iris_diameter ) { break; }
		upperbound++;
	}

	if ( upperbound == grid.end() ) {

		cout << "No upper bound found" << endl;
		return false;
	}
	if ( lowerbound == grid.rend() ) {

		cout << "No lower bound found" << endl;
		return false;
	}

	const float x0 = lowerbound->iris_diameter;
	const float x1 = upperbound->iris_diameter;
	const float s = (x0 == x1)? 1.0: (iris_diameter - x0) / (x1 - x0);

	for ( size_t v = 0; v < ny; v++ ) {

		const float y = yf - (v +0.5)*dy;
		for ( size_t u = 0; u < nx; u++ ) {

			const float x = xi + (u+0.5)*dx;

			const float y0 = interpolate_slice( *lowerbound, x, y );
			const float y1 = interpolate_slice( *upperbound, x, y );
			const float efficiency = y0 + (y1 - y0)*s;

			const float e0 = interpolate_slice( *lowerbound, 0., 0. );
			const float e1 = interpolate_slice( *upperbound, 0., 0. );
			const float norm = e0 + (e1 - e0)*s;

			if ( efficiency == 0. ) {

				table.at<float>(v, u) = 0.;
			}
			else {

				table.at<float>(v, u) = norm / efficiency;
			}
		}
	}

	return true;
}
#else
/** Creates the efficiency table using grid data and view screen calibration
 */
bool NDPluginEfficiencyCorrection::create_efficiency_correction_table ( const efficiency_correction_table_parameters_t parameters, std::vector<float> &table ) {

	/* Ensure that we have a grid of calibration points which match the target information. */
	auto grid_iterator = this->efficiency_grids.find ( TargetInfo( parameters.target_material, parameters.target_light_distribution ) );
	if ( efficiency_grids.end() == grid_iterator ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Aborting; efficiency data unavailable for material=%s, light=%s.\n", pluginName, __func__, parameters.target_material.c_str(), parameters.target_light_distribution.c_str() );
		return false;
	}

	const std::vector<grid_slice> &grid = grid_iterator->second;

	table.clear ();
	table.resize ( this->get_output_image_width() * this->get_output_image_height() );

	// pick the two grid slices which bracket the given iris diameter
	auto lowerbound = grid.rbegin();
	while ( lowerbound != grid.rend() ) {

		if (lowerbound->iris_diameter <= parameters.iris_diameter ) { break; }
		lowerbound++;
	}
	auto upperbound = grid.begin();
	while ( upperbound != grid.end() ) {

		if (upperbound->iris_diameter >= parameters.iris_diameter ) { break; }
		upperbound++;
	}

	if ( upperbound == grid.end() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s(irisdiameter=%f): Iris diameter is outside the range of interpolation.\n", pluginName, __func__, parameters.iris_diameter );
		return false;
	}
	if ( lowerbound == grid.rend() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s(irisdiameter=%f): Iris diameter is outside the range of interpolation.\n", pluginName, __func__, parameters.iris_diameter );
		return false;
	}

	/* Interpolate the iris diameter */
	const float d0 = lowerbound->iris_diameter;
	const float d1 = upperbound->iris_diameter;
	const float s = (d0 == d1)? 1.0: (parameters.iris_diameter - d0) / (d1 - d0);

	const size_t oimage_width = this->get_output_image_width();
	const size_t oimage_height = this->get_output_image_height();

	for ( size_t v = 0; v < oimage_height; v++ ) {

		for ( size_t u = 0; u < oimage_width; u++ ) {

			double x, y;
			this->oimage_to_beamspace ( u, v, x, y );

			const float y0 = interpolate_grid_slice( *lowerbound, x, y );
			const float y1 = interpolate_grid_slice( *upperbound, x, y );
			const float efficiency = y0 + (y1 - y0)*s;

			const float e0 = interpolate_grid_slice( *lowerbound, 0., 0. );
			const float e1 = interpolate_grid_slice( *upperbound, 0., 0. );
			const float norm = e0 + (e1 - e0)*s;

			#ifdef USE_OPENCV
			table.at<float>(v, u) = (efficiency == 0.)? 0.: norm / efficiency;
			#else
			table[v*oimage_width + u] = (efficiency == 0.)? 0.: norm / efficiency;
			#endif
		}
	}

	return true;
}
#endif


/** These are the helper functions for reading and loading the map **/
template <typename T>
bool NDPluginEfficiencyCorrection::read_parameter( ifstream &stream, T &mapread, string &parameter_name ) {

	if ( mapread.count(parameter_name) == 1) {

		stream >> (*get<0>(mapread[parameter_name]));
		get<1>(mapread[parameter_name]) = true;
		return true;
	}
	return false;
}

template <typename T, typename T_param>
bool NDPluginEfficiencyCorrection::read_vectorparameter( ifstream &stream, T &mapread, string &parameter_name ) {

	if ( mapread.count(parameter_name) ) {

		get<2>(mapread[parameter_name]) = true;
		auto &vec = get<1>(mapread[parameter_name]);
		T_param value;
		while ( stream >> value ) {

			vec.push_back(value);
		}
		stream.clear();
		return true;
	}
	return false;
}

template <typename T>
bool NDPluginEfficiencyCorrection::check_parameters( T &mapcheck ) {

	bool valid = true;
	for ( auto parameter = mapcheck.begin(); parameter != mapcheck.end(); parameter++ ) {

		if ( get<1>(parameter->second) == false ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:check_vector_parameters: %s is undefined.\n", pluginName, (parameter->first).c_str() );
			valid = false;
		}
		/*else {

			cout << *get<0>(parameter->second);
		}
		cout << endl;*/
	}
	return valid;
}

template <typename T>
bool NDPluginEfficiencyCorrection::check_vector_parameters( T &mapcheck ) {

	bool valid = true;
	for ( auto parameter = mapcheck.begin(); parameter != mapcheck.end(); parameter++ ) {

		if ( get<2>(parameter->second) != true ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:check_vector_parameters: %s is undefined.\n", pluginName, get<0>(parameter->second).c_str() );
			valid = false;
		}
		/*else {

			auto vec = get<1>(parameter->second);
			for ( auto el = vec.begin(); el != vec.end(); el++ ) {

				cout << *el;
				if ( (el+1) != vec.end() ) {

					cout << ", ";
				}
				else {

					cout << endl;
				}
			}
		}*/
	}
	return valid;
}
