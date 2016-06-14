
#include <array>
#include <tuple>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <gsl/gsl_poly.h>

#include "ViewScreenConfiguredNDPlugin.h"

static const char* pluginName = "ViewScreenConfiguredNDPlugin";

using namespace std;
using namespace tinyxml2;

// Static configuration variables
std::string ViewScreenConfiguredNDPlugin::directory_configuration_files ( "/var/viewscreen/configuration/" );
std::map<std::pair<std::string,std::string>, std::string> ViewScreenConfiguredNDPlugin::efficiency_map_directories;

#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
class to_beamspace {
	private:
		const double &u, &v;
		const ViewScreenConfiguredNDPlugin &vsplugin;
	public:
		to_beamspace(const double &u, const double &v, const ViewScreenConfiguredNDPlugin &vsplugin): u(u), v(v), vsplugin(vsplugin) {};
		tuple<double,double> operator () (tuple<double,double> &uvpoint) {

			double x, y;
			vsplugin.oimage_to_beamspace ( u + get<0>(uvpoint), v + get<1>(uvpoint), x, y );
			return make_tuple ( x, y );
		}
};

class to_imagespace {
	private:
		const ViewScreenConfiguredNDPlugin &vsplugin;
	public:
		to_imagespace(const ViewScreenConfiguredNDPlugin &vsplugin): vsplugin(vsplugin) {};
		tuple<double,double> operator () (tuple<double,double> &xypoint) {

			double pu, pv;
			vsplugin.beamspace_to_iimage ( get<0>(xypoint), get<1>(xypoint), pu, pv );
			return make_tuple ( pu, pv );
		}
};

class by_clockwise_angle_about {
	private:
		const double uc, vc;
	public:
		by_clockwise_angle_about ( double &uc, double& vc ): uc(uc), vc(vc) {};
		bool operator () ( tuple<double,double> uv1, tuple<double,double> uv2 ) {
			// the "y" coordinate is reflected in the ccd coordinate system (y increases downward)
			const double angle1 = atan2 ( vc - get<1>(uv1), get<0>(uv1) - uc );
			const double angle2 = atan2 ( vc - get<1>(uv2), get<0>(uv2) - uc );
			return angle1 > angle2;
		}
};
#endif


ViewScreenConfiguredNDPlugin::ViewScreenConfiguredNDPlugin ( const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxAddr, int numParams, int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask, int asynFlags, int autoConnect, int priority, int stackSize ):
	NDPluginDriver ( portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxAddr, numParams + NUM_ViewScreenConfiguredNDPlugin_PARAMS, maxBuffers, maxMemory, interfaceMask, interruptMask, asynFlags, autoConnect, priority, stackSize ),
	order ( -1 ),
	x_orientation ( 0 ),
	y_orientation ( 0 ) {

	createParam ( ViewScreenConfiguredNDPluginConfigurationStatusString, asynParamInt32, &ViewScreenConfiguredNDPluginConfigurationStatus );
	createParam ( ViewScreenConfiguredNDPluginConfigurationFileString, asynParamOctet, &ViewScreenConfiguredNDPluginConfigurationFile );

	setStringParam  ( NDPluginDriverPluginType, "ViewScreenConfiguredNDPlugin" );
	setIntegerParam ( ViewScreenConfiguredNDPluginConfigurationStatus, ConfigurationStatusUnconfigured );
	setStringParam  ( ViewScreenConfiguredNDPluginConfigurationFile, "" );

	callParamCallbacks ();
}



std::string ViewScreenConfiguredNDPlugin::get_efficiency_map_directory ( const size_t target_number ) const {

	if ( target_number >= targets.size() ) return std::string ( "" );

	auto entry = efficiency_map_directories.find ( make_pair ( this->geometry, targets[target_number].light_distribution ) );

	if ( entry == efficiency_map_directories.end() ) return std::string ( "" );

	return entry->second;
}


ViewScreenConfiguredNDPlugin::TargetInfo ViewScreenConfiguredNDPlugin::get_target_info ( const size_t target_number ) const {

	if ( target_number >= targets.size() ) {

		return ViewScreenConfiguredNDPlugin::TargetInfo ( "", "" );
	}

	return this->targets.at ( target_number );
}

