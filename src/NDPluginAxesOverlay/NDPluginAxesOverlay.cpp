/*
 * NDPluginAxesOverlay.cpp
 *
 * Draws axes on an NDArray image
 * Author: Jason Abernathy
 *
 * Created Apr 31, 2012
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "NDPluginAxesOverlay.h"
#include <epicsExport.h>

using namespace std;
using namespace Magick;

static const char *pluginName="NDPluginAxesOverlay";


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including transform type and origin.
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginAxesOverlay::writeInt32(asynUser *pasynUser, epicsInt32 value) {

	int function = pasynUser->reason;
	asynStatus status = NDPluginDriver::writeInt32(pasynUser, value );

	if ( function == NDPluginAxesOverlayAxesOrigin ) {

		NDArray *pArray = this->pArrayLast;
		if ( NULL != pArray ) {

			pArray->reserve();
			this->processCallbacks( pArray );
			pArray->release();
		}
	}

	return status;
}


/** Called when asyn clients call pasynOctet->write().
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDPluginAxesOverlay::writeOctet(asynUser *pasynUser, const char *value,
                                   size_t nChars, size_t *nActual)
{
	int addr=0;
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *functionName = "writeOctet";

	status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
	status = asynNDArrayDriver::writeOctet(pasynUser, value, nChars, nActual);

	if ( function == NDPluginAxesOverlayTickLabelColour ) {

		try { this->ticklabel_colour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->ticklabel_colour = DrawableStrokeColor( "#00000000" ); }
	}
	else if ( function == NDPluginAxesOverlayBackgroundColour ) {

		try { this->background_colour = Color( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->background_colour = Color("#00000000"); }
	}
	else if ( function == NDPluginAxesOverlayAxesColour ) {

		try { this->axes_colour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->axes_colour = DrawableStrokeColor( "#00000000" ); }
	}	
	else if ( function == NDPluginAxesOverlayGridColour ) {

		try { this->grid_colour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->grid_colour = DrawableStrokeColor( "#00000000" ); }
	}

	if ( function >= FIRST_NDPLUGIN_AXES_OVERLAY_PARAM && function <= LAST_NDPLUGIN_AXES_OVERLAY_PARAM ) {

		NDArray *pArray = this->pArrayLast;
		if ( NULL != pArray ) {

			pArray->reserve();
			this->processCallbacks( pArray );
			pArray->release();
		}
	}

	if (status) {

		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, value=%s",
                  pluginName, functionName, status, function, value);
	}
	else {
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%s\n",
              pluginName, functionName, function, value);
	}

	return status;
}


std::vector<double> NDPluginAxesOverlay::find_divisions(const double start, const double end, const double scale) const {

	std::vector<double> divisions;

	const double xi = min(start, end);
	const double xf = max(start, end);

	const size_t output_width = xf - xi;

	const double xx = log10(output_width);

	double xa;
	const double xb = modf( xx, &xa );

	double xc;
	if ( xb > 0.699 ) {
		xc = 1.;
	}
	else if ( xb < 0.699 ) {
		xc = 2.;
	}
	else xc = 4.;
	const double xd = scale*pow(10., xa)/xc;

	for ( double x = xd*ceil(xi/xd); x < xf; x += xd ) {

		divisions.push_back(x);
	}

	return divisions;
}


/** This function draws the axes */
void NDPluginAxesOverlay::draw_axes( NDArray *pArray ) {

	/* This function needs to be called in a locked state with pArray reserved */

	const char *functionName = "draw_axes";

	if ( !pArray ) return;

	// determine where to draw the axes
	NDAxesOrigin_t origin = NDAxesOriginNone;
	getIntegerParam( NDPluginAxesOverlayAxesOrigin, (int*)&origin );

	if ( origin == NDAxesOriginNone ) {

		return;
	}

	// determine the output viewing area of the image
	NDAttribute *attribute;
	double startx, endx, starty, endy;
	bool found_attributes = true;

	attribute = pArray->pAttributeList->find( "ConfigBeamspaceStartX" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&startx, 0 );
	else found_attributes = false;

	attribute = pArray->pAttributeList->find( "ConfigBeamspaceEndX" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&endx, 0 );
	else found_attributes = false;

	attribute = pArray->pAttributeList->find( "ConfigBeamspaceStartY" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&starty, 0 );
	else found_attributes = false;

	attribute = pArray->pAttributeList->find( "ConfigBeamspaceEndY" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&endy, 0 );
	else found_attributes = false;

	NDArrayInfo_t array_info;
	pArray->getInfo( &array_info );
	const size_t image_width = array_info.xSize;
	const size_t image_height = array_info.ySize;

	if ( !found_attributes ) {

		startx = pArray->dims[array_info.xDim].offset;
		endx = array_info.xSize;
		starty = pArray->dims[array_info.yDim].offset;
		endy = array_info.ySize;
	}

	std::vector<double> uticks = this->find_divisions(startx, endx);
	std::vector<double> uminiticks = this->find_divisions(startx, endx, 0.2);
	std::vector<double> vticks = this->find_divisions(starty, endy);
	std::vector<double> vminiticks = this->find_divisions(starty, endy, 0.2);
	
	if ( !found_attributes )  {

		// we couldn't find the attributes from the view screen configuration plugin so use the ones which are in pixel coordinates		
		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:%s: Unable to convert from image to beamspace coordinates.\n", pluginName, functionName );
	}

	axes.clear();
	grid.clear();
	ticklabels.clear();

	// do the vertical ticks (for the horizontal axis)
	const double boxyi = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight))? -0.5: image_height - 1. + 0.5;
	const double text_y = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? 14: image_height - 6;
	const double horz_y = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? text_y + 6: text_y - 14;
	const double tick_yf = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? horz_y + 6: horz_y - 6;
	const double boxyf = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight))? tick_yf + 1: tick_yf - 1;
	const double axis_yf = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight))? image_height - 1. + 0.5: -0.5;
	const double min_v = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight))? horz_y: axis_yf;
	const double max_v = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight))? axis_yf: horz_y;
	//const double tick_yf = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? 9: image_height - 10;
	//const double minitick_yf = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? 5: image_height - 6;
	//const double text_y = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? tick_yf + 15: tick_yf - 3;
	// do the horizontal ticks (for the vertical axis)
	const double box_xi = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft))? -0.5: image_width - 1. - 0.5;
	const double text_x = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft) )? 6: image_width - 24;
	const double vert_x = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft) )? text_x + 24: text_x - 6;
	const double tick_xf = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft) )? vert_x + 6: vert_x - 6;
	const double box_xf = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft))? tick_xf + 1: tick_xf - 1;
	const double axis_xf = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft))? image_width - 1. + 0.5: -0.5;
	const double min_u = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft))? vert_x: axis_xf;
	const double max_u = ((origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft))? axis_xf: vert_x;

	std::list<Magick::Drawable> background;
	background.push_back(DrawableRectangle(-0.5, boxyi, image_width - 1. + 0.5, boxyf));
	background.push_back(DrawableRectangle(box_xi, -0.5, box_xf, image_height - 1. + 0.5));
	// the axes
	char ticklabelbuffer[8];
	axes.push_back( DrawableLine(vert_x, horz_y, axis_xf, horz_y) );
	axes.push_back( DrawableLine(vert_x, horz_y, vert_x, axis_yf) );
	
	for ( std::vector<double>::const_iterator utick = uticks.begin(); utick != uticks.end(); utick++ ) {

		// convert to pixel coordinaites (if necessary)
		const double u = !found_attributes? *utick: image_width*(*utick-startx)/(endx-startx) - 0.5;

		// don't draw the tick if it will overlap with the perpendicular axis
		if ( u <= min_u || u >= max_u ) continue;

		// draw the tick
		axes.push_back( DrawableLine(u, horz_y, u, tick_yf) );

		// draw the grid
		grid.push_back( DrawableLine(u, tick_yf + 1., u, axis_yf) );

		// draw the tick label
		char ticklabelbuffer[8];
		const int count = snprintf( ticklabelbuffer, 8, "%.3g", *utick );
		if ( count > 0 ) ticklabels.push_back( DrawableText(u-10, text_y, ticklabelbuffer ) );

	}

	for ( std::vector<double>::const_iterator minitick = uminiticks.begin(); minitick != uminiticks.end(); minitick++ ) {

		// convert to pixel coordinaites (if necessary)
		const double u = !found_attributes? *minitick: image_width*(*minitick-startx)/(endx-startx) - 0.5;

		// don't draw the tick if it will overlap with the perpendicular axis
		if ( u <= min_u || u >= max_u ) continue;

		// draw the tick
		axes.push_back( DrawableLine(u, horz_y, u, (tick_yf+horz_y)/2.) );
	}

	for ( std::vector<double>::const_iterator tick = vticks.begin(); tick != vticks.end(); tick++ ) {
	
		// convert to pixel coordinaites (if necessary)
		const double v = !found_attributes? *tick: image_height*(endy-(*tick))/(endy-starty) - 0.5;

		// don't draw the tick if it will overlap with the perpendicular axis
		if ( v <= min_v || v >= max_v ) continue;

		// draw the tick
		axes.push_back( DrawableLine( vert_x, v, tick_xf, v ) );

		// draw the grid
		grid.push_back( DrawableLine( tick_xf + 1., v, axis_xf, v) );

		// draw the tick label
		const int count = snprintf( ticklabelbuffer, 8, "%.3g", *tick );
		if ( count > 0 ) ticklabels.push_back( DrawableText( text_x, v+4, ticklabelbuffer ) );

	}

	for ( std::vector<double>::const_iterator minitick = vminiticks.begin(); minitick != vminiticks.end(); minitick++ ) {

		// convert to pixel coordinaites (if necessary)
		const double v = !found_attributes? *minitick: image_height*(endy-(*minitick))/(endy-starty) - 0.5;

		// don't draw the tick if it will overlap with the perpendicular axis
		if ( v <= min_v || v >= max_v ) continue;

		// draw the tick
		axes.push_back( DrawableLine( vert_x, v, (tick_xf+vert_x)/2., v ) );
	}

	StorageType stype;
	switch ( pArray->dataType ) {

		case NDInt8:
		case NDUInt8: {
			stype = CharPixel;
			break;
		}
		case NDInt16:
		case NDUInt16: {
			stype = ShortPixel;
			break;
		}
		case NDInt32:
		case NDUInt32: {
			stype = IntegerPixel;
			break;
		}
		case NDFloat32: {
			stype = FloatPixel;
			break;
		}
		case NDFloat64: {
			stype = DoublePixel;
			break;
		}
		default: return;
	}

	char map[8];
	switch ( array_info.colorMode ) {

		case NDColorModeMono:
			snprintf( map, 8, "I" );
			break;
		case NDColorModeRGB1:
			snprintf( map, 8, "RGB" );
			break;
		default: return;
	}

	Image image( image_width, image_height, map, stype, (void*)pArray->pData );

	background.push_front(DrawableFillColor(background_colour));
	image.draw(background);

	axes.push_front( DrawableStrokeWidth( 1. ) );
	axes.push_front( axes_colour );
	image.draw( axes );
	axes.pop_front();
	axes.pop_front();

	double dash_array[4] = {3., 5., 0.};
	grid.push_front( DrawableDashArray( dash_array ) );
	grid.push_front( DrawableFillOpacity( 0. ) );
	grid.push_front( DrawableStrokeColor( grid_colour ) );
	image.draw( grid );
	
	ticklabels.push_front( DrawableStrokeWidth( 1. ) );
	ticklabels.push_front( DrawableStrokeColor( ticklabel_colour ) );
	image.draw( ticklabels );

	//ticklabels.pop_front();
	//ticklabels.pop_front();
	//ticklabels.push_front( DrawableTranslation( 20., 0. ) );
	//ticklabels.push_front( DrawableTextUnderColor(Color("#aaaaaaaa")) );
	//ticklabels.push_front( DrawableStrokeWidth( 1. ) );
	//ticklabels.push_front( DrawableStrokeColor( ticklabel_colour ) );
	//image.draw( ticklabels );


	image.write( 0, 0, image_width, image_height, map, stype, (void*)pArray->pData );
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Add the AxesOverlay to the NDArray
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginAxesOverlay::processCallbacks ( NDArray *pArray )
{
    const char* functionName = "processCallbacks";
	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "%s:%s: pArray=%p\n", pluginName, functionName, pArray );

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

	if ( NULL == pArray ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s(pArray=NULL): Invalid array; processing aborted.\n", pluginName, functionName );
		return;
	}

	// Make a copy of the input array, with the data
	if ( pArray != this->pArrayLast ) {

		if ( pArrayLast ) {

			pArrayLast->release();
		}

		this->pArrayLast = this->pNDArrayPool->copy ( pArray, NULL, true );

		if ( NULL == this->pArrayLast ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:%s: Unable to copy the input array for later processing.\n", pluginName, functionName );
			return;
		}
	}

	// Make a copy of the input array, with the data
	NDArray *pArrayOut = this->pNDArrayPool->copy ( this->pArrayLast, NULL, true );

	if ( NULL == pArrayOut ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_WARNING, "%s:%s: Unable to make a copy of the input array.\n", pluginName, functionName );
	}
	else {

		this->getAttributes( pArrayOut->pAttributeList );
		this->unlock();
		/* Draw the axes */
		this->draw_axes ( pArrayOut );
		doCallbacksGenericPointer ( pArrayOut, NDArrayData, 0 );
		this->lock();

		if( this->pArrays[0] ) this->pArrays[0]->release();
		// reserve a copy of this array and hold it
		this->pArrays[0] = pArrayOut;
	}

    callParamCallbacks();
}


