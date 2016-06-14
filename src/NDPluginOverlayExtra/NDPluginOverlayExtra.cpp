/*
 * NDPluginOverlayExtra.cpp
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
#include "NDPluginOverlayExtra.h"
#include <epicsExport.h>

using namespace std;
using namespace Magick;

static const char *driverName="NDPluginOverlayExtra";


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including transform type and origin.
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginOverlayExtra::writeInt32(asynUser *pasynUser, epicsInt32 value) {

	int function = pasynUser->reason;
	asynStatus status = NDPluginDriver::writeInt32(pasynUser, value );

	if ( function == NDPluginOverlayExtraAxesOrigin ) {

		draw_overlays();
	}

	return status;
}


/** Called when asyn clients call pasynOctet->write().
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDPluginOverlayExtra::writeOctet(asynUser *pasynUser, const char *value,
                                   size_t nChars, size_t *nActual)
{
	int addr=0;
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *functionName = "writeOctet";

	status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
	status = asynNDArrayDriver::writeOctet(pasynUser, value, nChars, nActual);

	if ( function == NDPluginOverlayExtraTickLabelColour ) {

		try { this->ticklabel_colour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->ticklabel_colour = DrawableStrokeColor( "#000000ff" ); }
	}
	else if ( function == NDPluginOverlayExtraTickLabelBkgColour ) {

		try { this->ticklabel_bkgcolour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->ticklabel_bkgcolour = DrawableStrokeColor( "#000000ff" ); }
	}
	else if ( function == NDPluginOverlayExtraAxesColour ) {

		try { this->axes_colour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->axes_colour = DrawableStrokeColor( "#000000ff" ); }
	}
	else if ( function == NDPluginOverlayExtraAxesBkgColour ) {

		try { this->axes_bkgcolour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->axes_bkgcolour = DrawableStrokeColor( "#000000ff" ); }
	}
	else if ( function == NDPluginOverlayExtraGridColour ) {

		try { this->grid_colour = DrawableStrokeColor( string( value, nChars ).c_str() ); }
		catch ( ... ) { this->grid_colour = DrawableStrokeColor( "#000000ff" ); }
	}
	draw_overlays();

	if (status) {

		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, value=%s",
                  driverName, functionName, status, function, value);
	}
	else {
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%s\n",
              driverName, functionName, function, value);
	}

	return status;
}

void NDPluginOverlayExtra::draw_overlays() {

	// reserve the current array so that another thread doesn't remove it
	this->lock();
	NDArray *pArray = this->pArrays[0];
	NDArray *pArrayOut = 0;
	if ( pArray ) {

		// ok, we're going to draw some axes. copy the current array so that we don't modify the "clean" version
		pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
	}
	this->unlock();

	// check to ensure that pArrayOut points to a valid array
	if ( !pArrayOut ) return;

	// collect the array information
	NDArrayInfo_t array_info;
	pArray->getInfo( &array_info );
	const size_t image_width = array_info.xSize;
	const size_t image_height = array_info.ySize;

	/* convert the raw data into an Image */
	StorageType stype;
	switch ( pArrayOut->dataType ) {

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
		default:
			pArrayOut->release();
			return;
	}

	char map[8];
	switch ( array_info.colorMode ) {

		case NDColorModeMono:
			snprintf( map, 8, "I" );
			break;
		case NDColorModeRGB1:
			snprintf( map, 8, "RGB" );
			break;
		default:
			pArrayOut->release();
			return;
	}

	Image image( image_width, image_height, map, stype, (void*)pArrayOut->pData );

	list<Drawable> drawables;
	generate_axes_drawables( pArrayOut, drawables );
	generate_marginal_drawables( pArrayOut, drawables );

	image.write( 0, 0, image_width, image_height, map, stype, (void*)pArrayOut->pData );

	// notify the listeners of the "new" array
	doCallbacksGenericPointer(pArrayOut, NDArrayData, 0);
	// and release it when they've finished their processing
	pArrayOut->release();
}

