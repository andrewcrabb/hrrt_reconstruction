//==========================================================================
// File			:	panelModel2393.h
//					Copyright 2002 by CTI
// Description	:	Contains the function prototypes for the derived class
//					Phase II Electronics
//	
// Author		:	Selene F. Tolbert
//
// Date			:	12 March 2003
//
// Author		Date			Update
// S Tolbert	12 March 03		Created
// S Tolbert	16 March 03		New APIs added
// S Tolbert	17 March 03		New APIs added
//==========================================================================

// Set guards to prevent multiple header file inclusion
//
#ifndef MODEL2393_CLASS_H
#define MODEL2393_CLASS_H

#ifdef WIN32
#ifdef GM_EXPORTS
#define MODEL2393_API __declspec(dllexport)
#else
#define MODEL2393_API __declspec(dllimport)
#endif
#endif

#include "baseModelClass.h"	// base class and interface class
#include "gantryModel.h"

//
// CPanelModel2393 derived class
//

class CPanelModel2393 : public CModel {
public:
	CPanelModel2393();
	virtual ~CPanelModel2393();

	// access functions
	virtual long model() const { return 2393; }
	virtual char* mName();

	// ini information
	void getINIVals();
	void initVals();

	// Miscellaneous 
	virtual char* logdir();
	virtual char* setupdir();

	virtual bool isHeadScanner() const { return 1; }

