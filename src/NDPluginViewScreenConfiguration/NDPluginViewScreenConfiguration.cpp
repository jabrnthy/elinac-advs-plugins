#include <string>
#include <iterator>
#include <sys/stat.h>

#include "NDPluginViewScreenConfiguration.h"

using namespace std;

static const char* pluginName = "NDPluginViewScreenConfiguration";

NDPluginViewScreenConfiguration::NDPluginViewScreenConfiguration ( const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory, int priority, int stackSize ):
	ViewScreenConfiguredNDPlugin (
		portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 1,
		NUM_NDPluginViewScreenConfiguration_PARAMS, maxBuffers, maxMemory,
		asynGenericPointerMask,
		asynGenericPointerMask, 0, 1, priority, stackSize ) {


	createParam ( NDPluginViewScreenConfigurationTarget0MaterialString, asynParamOctet, &NDPluginViewScreenConfigurationTarget0Material );
	createParam ( NDPluginViewScreenConfigurationTarget1MaterialString, asynParamOctet, &NDPluginViewScreenConfigurationTarget1Material );
	createParam ( NDPluginViewScreenConfigurationTarget2MaterialString, asynParamOctet, &NDPluginViewScreenConfigurationTarget2Material );

	createParam ( NDPluginViewScreenConfigurationTarget0LightDistributionString, asynParamOctet, &NDPluginViewScreenConfigurationTarget0LightDistribution );
	createParam ( NDPluginViewScreenConfigurationTarget1LightDistributionString, asynParamOctet, &NDPluginViewScreenConfigurationTarget1LightDistribution );
	createParam ( NDPluginViewScreenConfigurationTarget2LightDistributionString, asynParamOctet, &NDPluginViewScreenConfigurationTarget2LightDistribution );

	createParam ( NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryString, asynParamOctet, &NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectory );
	createParam ( NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryString, asynParamOctet, &NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectory );
	createParam ( NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryString, asynParamOctet, &NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectory );
	createParam ( NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryExistsString, asynParamInt32, &NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryExists );
	createParam ( NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryExistsString, asynParamInt32, &NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryExists );
	createParam ( NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryExistsString, asynParamInt32, &NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryExists );

	createParam ( NDPluginViewScreenConfigurationConfigurationFileDirectoryString, asynParamOctet, &NDPluginViewScreenConfigurationConfigurationFileDirectory );

	createParam ( NDPluginViewScreenConfigurationBeamspaceStartXString, asynParamFloat64, &NDPluginViewScreenConfigurationBeamspaceStartX );
	createParam ( NDPluginViewScreenConfigurationBeamspaceEndXString, asynParamFloat64, &NDPluginViewScreenConfigurationBeamspaceEndX );
	createParam ( NDPluginViewScreenConfigurationBeamspaceStartYString, asynParamFloat64, &NDPluginViewScreenConfigurationBeamspaceStartY );
	createParam ( NDPluginViewScreenConfigurationBeamspaceEndYString, asynParamFloat64, &NDPluginViewScreenConfigurationBeamspaceEndY );

	createParam ( NDPluginViewScreenConfigurationInputImageWidthString, asynParamInt32, &NDPluginViewScreenConfigurationInputImageWidth );
	createParam ( NDPluginViewScreenConfigurationInputImageHeightString, asynParamInt32, &NDPluginViewScreenConfigurationInputImageHeight );
	createParam ( NDPluginViewScreenConfigurationOutputImageWidthString, asynParamInt32, &NDPluginViewScreenConfigurationOutputImageWidth );
	createParam ( NDPluginViewScreenConfigurationOutputImageHeightString, asynParamInt32, &NDPluginViewScreenConfigurationOutputImageHeight );

	/* Put these in the constructor for now because they are hard-coded into the ViewScreenConfiguredNDPlugin class */
	setStringParam ( NDPluginViewScreenConfigurationTarget0Material, "" );
	setStringParam ( NDPluginViewScreenConfigurationTarget1Material, "" );
	setStringParam ( NDPluginViewScreenConfigurationTarget2Material, "" );

	setStringParam ( NDPluginViewScreenConfigurationTarget0LightDistribution, "" );
	setStringParam ( NDPluginViewScreenConfigurationTarget1LightDistribution, "" );
	setStringParam ( NDPluginViewScreenConfigurationTarget2LightDistribution, "" );

	setStringParam ( NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectory, "" );
	setStringParam ( NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectory, "" );
	setStringParam ( NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectory, "" );
	setIntegerParam ( NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryExists, false );
	setIntegerParam ( NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryExists, false );
	setIntegerParam ( NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryExists, false );
	setStringParam ( NDPluginViewScreenConfigurationConfigurationFileDirectory, this->get_configuration_directory().c_str() );

	/* Try to connect to the NDArray port */
	this->connectToArrayPort();
}


