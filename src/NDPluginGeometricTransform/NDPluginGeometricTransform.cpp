/*
 * NDPluginGeometricTransform.cpp
 *
 * Geometric correction plugin
 * Author: Jason Abernathy
 *
 * Created Sept. 07, 2011
 */


#include <cmath>

#include <epicsString.h>
#include <epicsMutex.h>
#include <epicsExport.h>
#include <iocsh.h>

#include <algorithm>

#include "NDArray.h"
#include <paramAttribute.h>


#include "ViewScreenConfiguredNDPlugin.h"

#include "NDPluginGeometricTransform.h"

using namespace std;

static const char* pluginName = "NDPluginGeometricTransform";


/** Constructor **/
NDPluginGeometricTransform::NDPluginGeometricTransform(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
                         int priority, int stackSize):
	/* Invoke the base class constructor */
	ViewScreenConfiguredNDPlugin (
		portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 1,
		NUM_GEOMTRANSFORM_PARAMS, maxBuffers, maxMemory,
		asynGenericPointerMask,
		asynGenericPointerMask, ASYN_CANBLOCK, 1, priority, stackSize ) {

    asynStatus status;
    const char *functionName = "NDPluginGeometricTransform";

 	/* Set the plugin type string */
	setStringParam(NDPluginDriverPluginType, "NDPluginGeometricTransform");

	/* Try to connect to the array port */
    status = connectToArrayPort();
}


#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
struct sort_indices { 
	sort_indices(std::array<double,4> &angles): angles(angles) {};
	bool operator() (const size_t &a, const size_t &b) const { return (angles[a] < angles[b]); };
	std::array<double,4> &angles;
};
#endif


/** NDPluginGeometricTransform::produce_corner_offsets
 * This function produces an array of offsets which can be added to a point in output image-space and produce a counter-clockwise convex quadrilateral in input image-space. If the offsets are not sorted correctly, then they could produce a concave quadrilateral which will cause undefined behaviour during the calibration process.
 */
std::array< std::tuple<double,double>,4 > NDPluginGeometricTransform::produce_corner_offsets() {

	std::array< std::tuple<double,double>,4 > offsets = {{ make_tuple(0.5,0.5), make_tuple(0.5,-0.5), make_tuple(-0.5,0.5), make_tuple(-0.5,-0.5) }};

	#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
	for( size_t i = 0; i < offsets.size(); i++ ) {
		this->oimage_to_beamspace( get<0>(offsets[i])+(this->get_output_image_width()-1.)/2., get<1>(offsets[i])+(this->get_output_image_height()-1.)/2., get<0>(offsets[i]), get<1>(offsets[i]) );
		this->beamspace_to_iimage( get<0>(offsets[i]), get<1>(offsets[i]), get<0>(offsets[i]), get<1>(offsets[i]) );
	}
	#else
	std::transform( offsets.begin(), offsets.end(), offsets.begin(),
		[this] (const std::tuple<float,float> &pt) {
			std::tuple<double,double> new_point;
			this->oimage_to_beamspace( get<0>(pt)+(this->get_output_image_width()-1.)/2., get<1>(pt)+(this->get_output_image_height()-1.)/2., get<0>(new_point), get<1>(new_point) );
			this->beamspace_to_iimage( get<0>(new_point), get<1>(new_point), get<0>(new_point), get<1>(new_point) );
			return new_point;
		}
	);
	#endif

	std::array<double,4> angles;
	double centroidx = 0.;
	double centroidy = 0.;

	for ( size_t i = 0; i < offsets.size(); i++ ) {

		centroidx += get<0>(offsets[i]);
		centroidy += get<1>(offsets[i]);
	}
	centroidx /= offsets.size();
	centroidy /= offsets.size();

	for ( size_t i = 0; i < offsets.size(); i++ ) {

		angles[i] = -1.*atan2(get<1>(offsets[i])-centroidy, get<0>(offsets[i])-centroidx);
	}

	std::array<size_t,4> indices {{ 0, 1, 2, 3 }};

	#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
	std::sort( indices.begin(), indices.end(), sort_indices(angles) );
	#else
	std::sort( indices.begin(), indices.end(),
		[angles] (const size_t &a, const size_t &b) {

			return (angles[a] < angles[b]);
		}
	);
	#endif

	for ( size_t i = 0; i < indices.size(); i++ ) {

		const size_t index = indices[i];
		offsets[i] = make_tuple( (index<2)?0.5:-0.5, (index%2)==0?0.5:-0.5 );
	}

	return offsets;
}