	virtual long nBlocks() const { return (_nblocks != BADINIKEY) ? _nblocks : 84; }
	virtual long defLLD() const { return (_LLD != BADINIKEY) ? _LLD : 400; }
	virtual long defULD() const { return (_ULD != BADINIKEY) ? _ULD : 650; }
	virtual long defAngles() const { return (_angles != BADINIKEY) ? _angles : 256; }
	virtual long defElements() const { return (_elements != BADINIKEY) ? _elements : 320; }
	virtual long totSinograms() const { return (_totSino != BADINIKEY) ? _totSino : 3935; }
	virtual long transBlks() const { return (_tranBlk != BADINIKEY) ? _tranBlk : 7; }
	virtual long axialBlks() const { return (_axialBlk != BADINIKEY) ? _axialBlk : 10; } 
	virtual long angCrystals() const { return (_angCry != BADINIKEY) ? _angCry : 12; }
	virtual long axialCryst() const { return (_axialCry != BADINIKEY) ? _axialCry : 12; }
	virtual long nTubes() const { return (_ntbs != BADINIKEY) ? _ntbs : 4; }
	virtual long isprt() const { return (_isPRT != BADINIKEY) ? _isPRT : 1; }
	virtual long axTubes() const { return (_axTubes != BADINIKEY) ? _axTubes : 2; }
	virtual long transTubes() const { return (_transTubes != BADINIKEY) ? _transTubes : 2; }
	virtual long bktOutputChannels() const { return (_outchnl != BADINIKEY) ?  _outchnl : 1; } // single channel=1, dual channel=2
	virtual long timeCorrectionBits() const { return (_timecorbit != BADINIKEY) ? _timecorbit : 5; }
	virtual double crystRad() const { return (_crystalRad != BADINIKEY) ? _crystalRad : (43.6); }		
	virtual double crystSize() const { return (_crystalSize != BADINIKEY) ? _crystalSize : 2.0f; }
	virtual double planeSeparation() const { return (_plnSep != BADINIKEY) ? _plnSep : 0.2208; }
	virtual long dPlanes() const { return (_dPlane != BADINIKEY) ? _dPlane : 120; }
	virtual double binSz() const { return (_binSize != BADINIKEY) ? _binSize : 0.22; }
	virtual long rEmLUTMode0Span() const { return (_EmMode0Span != BADINIKEY) ? _EmMode0Span : 7; }	
	virtual long rEmLUTMode0RingDiff() const { return (_EmMode0RingDiff != BADINIKEY) ? _EmMode0RingDiff : 87; }
	virtual long rEmLUTMode1Span() const { return (_EmMode1Span != BADINIKEY) ? _EmMode1Span : 9;  }		
	virtual long rEmLUTMode1RingDiff() const { return (_EmMode1RingDiff != BADINIKEY) ? _EmMode1RingDiff : 85; }
	virtual long rTxLUTMode0Span() const { return (_TxMode0Span != BADINIKEY) ? _TxMode0Span : 7; }	
	virtual long rTxLUTMode0RingDiff() const { return (_TxMode0RingDiff != BADINIKEY) ? _TxMode0RingDiff : 3; }
	virtual long rTxLUTMode1Span() const { return (_TxMode1Span != BADINIKEY) ? _TxMode1Span : 9; }			
	virtual long rTxLUTMode1RingDiff() const { return (_TxMode1RingDiff != BADINIKEY) ? _TxMode1RingDiff : 4; }		
	virtual long numberPolySides() const { return (_polySides != BADINIKEY) ? _polySides : 6; }
	virtual long optBedOverlap3D() const { return (_optBedOverlap3D != BADINIKEY) ? _optBedOverlap3D : 75; }
	virtual long coincWindow() const { return (_coinWnd != BADINIKEY) ? _coinWnd : 6; }
	virtual char* coincProcName();
	virtual long numberPanels() const { return (_nheads != BADINIKEY) ? _nheads : 3; }
	virtual long cpCommunications() const { return (_cpComm != BADINIKEY) ? _cpComm : 0; }
	virtual char* cpCommTypeDesc();	
	virtual long headRotation() const { return (_headRot != BADINIKEY) ? _headRot:1; }
	virtual pPanel panels();
	virtual long numberConfigs() const { return (_config != BADINIKEY) ?  _config : 1; }
	virtual pConf configurations();
	virtual long numberAnalogSet() const { return (_analogSet != BADINIKEY) ? _analogSet : 10; }
	virtual long LLDprof() const { return (_LLDprofile != BADINIKEY) ?  _LLDprofile : 80; }
	virtual long ULDprof() const { return (_ULDprofile != BADINIKEY) ? _ULDprofile : 255; }
	virtual long LLDshp() const { return (_LLDshape != BADINIKEY) ? _LLDshape : 80; }
	virtual long ULDshp() const { return (_ULDshape != BADINIKEY) ? _ULDshape : 255; }
	virtual long LLDtubeEner() const { return (_LLDtubeEnergy != BADINIKEY) ? _LLDtubeEnergy : 0; }
	virtual long ULDtubeEner() const { return (_ULDtubeEnergy != BADINIKEY) ? _ULDtubeEnergy : 255; }
	virtual long LLDcrystalEner() const { return (_LLDcryEnergy != BADINIKEY) ? _LLDcryEnergy : 70; }
	virtual long ULDcrystalEner() const { return (_ULDcryEnergy != BADINIKEY) ? _ULDcryEnergy : 255; }
	virtual long pmtaSet() const { return (_pmta != BADINIKEY) ? _pmta : 0; }
	virtual long pmtbSet() const { return (_pmtb != BADINIKEY) ? _pmtb : 1; }
	virtual long pmtcSet() const { return (_pmtc != BADINIKEY) ? _pmtc : 2; }
	virtual long pmtdSet() const { return (_pmtd != BADINIKEY) ? _pmtd : 3; }
	virtual long cfdSet() const { return (_cfdsetting != BADINIKEY) ? _cfdsetting : 4; }
	virtual long cfdDelaySet() const { return (_cfdsetdelay != BADINIKEY) ? _cfdsetdelay : -1; }
	virtual long xOffsetSet() const { return (_xOffset != BADINIKEY) ? _xOffset : -1; }
	virtual long yOffsetSet() const { return (_yOffset != BADINIKEY) ? _yOffset : -1; }
	virtual long eOffsetSet() const { return (_eOffset != BADINIKEY) ? _eOffset : -1; }
	virtual long dhiModeSet() const { return (_dhisetting != BADINIKEY) ? _dhisetting : -1; }
	virtual long engSetupSet() const { return (_engsetup != BADINIKEY) ? _engsetup : 9; }
	virtual long thresholdShapeSet() const { return (_thresholdshp != BADINIKEY) ? _thresholdshp : -1; }
	virtual long slowLowSet() const { return (_slowlow != BADINIKEY) ? _slowlow : -1; }
	virtual long slowHighSet() const { return (_slowhigh != BADINIKEY) ? _slowhigh : -1; }
	virtual long fastLowSet() const { return (_fastlow != BADINIKEY) ? _fastlow : -1; }
	virtual long fastHighSet() const { return (_fasthigh != BADINIKEY) ? _fasthigh : -1; }
	virtual long fineGainIter() const { return (_fgainiter != BADINIKEY) ? _fgainiter : 15; }
	virtual long tubeGainIter() const { return (_tgainiter != BADINIKEY) ? _tgainiter : 15; }
	virtual long cfdIter() const { return (_cfditer != BADINIKEY) ? _cfditer : 4; }
	virtual long offsetmin() const { return (_offsetmin != BADINIKEY) ? _offsetmin : 0; }
	virtual long offsetmax() const { return (_offsetmax != BADINIKEY) ? _offsetmax : 255; }
	//virtual long mincfddelay() const { return (_minCFDDelay != BADINIKEY) ? _minCFDDelay : 3; }
	//virtual long maxcfddelay() const { return (_maxCFDDelay != BADINIKEY) ? _maxCFDDelay : 123; }
	virtual double maxScattzfov() const { return (_maxScatterZfov != BADINIKEY) ? _maxScatterZfov : 54.0; }	
	virtual long enerOffset() const { return (_eneroffset != BADINIKEY) ? _eneroffset : 0; }
	virtual double interacDepth() const { return (_intDepth != BADINIKEY) ? _intDepth : 0; }
	virtual char* layerMat(long);
	virtual long numberCrystaLayers() const { return (_crystalLayer != BADINIKEY) ? _crystalLayer : 1; }
	virtual long LayerBackErg(long);
	virtual double LayerfwhmErgRes(long);
	virtual double LayerCrystalDepth(long);	
	virtual pPtSrc headPtSrc(long);
	virtual long numberPointSources(long);
	virtual long ptSrcStart(long);
	virtual long timBins() const { return (_timingBins != BADINIKEY) ? _timingBins : 32; }
	virtual long minGain() const { return (_minCoarseGain != BADINIKEY) ? _minCoarseGain : 8; }
	virtual long maxGain() const { return (_maxCoarseGain != BADINIKEY) ? _maxCoarseGain : 232; }
	virtual long stepGain() const { return (_stepCoarseGain != BADINIKEY) ? _stepCoarseGain : 8; }
	virtual long maxExp() const { return (_maxExpandCRM != BADINIKEY) ? _maxExpandCRM : 15; }
	virtual double emissionCFDRatio() const { return (_emCFDRatio != BADINIKEY) ? _emCFDRatio : 0.0025; }
	virtual double transCFDRatio() const { return (_txCFDRatio != BADINIKEY) ? _txCFDRatio : 0.01; }
	virtual long emissionTimeAlignDur() const { return (_emTimeAlignDuration != BADINIKEY) ? _emTimeAlignDuration : 600; }
	virtual long transTimeAlignDur() const { return (_txTimeAlignDuration != BADINIKEY) ? _txTimeAlignDuration : 30; }
	virtual long timeAlignIter() const { return (_timeAlignIter != BADINIKEY) ? _timeAlignIter : 4; }
	virtual long cfdSetupSet() const { return (_cfdSetupSetting != BADINIKEY) ? _cfdSetupSetting : 5; }
	virtual long tdcGainSet() const { return (_tdcGainSetting != BADINIKEY) ? _tdcGainSetting : 6; }
	virtual long tdcOffsetSet() const { return (_tdcOffsetSetting != BADINIKEY) ? _tdcOffsetSetting : 7; }
	virtual long tdcSetupSet() const { return (_tdcSetupSetting != BADINIKEY) ? _tdcSetupSetting : 8; }
	virtual long txCFDIter() const { return (_txcfditer != BADINIKEY) ? _txcfditer : 5; }
	virtual long txCFDSt() const { return (_txcfdstart != BADINIKEY) ? _txcfdstart : 110; }
	virtual long emModPair() const { return (_emModePair != BADINIKEY) ? _emModePair : 0xA0; }
	virtual long txModPair() const { return (_txModePair != BADINIKEY) ? _txModePair : 0xD40; }
	virtual long emtxModPair() const { return (_emtxModePair != BADINIKEY) ? _emtxModePair : 0xDE0; }
	virtual long resetWaitDur() const { return (_resetWaitDuration != BADINIKEY) ? _resetWaitDuration : 5; }

private:
	bool	bFoundPanelInfo;
	bool	bFoundConfigInfo;
	bool	bFoundPointSourceInfo;
	bool	bFoundPointSourceStartInfo;
	long	numberOfPanels;
	long	numberOfConfigs;
	long	numberOfPointSources;
	char*	pKey;
	char	cKey[BSIZE];
	char	Buffer[BUFSIZE];
	long	pointSourceStart;
	double	crystalDepth;
	long	layerBackErg;
	double	layerfwhmErgRes;
	char*	layerMaterial;

