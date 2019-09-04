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

#pragma once
#include <string>
//
// Static class to access configuration file GMXXX.ini
//
namespace cps
{
	class GantryInfo
	{
		static int state;	// internal state :
							// 0=no file loaded, -1=file load fail, 1=file loaded
	public:
		static int load(int model_number); // load file GMXXX.ini, where XXX is model_number
		static int get(const char *key, std::string &value); // get string value
		static int get(const char *key, int &value);		// get integer value
		static int get(const char *key, float &value);		// get float value
		static int get(const char *format, int index, std::string &value); // get string value from array 
		static int get(const char *format, int index, int &value);		// get integer value from array
		static int get(const char *format, int index, float &value);		// get float value from array
	};
}
