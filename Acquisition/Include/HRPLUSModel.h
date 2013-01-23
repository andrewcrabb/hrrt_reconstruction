//==========================================================================
// File			:	HRPLUSModel.h
//					Copyright 2002 by CTI
// Description	:	Contains the function prototypes for the derived class
//	
// Author		:	Selene F. Tolbert
//
// Date			:	16 July 2002
//
// Author		Date			Update
// S Tolbert	16 July 02		Created
// S Tolbert	19 July 02		Changed int to long (32 bit signed integer)
// S Tolbert	08 August 02	Added INI file access
// S Tolbert	15 August 02	Updated APIs.
// S Tolbert	25 Nov 02		Added virtual to the destructor
// S Tolbert	25 Nov 02		Added ini access to maxScattzfov data
//==========================================================================

// Set guards to prevent multiple header file inclusion
//
#ifndef MODELHRPLUS_CLASS_H
#define MODELHRPLUS_CLASS_H

#ifdef WIN32
#ifdef GM_EXPORTS
#define MODELHRPLUS_API __declspec(dllexport)
#else
#define MODELHRPLUS_API __declspec(dllimport)
#endif
#endif

#include "baseModelClass.h"	// base class and longerface class
#include "gantryModel.h"

//
// CHRRTModel derived class
//

class CHRPLUSModel : public CModel {
public:
	CHRPLUSModel();
	virtual ~CHRPLUSModel();

	// access functions
	virtual long model() const { return 1062; }
	virtual char* mName();
	void calculate();

	// ini information
	void getINIVals();
	void initVals();

	// Miscellaneous 
	virtual char* logdir();
	virtual char* setupdir();

	virtual bool isRingScanner() const { return 1; }

