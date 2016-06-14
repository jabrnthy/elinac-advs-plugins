#include <string>
#include <cmath>

// FOR TESTING
#include <iostream>
#include <fstream>

#include "NDPluginBeamStats.h"

using namespace std;

static const char* pluginName = "NDPluginBeamStats";

NDPluginBeamStats::NDPluginBeamStats ( const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory, int priority, int stackSize ):
	ViewScreenConfiguredNDPlugin (
		portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 1,
		NUM_NDPluginBeamStats_PARAMS, maxBuffers, maxMemory,
		asynGenericPointerMask,
		asynGenericPointerMask,
		ASYN_CANBLOCK, 1, priority, stackSize ) {

	createParam ( NDPluginBeamStatsM00String, asynParamFloat64, &NDPluginBeamStatsM00 );
	createParam ( NDPluginBeamStatsM10String, asynParamFloat64, &NDPluginBeamStatsM10 );
	createParam ( NDPluginBeamStatsM01String, asynParamFloat64, &NDPluginBeamStatsM01 );
	createParam ( NDPluginBeamStatsM20String, asynParamFloat64, &NDPluginBeamStatsM20 );
	createParam ( NDPluginBeamStatsM11String, asynParamFloat64, &NDPluginBeamStatsM11 );
	createParam ( NDPluginBeamStatsM02String, asynParamFloat64, &NDPluginBeamStatsM02 );
	createParam ( NDPluginBeamStatsU00String, asynParamFloat64, &NDPluginBeamStatsU00 );
	createParam ( NDPluginBeamStatsU10String, asynParamFloat64, &NDPluginBeamStatsU10 );
	createParam ( NDPluginBeamStatsU01String, asynParamFloat64, &NDPluginBeamStatsU01 );
	createParam ( NDPluginBeamStatsU20String, asynParamFloat64, &NDPluginBeamStatsU20 );
	createParam ( NDPluginBeamStatsU11String, asynParamFloat64, &NDPluginBeamStatsU11 );
	createParam ( NDPluginBeamStatsU02String, asynParamFloat64, &NDPluginBeamStatsU02 );
	createParam ( NDPluginBeamStatsBeamCentroidXString, asynParamFloat64, &NDPluginBeamStatsBeamCentroidX );
	createParam ( NDPluginBeamStatsBeamCentroidYString, asynParamFloat64, &NDPluginBeamStatsBeamCentroidY );
	createParam ( NDPluginBeamStatsBeamStDevXString, asynParamFloat64, &NDPluginBeamStatsBeamStDevX );
	createParam ( NDPluginBeamStatsBeamStDevYString, asynParamFloat64, &NDPluginBeamStatsBeamStDevY );
	createParam ( NDPluginBeamStatsBeamCorrelationString, asynParamFloat64, &NDPluginBeamStatsBeamCorrelation );

 	/* Set the plugin type string */
	setStringParam ( NDPluginDriverPluginType, "NDPluginBeamStats");

	this->callParamCallbacks();

	/* Try to connect to the NDArray port */
	this->connectToArrayPort();
}


