//==========================================================================
// File			:	baseModel.cpp
//					Copyright 2002 by CTI
// Description	:	Contains the function implementation for the base class object
//	
// Author		:	Selene F. Tolbert
//
// Date			:	14 May 2002
//
// Author		Date			Update
// S Tolbert	14 May 02		Created
// S Tolbert	02 July 02		Added cpCommTypeDesc function
// S Tolbert	19 July 02		Changed return value for function headRotation
//								from int to bool
// S Tolbert	19 July 02		Changed int to long (32 bit signed integer)
// S Tolbert	24 July 02		Changed the 0 to -1 as invalid values
// S Tolbert	15 August 02	Updated APIs.
// S Tolbert	25 Nov 02		Added virtual to the destructor
// S Tolbert	25 Nov 02		Changed UINT (insigned int) to long
//								Eliminated #define UINT signed int
// S Tolbert	02 Dec 02		Added function maxBedTrav
// S Tolbert	18 Feb 03		Added new Panel scanner APIs
// S Tolbert	16 March 03		Added new APIs cfdSetupSet, tdcGainSet,
//								tdcOffsetSet and tdcSetupSet
// S Tolbert	17 Match 03		Added new APIs resetWaitDur, txCFDIter,
//								txCFDSt, emModPair, txModPair. emtxModPair
//==========================================================================

// Set guards to prevent multiple header file inclusion
//
#ifndef BASE_GANTRY_MODEL_H
#define BASE_GANTRY_MODEL_H

#include <cstdio>
#include "gantryModel.h"	// S Tolbert 10 July 02 - used to be gantryModelDef.h 

#ifdef WIN32
#ifdef BASEMODEL_EXPORTS
#define BASEMODEL_API __declspec(dllexport)
#else
#define BASEMODEL_API __declspec(dllimport)
#endif
#endif

//
// CModel base class - contains 7X, 8X and Panel information (Polymorphism)
//

#ifdef WIN32
class BASEMODEL_API CModel {
#endif
#if defined(__linux__) || defined(__SOLARIS__)
class CModel {
#endif
public:
	// initialization
	CModel();
	virtual ~CModel();
	
	// virtual function
	virtual char* mName();
	virtual long model() const { return 0l; }
	char* iniName();
	bool openINI();				// called by the derived objects
	void closeINI();
	bool validateModelKey(long); 
	char* locateToken(const char*, long, const char*, char*);

	// Important: if new models get added, this function values returns and models array needs to be updated.
	long totModels() const { return 14;}
	long* modelNums();

	// Miscellaneous 
	virtual char* logdir();
	virtual char* setupdir();

	virtual bool isRingScanner() const { return 0; }
	virtual bool isHeadScanner() const { return 0; }

	virtual double planeSeparation() const { return -1; }
	virtual long nBlocks() const { return -1; }
	virtual double blkDepth() const { return -1; }
	virtual long angBlk() const { return -1; }
	virtual long totBlk() const { return -1; }
	virtual long blkCrys() const { return -1; }
	virtual long logAngBlks() const { return -1; }
	virtual long logAxialBlks() const { return -1; }
	virtual long logBlks() const { return -1; }
	virtual long* logBlkMap();
	virtual long nBukets() const { return -1; }
	virtual long bktDbSize() const { return -1; }
	virtual long bktDBStart() const { return -1; }
	virtual long bktRings() const { return -1; }
	virtual long bktsPerRing() const { return -1; }
	virtual long bktOutputChannels() const { return -1; } // single channel=1, dual channel=2
	virtual long logBkts() const { return -1; }
	virtual long bktsInCoinc() const { return -1; }
	virtual long defLLD() const { return -1; }
	virtual long defTxULD() const { return -1; }
	virtual long defULD() const { return -1; }
	virtual long defAngles() const { return -1; }
	virtual long defElements() const { return -1; }
	virtual long defAngComp() const { return -1; }
	virtual long defPreInjTxLLD() const { return -1; }
	virtual long defPostInjTxLLD() const { return -1; }
	virtual long defScatThresh() const { return -1; }
	virtual double defAxialFOV() const { return -1; }
	virtual double defTransFOV() const { return -1; }
	virtual long def2DSpan() const { return -1; }
	virtual long def3DSpan() const { return -1; }
	virtual long defMaxRingDiff() const { return -1; }
	virtual long defMaxTxRingDiff() const { return -1; }
	virtual double interacDepth() const { return -1; }
	virtual long transBlks() const { return -1; }
	virtual long axialBlks() const { return -1; }
	virtual long angCrystals() const { return -1; }
	virtual long axialCryst() const { return -1; }
	virtual long totCryst() const { return -1; }
	virtual double crystRad() const { return -1; }
	virtual double crystSize() const { return -1; }
	virtual long plnCryst() const { return -1; }
	virtual long dPlanes() const { return -1; }
	virtual double binSz() const { return -1; }
	virtual long nTubes() const { return -1; }
	virtual long axTubes() const { return -1; }
	virtual long transTubes() const { return -1; }
	virtual long tubePlns() const { return -1; }
	virtual long totTubes() const { return -1; }
	virtual long* tubeMaps();
	virtual long timeCorrectionBits() const { return -1; }
	virtual long maxCodePg() const { return -1; }
	virtual long minElems() const { return -1; }
	virtual EN_PositionProfileOrder posProfileOrder() const { return PPO_LR; }
	virtual long txSourceType() const { return -1; }
	virtual double txSourceRad() const { return -1; }
	virtual EN_SeptaType septaTyp() const { return SPT_None; }
	virtual double maxScanLen() const { return -1; }
	virtual double pileUpCor() const { return -1; }
	virtual double plnCor() const { return -1; }
	virtual long rodOffSet() const { return -1; }
	virtual double hBedStp() const { return -1; }
	virtual double vBedStp() const { return -1; }
	virtual long intrTilt() const { return 0; }
	virtual long defWobbleSp() const { return -1; }
	virtual long tZero() const { return -1; }
	virtual long rZero() const { return -1; }
	virtual long wZero() const { return -1; }
	virtual long optBedOverlap() const { return -1; }
	virtual long optBedOverlap3D() const { return -1; }
	virtual long isprt() const { return -1; }
	virtual EN_ExtraReconCor corMsk() const { return ERC_NoExtraReconCor; }
	virtual long analogAsicBkt() const { return -1; }
	virtual long nCassettes() const { return 0; }
	virtual long coincWindow() const { return -1; }
	virtual long nContrls() const { return 0; }
	virtual pcontroller Contrls();
	virtual bool isContrlInstalled(EN_Controller) const { return false; }
	virtual bool isContrlInstalled(char*) const { return false; }
	virtual long numberCrystaLayers() const { return -1; }
	virtual double maxBedTrav() const { return -1; }
	virtual double LayerCrystalDepth(long); 
	virtual long LayerBackErg(long);
	virtual double LayerfwhmErgRes(long);
	virtual char* layerMat(long);