	virtual long nBlocks() const { return (_nblocks != BADINIKEY) ? _nblocks : nblocks; }
	virtual double blkDepth() const { return (_blkDepth != BADINIKEY) ? _blkDepth : 3.0f; }
	virtual long angBlk() const { return (_angBlock != BADINIKEY) ? _angBlock : 3; }
	virtual long totBlk() const { return (_totBlock != BADINIKEY) ? _totBlock : ntotalBlocks; }
	virtual long blkCrys() const { return (_blkCrystal != BADINIKEY) ? _blkCrystal : nblockCrystals; }
	virtual long logAngBlks() const { return (_logAngBlks != BADINIKEY) ? _logAngBlks : nlogicalAngularBlocks; }
	virtual long logAxialBlks() const { return (_logAxBlks != BADINIKEY) ? _logAxBlks : 4; }
	virtual long logBlks() const { return (_logBlks != BADINIKEY) ? _logBlks : nlogicalBlocks; }
	virtual long* logBlkMap();
	virtual long nBukets() const { return (_nbuckets != BADINIKEY) ? _nbuckets : nbuckets; }
	virtual long bktDbSize() const { return (_bktDbSz != BADINIKEY) ? _bktDbSz : 0x1600; }
	virtual long bktDBStart() const { return (_bktDBSt != BADINIKEY) ? _bktDBSt : 0x2000; }
	virtual long bktRings() const { return (_bktRings != BADINIKEY) ? _bktRings : 1; }
	virtual long bktsPerRing() const { return (_bktsPerRing != BADINIKEY) ? _bktsPerRing : 24; }
	virtual long bktOutputChannels() const { return (_bktOutChnl != BADINIKEY) ? _bktOutChnl : 1; } // single channel=1, dual channel=2
	virtual long logBkts() const { return (_logBkts != BADINIKEY) ? _logBkts : 12; }
	virtual long bktsInCoinc() const { return (_bktsInCoinc != BADINIKEY) ? _bktsInCoinc : 13; }
	virtual long defLLD() const { return (_LLD != BADINIKEY) ? _LLD : 350; }
	virtual long defTxULD() const { return (_defTxULD != BADINIKEY) ? _defTxULD : 650; }
	virtual long defULD() const { return (_ULD != BADINIKEY) ? _ULD : 650; }
	virtual long defAngles() const { return (_angles != BADINIKEY) ? _angles : 288; }
	virtual long defElements() const { return (_elements != BADINIKEY) ? _elements : 288; }
	virtual long defAngComp() const { return (_defAngComp != BADINIKEY) ? _defAngComp : 0; }		// default mash value
	virtual long defPreInjTxLLD() const { return (_preInjTxLLD != BADINIKEY) ? _preInjTxLLD : 350; }
	virtual long defPostInjTxLLD() const { return (_postInjTxLLD != BADINIKEY) ? _postInjTxLLD : 350; }
	virtual long defScatThresh() const { return (_scatThresh != BADINIKEY) ? _scatThresh : 0; }
	virtual double defAxialFOV() const { return (_defAxFOV != BADINIKEY) ? _defAxFOV : defAxFOV; }
	virtual double defTransFOV() const { return (_defTransFOV != BADINIKEY) ? _defTransFOV : 58.3f; }
	// S Tolbert 15 July 02 - use SegmentTable.cpp to get total sinograms for the ring topography scanners
	//virtual long totSinograms() const { return readINI? _totSino : 0; }
	virtual long def2DSpan() const { return (_2DSpan != BADINIKEY) ? _2DSpan : 15; }
	virtual long def3DSpan() const { return (_3DSpan != BADINIKEY) ? _3DSpan : 9; }
	virtual long defMaxRingDiff() const { return (_defMaxRingDiff != BADINIKEY) ? _defMaxRingDiff : 22; }
	virtual long defMaxTxRingDiff() const { return (_defMaxTxRingDiff != BADINIKEY) ? _defMaxTxRingDiff : 7; }
	virtual double maxScattzfov() const { return (_maxScatterZfov != BADINIKEY) ? _maxScatterZfov : 40.0; }
	virtual double interacDepth() const { return (_intDepth != BADINIKEY) ? _intDepth : 1.3; }
	virtual long transBlks() const { return (_tranBlk != BADINIKEY) ? _tranBlk : 3; }
	virtual long axialBlks() const { return (_axialBlk != BADINIKEY) ? _axialBlk : 4; } 
	virtual long angCrystals() const { return (_angCry != BADINIKEY) ? _angCry : 8; }
	virtual long axialCryst() const { return (_axialCry != BADINIKEY) ? _axialCry : 8; }
	virtual long totCryst() const { return (_totCrystals != BADINIKEY) ? _totCrystals : ntotalCrystals; }
	virtual double crystRad() const { return (_crystalRad != BADINIKEY) ? _crystalRad : 41.25; }
	virtual long plnCryst() const { return (_planeCrystal != BADINIKEY) ? _planeCrystal : nplaneCrystals; }
	virtual long dPlanes() const { return (_dPlane != BADINIKEY) ? _dPlane : dirPlanes; }
	virtual double planeSeparation() const { return (_plnSep != BADINIKEY) ? _plnSep : 0.2425f; }
	virtual double binSz() const { return (_binSize != BADINIKEY) ? _binSize : 0.225f; }
	virtual long nTubes() const { return (_nTubes != BADINIKEY) ? _nTubes : 4; }
	virtual long axTubes() const { return (_axTubes != BADINIKEY) ? _axTubes : 2; }
	virtual long transTubes() const { return (_transTubes != BADINIKEY) ? _transTubes : 2; }
	virtual long tubePlns() const { return (_tubePlanes != BADINIKEY) ? _tubePlanes : ntubePlanes; }
	virtual long totTubes() const { return (_totTubes != BADINIKEY) ? _totTubes : ntotalTubes; }
	virtual long* tubeMaps();
	virtual long timeCorrectionBits() const { return (_timeCorrBits != BADINIKEY) ? _timeCorrBits : 4; }
	virtual long maxCodePg() const { return (_maxCodePage != BADINIKEY) ? _maxCodePage : 1; }
	virtual long minElems() const { return (_minElems != BADINIKEY) ? _minElems : 288; }
	virtual EN_PositionProfileOrder posProfileOrder() const { return (_posProfOrd != BADINIKEY) ? (EN_PositionProfileOrder)_posProfOrd : PPO_LR; }
	virtual long txSourceType() const { return (_txSrcType != BADINIKEY) ? _txSrcType : TST_None; }
	virtual double txSourceRad() const { return (_txSrcRad != BADINIKEY) ? _txSrcRad : 39.75f; }
	virtual EN_SeptaType septaTyp() const { return (_septaType != BADINIKEY) ? (EN_SeptaType)_septaType : SPT_None; }
	virtual double maxScanLen() const { return (_maxScanLen != BADINIKEY) ? _maxScanLen : -1; }
	virtual double pileUpCor() const { return (_pileupCor != BADINIKEY) ? _pileupCor : 30.e-8f; }
	virtual double plnCor() const { return (_planeCor != BADINIKEY) ? _planeCor : 1.0f; }
	virtual long rodOffSet() const { return (_rodoffset != BADINIKEY) ? _rodoffset : 12; }
	virtual double hBedStp() const { return (_hBedStep != BADINIKEY) ? _hBedStep : 1.0f; }
	virtual double vBedStp() const { return (_vBedStep != BADINIKEY) ? _vBedStep : 0; }
	virtual long intrTilt() const { return (_intrTilt != BADINIKEY) ? _intrTilt:0; }
	virtual long defWobbleSp() const { return (_defWobble != BADINIKEY) ? _defWobble : 0; }
	virtual long tZero() const { return (_tiltZero != BADINIKEY) ? _tiltZero : -1; }				    // tilt zero
	virtual long rZero() const { return (_rotateZero != BADINIKEY) ? _rotateZero : -1; }				// rotate zero
	virtual long wZero() const { return (_wobbleZero != BADINIKEY) ? _wobbleZero : -1; }				// wobble zero
	virtual long optBedOverlap() const { return (_optBedOverlap != BADINIKEY) ? _optBedOverlap : 7; }
	virtual long optBedOverlap3D() const { return (_optBedOverlap3D != BADINIKEY) ? _optBedOverlap3D : 15; }
	virtual long isprt() const { return (_isPRT != BADINIKEY) ? _isPRT : 0; }
	virtual EN_ExtraReconCor corMsk() const { return (_corMask != BADINIKEY) ? (EN_ExtraReconCor)_corMask : ERC_AxialAngleCor; }
	virtual long analogAsicBkt() const { return (_analogAsicBkt != BADINIKEY) ? _analogAsicBkt : 1; }
	virtual long nCassettes() const { return (_nCassettes != BADINIKEY) ? _nCassettes : ncassettes; }
	virtual long coincWindow() const { return (_coinWnd != BADINIKEY) ? _coinWnd : 12; }
	virtual long nContrls() const { return (_nControllers != BADINIKEY) ? _nControllers : ncontrollers; }
	virtual pcontroller Contrls();
	virtual bool isContrlInstalled(EN_Controller id=(EN_Controller)-1);
	virtual bool isContrlInstalled(char* name=NULL);
	virtual char* layerMat(long);
	virtual long numberCrystaLayers() const { return (_crystalLayer != BADINIKEY) ? _crystalLayer : 1; }
	virtual long LayerBackErg(long);
	virtual double LayerfwhmErgRes(long);
	virtual double LayerCrystalDepth(long);	


private:
	// calculated values
	long	ncontrollers;
	long	nbuckets;
	long	nblocks;
	long	dirPlanes;
	double	defAxFOV;
	long	ncassettes;
	long	ntotalBlocks;
	long	nblockCrystals;
	long	ntotalCrystals;
	long	nplaneCrystals;
	long	ntubePlanes;
	long	ntotalTubes;
	long	nlogicalAngularBlocks;
	long	nlogicalBlocks;

