
#include "ViewScreenConfiguredNDPlugin.h"

#include <sys/stat.h>

// EPICS IOC shell functions for ViewScreenConfiguredNDPlugin
const iocshArg ViewScreenConfiguredNDPlugin::viewscreen_set_configuration_file_directory_Arg0 = { "directory name (ending with '/')", iocshArgString };
const iocshArg *ViewScreenConfiguredNDPlugin::viewscreen_set_configuration_file_directory_Args[] = { &viewscreen_set_configuration_file_directory_Arg0 };
const iocshFuncDef ViewScreenConfiguredNDPlugin::viewscreen_set_configuration_file_directory_FuncDef = { "viewscreen_set_configuration_file_directory", 1, viewscreen_set_configuration_file_directory_Args };
void ViewScreenConfiguredNDPlugin::viewscreen_set_configuration_file_directory ( const iocshArgBuf *args ) {

	directory_configuration_files = std::string ( args[0].sval );
	errlogSevPrintf ( errlogInfo, "viewscreen configuration file directory changed; directory = %s\n", directory_configuration_files.c_str() );
}

const iocshArg ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory_Arg0 = { "geometry (elbt, embt or elbt)", iocshArgString };
const iocshArg ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory_Arg1 = { "light distribution (scintillation or otr)", iocshArgString };
const iocshArg ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory_Arg2 = { "directory name (ending with /)", iocshArgString };
const iocshArg *ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory_Args[] = { &viewscreen_set_efficiency_map_directory_Arg0, &viewscreen_set_efficiency_map_directory_Arg1, &viewscreen_set_efficiency_map_directory_Arg2 };
const iocshFuncDef ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory_FuncDef = { "viewscreen_set_efficiency_map_directory", 3, ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory_Args };
void ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory ( const iocshArgBuf *args ) {

	std::string geometry ( args[0].sval );
	std::string light_distribution ( args[1].sval );
	std::string directory ( args[2].sval );

	if ( geometry.compare ( "elbt" ) && geometry.compare ( "embt" ) && geometry.compare ( "ehbt" ) ) {

		errlogSevPrintf ( errlogMinor, "directory location unchanged; unrecognized geometry type: %s\n", geometry.c_str() );
		return;
	}

	if ( light_distribution.compare ( "scintillation" ) && light_distribution.compare ( "otr" ) ) {

		errlogSevPrintf ( errlogMinor, "directory location unchanged; unrecognized light distribution type: %s\n", light_distribution.c_str() );
		return;
	}

	struct stat buffer;
	if ( (0 != stat ( directory.c_str(), &buffer )) || (!S_ISDIR ( buffer.st_mode )) ) {

		errlogSevPrintf ( errlogMinor, "directory location unchanged; location does not exist : %s\n", directory.c_str() );
		return;
	}

	efficiency_map_directories[make_pair ( geometry, light_distribution )] = directory;
	errlogSevPrintf ( errlogInfo, "efficiency map directory location updated; geometry: %s, light distribution: %s, directory: %s\n", geometry.c_str(), light_distribution.c_str(), directory.c_str() );
}

extern "C" void ViewScreenConfiguredNDPluginRegister ( void )
{
	iocshRegister ( &ViewScreenConfiguredNDPlugin::viewscreen_set_configuration_file_directory_FuncDef, ViewScreenConfiguredNDPlugin::viewscreen_set_configuration_file_directory );
	iocshRegister ( &ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory_FuncDef, ViewScreenConfiguredNDPlugin::viewscreen_set_efficiency_map_directory );
}

extern "C" {
	epicsExportRegistrar ( ViewScreenConfiguredNDPluginRegister );
}