ViewScreenConfiguredNDPlugin::ConfigurationStatus_t ViewScreenConfiguredNDPlugin::load_configuration ( ) {

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::load_configuration: Begin.\n", pluginName );

	char filename[128];

	const asynStatus status = getStringParam ( ViewScreenConfiguredNDPluginConfigurationFile, 128, filename );

	if ( asynSuccess != status ) {

		return ConfigurationStatusAsynError;
	}

	string full_filename = this->get_configuration_directory() + string ( filename );

	asynPrint ( this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::load_configuration: Loading %s.\n", pluginName, full_filename.c_str() );

	XMLDocument doc;
	XMLError xml_error = doc.LoadFile( full_filename.c_str() );
	if ( XML_NO_ERROR != xml_error ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::load_configuration: XML document error; ErrorID=%d\n", pluginName, doc.ErrorID() );
		return this->xmlerror_to_pluginstatus ( xml_error );
	}

	XMLElement *configuration_element = doc.FirstChildElement ( "configuration" );
	if ( NULL == configuration_element ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; <configuration> element missing.\n", pluginName, __func__ );
		return this->xmlerror_to_pluginstatus ( doc.ErrorID() );

	}

	/* Grab the geometry */
	{
		XMLElement *element = configuration_element->FirstChildElement ( "geometry" );
		if ( NULL == element ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; <geometry> element missing.\n", pluginName, __func__ );
			return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
		}

		const char *text = element->GetText();

		if ( NULL == text ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; <geometry> element is empty.\n", pluginName, __func__ );
			return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
		}

		this->geometry = string ( text );

		if ( geometry.compare ( "elbt" ) && geometry.compare ( "embt" ) && geometry.compare ( "ehbt" ) ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; Unsupported geometry type: %s.\n", pluginName, __func__, text );
			return ConfigurationStatusBadParameter;
		}
	}

	/* Grab the orientation */
	{
		XMLElement *element = configuration_element->FirstChildElement ( "orientation" );
		if ( NULL == element ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; <orientation> element missing.\n", pluginName, __func__ );
			return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
		}

		XMLError error = element->QueryIntAttribute ( "x", &this->x_orientation );
		if ( XML_NO_ERROR != error ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; \"x\" attribute missing from <orientation> element.\n", pluginName, __func__ );
			return this->xmlerror_to_pluginstatus ( error );
		}

		error = element->QueryIntAttribute ( "y", &this->y_orientation );
		if ( XML_NO_ERROR != error ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; \"y\" attribute missing from <orientation> element.\n", pluginName, __func__ );
			return this->xmlerror_to_pluginstatus ( error );
		}
	}

	/* Grab the target information */
	{
		XMLElement *element = configuration_element->FirstChildElement ( "target" );

		if ( NULL == element ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; \"y\" <target> element missing.\n", pluginName, __func__ );
			return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
		}

		while ( NULL != element ) {

			int target_number;
			const char *material;
			const char *light_distribution;
			XMLError error;

			if ( XML_NO_ERROR != (error = element->QueryIntAttribute ( "number", &target_number ) ) ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; \"number\" attribute missing from <target> element.\n", pluginName, __func__ );
				return this->xmlerror_to_pluginstatus ( error );
			}

			if ( NULL == (material = element->Attribute ( "material" ) ) ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; \"material\" attribute missing from <target> element.\n", pluginName, __func__ );
				return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
			}

			if ( NULL == (light_distribution = element->Attribute ( "light_distribution" ) ) ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: XML Document Error; \"light_distribution\" attribute missing from <target> element.\n", pluginName, __func__ );
				return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
			}

			this->targets[(size_t)target_number] = TargetInfo ( material, light_distribution );

			element = element->NextSiblingElement ( "target" );
		}
	}

	/* Grab calibration parameters from the configuration file */

	XMLElement* calibration_element = configuration_element->FirstChildElement ( "calibration" );
	if ( NULL == calibration_element ) {

		asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::load_configuration: XML Document Error; couldn't find the <calibration> element.\n", pluginName );
		return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
	}

	/* Integer parameters */
	vector< tuple<string,int*> > integer_parameters = {
		make_tuple( string("BeamspaceImageWidth"), &(this->nx)),
		make_tuple( string("BeamspaceImageHeight"), &(this->ny)),
		make_tuple( string("MappingOrder"), &(this->order))
	};
	for ( auto parameter = integer_parameters.begin(); parameter != integer_parameters.end(); parameter++ ) {

		XMLElement* element = calibration_element->FirstChildElement ( get<0>(*parameter).c_str() );
		if ( NULL == element ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::load_configuration: XML Document Error; couldn't find the %s element.\n", pluginName, get<0>(*parameter).c_str() );
			return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
		}
		else {

			*(get<1>(*parameter)) = atoi ( element->GetText() );
		}
	}

	/* Floating-point parameters */
	vector< tuple<string,double*> > double_parameters = { 
		make_tuple( string("BeamspaceStartX"), &(this->xi)),
		make_tuple( string("BeamspaceEndX"), &(this->xf)),
		make_tuple( string("BeamspaceStartY"), &(this->yi)),
		make_tuple( string("BeamspaceEndY"), &(this->yf))
	};
	for ( auto parameter = double_parameters.begin(); parameter != double_parameters.end(); parameter++ ) {

		XMLElement* element = calibration_element->FirstChildElement ( get<0>(*parameter).c_str() );
		if ( NULL == element ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::load_configuration: XML Document Error; couldn't find the %s element.\n", pluginName, get<0>(*parameter).c_str() );
			return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
		}
		else {

			*(get<1>(*parameter)) = atof ( element->GetText() );
		}
	}

	/* Now grab the mapping type */
	{
		XMLElement* element = calibration_element->FirstChildElement ( "MappingType" );
		if ( NULL == element ) {

			asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::load_configuration: XML Document Error; couldn't find the MappingType element.\n", pluginName );
			return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
		}
		else {

			if ( 0 != string("Multivariate Polynomial").compare ( element->GetText() ) ) {

				asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::load_configuration: XML Document Error; only mapping type of multivariate polynomial is supported.\n", pluginName );
				return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
			}
			else {

				/* Load the mapping coefficients */
				vector< tuple<string,vector<double>*> > array_parameters = {
					make_tuple( string("GUCoefficients"), &this->guc),
					make_tuple( string("GVCoefficients"), &this->gvc),
					make_tuple( string("FXCoefficients"), &this->fxc),
					make_tuple( string("FYCoefficients"), &this->fyc)
				};
				for ( auto parameter = array_parameters.begin(); parameter != array_parameters.end(); parameter++ ) {

					element = calibration_element->FirstChildElement ( get<0>(*parameter).c_str() );
					if ( NULL == element ) {

						asynPrint ( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::load_configuration: XML Document Error; couldn't find the GUCoefficients element.\n", pluginName );
						return this->xmlerror_to_pluginstatus ( doc.ErrorID() );
					}

					stringstream cs( string( element->GetText() ) );
					while ( cs.good() ) {

						double coefficient;
						cs >> coefficient;
						get<1>(*parameter)->push_back ( coefficient );
					}
				}
			}
		}
	}

	/*cout << "Calibration\n-----------" << endl;
	cout << "nx: " << nx << endl;
	cout << "ny: " << nx << endl;
	cout << "order: " << this->order << endl;
	cout << "(xi,xf): (" << xi << "," << xf << ")" << endl;
	cout << "(yi,yf): (" << yi << "," << yf << ")" << endl;

	cout << "guc: ";
	for_each ( guc.begin(), guc.end(), [](float f){cout << f << ", ";} );
	cout << endl;

	cout << "gvc: ";
	for_each ( gvc.begin(), gvc.end(), [](float f){cout << f << ", ";} );
	cout << endl;

	cout << "fxc: ";
	for_each ( fxc.begin(), fxc.end(), [](float f){cout << f << ", ";} );
	cout << endl;

	cout << "fyc: ";
	for_each ( fyc.begin(), fyc.end(), [](float f){cout << f << ", ";} );
	cout << endl;*/

	return ConfigurationStatusConfigured;
}