	long	numberOfTubes;
	long	numberOfControllers;
	long	numberOfLogBlocks;
	bool	bFoundLogBlockInfo;
	bool	bFoundControllerInfo;
	bool	bFoundTubeInfo;
	char*	pKey;
	char	cKey[BSIZE];
	char	Buffer[BUFSIZE];
	double	crystalDepth;
	long	layerBackErg;
	double	layerfwhmErgRes;
	char*	layerMaterial;

	double	_layOneCrysDepth;
	double	_layTwoCrysDepth;
	double	_layOnefwhm;
	double	_layTwofwhm;
	long	_layTwoBackErg;
	long	_layOneBackErg;
	long	_crystalLayer;
	char	_layOneMat[BSIZE];
	char	_layTwoMat[BSIZE];
	char	_logDir[BSIZE];
	char	_setupDir[BSIZE];
	char	_cpname[BSIZE];
	char	_cpcommdesc[BSIZE];
	long	_nblocks;
	double	_blkDepth;
	double	_plnSep;
	double	_binSize;
	long	_angBlock;
	long	_totBlock;
	long	_blkCrystal;
	long	_logAngBlks;
	long	_logAxBlks;
	long	_logBlks;
	long	_nbuckets;
	long	_bktDbSz;
	long	_bktDBSt;
	long	_bktRings;
	long	_bktsPerRing;
	long	_bktOutChnl;
	long	_logBkts;
	long	_bktsInCoinc;
	long	_LLD;
	long	_defTxULD;
	long	_ULD;
	long	_angles;
	long	_elements;
	long	_defAngComp;
	long	_preInjTxLLD;
	long	_postInjTxLLD;
	long	_scatThresh;
	double	_defAxFOV;
	double	_defTransFOV;
	long	_totSino;
	long	_2DSpan;
	long	_3DSpan;
	long	_defMaxRingDiff;
	long	_defMaxTxRingDiff;
	double	_intDepth;
	long	_tranBlk;
	long	_axialBlk;
	long	_angCry;
	long	_axialCry;
	long	_totCrystals;
    long	_maxScatterZfov;
	double	_crystalRad;
	long	_planeCrystal;
	long	_dPlane;
	long	_nTubes;
	long	_axTubes;
	long	_transTubes;
	long	_tubePlanes;
	long	_totTubes;
	long	_timeCorrBits;
	long	_maxCodePage;
	long	_minElems;
	long	_posProfOrd;
	long	_txSrcType;
	double	_txSrcRad;
	long	_septaType;
	double	_maxScanLen;
	double	_pileupCor;
	double	_planeCor;
	long	_rodoffset;
	double	_hBedStep;
	double	_vBedStep;
	long	_intrTilt;
	long	_defWobble;
	long	_tiltZero;
	long	_rotateZero;
	long	_wobbleZero;
	long	_optBedOverlap;
	long	_optBedOverlap3D;
	long	_isPRT;
	long	_corMask;
	long	_analogAsicBkt;
	long	_nCassettes;
	long	_coinWnd;
	long	_nControllers;
	controller installedController[4];

};

#endif