/** NDPluginGeometricTransform::calculate_gpc_polygon_area
 * This function calculates the area contained within the first contour of a gpc_polygon
 */
double NDPluginGeometricTransform::calculate_gpc_polygon_area(gpc_polygon &polygon) {

	double area = 0.;				

	if (polygon.num_contours <= 0) return area;

	gpc_vertex_list &contour = polygon.contour[0];

	// calculate the area of the results
	int aj = polygon.contour->num_vertices - 1;
	for ( int ai = 0; ai < contour.num_vertices; ai++ ) {

		gpc_vertex &I = contour.vertex[ai];
		gpc_vertex &J = contour.vertex[aj];
		area += (J.x+I.x)*(J.y-I.y);
		aj = ai;
	}
	area = fabs( area / 2. );

	return area;
}


/** ViewScreenConfiguredNDPlugin::configuration_change_callback
 *	This function is called when the configuration changes
 */
ViewScreenConfiguredNDPlugin::ConfigurationStatus_t NDPluginGeometricTransform::configuration_change_callback() {

	/* This function should be called from a locked state */

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "NDPluginMagnificationCorrection::Calibrate: Beginning calibration.\n" );

	const size_t output_image_width = this->get_output_image_width();
	const size_t output_image_height = this->get_output_image_height();

	const size_t input_image_width = this->get_input_image_width();
	const size_t input_image_height = this->get_input_image_height();


	if ( 0 == output_image_width || 0 == output_image_height ) {

		return ConfigurationStatusBadParameter;
	}

	this->geometric_correction_table.clear();
	this->geometric_correction_table.reserve(output_image_width*output_image_height);
	
	gpc_polygon subject, clip, result;
	
	gpc_vertex subject_vertices[4];
	gpc_vertex_list subject_contour;
	subject_contour.num_vertices = 4;
	subject_contour.vertex = subject_vertices;
	int hole = false;
	subject.num_contours = 1;
	subject.contour = &subject_contour;
	subject.hole = &hole;
	
	gpc_vertex clip_vertices[4];
	gpc_vertex_list clip_contour;
	clip_contour.num_vertices = 4;
	clip_contour.vertex = clip_vertices;
	clip.num_contours = 1;
	clip.contour = &clip_contour;
	subject.hole = &hole;

	//const double pixel_width = output_image_width / (endx - startx);
	//const double pixel_height = output_image_height / (endy - starty);

	/* these are the offsets which are added to the pixel centroid in output image space to produce a quadrilateral */
	std::array< std::tuple<double,double>,4 > output_corner_offsets = this->produce_corner_offsets();

	// map the output pixels onto the input image
	for ( size_t vc = 0; vc < output_image_height; vc++ ) {

		for ( size_t uc = 0; uc < output_image_width; uc++ ) {

			for ( size_t i = 0; i < output_corner_offsets.size(); i++ ) {

				this->oimage_to_beamspace(
					uc+get<0>(output_corner_offsets[i]),
					vc+get<1>(output_corner_offsets[i]),
					clip_vertices[i].x,
					clip_vertices[i].y);
				this->beamspace_to_iimage(
					clip_vertices[i].x,
					clip_vertices[i].y,
					clip_vertices[i].x,
					clip_vertices[i].y);
			}

			/*// to save some typing
			double &p1u = clip_vertices[0].x;
			double &p1v = clip_vertices[0].y;
			double &p2u = clip_vertices[1].x;
			double &p2v = clip_vertices[1].y;
			double &p3u = clip_vertices[2].x;
			double &p3v = clip_vertices[2].y;
			double &p4u = clip_vertices[3].x;
			double &p4v = clip_vertices[3].y;

			double tx, ty;

			// transform the input pixel to output image coordinates
			this->iimage_to_beamspace ( uc - 0.5, vc - 0.5, tx, ty );
			this->beamspace_to_oimage ( tx, ty, p1u, p1v );

			this->iimage_to_beamspace ( uc - 0.5, vc + 0.5, tx, ty );
			this->beamspace_to_oimage ( tx, ty, p2u, p2v );

			this->iimage_to_beamspace ( uc + 0.5, vc + 0.5, tx, ty );
			this->beamspace_to_oimage ( tx, ty, p3u, p3v );

			this->iimage_to_beamspace ( uc + 0.5, vc - 0.5, tx, ty );
			this->beamspace_to_oimage ( tx, ty, p4u, p4v );
			*/

			// the corners _should_ be sorted in a counter-clockwise order with clip_vertices[0] at the -x,+y position
			const size_t vcii = max( (int)min( round(clip_vertices[2].y), round(clip_vertices[3].y) ), 0 );
			const size_t vcif = max( min( (int)max( round(clip_vertices[0].y), round(clip_vertices[1].y) ), (int)input_image_height - 1 ), 0 );
			const size_t ucii = max( (int)min( round(clip_vertices[0].x), round(clip_vertices[3].x) ), 0 );
			const size_t ucif = max( min( (int)max( round(clip_vertices[1].x), round(clip_vertices[2].x) ), (int)input_image_width - 1 ), 0 );

			subject_vertices[0].x = -0.5;
			subject_vertices[0].y = input_image_height - 1.0 + 0.5;
			subject_vertices[1].x = input_image_width - 1.0 + 0.5;;
			subject_vertices[1].y = input_image_height - 1.0 + 0.5;
			subject_vertices[2].x = input_image_width - 1.0 + 0.5;
			subject_vertices[2].y = -0.5;
			subject_vertices[3].x = -0.5;
			subject_vertices[3].y = -0.5;
			gpc_polygon_clip(GPC_INT, &subject, &clip, &result);

			const double imagespace_area = this->calculate_gpc_polygon_area(result);
			gpc_free_polygon(&result);

			std::array< std::tuple<unsigned int, float>, 8 > entries;
			std::tuple<unsigned int, float> temp_entry = make_pair(vcii*input_image_width + ucii, 0.);
			entries.fill(temp_entry);

			if ( imagespace_area <= 0. ) {

				this->geometric_correction_table.push_back(entries);
				continue;
			}

			size_t next_entry_index = 0;

			for ( size_t vci = vcii; vci <= vcif; vci++ ) {

				for ( size_t uci = ucii; uci <= ucif; uci++ ) {

					subject_vertices[0].x = uci - 0.5;
					subject_vertices[0].y = vci + 0.5;
					subject_vertices[1].x = uci + 0.5;
					subject_vertices[1].y = vci + 0.5;
					subject_vertices[2].x = uci + 0.5;
					subject_vertices[2].y = vci - 0.5;
					subject_vertices[3].x = uci - 0.5;
					subject_vertices[3].y = vci - 0.5;
					
					// perform the clipping
					gpc_polygon_clip(GPC_INT, &subject, &clip, &result);	

					// calculate the area of the results
					const double area = this->calculate_gpc_polygon_area(result);
					gpc_free_polygon(&result);

					if ( area > 0. ) {

						const uint iindex = vci*input_image_width + uci;
						const double iarea = area/imagespace_area;
						if ( next_entry_index < entries.size() ) {

							entries[next_entry_index] = make_tuple(iindex, iarea);
							next_entry_index++;
						}
						else {

							// find the entry with the smallest area
							auto min = entries.begin();
							for ( auto entry = entries.begin()+1; entry != entries.end(); entry++ ) {

								if ( get<1>(*entry) < get<1>(*min) ) min = entry;
							}
							if ( iarea > get<1>(*min) ) {

								asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Correction table overflow; at (%u,%u), replacing index %u area %f with index %u area %f.\n", pluginName, __func__, uc, vc, get<0>(*min), get<1>(*min), iindex, iarea );
							}
							else {

								asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s: Correction table overflow; at (%u,%u), dropping index %u area %f.\n", pluginName, __func__, uc, vc, iindex, iarea );
							}
						}
					}
				}
			}

			this->geometric_correction_table.push_back(entries);
		}
	}

	// make the mapping more efficient by spreading valid pixel indexes into neighbouring entries
	/*int last_index = 0;
	for ( auto map = this->geometric_correction_table.begin(); map != this->geometric_correction_table.end(); map++ ) {

		for ( auto entry = map->begin(); entry != map->end(); entry++ ) {

			if ( get<1>(*entry) == 0. ) {

				get<0>(*entry) = last_index;
			}
			else {

				last_index = get<0>(*entry);
			}
		}
	}
	*/

	return ConfigurationStatusConfigured;
}