/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including NDPluginDriverArrayPort.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus ViewScreenConfiguredNDPlugin::writeOctet( asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual ) {

	int addr=0;
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	static const char *functionName = "writeOctet";

	if ( function < FIRST_ViewScreenConfiguredNDPlugin_PARAM ) {

		return NDPluginDriver::writeOctet ( pasynUser, value, nChars, nActual );
	}

	status = getAddress(pasynUser, &addr); if ( asynSuccess != status ) return ( status );

	/* Set the parameter in the parameter library. */
	status = (asynStatus) setStringParam ( addr, function, (char *)value );

	if ( function == ViewScreenConfiguredNDPluginConfigurationFile ) {

		setIntegerParam ( ViewScreenConfiguredNDPluginConfigurationStatus, ConfigurationStatusConfiguring );
		this->callParamCallbacks (addr);

		const ConfigurationStatus_t configuration_status = this->load_configuration ();
		if ( ConfigurationStatusConfigured != configuration_status ) {

			asynPrint ( pasynUser, ASYN_TRACE_ERROR, "%s:%s; Unable to load configuration: status=%d.\n", pluginName, functionName, configuration_status );
			setIntegerParam ( ViewScreenConfiguredNDPluginConfigurationStatus, configuration_status );
		}
		else {

			const ConfigurationStatus_t callback_status = this->configuration_change_callback();
			if ( ConfigurationStatusConfigured != callback_status ) {

				asynPrint ( pasynUser, ASYN_TRACE_ERROR, "%s:%s; Configuration change callback returned with an error code.\n", pluginName, functionName );
			}
			setIntegerParam ( ViewScreenConfiguredNDPluginConfigurationStatus, callback_status );
		}
    }
    
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks ( addr );

    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  pluginName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%s\n", pluginName, functionName, function, value);
    *nActual = nChars;
    return status;
}


