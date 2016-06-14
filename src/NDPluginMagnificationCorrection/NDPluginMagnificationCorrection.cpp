//#include <iostream>
#include <string>
#include <iterator>
//#include <vector>
//#include <array>
//#include <map>
//#include <tuple>
//#include <sstream>
//#include <algorithm>

#include "NDPluginMagnificationCorrection.h"

using namespace std;

static const char* pluginName = "NDPluginMagnificationCorrection";

NDPluginMagnificationCorrection::NDPluginMagnificationCorrection ( const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory, int priority, int stackSize ):
	ViewScreenConfiguredNDPlugin (
		portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 1,
		NUM_NDPluginMagnificationCorrection_PARAMS, maxBuffers, maxMemory,
		asynInt32ArrayMask | asynGenericPointerMask,
		asynInt32ArrayMask | asynGenericPointerMask, ASYN_CANBLOCK, 1, priority, stackSize ) {


	/* Create an empty magnification correction table */
	#ifdef USE_OPENCV
	this->magnification_correction_table.create ( 0, 0, cv::CV_32FC1 );
	#else
	this->magnification_correction_table.clear ( );
	#endif

	/* Try to connect to the NDArray port */
	this->connectToArrayPort();
}


ViewScreenConfiguredNDPlugin::ConfigurationStatus_t NDPluginMagnificationCorrection::configuration_change_callback ( ) {

	/* This function should be called from a locked state */

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "NDPluginMagnificationCorrection::Calibrate: Beginning calibration.\n" );

	const size_t image_width = this->get_output_image_width();
	const size_t image_height = this->get_output_image_height();

	if ( 0 == image_width || 0 == image_height ) {

		return ConfigurationStatusBadParameter;
	}


	#ifdef USE_OPENCV
	this->magnification_correction_table.create ( image_height, image_width, CV_32FC1 );
	#else
	this->magnification_correction_table.resize ( image_height * image_width );
	#endif

	const float normalization_area = this->ccd_area_covered_by_output_pixel ( (image_width-1) / 2., (image_height-1) / 2. );

	for ( size_t v = 0; v < image_height; v++ ) {

		for ( size_t u = 0; u < image_width; u++ ) {

			const double area = this->ccd_area_covered_by_output_pixel ( u, v );

			#ifdef USE_OPENCV
			this->magnification_correction_table.at<float>( v, u ) = (float)(area / normalization_area);
			#else
			this->magnification_correction_table.at ( v*image_width + u ) = (float)(area / normalization_area);
			#endif
		}
	}

	return ConfigurationStatusConfigured;
}


/** Report the compatibility of the input array and correction table
  * \param[in] pArray  Pointer to the NDArray to check
  * @return true if the correction may be applied; false otherwise.
  */
bool NDPluginMagnificationCorrection::preprocess_check ( NDArray *pArray ) {

	/* This function should be called while locked */

	/* Check the configuration status */
	if ( false == this->is_configured() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "NDPluginMagnificationCorrection::preprocess_check: View screen configuration not loaded.\n" );
		// The plugin is uncalibrated; there is no point in performing further checks.
		return false;
	}

	/* Let's learn a little bit about this array */
	NDArrayInfo_t ndarray_info;
	pArray->getInfo ( &ndarray_info );

	bool perform_correction = true;
	#ifdef USE_OPENCV
	const Size table_size = this->magnification_correction_table.size();
	#else
	const size_t table_size = this->magnification_correction_table.size();
	#endif

	/* Validate the correction table */
	#ifdef USE_OPENCV
	if ( (-1 == table_size.width) || (-1 == table_size.height) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Correction table dimensions (%ix%i) invalid.\n", pluginName, table_size.width, table_size.height );
		perform_correction = false;
	}
	#else
	if ( 0 == table_size ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Magnification correction table is empty.\n", pluginName );
		perform_correction = false;
	}
	#endif

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

	/* Confirm that the dimensions of the input array and correction table are identical */
	// for now, we are only using 2-dimensional arrays so the Size() method is sufficient
	#ifdef USE_OPENCV
	if ( (size_t)table_size.width != ndarray_info.xSize || (size_t)table_size.height != ndarray_info.ySize ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Correction table dimensions (%dx%d) do not match image dimensions (%lux%lu).\n", pluginName, table_size.width, table_size.height, ndarray_info.xSize, ndarray_info.ySize );
		perform_correction = false;
	}
	#else
	if ( table_size != (size_t)(ndarray_info.xSize * ndarray_info.ySize) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Magnification correction table size (%lu) does not match image dimensions (%lux%lu).\n", pluginName, table_size, ndarray_info.xSize, ndarray_info.ySize );
		perform_correction = false;
	}
	#endif

	return perform_correction;
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Corrects for a
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginMagnificationCorrection::processCallbacks ( NDArray *pArray ) {

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
			opencv_array = opencv_array.mul ( this->magnification_correction_table );
			#else
			float *pixel = (float*)pArrayOut->pData;
			for ( auto correction_factor = this->magnification_correction_table.cbegin(); correction_factor != this->magnification_correction_table.cend(); correction_factor++, pixel++ ) {

				(*pixel) *= *correction_factor;
			}
			#endif
		}
	}
	else {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "NDPluginMagnificationCorrection::processCallbacks: Preprocessing check failed; magnification corrections will not be applied to the input image.\n" );
	}

	if ( NULL != pArrayOut ) {

		/* Release the last array */
		if ( NULL != this->pArrays[0] ) {

			this->pArrays[0]->release ();
		}
		this->pArrays[0] = pArrayOut;
		this->unlock();
		doCallbacksGenericPointer ( pArrayOut, NDArrayData, 0 );
		this->lock();
	}
	else {

		//doCallbacksGenericPointer ( pArray, NDArrayData, 0 );
	}

	/* This isn't called by NDPluginDriver::processCallbacks; therefore, it needs to be done manually. */
	callParamCallbacks();
}