/** This function generates the drawing objects for the axes */
void NDPluginOverlayExtra::generate_axes_drawables ( const NDArray *pArrayOut, std::list<Drawable> &drawables ) {


	// determine where to draw the axes
	NDAxesOrigin_t origin = NDAxesOriginNone;
	getIntegerParam( NDPluginOverlayExtraAxesOrigin, (int*)&origin );

	if ( origin == NDAxesOriginNone ) {

		return;
	}

	// determine the output viewing area of the image
	NDAttribute *attribute;
	double startx, endx, starty, endy;
	bool found_attributes = true;

	attribute = pArrayOut->pAttributeList->find( "GeomStartX" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&startx, 0 );
	else found_attributes = false;

	attribute = pArrayOut->pAttributeList->find( "GeomEndX" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&endx, 0 );
	else found_attributes = false;

	attribute = pArrayOut->pAttributeList->find( "GeomStartY" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&starty, 0 );
	else found_attributes = false;

	attribute = pArrayOut->pAttributeList->find( "GeomEndY" );
	if ( attribute ) attribute->getValue( NDAttrFloat64, (void*)&endy, 0 );
	else found_attributes = false;

	// collect the array information
	NDArrayInfo_t array_info;
	(const_cast<NDArray*>(pArrayOut))->getInfo( &array_info ); /* FIXME: remove this when the NDArray methods are made const-aware */
	const size_t image_width = array_info.xSize;
	const size_t image_height = array_info.ySize;
	
	if ( !found_attributes )  {

		// we couldn't find the attributes from the Geometric transformation plugin so use the ones which are in pixel coordinates
		startx = pArrayOut->dims[array_info.xDim].offset;
		endx = array_info.xSize;
		starty = pArrayOut->dims[array_info.yDim].offset;
		endy = array_info.ySize;
	}

	const double output_width = endx - startx;
	const double output_height = endy - starty;

	std::list<Magick::Drawable> axes;
    std::list<Magick::Drawable> grid;
    std::list<Magick::Drawable> ticklabels;

	axes.clear();
	grid.clear();
	ticklabels.clear();

	const double horz_y = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? 0: image_height - 1;
	const double vert_x = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft) )? 0: image_width - 1;

	// we can unlock this plugin while the axes are being drawn
	this->unlock();

	// the axes
	char ticklabelbuffer[8];
	axes.push_back( DrawableLine( 0, horz_y, image_width-1, horz_y ) );
	axes.push_back( DrawableLine( vert_x, 0, vert_x, image_height-1 ) );

	const double xx = log10( output_width * 75. / image_width );
	double xa;
	const double xb = modf( xx, &xa );
	double xc;
	if ( xb < 0.398 ) xc = 2.5;
	else if ( xb < 0.699 ) xc = 5.;
	else xc = 10.0;
	const double xd = xc * pow( 10., xa );

	// do the vertical ticks (for the horizontal axis)
	const double tick_yf = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? 9: image_height - 10;
	const double minitick_yf = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? 5: image_height - 6;
	const double text_y = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginTopRight) )? tick_yf + 15: tick_yf - 3;
	for( double i = xd*(floor(startx/xd)+1); i < output_width; i+= xd ) {

		// convert to pixel coordinates
		const double x = (i - startx) * image_width / output_width;
		axes.push_back( DrawableLine( x, horz_y, x, tick_yf ) );
		axes.push_back( DrawableLine( x, horz_y, x, minitick_yf ) );

		// draw the grid
		grid.push_back( DrawableLine( x, tick_yf, x, image_height - tick_yf - 1 ) );

		// draw the tick label
		const int count = snprintf( ticklabelbuffer, 8, "%.3g", i );
		if ( count > 0 ) ticklabels.push_back( DrawableText( x-10, text_y, ticklabelbuffer ) );
	}
	
	const double yy = log10( output_height * 75. / image_height );
	double ya;
	const double yb = modf( yy, &ya );
	double yc;
	if ( yb < 0.398 ) yc = 2.5;
	else if ( yb < 0.699 ) yc = 5.;
	else yc = 10.0;
	const double yd = yc * pow( 10., ya );
	
	// do the horizontal ticks (for the vertical axis)
	const double tick_xf = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft) )? 9: image_width - 10;
	const double text_x = ( (origin == NDAxesOriginTopLeft) || (origin == NDAxesOriginBottomLeft) )? tick_xf + 5: tick_xf - 22;
	for( double i = yd*(floor(starty/yd)+1); i < output_height; i+= yd ) {

		// convert to pixel coordinaites
		const double y = (endy - i) * image_height / output_height;
		axes.push_back( DrawableLine( vert_x, y, tick_xf, y ) );

		// draw the grid
		grid.push_back( DrawableLine( tick_xf, y, image_width - tick_xf - 1, y ) );

		// draw the tick labels
		const int count = snprintf( ticklabelbuffer, 8, "%.3g", i );
		if ( count > 0 ) ticklabels.push_back( DrawableText( text_x, y+4, ticklabelbuffer ) );
	}

	// add the axes
	drawables.push_back( DrawableStrokeWidth( 3. ) );
	drawables.push_back( axes_bkgcolour );
	drawables.insert( drawables.end(), axes.begin(), axes.end() );
	drawables.push_back( DrawableStrokeWidth( 1. ) );
	drawables.push_back( axes_colour );
	drawables.insert( drawables.end(), axes.begin(), axes.end() );

	// add the grid lines
	double dash_array[4] = {3., 5., 0.};
	drawables.push_back( DrawableDashArray( dash_array ) );
	drawables.push_back( DrawableFillOpacity( 0. ) );
	drawables.push_back( DrawableStrokeColor ( grid_colour ) );
	drawables.insert( drawables.end(), grid.begin(), grid.end() );

	// add the tick labels
	drawables.push_back( DrawableStrokeWidth( 1.5 ) );
	drawables.push_back( DrawableStrokeColor( ticklabel_bkgcolour ) );
	drawables.insert( drawables.end(), ticklabels.begin(), ticklabels.end() );
	drawables.push_back( DrawableStrokeWidth( 1. ) );
	drawables.push_back( DrawableStrokeColor( ticklabel_colour ) );
	drawables.insert( drawables.end(), ticklabels.begin(), ticklabels.end() );
}