double ViewScreenConfiguredNDPlugin::fx ( const double u, const double v ) const {

	if ( this->order < 0 ) {
		return 0.;
	}

	vector<double> xc;
	for ( size_t i = 0; i <= (size_t)this->order; i++ ) {

		xc.push_back ( gsl_poly_eval ( fxc.data()+(i*(order+1)), this->order+1, v ) );
	}

	return gsl_poly_eval ( xc.data(), this->order+1, u );
}


double ViewScreenConfiguredNDPlugin::fy ( const double u, const double v ) const {

	if ( this->order < 0 ) {
		return 0.;
	}

	vector<double> xc;
	for ( size_t i = 0; i <= (size_t)this->order; i++ ) {

		xc.push_back ( gsl_poly_eval ( fyc.data()+(i*(order+1)), this->order+1, v ) );
	}

	return gsl_poly_eval ( xc.data(), this->order+1, u );
}


double ViewScreenConfiguredNDPlugin::gu ( const double x, const double y ) const {

	if ( this->order < 0 ) {
		return 0.;
	}

	vector<double> vc;
	for ( size_t i = 0; i <= (size_t)this->order; i++ ) {

		vc.push_back ( gsl_poly_eval ( guc.data()+(i*(order+1)), this->order+1, y ) );
	}

	return gsl_poly_eval ( vc.data(), this->order+1, x );
}


double ViewScreenConfiguredNDPlugin::gv ( const double x, const double y ) const {

	if ( this->order < 0 ) {
		return 0.;
	}

	vector<double> vc;
	for ( size_t i = 0; i <= (size_t)this->order; i++ ) {

		vc.push_back ( gsl_poly_eval ( gvc.data()+(i*(order+1)), this->order+1, y ) );
	}

	return gsl_poly_eval ( vc.data(), this->order+1, x );
}

/* Output image -> beamspace */
void ViewScreenConfiguredNDPlugin::oimage_to_beamspace ( const double u, const double v, double &x, double &y ) const {

	x = this->xi + (u + 0.5)*(this->xf - this->xi)/this->nx;
	y = this->yf - (v + 0.5)*(this->yf - this->yi)/this->ny;
}

/* Beamspace -> output image */
void ViewScreenConfiguredNDPlugin::beamspace_to_oimage ( const double x, const double y, double &u, double &v ) const {

	u = this->nx*(x - this->xi)/(this->xf - this->xi) - 0.5;
	v = this->ny*(this->yf - y)/(this->yf - this->yi) - 0.5;
}

/* Input image (ccd) -> beamspace */
void ViewScreenConfiguredNDPlugin::iimage_to_beamspace ( const double u, const double v, double &x, double &y ) const {

	x = this->fx ( u, v );
	y = this->fy ( u, v );
}

/* Beamspace -> input image (ccd) */
void ViewScreenConfiguredNDPlugin::beamspace_to_iimage ( const double x, const double y, double &u, double &v ) const {

	u = this->gu ( x, y );
	v = this->gv ( x, y );
}