NDPluginAxesOverlay::~NDPluginAxesOverlay() {

}


/** Constructor for NDPluginAxesOverlay; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginAxesOverlay::NDPluginAxesOverlay(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_AXES_OVERLAY_PARAMS, maxBuffers, maxMemory,
                   asynGenericPointerMask,
                   asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize),
	ticklabel_colour( "#00000000" ),
	background_colour( "#aaaaaaff" ),
	axes_colour( "#00000000" ),
	grid_colour( "#00000000" ),
	pArrayLast ( NULL ) {

    asynStatus status;
    //const char *functionName = "NDPluginAxesOverlay";

    InitializeMagick( "" );
    
    /* File and Macro Strings */
	createParam( NDPluginAxesOverlayTickLabelColourString, asynParamOctet, &NDPluginAxesOverlayTickLabelColour );
	createParam( NDPluginAxesOverlayBackgroundColourString, asynParamOctet, &NDPluginAxesOverlayBackgroundColour );
	createParam( NDPluginAxesOverlayAxesColourString, asynParamOctet, &NDPluginAxesOverlayAxesColour );
	createParam( NDPluginAxesOverlayGridColourString, asynParamOctet, &NDPluginAxesOverlayGridColour );
	createParam( NDPluginAxesOverlayAxesOriginString, asynParamInt32, &NDPluginAxesOverlayAxesOrigin );
    
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginAxesOverlay");

	/* Initialize the status of each attribute file */
	setStringParam( NDPluginAxesOverlayTickLabelColour, "" );
	setStringParam( NDPluginAxesOverlayBackgroundColour, "" );
	setStringParam( NDPluginAxesOverlayAxesColour, "" );
	setStringParam( NDPluginAxesOverlayGridColour, "" );
	setIntegerParam( NDPluginAxesOverlayAxesOrigin, 0 );

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDAxesOverlayConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginAxesOverlay *pPlugin =
        new NDPluginAxesOverlay(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDAxesOverlayConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDAxesOverlayConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDAxesOverlayRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDAxesOverlayRegister);
}
