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

#include "gantryinfo.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <iostream>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif
#define LINE_SIZE 256

using namespace std;
using namespace HRRT;

int GantryInfo::state = 0;

typedef map <string,string> InfoMap;
static InfoMap gantry_info;
static int current_model = -1;

bool GantryInfo::initialized() { return state == 1; }
/**
 * Loads the gmXXX.ini file from the directory specified by GMINI environment variable,
 * where XXX is the model_number.
 * Returns the table size on success fails, otherwise set the state to fail (-1) and returns -1.
 */
int GantryInfo::load(int model_number)
{
  char filename[FILENAME_MAX];
  char line[LINE_SIZE];
  const char *gmini_dir=NULL;
  FILE *fp=NULL;
  if (state < 0) 
    return state;
  if (state>0) {
    if (model_number == current_model) 
      return gantry_info.size();
    else 
      gantry_info.clear();
  }
  if ((gmini_dir = getenv("GMINI")) != NULL) {
#ifdef unix
    sprintf(filename, "%s/gm%d.ini", gmini_dir, model_number);
#else
    sprintf(filename, "%s\\gm%d.ini", gmini_dir, model_number);
#endif
    fp = fopen(filename,"rb");
  } else {
    std::cerr << "ERROR: gantryinfo.cpp: GantryInfo::load(" << model_number << "): envt var GMINI is not set: Exiting" << std::endl;
    exit(-1);
  }
  if (fp == NULL) {
    std::cerr << "ERROR: gantryinfo.cpp: GantryInfo::load(" << model_number << "): could not open GMINI file " << filename <<" : Exiting" << std::endl;   
    exit(-1);
  }
  while (fgets(line,LINE_SIZE,fp) != NULL)
    {
      line[strlen(line)] = '\0'; //clear endl
      char *end = line + strlen(line);
      char *w1 = line;
      char *sep = strchr(line,'=');
      if (sep == NULL) 
	continue;
      while (isspace(*w1) && w1<sep) 
	w1++;
      if (w1==sep) 
	continue;
      char *p = sep-1;
      while (isspace(*p)) 
	*p-- = '\0';
      *sep = '\0';
      char *w2 = sep+1;
      while (isspace(*w2) && w2<end) 
	w2++;
      if (w2==end) 
	continue;
      end--;
      while (isspace(*end)) 
	*end-- = '\0';
      string key = w1, value = w2;
      gantry_info.insert(make_pair(key,value));
    }
  fclose(fp);
  state = 1;
  return gantry_info.size();
}

/**
 *	Get a string value
  * Returns 1 if found or 0 otherwise
 */
int GantryInfo::get(const char *key, string &value)
{
	string skey = key;
	InfoMap::iterator i = gantry_info.find(skey);
	if (i != gantry_info.end()) 
	{
		value = i->second;
		return 1;
	}
	return 0;
}

/**
 *	Get an integer value
 *  Returns 1 if found or 0 otherwise
 */
int GantryInfo::get(const char *key, int &ivalue)
{
	string value;
	if (get(key, value))
	{
		if (sscanf(value.c_str(),"%d",&ivalue) == 1) return 1;
	}
	return 0;
}

/**
 *	Get an float value
 *  Returns 1 if found or 0 otherwise
 */
int GantryInfo::get(const char *key, float &fvalue)
{
	string value;
	if (get(key, value))
	{
		if (sscanf(value.c_str(),"%g",&fvalue) == 1) return 1;
	}
	return 0;
}


int GantryInfo::get(const char *format, int index, string &value)
{
	char key[LINE_SIZE];
	sprintf(key, format, index);
	return get(key, value);
}

int GantryInfo::get(const char *format, int index, float &value)
{
	char key[LINE_SIZE];
	sprintf(key, format, index);
	return get(key, value);
}

int GantryInfo::get(const char *format, int index, int &value)
{
	char key[LINE_SIZE];
	sprintf(key, format, index);
	return get(key, value);
}

/**
 *	Set a string value
  * Returns 1 if found or 0 otherwise
 */
int GantryInfo::set(const char *key, string value)
{
	string skey = key;
	InfoMap::iterator i = gantry_info.find(skey);
	if (i != gantry_info.end()) 
	{
    i->second = value;
		return 1;
	}
  gantry_info.insert(make_pair(key,value));
	return 0;
}
/**
 *	set an float value
 *  Returns 1 if found or 0 otherwise
 */