double ViewScreenConfiguredNDPlugin::ccd_area_covered_by_output_pixel ( const double u, const double v ) const {

	array< tuple<double, double>, 5 > deltas = { make_tuple(0.,0.), make_tuple(-0.5,0.5), make_tuple(-0.5,-0.5), make_tuple(0.5,0.5), make_tuple(0.5,-0.5) };

	array<tuple<double,double>,5> points;

	// transform the corners of the quadrilateral centred on (u,v) to beam space
	transform ( deltas.begin(), deltas.end(), points.begin(),
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		to_beamspace ( u, v, *this )
		#else
		[this,u,v] (tuple<double,double> &delta) {
			double x, y;
			this->oimage_to_beamspace ( u + get<0>(delta), v + get<1>(delta), x, y );
			return make_tuple ( x, y );
		}
		#endif
	);

	/*cout << "The xy points are: ";
	for_each ( points.begin()+1, points.end(),
		[] (tuple<double,double> point) {

			cout << "(" << get<0>(point) << "," << get<1>(point) << "), ";
		}
	);
	cout << endl;*/

	// now transform them to input image (ccd) space
	transform ( points.begin(), points.end(), points.begin(),
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		to_imagespace ( *this )
		#else
		[this] (tuple<double,double> &xypoint) {
			double pu, pv;
			this->beamspace_to_iimage ( get<0>(xypoint), get<1>(xypoint), pu, pv );
			return make_tuple ( pu, pv );
		}
		#endif
	);

	/*cout << "The uv points are: ";
	for_each ( points.begin()+1, points.end(),
		[] (tuple<double,double> point) {

			cout << "{" << get<0>(point) << "," << get<1>(point) << "}, ";
		}
	);
	cout << endl;*/

	// sort the points in a clockwise order
	sort ( points.begin()+1, points.end(),
		#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
		by_clockwise_angle_about ( get<0>(points[0]), get<1>(points[0]) )
		#else
		[points] ( tuple<double,double> uv1, tuple<double,double> uv2 ) {
			const double uc = get<0>(points[0]);
			const double vc = get<1>(points[0]);
			// the "y" coordinate is reflected in the ccd coordinate system (y increases downward)
			const double angle1 = atan2 ( vc - get<1>(uv1), get<0>(uv1) - uc );
			const double angle2 = atan2 ( vc - get<1>(uv2), get<0>(uv2) - uc );
			return angle1 > angle2;
		}
		#endif
	);

	/*cout << "The sorted points are: ";
	for_each ( points.begin(), points.end(),
		[] (tuple<double,double> point) {

			cout << "{" << get<0>(point) << "," << get<1>(point) << "}, ";
		}
	);
	cout << endl;*/

	const double ACx = get<0>(points[3]) - get<0>(points[1]);
	const double ACy = get<1>(points[3]) - get<1>(points[1]);
	const double BDx = get<0>(points[4]) - get<0>(points[2]);
	const double BDy = get<1>(points[4]) - get<1>(points[2]);

	//cout << "AC: " << ACx << "," << ACy << "\t BD: " << BDx << "," << BDy << endl;

	const double area = 0.5 * fabs ( ACx*BDy - BDx*ACy );

	return area;
}


bool ViewScreenConfiguredNDPlugin::is_configured () const {

	ConfigurationStatus_t configuration_status = ConfigurationStatusUnconfigured;
	if ( asynSuccess != const_cast<ViewScreenConfiguredNDPlugin*>(this)->getIntegerParam ( this->ViewScreenConfiguredNDPluginConfigurationStatus, (int*)&configuration_status ) ) {

		return false;
	}

	return ( ConfigurationStatusConfigured == configuration_status );
}


/** Converts a tinyxml2::XMLError into an NDPluginMagnificationCorrectionConfigurationStatus_t
 */
ViewScreenConfiguredNDPlugin::ConfigurationStatus_t ViewScreenConfiguredNDPlugin::xmlerror_to_pluginstatus ( const XMLError xml_error ) {

	switch ( xml_error ) {

		case XML_ERROR_FILE_NOT_FOUND:
			return ConfigurationStatusXMLErrorFileNotFound;
		default:
			return ConfigurationStatusXMLError;
	}
}