/** Report the compatibility of the input array and correction table
  * \param[in] pArray  Pointer to the NDArray to check
  * @return true if the correction may be applied; false otherwise.
  */
bool NDPluginGeometricTransform::preprocess_check ( NDArray *pArray ) {

	/* This function should be called while locked */

	/* Check the configuration status */
	if ( false == this->is_configured() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: View screen configuration not loaded.\n", pluginName );
		// The plugin is uncalibrated; there is no point in performing further checks.
		return false;
	}

	/* Let's learn a little bit about this array */
	NDArrayInfo_t ndarray_info;
	pArray->getInfo ( &ndarray_info );

	bool perform_correction = true;

	/* Validate the correction table */
	if ( 0 == geometric_correction_table.size() ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: The geometric correction table dimensions are invalid.\n", pluginName );
		perform_correction = false;
	}

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
	if ( (ndarray_info.xSize != this->get_input_image_width()) || (ndarray_info.ySize != this->get_input_image_height()) ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::preprocess_check: Input image dimensions (%lu,%lu) do not match configuration input image dimensions (%lux%lu).\n", ndarray_info.xSize, ndarray_info.ySize, this->get_input_image_width(), this->get_input_image_height() );
		perform_correction = false;
	}

	return perform_correction;
}


/** Performs a geometric transformation to produce the output image
 * \param[in] pArrayIn the input array
 * param[out] pArrayOut the output array (already allocated)
*/
template <typename epicsType>
asynStatus NDPluginGeometricTransform::transform_array ( NDArray *pArrayIn, NDArray &pArrayOut ) {

	float *pDataOut = (float*)pArrayOut.pData;
	epicsType *pDataIn = (epicsType*)pArrayIn->pData;

	for ( auto pixels = this->geometric_correction_table.cbegin(); pixels != this->geometric_correction_table.cend(); pixels++, pDataOut++ ) {

		float &value = *pDataOut;
		value = 0.;

		/*for ( auto entry = map->begin(); entry != map->end(); entry++ ) {

			value += get<1>(*entry) * (pDataIn[get<0>(*entry)]);
		}*/

		/* Unwinding the loop improves performance. */
		//value += get<1>((*pixels)[0]) * (pDataIn[get<0>((*pixels)[0])]);
		//value += get<1>((*pixels)[1]) * (pDataIn[get<0>((*pixels)[1])]);
		//value += get<1>((*pixels)[2]) * (pDataIn[get<0>((*pixels)[2])]);
		//value += get<1>((*pixels)[3]) * (pDataIn[get<0>((*pixels)[3])]);
		//value += get<1>((*pixels)[4]) * (pDataIn[get<0>((*pixels)[4])]);
		//value += get<1>((*pixels)[5]) * (pDataIn[get<0>((*pixels)[5])]);
		//value += get<1>((*pixels)[6]) * (pDataIn[get<0>((*pixels)[6])]);
		//value += get<1>((*pixels)[7]) * (pDataIn[get<0>((*pixels)[7])]);

		for ( auto entry = pixels->cbegin(); entry != pixels->cend(); entry++ ) {

			value += get<1>(*entry) * pDataIn[get<0>(*entry)];
		}
	}

	return asynSuccess;
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Grabs the current NDArray and applies the selected transforms to the data.  Apply the transforms in order.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginGeometricTransform::processCallbacks(NDArray *pArray) {

	/* This function is called with a lock and is unlocked afterward */

	assert ( NULL != pArray );

	/* Call the base class method */
	NDPluginDriver::processCallbacks ( pArray );

	const bool perform_correction = this->preprocess_check ( pArray );

	/* This will hold the converted array */
	NDArray *pArrayOut = NULL;

	if ( perform_correction ) {

		const int ndims = 2;
		size_t dims[ndims];
		dims[0] = this->get_output_image_width();
		dims[1] = this->get_output_image_height();
		const size_t dataSize = 0;	// let alloc compute the required size

		pArrayOut = this->pNDArrayPool->alloc ( ndims, dims, NDFloat32, dataSize, NULL );

		if ( NULL == pArrayOut ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::processCallbacks: Unable to allocate an output array.\n", pluginName );
		}
		else {

			pArray->pAttributeList->copy ( pArrayOut->pAttributeList );
			pArrayOut->uniqueId = pArray->uniqueId;
			pArrayOut->timeStamp = pArray->timeStamp;
			pArrayOut->epicsTS = pArray->epicsTS;

			asynStatus transformation_status = asynSuccess;

			switch ( pArray->dataType ) {
				case NDInt8:
					transformation_status = this->transform_array<epicsInt8>( pArray, *pArrayOut );
					break;
				case NDUInt8:
					transformation_status = this->transform_array<epicsUInt8>( pArray, *pArrayOut );
					break;
				case NDInt16:
					transformation_status = this->transform_array<epicsInt16>( pArray, *pArrayOut );
					break;
				case NDUInt16:
					transformation_status = this->transform_array<epicsUInt16>( pArray, *pArrayOut );
					break;
				case NDInt32:
					transformation_status = this->transform_array<epicsInt32>( pArray, *pArrayOut );
					break;
				case NDUInt32:
					transformation_status = this->transform_array<epicsUInt32>( pArray, *pArrayOut );
					break;
				case NDFloat32:
					transformation_status = this->transform_array<epicsFloat32>( pArray, *pArrayOut );
					break;
			    case NDFloat64:
					transformation_status = this->transform_array<epicsFloat64>( pArray, *pArrayOut );
					break;
				default:
					asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: ERROR: unknown data type=%d\n", pluginName, "processCallbacks", pArray->dataType);
			}	
		}
	}
	else {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "NDPluginMagnificationCorrection::processCallbacks: Preprocessing check failed; magnification corrections will not be applied to the input image.\n" );
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

	/* This isn't called by NDPluginDriver::processCallbacks; therfore, it needs to be done manually. */
	callParamCallbacks();
}




/** Configuration command */
extern "C" int NDGeometricTransformConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginGeometricTransform *pPlugin =
    new NDPluginGeometricTransform(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                    maxBuffers, maxMemory, priority, stackSize);
    pPlugin = NULL;  /* This is just to eliminate compiler warning about unused variables/objects */
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDGeometricTransformConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDGeometricTransformConfigure(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].ival,
                   args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDGeometricTransformRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDGeometricTransformRegister);
}
