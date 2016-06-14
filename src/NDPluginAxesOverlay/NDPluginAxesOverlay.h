#ifndef NDPluginAxesOverlay_H
#define NDPluginAxesOverlay_H

#include <epicsTypes.h>
#include <Magick++.h>
#include <list>
#include <vector>

#include "NDPluginDriver.h"

/* PV Names */
#define NDPluginAxesOverlayTickLabelColourString		"TICKLABEL_COLOUR"
#define NDPluginAxesOverlayBackgroundColourString		"BACKGROUND_COLOUR"
#define NDPluginAxesOverlayAxesColourString				"AXES_COLOUR"
#define NDPluginAxesOverlayGridColourString				"GRID_COLOUR"
#define NDPluginAxesOverlayAxesOriginString				"AXES_ORIGIN"

typedef enum { NDAxesOriginNone, NDAxesOriginTopLeft, NDAxesOriginBottomLeft, NDAxesOriginTopRight, NDAxesOriginBottomRight } NDAxesOrigin_t;

/** Performs macro substitution on an attribute file then adds the AxesOverlay to the current NDArray
  */
class NDPluginAxesOverlay : public NDPluginDriver {
public:
    NDPluginAxesOverlay(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
	~NDPluginAxesOverlay();
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeOctet (asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    
protected:
    #define FIRST_NDPLUGIN_AXES_OVERLAY_PARAM NDPluginAxesOverlayTickLabelColour
    /* File and Macro Strings */
    int NDPluginAxesOverlayTickLabelColour;
    int NDPluginAxesOverlayBackgroundColour;
    int NDPluginAxesOverlayAxesColour;
    int NDPluginAxesOverlayGridColour;
    int NDPluginAxesOverlayAxesOrigin;
	#define LAST_NDPLUGIN_AXES_OVERLAY_PARAM NDPluginAxesOverlayAxesOrigin

private:

	std::list<Magick::Drawable> axes;
	std::list<Magick::Drawable> grid;
	std::list<Magick::Drawable> ticklabels;
	Magick::DrawableStrokeColor ticklabel_colour;
	Magick::Color				background_colour;
	Magick::DrawableStrokeColor axes_colour;
	Magick::DrawableStrokeColor grid_colour;

	/* This is a helper function that divides a range into nice divisions */
	std::vector<double> find_divisions(const double start, const double end, const double scale = 1.0) const;

	/* This function draws the axes */
	void draw_axes ( NDArray *pArray );

	/* This is a copy of the last input array which is used when the operator changes how the axes are drawn */
	NDArray *pArrayLast;

};
#define NUM_NDPLUGIN_AXES_OVERLAY_PARAMS (&LAST_NDPLUGIN_AXES_OVERLAY_PARAM - &FIRST_NDPLUGIN_AXES_OVERLAY_PARAM + 1)
    
#endif
