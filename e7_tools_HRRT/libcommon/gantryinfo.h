// GantryInfo : Defines the entry point for the console application.
//
/*[ gantryinfo.h
------------------------------------------------

   Component       : HRRT

   Name            : gantryinfo.h - GantryInfo class interface
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 11/25/2003
   Description     :  GantryInfo class loads configuration table "GM328.INI" in memory
					  and provides methods to get the values.

               Copyright (C) CPS Innovations 2003 All Rights Reserved.

---------------------------------------------------------------------*/

#ifndef cps_gantry_info
#define cps_gantry_info

#include <string>
//
// Static class to access configuration file GMXXX.ini
//
namespace HRRT
{
    typedef enum { LSO_LSO=0,LSO_NAI=1, LSO_ONLY=2, LSO_GSO=3, LSO_LYSO=4 } HeadType;
	class GantryInfo
	{
		static int state;	// internal state :
							// 0=no file loaded, -1=file load fail, 1=file loaded
	public:
    typedef enum { UnkownCrystal=0, LSO=1, LYSO=2, GSO=3, BGO=4, NAI=5} CrystalType;
    static bool initialized();
		static int load(int model_number); // load file GMXXX.ini, where XXX is model_number

		static int get(const char *key, std::string &value); // get string value
		static int get(const char *key, int &value);		// get integer value
		static int get(const char *key, float &value);		// get float value
		static int get(const char *format, int index, std::string &value); // get string value from array 
		static int get(const char *format, int index, int &value);		// get integer value from array
		static int get(const char *format, int index, float &value);		// get float value from array

		static int set(const char *key, std::string value); // get string value
		static int set(const char *key, float value);		// set float value

        // shortcuts
    static int axialBlocks();
    static int axialCrystals();
    static float binSize();
    static int crystalLayers();
    static int crystalLayerMaterial(int layer);
    static float crystalLayerFWHMErgRes(int layer);
    static float crystalLayerBackgroundErgRatio(int layer);
    static float crystalLayerDepth(int layer);
    static int defaultAngles();
    static int defaultElements();
    static int defaultLLD();
    static int defaultPlanes();
    static int defaultULD();
    static int directPlanes();
    static float DOI();
    static int mrd();
    static float ringRadius();
    static float intrinsicTilt();
    static float planeSep();
		static float maxScatterZfOV();
    static int span();
    static int transBlocks();
		static int transCrystals();
	};
}
#endif