void NDPluginOverlayExtra::generate_marginal_drawables ( const NDArray *pArrayOut, std::list<Drawable> &drawables ) {

	// collect the array information
	NDArrayInfo_t array_info;
	(const_cast<NDArray*>(pArrayOut))->getInfo( &array_info ); /* FIXME: remove this when the NDArray methods are made const-aware */
	const size_t image_width = array_info.xSize;
	const size_t image_height = array_info.ySize;

	
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Add the OverlayExtra to the NDArray
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginOverlayExtra::processCallbacks(NDArray *pArray)
{
    const char* functionName = "processCallbacks";

	this->lock();
	if( this->pArrays[0] ) this->pArrays[0]->release();
	// reserve a copy of this array and hold it
	this->pArrays[0] = pArray;
	pArray->reserve();
	this->unlock();

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
	
	/* Draw the axes */
	draw_overlays( );

	asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
		  "%s:%s: pArray=%p\n",
		  "NDPluginGeometricTransform", functionName, pArray);

    callParamCallbacks();
}


NDPluginOverlayExtra::~NDPluginOverlayExtra() {

}


/** Constructor for NDPluginOverlayExtra; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginOverlayExtra::NDPluginOverlayExtra(const char *portName, int queueSize, int blockingCallbacks,
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
	ticklabel_bkgcolour( "#00000000" ),
	axes_colour( "#00000000" ),
	axes_bkgcolour( "#00000000" ),
	grid_colour( "#00000000" ) {
    asynStatus status;
    //const char *functionName = "NDPluginOverlayExtra";

    InitializeMagick( "" );
    
    /* File and Macro Strings */
	createParam( NDPluginOverlayExtraTickLabelColourString, asynParamOctet, &NDPluginOverlayExtraTickLabelColour );
	createParam( NDPluginOverlayExtraTickLabelBkgColourString, asynParamOctet, &NDPluginOverlayExtraTickLabelBkgColour );
	createParam( NDPluginOverlayExtraAxesColourString, asynParamOctet, &NDPluginOverlayExtraAxesColour );
	createParam( NDPluginOverlayExtraAxesBkgColourString, asynParamOctet, &NDPluginOverlayExtraAxesBkgColour );
	createParam( NDPluginOverlayExtraGridColourString, asynParamOctet, &NDPluginOverlayExtraGridColour );
	createParam( NDPluginOverlayExtraAxesOriginString, asynParamInt32, &NDPluginOverlayExtraAxesOrigin );
    
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginOverlayExtra");

	/* Initialize the status of each attribute file */
	setStringParam( NDPluginOverlayExtraTickLabelColour, "" );
	setStringParam( NDPluginOverlayExtraTickLabelBkgColour, "" );
	setStringParam( NDPluginOverlayExtraAxesColour, "" );
	setStringParam( NDPluginOverlayExtraAxesBkgColour, "" );
	setStringParam( NDPluginOverlayExtraGridColour, "" );
	setIntegerParam( NDPluginOverlayExtraAxesOrigin, 0 );

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDOverlayExtraConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginOverlayExtra *pPlugin =
        new NDPluginOverlayExtra(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDOverlayExtraConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDOverlayExtraConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDOverlayExtraRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDOverlayExtraRegister);
}