template <typename dataType, typename accumulatorType>
void NDPluginBeamStats::calculate_beam_statistics ( NDArray *pArray ) {

	/* get the pre-processing information */
	float background_threshold;
	float beam_sigma_cutoff;

	const dataType *pData = (dataType*)pArray->pData;

	NDArrayInfo_t array_info;
	pArray->getInfo ( &array_info );

	const size_t array_width = array_info.xSize;
	const size_t array_height = array_info.ySize;

	// FOR TESTING
	/* A single loop is used because gcc isn't very good at vectorizing inner loops */
	/*ofstream fout ( "/home/vsadmin/test.dat" );
	if ( fout.is_open() ) {
		for ( size_t px = 0; px < array_height*array_width; px++ ) {

			fout << pData[px];
			if ( px != (array_height*array_width-1) ) fout << ", ";
		}
		fout.close();
	}
	else {
		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:COULDN'T OPEN FILE.\n", pluginName );
	}*/

	/* Calculate the raw moments in image coordinates */
	accumulatorType m00, m10, m01, m11, m20, m02;
	m00 = 0;
	m10 = 0;
	m01 = 0;
	m11 = 0;
	m20 = 0;
	m02 = 0;

	/* A single loop is used because gcc isn't very good at vectorizing inner loops */
	for ( size_t px = 0; px < array_height*array_width; px++ ) {

		const size_t v = px / array_width;
		const size_t u = px % array_width;

		accumulatorType su = pData[px];
		accumulatorType sv = su;

		m00 += su;
		su *= u;
		sv *= v;
		m10 += su;
		m01 += sv;
		m20 += su*u;
		m11 += sv*u;
		m02 += sv*v;
	}

	/*cout << "m00: " << m00 << endl;
	cout << "m10: " << m10 << endl;
	cout << "m01: " << m01 << endl;
	cout << "m20: " << m20 << endl;
	cout << "m11: " << m11 << endl;
	cout << "m02: " << m02 << endl;*/

	/* Scale the raw image moments into beamspace */
	// x = a*u+b; y = c*v+d
	const double a = ( this->get_xf() - this->get_xi() ) / this->get_output_image_width();
	const double b = 0.5*a + this->get_xi();
	const double c = -1.*( this->get_yf() - this->get_yi() ) / this->get_output_image_height();
	const double d = 0.5*c + this->get_yf();

	double M00, M10, M01, M11, M20, M02;
	M00 = m00;
	M10 = a*m10 + b*m00;
	M01 = c*m01 + d*m00;
	M20 = a*(a*m20 + b*m10) + b*M10;
	M11 = a*(c*m11 + d*m10) + b*(c*m01 + d*m00);
	M02 = c*(c*m02 + d*m01) + d*M01;

	/* Calculate the central moments in beamspace */
	double U00, U10, U01, U20, U11, U02;
	U00 = m00;
	U01 = 0.;
	U10 = 0.;
	U11 = M11 - M01*M10/M00;
	U20 = M20 - M10*M10/M00;
	U02 = M02 - M01*M01/M00;

	
	/*cout << "a: " << a << ", b: " << b << endl;
	cout << "c: " << c << ", d: " << d << endl;
	cout << "M00: " << M00 << endl;
	cout << "M10: " << M10 << endl;
	cout << "M01: " << M01 << endl;
	cout << "M20: " << M20 << endl;
	cout << "M11: " << M11 << endl;
	cout << "M02: " << M02 << endl;

	cout << "U00: " << U00 << endl;
	cout << "U10: " << U10 << endl;
	cout << "U01: " << U01 << endl;
	cout << "U20: " << U20 << endl;
	cout << "U11: " << U11 << endl;
	cout << "U02: " << U02 << endl;*/

	/* Calculate the derived statistics */
	double centroidx, centroidy, correlation;
	centroidx = M10/M00;
	centroidy = M01/M00;

	double stdevx, stdevy;
	stdevx = sqrt ( U20 / M00 );
	stdevy = sqrt ( U02 / M00 );

	correlation = U11 / sqrt ( U20 * U02 );

	/* Normalize the moments */
	M10 /= M00;
	M01 /= M00;
	M20 /= M00;
	M11 /= M00;
	M02 /= M00;

	U10 /= U00;
	U01 /= U00;
	U20 /= U00;
	U11 /= U00;
	U02 /= U00;

	this->setDoubleParam ( NDPluginBeamStatsM00, M00 );
	this->setDoubleParam ( NDPluginBeamStatsM10, M10 );
	this->setDoubleParam ( NDPluginBeamStatsM01, M01 );
	this->setDoubleParam ( NDPluginBeamStatsM20, M20 );
	this->setDoubleParam ( NDPluginBeamStatsM11, M11 );
	this->setDoubleParam ( NDPluginBeamStatsM02, M02 );
	this->setDoubleParam ( NDPluginBeamStatsU00, U00 );
	this->setDoubleParam ( NDPluginBeamStatsU10, U10 );
	this->setDoubleParam ( NDPluginBeamStatsU01, U01 );
	this->setDoubleParam ( NDPluginBeamStatsU20, U20 );
	this->setDoubleParam ( NDPluginBeamStatsU11, U11 );
	this->setDoubleParam ( NDPluginBeamStatsU02, U02 );
	this->setDoubleParam ( NDPluginBeamStatsBeamCentroidX, centroidx );
	this->setDoubleParam ( NDPluginBeamStatsBeamCentroidY, centroidy );
	this->setDoubleParam ( NDPluginBeamStatsBeamStDevX, stdevx );
	this->setDoubleParam ( NDPluginBeamStatsBeamStDevY, stdevy );
	this->setDoubleParam ( NDPluginBeamStatsBeamCorrelation, correlation );
}


