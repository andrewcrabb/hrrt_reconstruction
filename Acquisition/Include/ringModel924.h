//==========================================================================
// File			:	RingModel924.h  (Phoenix)
//					Copyright 2002 by CTI
// Description	:	Contains the function prototypes for the derived class
//	
// Author		:	Selene F. Tolbert
//
// Date			:	18 February 2003
//
// Author		Date			Update
// S Tolbert	18 Feb 2003		Created
//==========================================================================

// Set guards to prevent multiple header file inclusion
//
#ifndef MODEL924_CLASS_H
#define MODEL924_CLASS_H

#include "baseModelClass.h"	// base class and interface class

//
// CRingModel924 derived class
//

class CRingModel924 : public CModel {
public:
	CRingModel924();
	virtual ~CRingModel924();

	// access functions
	virtual long model() const { return 924; }
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
	virtual double blkDepth() const { return (_blkDepth != BADINIKEY) ? _blkDepth : 2.5f; }
	virtual long angBlk() const { return (_angBlock != BADINIKEY) ? _angBlock : 4; }
	virtual long totBlk() const { return (_totBlock != BADINIKEY) ? _totBlock : ntotalBlocks; }
	virtual long blkCrys() const { return (_blkCrystal != BADINIKEY) ? _blkCrystal : nblockCrystals; }
	virtual long* logBlkMap();
	virtual long nBukets() const { return (_nbuckets != BADINIKEY) ? _nbuckets : nbuckets; }
	virtual long bktDbSize() const { return (_bktDbSz != BADINIKEY) ? _bktDbSz : 0x1600; }
	virtual long bktDBStart() const { return (_bktDBSt != BADINIKEY) ? _bktDBSt : 0x2000; }
	virtual long bktRings() const { return (_bktRings != BADINIKEY) ? _bktRings : 1; }
	virtual long bktsPerRing() const { return (_bktsPerRing != BADINIKEY) ? _bktsPerRing : 12; }
	virtual long bktOutputChannels() const { return (_bktOutChnl != BADINIKEY) ? _bktOutChnl : 2; } // single channel=1, dual channel=2
	virtual long bktsInCoinc() const { return (_bktsInCoinc != BADINIKEY) ? _bktsInCoinc : 7; }
	virtual long defLLD() const { return (_LLD != BADINIKEY) ? _LLD : 350; }
	virtual long defTxULD() const { return (_defTxULD != BADINIKEY) ? _defTxULD : 650; }
	virtual long defULD() const { return (_ULD != BADINIKEY) ? _ULD : 650; }
	virtual long defAngles() const { return (_angles != BADINIKEY) ? _angles : 192; }
	virtual long defElements() const { return (_elements != BADINIKEY) ? _elements : 192; }
	virtual long defAngComp() const { return (_defAngComp != BADINIKEY) ? _defAngComp : 0; }		// default mash value
	virtual long defPreInjTxLLD() const { return (_preInjTxLLD != BADINIKEY) ? _preInjTxLLD : 350; }
	virtual long defPostInjTxLLD() const { return (_postInjTxLLD != BADINIKEY) ? _postInjTxLLD : 350; }
	virtual long defScatThresh() const { return (_scatThresh != BADINIKEY) ? _scatThresh : 0; }
	virtual double defAxialFOV() const { return (_defAxFOV != BADINIKEY) ? _defAxFOV : defAxFOV; }
	virtual double defTransFOV() const { return (_defTransFOV != BADINIKEY) ? _defTransFOV : 58.3f; }
	virtual long def2DSpan() const { return (_2DSpan != BADINIKEY) ? _2DSpan : 11; }
	virtual long def3DSpan() const { return (_3DSpan != BADINIKEY) ? _3DSpan : 7; }
	virtual long defMaxRingDiff() const { return (_defMaxRingDiff != BADINIKEY) ? _defMaxRingDiff : 17; }
	virtual long defMaxTxRingDiff() const { return (_defMaxTxRingDiff != BADINIKEY) ? _defMaxTxRingDiff : 5; }
	virtual double maxScattzfov() const { return (_maxScatterZfov != BADINIKEY) ? _maxScatterZfov : 40.0; }
	virtual double interacDepth() const { return (_intDepth != BADINIKEY) ? _intDepth : 1.9; }
	virtual long transBlks() const { return (_tranBlk != BADINIKEY) ? _tranBlk : 4; }
	virtual long axialBlks() const { return (_axialBlk != BADINIKEY) ? _axialBlk : 3; } 
	virtual long angCrystals() const { return (_angCry != BADINIKEY) ? _angCry : 8; }
	virtual long axialCryst() const { return (_axialCry != BADINIKEY) ? _axialCry : 8; }
	virtual long totCryst() const { return (_totCrystals != BADINIKEY) ? _totCrystals : ntotalCrystals; }
	virtual double crystRad() const { return (_crystalRad != BADINIKEY) ? _crystalRad : 41.25; }
	virtual long plnCryst() const { return (_planeCrystal != BADINIKEY) ? _planeCrystal : nplaneCrystals; }
	virtual long dPlanes() const { return (_dPlane != BADINIKEY) ? _dPlane : dirPlanes; }
	virtual double planeSeparation() const { return (_plnSep != BADINIKEY) ? _plnSep : 0.3375; }
	virtual double binSz() const { return (_binSize != BADINIKEY) ? _binSize : 0.3375; }
	virtual long nTubes() const { return (_nTubes != BADINIKEY) ? _nTubes : 4; }
	virtual long axTubes() const { return (_axTubes != BADINIKEY) ? _axTubes : 2; }
	virtual long tubePlns() const { return (_tubePlanes != BADINIKEY) ? _tubePlanes : 0; }
	virtual long totTubes() const { return (_totTubes != BADINIKEY) ? _totTubes : ntotalTubes; }
	virtual long transTubes() const { return (_transTubes != BADINIKEY) ? _transTubes : 6; }
	virtual long* tubeMaps();
	virtual long timeCorrectionBits() const { return (_timeCorrBits != BADINIKEY) ? _timeCorrBits : 5; }
	virtual long maxCodePg() const { return (_maxCodePage != BADINIKEY) ? _maxCodePage : 0; }
	virtual long minElems() const { return (_minElems != BADINIKEY) ? _minElems : 192; }
	virtual EN_PositionProfileOrder posProfileOrder() const { return (_posProfOrd != BADINIKEY) ? (EN_PositionProfileOrder)_posProfOrd : PPO_RL; }
	virtual long txSourceType() const { return (_txSrcType != BADINIKEY) ? _txSrcType : TST_Rod; }
	virtual double txSourceRad() const { return (_txSrcRad != BADINIKEY) ? _txSrcRad : 30.9f; }
	virtual EN_SeptaType septaTyp() const { return (_septaType != BADINIKEY) ? (EN_SeptaType)_septaType : SPT_Retractable; }
	virtual double maxScanLen() const { return (_maxScanLen != BADINIKEY) ? _maxScanLen : 236.2; }
	//virtual double pileUpCor() const { return (_pileupCor != BADINIKEY) ? _pileupCor : 30.e-8; }
	//virtual double plnCor() const { return (_planeCor != BADINIKEY) ? _planeCor : 1.0; }
	virtual long rodOffSet() const { return (_rodoffset != BADINIKEY) ? _rodoffset : 0; }
	virtual double hBedStp() const { return (_hBedStep != BADINIKEY) ? _hBedStep : 1.0; }
	virtual double vBedStp() const { return (_vBedStep != BADINIKEY) ? _vBedStep : 1.0; }
	virtual long intrTilt() const { return (_intrTilt != BADINIKEY) ? _intrTilt: 15; }
	virtual long optBedOverlap() const { return (_optBedOverlap != BADINIKEY) ? _optBedOverlap : 5; }
	virtual long optBedOverlap3D() const { return (_optBedOverlap3D != BADINIKEY) ? _optBedOverlap3D : 11; }
	virtual long isprt() const { return (_isPRT != BADINIKEY) ? _isPRT : 0; }
	virtual EN_ExtraReconCor corMsk() const { return (_corMask != BADINIKEY) ? (EN_ExtraReconCor)_corMask : ERC_NoExtraReconCor; }
	virtual long analogAsicBkt() const { return (_analogAsicBkt != BADINIKEY) ? _analogAsicBkt : 1; }
	virtual long coincWindow() const { return (_coinWnd != BADINIKEY) ? _coinWnd : 6; }
	virtual long nContrls() const { return (_nControllers != BADINIKEY) ? _nControllers : ncontrollers; }
	virtual pcontroller Contrls();
	virtual bool isContrlInstalled(EN_Controller id=(EN_Controller)-1);
	virtual bool isContrlInstalled(char* name=NULL);
	virtual char* layerMat(long);
	virtual long numberCrystaLayers() const { return (_crystalLayer != BADINIKEY) ? _crystalLayer : 1; }
	virtual long LayerBackErg(long);
	virtual double LayerfwhmErgRes(long);
	virtual double LayerCrystalDepth(long);	
	virtual double maxBedTrav() const { return (_bedTravel != BADINIKEY) ? _bedTravel : 2200.00; }


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

	double  _bedTravel;
	long	_maxScatterZfov;
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
