#ifndef ViewScreenConfiguredNDPlugin_H
#define ViewScreenConfiguredNDPlugin_H

#include "tinyxml2.h"

#include <iocsh.h>
#include <epicsExport.h>

#include <asynStandardInterfaces.h>
#include <NDPluginDriver.h>

#include <vector>
#include <string>
#include <map>
#include <utility>
#include <array>

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
#define ViewScreenConfiguredNDPluginConfigurationFileString	"CONFIGURATION_FILE"
#define ViewScreenConfiguredNDPluginConfigurationStatusString	"CONFIGURATION_STATUS"

/** These plugins accept an XML configuration file. */

class ViewScreenConfiguredNDPlugin: public NDPluginDriver {

public:

	ViewScreenConfiguredNDPlugin ( const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxAddr, int numParams, int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask, int asynFlags, int autoConnect, int priority, int stackSize );

	/* Coordinate conversions */
	// output image <-> beamspace
	void oimage_to_beamspace ( const double u, const double v, double &x, double &y ) const;
	void beamspace_to_oimage ( const double x, const double y, double &u, double &v ) const;
	// input image (ccd) <-> beamspace
	void iimage_to_beamspace ( const double u, const double v, double &x, double &y ) const;
	void beamspace_to_iimage ( const double x, const double y, double &u, double &v ) const;

	/** Helper function which calculates the area of input ccd covered by an output pixel **/
	double ccd_area_covered_by_output_pixel ( const double u, const double v ) const;

    /* These methods override the virtual methods in the base class */
	asynStatus writeOctet ( asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual );

	/* These are used to register functions with the EPICS IOC Shell */
	// The viewscreen_set_configuration_file_directory function definitions
	static const iocshArg viewscreen_set_configuration_file_directory_Arg0;
	static const iocshArg *viewscreen_set_configuration_file_directory_Args[1];
	static const iocshFuncDef viewscreen_set_configuration_file_directory_FuncDef;
	static void viewscreen_set_configuration_file_directory ( const iocshArgBuf *args );
	
	// The viewscreen_set_efficiency_map_directory function definitions
	static const iocshArg viewscreen_set_efficiency_map_directory_Arg0;
	static const iocshArg viewscreen_set_efficiency_map_directory_Arg1;
	static const iocshArg viewscreen_set_efficiency_map_directory_Arg2;
	static const iocshArg *viewscreen_set_efficiency_map_directory_Args[3];
	static const iocshFuncDef viewscreen_set_efficiency_map_directory_FuncDef;
	static void viewscreen_set_efficiency_map_directory ( const iocshArgBuf *args );

protected:

	typedef enum {
		ConfigurationStatusConfigured,
		ConfigurationStatusAsynError,
		ConfigurationStatusXMLErrorFileNotFound,
		ConfigurationStatusXMLError,
		ConfigurationStatusBadParameter,
		ConfigurationStatusUnconfigured,
		ConfigurationStatusConfiguring
	} ConfigurationStatus_t;

	class TargetInfo {

		public:
			TargetInfo ( ): material ( "undefined" ), light_distribution ( "undefined" ) {};
			TargetInfo ( const std::string material, const std::string light_distribution ): material ( material ), light_distribution ( light_distribution ) {};
			std::string material;
			std::string light_distribution;

			bool operator< ( const TargetInfo &rhs ) const {

				if ( material == rhs.material ) {

					return ( light_distribution < rhs.light_distribution );
				}
				else return ( material < rhs.material );
			}
	};

	virtual ConfigurationStatus_t configuration_change_callback () = 0;
	bool is_configured () const;

	/* The mapping functions */
	double fx ( const double u, const double v ) const;
	double fy ( const double u, const double v ) const;
	double gu ( const double x, const double y ) const;
	double gv ( const double x, const double y ) const;

	/* Get methods */
	size_t get_version () { return this->version; };
	std::string get_geometry () const { return this->geometry; };
	double get_xi () const { return this->xi; };
	double get_xf () const { return this->xf; };
	double get_yi () const { return this->yi; };
	double get_yf () const { return this->yf; };
	size_t get_output_image_width () const { if ( this->nx < 0 ) return 0; return (size_t)this->nx; };
	size_t get_output_image_height () const { if ( this->ny < 0 ) return 0; return (size_t)this->ny; };
	size_t get_input_image_width () const { return 780; };
	size_t get_input_image_height () const { return 580; };
	std::string get_configuration_directory () const { return directory_configuration_files; };
	int get_x_orientation () const { return this->x_orientation; };
	int get_y_orientation () const { return this->y_orientation; };

	TargetInfo get_target_info ( const size_t ) const;
	size_t get_maximum_target_count () const { return this->targets.size(); };
	std::string get_efficiency_map_directory ( const size_t target_number ) const;

	#define FIRST_ViewScreenConfiguredNDPlugin_PARAM ViewScreenConfiguredNDPluginConfigurationFile
	int ViewScreenConfiguredNDPluginConfigurationFile;
	int ViewScreenConfiguredNDPluginConfigurationStatus;
	#define LAST_ViewScreenConfiguredNDPlugin_PARAM ViewScreenConfiguredNDPluginConfigurationStatus

private:
	static std::string directory_configuration_files;
	static std::map<std::pair<std::string,std::string>, std::string> efficiency_map_directories;
	
	ConfigurationStatus_t load_configuration ( );
	ConfigurationStatus_t xmlerror_to_pluginstatus ( const tinyxml2::XMLError xml_error );

	size_t version;
	std::string geometry;			// the geometry of this view screen unit
	std::string orientation;		// the orientation of this view screen unit
	int order;						// the order of the interpolating multivariate polynomial
	int nx, ny;						// nx, ny: the width and height of the beamspace image
	double xi, xf, yi, yf;			// xi, xf, yi, yf: the extents of the beamspace image
	std::vector<double> fxc, fyc;		// fxc, fyc: the coefficients of interpolating multivariate polynomials
	std::vector<double> guc, gvc;		// guc, gvc: the coefficients of interpolating multivariate polynomials

	// orientation information
	int x_orientation;				// the sign of the x-orientation of the view screen unit (+1 for positive x, -1 for negative x)
	int y_orientation;				// the sign of the y-orientatino of the view screen unit (+1 for upward, -1 for downward)
	
	std::array<TargetInfo,3>	targets;	// the targets in this view screen unit
};

#define NUM_ViewScreenConfiguredNDPlugin_PARAMS (&LAST_ViewScreenConfiguredNDPlugin_PARAM - &FIRST_ViewScreenConfiguredNDPlugin_PARAM + 1)

#endif // ViewScreenConfiguredNDPlugin_H
