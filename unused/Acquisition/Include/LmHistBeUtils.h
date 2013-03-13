#ifndef LmHistBeUtils_h
#define LmHistBeUtils_h

// a class with callback functions for LM and HIST to invoke
class CCallback {
public:
	// enum indicating situation under which the scan is stopped
	static enum {Preset, Stop, Abort, Error};

	// callbacks
	// send info, warning and error
	virtual void notify(int error_code, char *extra_info=0) {};
	// notify scan has stopped and the stop condition
	virtual void acqComplete(int enum_flag) {};
	// notify if file save is successful after scan has stopped
	virtual void acqPostComplete(int enum_flag, int fileWriteSuccess) {};
};

// a class to implement the state machine for the acq backend
class CStateMC {
public:
	// operation codes
	static enum {Unconfig, Config, Start, Stop, Abort, AcqComplete, AcqPostComplete};
	// states
	static enum {Ready, Configured, Started, Stopping, Stopped};

	// check if opereation is legal and complete state transition
	virtual void updateState(int op, int op_par=0) {};
	virtual int opAllowed(int op) {return 1;}
};

#include "sinoInfo.h"

#endif