ViewScreenConfiguredNDPlugin::ConfigurationStatus_t NDPluginBeamStats::configuration_change_callback ( ) {

	/* This function should be called from a locked state */

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "NDPluginBeamStats::Calibrate: Beginning calibration.\n" );

	const size_t image_width = this->get_output_image_width();
	const size_t image_height = this->get_output_image_height();

	if ( 0 == image_width || 0 == image_height ) {

		return ConfigurationStatusBadParameter;
	}

	return ConfigurationStatusConfigured;
}


/** Report the compatibility of the input array and correction table
  * \param[in] pArray  Pointer to the NDArray to check
  * @return true if the correction may be applied; false otherwise.
  */
bool NDPluginBeamStats::preprocess_check ( NDArray *pArray ) {

	/* This function should be called while locked */

	/* Check the configuration status */
	if ( false == this->is_configured() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "NDPluginBeamStats::preprocess_check: View screen configuration not loaded.\n" );
		// The plugin is uncalibrated; there is no point in performing further checks.
		return false;
	}

	/* Let's learn a little bit about this array */
	NDArrayInfo_t ndarray_info;
	pArray->getInfo ( &ndarray_info );

	bool perform_correction = true;

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

	/* Confirm that the dimensions of the array match the ones provided by the configuration file */
	if ( (ndarray_info.xSize != this->get_output_image_width()) || (ndarray_info.ySize != this->get_output_image_height()) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Input image dimensions (%lu,%lu) do not match configuration input image dimensions (%lux%lu).\n", pluginName, ndarray_info.xSize, ndarray_info.ySize, this->get_input_image_width(), this->get_input_image_height() );
		perform_correction = false;
	}

	return perform_correction;
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Corrects for a
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginBeamStats::processCallbacks ( NDArray *pArray ) {

	/* This function is called with a lock and is unlocked afterward */

	assert ( NULL != pArray );

	/* Call the base class method */
	NDPluginDriver::processCallbacks ( pArray );

	NDArray *pArrayOut = NULL;

	const bool calculate_statistics = this->preprocess_check ( pArray );

	if ( calculate_statistics ) {

		// Make a copy of the input array, with the data
		pArrayOut = this->pNDArrayPool->copy ( pArray, pArrayOut, true );

		if ( NULL != pArrayOut ) {

			switch ( pArray->dataType ) {
				case NDInt8:
					this->calculate_beam_statistics<epicsInt8, int>( pArrayOut );
					break;
				case NDUInt8:
					this->calculate_beam_statistics<epicsUInt8, unsigned int>( pArrayOut );
					break;
				case NDInt16:
					this->calculate_beam_statistics<epicsInt16, long long>( pArrayOut );
					break;
				case NDUInt16:
					this->calculate_beam_statistics<epicsUInt16, unsigned long long>( pArrayOut );
					break;
				case NDInt32:
					this->calculate_beam_statistics<epicsInt32, long long>( pArrayOut );
					break;
				case NDUInt32:
					this->calculate_beam_statistics<epicsUInt32, unsigned long long>( pArrayOut );
					break;
				case NDFloat32:
					this->calculate_beam_statistics<epicsFloat32, double>( pArrayOut );
					break;
			    case NDFloat64:
					this->calculate_beam_statistics<epicsFloat64, double>( pArrayOut );
					break;
				default:
					asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: ERROR: unknown data type=%d\n", pluginName, "processCallbacks", pArrayOut->dataType);
			}
		}
		else {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:processCallbacks: Unable to allocate output array; processing terminated.\n", pluginName );
		}
	}
	else {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:processCallbacks: Preprocessing check failed; processing terminated.\n", pluginName );
	}

	if ( NULL != pArrayOut ) {

		// Add the configuration parameters to the NDArray
		this->getAttributes( pArrayOut->pAttributeList );

		this->unlock();
		doCallbacksGenericPointer ( pArrayOut, NDArrayData, 0 );
		this->lock();

		/* Release the last array */
		if ( this->pArrays[0] ) {

			this->pArrays[0]->release ();
			this->pArrays[0] = NULL;
		}
		this->pArrays[0] = pArrayOut;

	}
	else {

		// don't do anything if we can't perform a valid conversion
		// doCallbacksGenericPointer ( pArray, NDArrayData, 0 );
	}

	/* This isn't called by NDPluginDriver::processCallbacks; therefore, it needs to be done manually. */
	callParamCallbacks();
}