int GantryInfo::set(const char *key, float fvalue)
{
  char tmp[40];
  sprintf(tmp,"%g", fvalue);
  return set(key,tmp);
}


// shortcuts
float GantryInfo::binSize()
{
    float binsize=0.121875f; // default value in cm
    get("binSize",binsize);
    return binsize*10.0f;   // value in mm
}

int GantryInfo::crystalLayerMaterial(int layer)
{
    string s;
    if (layer<crystalLayers())
    {
        if (get("crystalLayerMaterial[%d]", layer, s))
        {
            if (strcasecmp(s.c_str(),"LSO") == 0) return GantryInfo::LSO;
            if (strcasecmp(s.c_str(),"LYSO") == 0) return GantryInfo::LYSO;
            if (strcasecmp(s.c_str(),"GSO") == 0) return GantryInfo::GSO;
        }
    }
    return GantryInfo::UnkownCrystal;
}

int GantryInfo::crystalLayers()
{
    int nlayers=2; // defualt value
    get("nCrystalLayers",nlayers);
    return nlayers;  
}

float GantryInfo::crystalLayerFWHMErgRes(int layer)
{
    float er=17.0f;
    if (layer<crystalLayers())
        get("crystalLayerFWHMErgRes[%d]", layer, er);
    return er;
}

float GantryInfo::crystalLayerBackgroundErgRatio(int layer)
{
    float bg=0.0;
    if (layer<crystalLayers())
        get("crystalLayerBackgroundErgRatio[%d]", layer, bg);
    return bg;
}

float GantryInfo::crystalLayerDepth(int layer)
{
    float depth=0.0;
    if (layer<crystalLayers())
        get("crystalLayerDepth[%d]", layer, depth);
    return depth;

   // return 1.0;
}

float GantryInfo::ringRadius()
{
    float radius=23.45f; // default value
    get("crystalRadius",radius);
    return radius*10.0f; //in mm    
}

int GantryInfo::defaultAngles()
{
    int nviews=288; // default value
    get("defaultAngles",nviews);
    return nviews;  
}

int GantryInfo::defaultElements()
{
    int nprojs=256; // default value
    get("defaultElements",nprojs);
    return nprojs;  
}

int GantryInfo::defaultLLD()
{
    int lld=400; // default value
    get("defaultLLD",lld);
    return lld;  
}

int GantryInfo::defaultPlanes()
{
    int directPlanes=104; // default value
    get("directPlanes",directPlanes);
    return directPlanes*2-1;  
}

int GantryInfo::defaultULD()
{
    int uld=650; // default value
    get("defaultULD",uld);
    return uld;  
}

int GantryInfo::directPlanes()
{
    int c=104; // default value
    get("directPlanes",c);
    return c;  
}
float GantryInfo::DOI()
{
/*
    float doi=1.0f; // default value
    get("interactionDepth",doi);
    return doi*10.0f; //in mm  
*/
    return 0.0f;
}

float GantryInfo::maxScatterZfOV()
{
    float z=40.0f;
    get("maxScatterZfOV",z);
    return z*10.0f; //in mm    
}

int GantryInfo::transBlocks()
{
    int n=9;   // default value
    get("transBlocks", n);
    return n;
}
int GantryInfo::axialBlocks()
{
    int n=13;  // default value
    get("axialBlocks", n);
    return n;
}
int GantryInfo::transCrystals()
{
    int n=8;    // default value
    get("transCrystals", n);
    return n;
}
int GantryInfo::axialCrystals()
{
    int n=8;    // default value
    get("axialCrystals", n);
    return n;
}

int GantryInfo::mrd()
{
    int rd=67;    // default value
    get("ringDiff", rd);
    return rd;
}

int GantryInfo::span()
{
    int n=3;    // default value
    get("span", n);
    return n;
}

float GantryInfo::planeSep()
{
    float f=0.121875f; // default value in cm
    get("planeSep",f);
    return f*10.0f;   // value in mm
}

float GantryInfo::intrinsicTilt()
{
    float f=0.121875f; // default value in cm
    get("intrinsicTilt",f);
    return f*10.0f;   // value in mm
}