ViewScreenConfiguredNDPlugin::ConfigurationStatus_t NDPluginViewScreenConfiguration::configuration_change_callback ( ) {

	/* This function should be called from a locked state */

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "%s:configuration_change_callback: Beginning calibration.\n", pluginName );

	setStringParam ( NDPluginViewScreenConfigurationTarget0Material, this->get_target_info(0).material.c_str() );
	setStringParam ( NDPluginViewScreenConfigurationTarget1Material, this->get_target_info(1).material.c_str() );
	setStringParam ( NDPluginViewScreenConfigurationTarget2Material, this->get_target_info(2).material.c_str() );

	setStringParam ( NDPluginViewScreenConfigurationTarget0LightDistribution, this->get_target_info(0).light_distribution.c_str() );
	setStringParam ( NDPluginViewScreenConfigurationTarget1LightDistribution, this->get_target_info(1).light_distribution.c_str() );
	setStringParam ( NDPluginViewScreenConfigurationTarget2LightDistribution, this->get_target_info(2).light_distribution.c_str() );

	setStringParam ( NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectory, this->get_efficiency_map_directory(0).c_str() );
	setStringParam ( NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectory, this->get_efficiency_map_directory(1).c_str() );
	setStringParam ( NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectory, this->get_efficiency_map_directory(2).c_str() );

	// check whether the directories exist
	struct stat buffer;

	if ( (0 == stat ( this->get_efficiency_map_directory(0).c_str(), &buffer )) && (S_ISDIR ( buffer.st_mode )) ) {

		setIntegerParam ( NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryExists, true );
	}
	else {

		setIntegerParam ( NDPluginViewScreenConfigurationTarget0EfficiencyMapDirectoryExists, false );
	}
	if ( (0 == stat ( this->get_efficiency_map_directory(1).c_str(), &buffer )) && (S_ISDIR ( buffer.st_mode )) ) {

		setIntegerParam ( NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryExists, true );
	}
	else {

		setIntegerParam ( NDPluginViewScreenConfigurationTarget1EfficiencyMapDirectoryExists, false );
	}
	if ( (0 == stat ( this->get_efficiency_map_directory(2).c_str(), &buffer )) && (S_ISDIR ( buffer.st_mode )) ) {

		setIntegerParam ( NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryExists, true );
	}
	else {

		setIntegerParam ( NDPluginViewScreenConfigurationTarget2EfficiencyMapDirectoryExists, false );
	}

	setStringParam ( NDPluginViewScreenConfigurationConfigurationFileDirectory, this->get_configuration_directory().c_str() );

	setDoubleParam ( NDPluginViewScreenConfigurationBeamspaceStartX, this->get_xi() );
	setDoubleParam ( NDPluginViewScreenConfigurationBeamspaceEndX, this->get_xf() );
	setDoubleParam ( NDPluginViewScreenConfigurationBeamspaceStartY, this->get_yi() );
	setDoubleParam ( NDPluginViewScreenConfigurationBeamspaceEndY, this->get_yf() );

	setIntegerParam ( NDPluginViewScreenConfigurationInputImageWidth, this->get_input_image_width() );
	setIntegerParam ( NDPluginViewScreenConfigurationInputImageHeight, this->get_input_image_height() );
	setIntegerParam ( NDPluginViewScreenConfigurationOutputImageWidth, this->get_output_image_width() );
	setIntegerParam ( NDPluginViewScreenConfigurationOutputImageHeight, this->get_output_image_height() );

	this->callParamCallbacks ();

	return ConfigurationStatusConfigured;
}


/** Report the compatibility of the input array and correction table
  * \param[in] pArray  Pointer to the NDArray to check
  * @return true if the correction may be applied; false otherwise.
  */
bool NDPluginViewScreenConfiguration::preprocess_check ( NDArray *pArray ) {

	/* This function should be called while locked */

	/* Check the configuration status */
	if ( false == this->is_configured() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:preprocess_check: View screen configuration not loaded.\n", pluginName );
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

	/* Confirm that the dimensions of the input array and correction table are identical */
	// for now, we are only using 2-dimensional arrays so the Size() method is sufficient
	if ( this->get_input_image_width() != ndarray_info.xSize || this->get_input_image_height() != ndarray_info.ySize ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Correction table dimensions (%lux%lu) do not match image dimensions (%lux%lu).\n", pluginName, this->get_input_image_width(), this->get_input_image_height(), ndarray_info.xSize, ndarray_info.ySize );
		perform_correction = false;
	}

	return perform_correction;
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Corrects for a
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginViewScreenConfiguration::processCallbacks ( NDArray *pArray ) {

	/* This function is called with a lock and is unlocked afterward */

	assert ( NULL != pArray );

	/* Call the base class method */
	NDPluginDriver::processCallbacks ( pArray );

	const bool perform_correction = this->preprocess_check ( pArray );

	NDArray *pArrayOut = NULL;

	if ( perform_correction ) {

		// Make a copy of the input array, with the data
		pArrayOut = this->pNDArrayPool->copy ( pArray, pArrayOut, true );

		if ( NULL == pArrayOut ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:processCallbacks: Unable to make a copy of the input array.\n", pluginName );
		}
		else {

			// Add the configuration parameters to the NDArray
			this->getAttributes( pArrayOut->pAttributeList );
		}
	}
	else {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:processCallbacks: Preprocessing check failed; configuration parameters will not be added to this array.\n", pluginName );
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