	// panel models extensions
	virtual long totSinograms() const { return -1; }
	virtual char* coincProcName();
	virtual long cpCommunications() const { return -1; }
	virtual char* cpCommTypeDesc();
	virtual long headRotation() const { return 0; }
	virtual long numberPanels() const { return 0; }
	virtual pPanel panels(); 
	virtual long numberPointSources(long);
	virtual long ptSrcStart(long);
	virtual pPtSrc headPtSrc(long);
	virtual long numberConfigs() const {return 0; }
	virtual pConf configurations();
	virtual long numberAnalogSet() const { return -1; }
	virtual long LLDprof() const { return -1; }
	virtual long ULDprof() const { return -1; }
	virtual long LLDshp() const { return -1; }
	virtual long ULDshp() const { return -1; }
	virtual long LLDtubeEner() const { return -1; }
	virtual long ULDtubeEner() const { return -1; }
	virtual long LLDcrystalEner() const { return -1; }
	virtual long ULDcrystalEner() const { return -1; }
	virtual long pmtaSet() const { return -1; }
	virtual long pmtbSet() const { return -1; }
	virtual long pmtcSet() const { return -1; }
	virtual long pmtdSet() const { return -1; }
	virtual long cfdSet() const { return -1; }
	virtual long cfdDelaySet() const { return -1; }
	virtual long xOffsetSet() const { return -1; }
	virtual long yOffsetSet() const { return -1; }
	virtual long eOffsetSet() const { return -1; }
	virtual long dhiModeSet() const { return -1; }
	virtual long engSetupSet() const { return -1; }
	virtual long thresholdShapeSet() const { return -1; }
	virtual long slowLowSet() const { return -1; }
	virtual long slowHighSet() const { return -1; }
	virtual long fastLowSet() const { return -1; }
	virtual long fastHighSet() const { return -1; }
	virtual long fineGainIter() const { return -1; }
	virtual long tubeGainIter() const { return -1; }
	virtual long cfdIter() const { return -1; }
	virtual long offsetmin() const { return -1; }
	virtual long offsetmax() const { return -1; }
	virtual long mincfddelay() const { return -1; }
	virtual long maxcfddelay() const { return -1; }
	virtual double maxScattzfov() const { return -1; }
	virtual long enerOffset() const { return -1; }
	virtual long rEmLUTMode0Span() const { return -1; }	
	virtual long rEmLUTMode0RingDiff() const { return -1; }
	virtual long rEmLUTMode1Span() const { return -1; }		
	virtual long rEmLUTMode1RingDiff() const { return -1; }
	virtual long rTxLUTMode0Span() const { return -1; }	
	virtual long rTxLUTMode0RingDiff() const { return -1; }
	virtual long rTxLUTMode1Span() const { return -1; }			
	virtual long rTxLUTMode1RingDiff() const { return -1; }
	virtual long numberPolySides() const { return -1; }
	virtual long timBins() const { return -1; }
	virtual long minGain() const { return -1; }
	virtual long maxGain() const { return -1; }
	virtual long stepGain() const { return -1; }
	virtual long maxExp() const { return -1; }
	virtual double emissionCFDRatio() const { return -1; }
	virtual double transCFDRatio() const { return -1; }
	virtual long emissionTimeAlignDur() const { return -1; }
	virtual long transTimeAlignDur() const { return -1; }
	virtual long timeAlignIter() const { return -1; }
	virtual long cfdSetupSet() const { return -1; }
	virtual long tdcGainSet() const { return -1; }
	virtual long tdcOffsetSet() const { return -1; }
	virtual long tdcSetupSet() const { return -1; }
	virtual long txCFDIter() const { return -1; }
	virtual long txCFDSt() const { return -1; }
	virtual long emModPair() const { return -1; }
	virtual long txModPair() const { return -1; }
	virtual long emtxModPair() const { return -1; }
	virtual long resetWaitDur() const { return -1; }

private:
	pPanel	installedPanel;	
	pConf	installedConfig;
	pPtSrc	installedPtSrc;
	char	cINIFile[BSIZE];
	char	cKey[BSIZE];
	char	layerMaterial[BSIZE];
	FILE	*m_pINIFile;
	long	nPointSources;
	long	pointSrcStart;
};

#endif
