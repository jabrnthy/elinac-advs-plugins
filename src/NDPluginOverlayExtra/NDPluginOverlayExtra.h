#ifndef NDPluginOverlayExtra_H
#define NDPluginOverlayExtra_H

#include <epicsTypes.h>
#include <Magick++.h>
#include <list>

#include "NDPluginDriver.h"

/* PV Names */
#define NDPluginOverlayExtraTickLabelColourString		"TICK_LABEL_COLOUR"
#define NDPluginOverlayExtraTickLabelBkgColourString		"TICK_LABEL_BKGCOLOUR"
#define NDPluginOverlayExtraAxesColourString				"AXES_COLOUR"
#define NDPluginOverlayExtraAxesBkgColourString			"AXES_BKGCOLOUR"
#define NDPluginOverlayExtraGridColourString				"GRID_COLOUR"
#define NDPluginOverlayExtraAxesOriginString				"AXES_ORIGIN"

typedef enum { NDAxesOriginNone, NDAxesOriginTopLeft, NDAxesOriginBottomLeft, NDAxesOriginTopRight, NDAxesOriginBottomRight } NDAxesOrigin_t;

/** Performs macro substitution on an attribute file then adds the OverlayExtra to the current NDArray
  */
class NDPluginOverlayExtra : public NDPluginDriver {
public:
    NDPluginOverlayExtra(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
	~NDPluginOverlayExtra();
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeOctet (asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    
protected:
    #define FIRST_NDPLUGIN_AXES_OVERLAY_PARAM NDPluginOverlayExtraTickLabelColour
    /* File and Macro Strings */
    int NDPluginOverlayExtraTickLabelColour;
    int NDPluginOverlayExtraTickLabelBkgColour;
    int NDPluginOverlayExtraAxesColour;
    int NDPluginOverlayExtraAxesBkgColour;
    int NDPluginOverlayExtraGridColour;
    int NDPluginOverlayExtraAxesOrigin;
	#define LAST_NDPLUGIN_AXES_OVERLAY_PARAM NDPluginOverlayExtraAxesOrigin

private:

	std::list<Magick::Drawable> axes;
	std::list<Magick::Drawable> grid;
	std::list<Magick::Drawable> ticklabels;
	Magick::DrawableStrokeColor ticklabel_colour;
	Magick::DrawableStrokeColor ticklabel_bkgcolour;
	Magick::DrawableStrokeColor axes_colour;
	Magick::DrawableStrokeColor axes_bkgcolour;
	Magick::DrawableStrokeColor grid_colour;

	/* This function draws the axes */
	void draw_overlays ();
	void generate_axes_drawables ( const NDArray *pArrayOut, std::list<Magick::Drawable> &drawables );
	void generate_marginal_drawables ( const NDArray *pArrayOut, std::list<Magick::Drawable> &drawables );

};
#define NUM_NDPLUGIN_AXES_OVERLAY_PARAMS (&LAST_NDPLUGIN_AXES_OVERLAY_PARAM - &FIRST_NDPLUGIN_AXES_OVERLAY_PARAM + 1)
    
#endif