	// ini values
	long	_resetWaitDuration;
	long	_emtxModePair;
	long	_txModePair;
	long	_emModePair;
	long	_txcfdstart;
	long	_txcfditer;
	long	_tdcSetupSetting;
	long	_tdcOffsetSetting;
	long	_tdcGainSetting;
	long	_cfdSetupSetting;
	long	_timingBins;
	long	_minCoarseGain;
	long	_maxCoarseGain;
	long	_stepCoarseGain;
	long	_maxExpandCRM;
	double	_emCFDRatio;
	double	_txCFDRatio;
	long	_emTimeAlignDuration;
	long	_txTimeAlignDuration;
	long	_timeAlignIter;
	long	_coinWnd;
	long	_optBedOverlap3D;
	long	_polySides;
	long	_EmMode0Span;
	long	_EmMode0RingDiff;
	long	_EmMode1Span;
	long	_EmMode1RingDiff;
	long	_TxMode0Span;
	long	_TxMode0RingDiff;
	long	_TxMode1Span;
	long	_TxMode1RingDiff;
	double	_binSize;
	long	_dPlane;
	double	_plnSep;
	long	_defRingDiff;
	long	_defTxRingDiff;
	long	_ptSrcStart;
	long	_nPointSources;
	double	_layOneCrysDepth;
	double	_layTwoCrysDepth;
	double	_layOnefwhm;
	double	_layTwofwhm;
	long	_layTwoBackErg;
	long	_layOneBackErg;
	long	_crystalLayer;
	char	_layOneMat[BSIZE];
	char	_layTwoMat[BSIZE];
	double	_intDepth;
	char	_logDir[BSIZE];
	char	_setupDir[BSIZE];
	char	_cpname[BSIZE];
	char	_cpcommdesc[BSIZE];
	long	_eneroffset;
	long	_maxScatterZfov;
	double	_crystalSize;
	double	_crystalRad;
	long	_outchnl;
	long	_timecorbit;
	long	_minCFDDelay;
	long	_maxCFDDelay;
	long	_fgainiter;
	long	_tgainiter;
	long	_cfditer;
	long	_offsetmin;
	long	_offsetmax;
	long	_axTubes;
	long	_transTubes;
	long	_ntbs;
	long	_isPRT;
	long	_nblocks;
	long	_nheads;
	long	_config;
	long	_cpComm;
	long	_headRot;
	long	_LLD;
	long	_ULD;
	long	_angles;
	long	_elements;
	long	_totSino;
	long	_analogSet;
	long	_tranBlk;
	long	_axialBlk;
	long	_angCry;
	long	_axialCry;
	long	_LLDprofile;
	long	_ULDprofile;
	long	_LLDshape;
	long	_ULDshape;
	long	_LLDtubeEnergy;
	long	_ULDtubeEnergy;
	long	_LLDcryEnergy;
	long	_ULDcryEnergy;
	long	_pmta;
	long	_pmtb;
	long	_pmtc;
	long	_pmtd;
	long	_cfdsetting;
	long	_cfdsetdelay;
	long	_xOffset;
	long	_yOffset;
	long	_eOffset;
	long	_dhisetting;
	long	_engsetup;
	long	_thresholdshp;
	long	_slowlow;
	long	_slowhigh;
	long	_fastlow;
	long	_fasthigh;
	pPanel	installedPanel;
	pConf	installedConfig;
	pPtSrc	installedPtSrc;

};


#endif
