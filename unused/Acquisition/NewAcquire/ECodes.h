#ifndef ERROR_CODES_H
#define ERROR_CODES_H

char *error_description(int);

#define OK 0
#define E_CHANNEL_OOR  -1		// channel address not in range
#define E_PARAMETER_OOR -2		// parameter value not in range
#define E_UNKNOWN_PARAMETER -3  // parameter not known to the set parameter command
#define E_NO_CONFIG -4			// configuration identifier is out of range [0-3]
#define E_NO_MODE -5			// specified mode is not available
#define E_WRITE_FAILED -6		// disk is full
#define E_OPEN_FAILED -7		// file not found or not available for write
#define E_BAD_FORMAT -8			// load file has improper format
#define E_PRIORITY_FAILED -9	// download to priority device failed
#define E_ANALOG_FAILED -10		// download to analog device failed
#define E_ANALOG_COMM -11		// communication (offsets) with analog subsection failed
#define E_CHECKSUM -12			// download checksum failed
#define E_CMD_IN_PROGRESS -13	// background command in progress, new command cannot be executed until previous command completes
#define E_UNKNOWN_LOAD_TYPE -14 // allowable load types are:  ASIC, Energy, Time
#define E_SYNTAX_ERROR -15		// incorrect number/type of parameters
#define E_UNKNOWN_CMD -16		// unknown command
#define E_TIMED_OUT -17       // command timed out
#define E_E2POT_READ_FAILED -18   // specified E2POT did not acknowledge read
#define E_E2POT_WRITE_FAILED -19  // specified E2POT did not acknowledge write
#define E_E2POT_TRANSFER_FAILED -20 // specified E2POT did not acknowledge transfer (save)
#define E_DIAG_TIMED_OUT -21       // diagnostic timed out
#define E_XY_OFFSET_OOR -22       // xy offset adjustment failed due to out of range condition
#define E_DELAY_OOR -23			// cfd delay adjustment failed due to out of range condition
#define E_ANALOG_DIAG_FAILED -24 // download to analog device of diagnostic equations failed
#define E_HIGH_VOLTAGE_ON    -25 // operation cannot proceed with high voltage enabled
#define E_HIGH_VOLTAGE_OFF	  -26 // operation cannot proceed with high voltage off
#define E_VOLTAGE_LOW        -27 // panel interface supply voltage (+5) too low
#define E_PI_OVER_TEMP       -28 // panel interface temperature too hot (over 50C)
#define E_DETECTOR_OVER_TEMP  -29 // detector tub too hot (over 50C), check cooling fan
#define E_PI_REF_WRITE_FAILED -30 // panel interface voltage reference eeprom write failed (is write protect tab on?)
#define E_PI_VOLTAGE_FAILED -31 // panel interface voltage ADC read failed
#define E_PI_TEMP_FAILED    -32 // panel interface
#define E_EOF					 -33 // End of File
#define E_DHI_LOW_TEMP      -34 // Temperature too low on DHI motherboard, condensation risk
#define E_DHI_HIGH_TEMP     -35 // Temperature too high on DHI motherboard
#define E_MEMORY            -36 // Can't allocate memory
#define E_READ_FAILED       -37 // file read failed
#define E_CC_DOWNLOAD_ERROR -38 // FPGA Download to CC Hardware failed
#define E_CC_SINGLES_TIMEOUT -39 // singles polling command failed due to timeout
#define E_CC_TEMP_TIMEOUT -40 // temperature polling of a specific head has failed
#define E_PS_AXIAL_FAILURE -41  // axial axis reported error
#define E_PS_TRANS_FAILURE -42  // transaxial axis reported error
#define E_PS_AXIAL_JAMMED -43 // axial axis mechanical failure
#define E_PS_TRANS_JAMMED -44 // transaxial axis mechanical failure
#define E_PS_MOTION_NOT_AT_REST -45   // can't start motion sequence if not at rest
#define E_PS_GENERAL_FAILURE -46 // point source not controlling
#define E_PS_MOTION_ALREADY_AT_REST -47  // point source motion is not currently scanning
#define E_ORDER -48 // parameters not in proper order
#define E_LIMIT_OOR -49 // energy limit out of range 0-750
#define E_FNF -50 // file not found
#define E_CC_REBOOT -51 // Coincidence Controller has rebooted
#define E_CC_BUCKET_COMM -52 // General failure communicating with buckets
#define E_CC_HW_FAILURE -53 // General failure with coincidence controller hardware
#define E_ESTOP -54 // System is in ESTOP
#define E_NO_SCAN -55 // no scan has been configured
#define E_SCAN_IN_PROGRESS -56 // scan already started
#define E_PS_NO_EXTEND -57 // can't extend pointsource (PETSPECT)
#define E_PS_NO_RETRACT -58 // can't retract pointsource (PETSPECT)
#define E_COMM_ERROR -59 // communication failure
#define E_RAMTEST_FAILED -60   // RAM test did not pass
#define E_SHUTDOWN -61 // Coincidence controller is in shutdown state (high temp in heads?)
#define E_PS_AXIAL_MISALIGNED -62  // axial pointsource limits not where we expect them to be
#define E_PS_TRANS_MISALIGNED -63  // transaxial pointsource limits not where we expect them to be
#define E_CMD_NOT_SUPPORTED -64    // command is not supported for this hardware
#define E_DIAG_MODE -65 // system is in diagnostic mode, reset to remove
#define E_ADC_READ_FAILED -66  // pointsource interface HV adc read failed
#define E_DAC_WRITE_FAILED -67  // pointsource interface HV dac write failed
#define E_REBOOT -68				// general system reboot code for all devices
#define E_SERIALBUS   -69        // RS485 serial bus conflict
#define E_HV_FAULT   -70		// High Voltage fault, HV shutdown initiated

// error codes returned by onboard diagnostics
#define E_1                                  -101        //error downloading pc_sd1.rbf file to interface device
#define E_2                                  -102        //error Test_SIN_SOUT(INTERFACE_DEVICE)
#define E_3                                  -103        //error Test_PC104_Interface()
#define E_4                                  -104        //error downloading pc_run.rbf to INTERFACE_DEVICE
#define E_5                                  -105        //error Test_Temperature_Sensors()
#define E_6                                  -106        //error downloading pe_sd1.rbf to PRIORITY_END_DEVICE
#define E_7                                  -107        //error Test_SIN_SOUT(PRIORITY_END_DEVICE)
#define E_8                                  -108        //error downloading pe_sd3.rbf to PRIORITY_END_DEVICE
#define E_9                                  -109        //error Test_Priority_End_Device()
#define E_10                                 -110       //error downloading pr_sd1.rbf to PRIORITY_DEVICE,
#define E_11											-111       //error Test_SIN_SOUT(PRIORITY_DEVICE)
#define E_12                                 -112       //error downloading pr_sd3.rbf to PRIORITY_DEVICE
#define E_13                                 -113       //error downloading pe_sd6.rbf PRIORITY_END_DEVICE
#define E_14                                 -114       //error Test_Priority_Serial_Lines
#define E_15                                 -115       //error downloading pe_sd2.rbf to PRIORITY_END_DEVICE
#define E_16                                 -116       //error Perform_Priority_Diagnostics
#define E_17                                 -117       //error Test_DIP_Switch()
#define E_18                                 -118       //error downloading pr_ld.rbf to PRIORITY_DEVICE
#define E_19                                 -119       //error downloading as_sd0.rbf to ANALOG_SUBSECTION_DEVICE
#define E_20                                 -120       //error downloading as_sd1.rbf to ANALOG_SUBSECTION_DEVICE
#define E_21                                 -121       //error downloading pr_sd2.rbf to PRIORITY_DEVICE
#define E_22                                 -122       //error Perform_AS_Diagnostics
#define E_23                                 -123       //error  downloading as_sd2.rbf to ANALOG_SUBSECTION_DEVICE
#define E_24                                 -124       //error Test_Clk_Sync (ANALOG_SUBSECTION_DEVICE)
#define E_25                                 -125       //error downloading as_clk.rbf to ANALOG_SUBSECTION_DEVICE
#define E_26                                 -126       //error downloading pr_sd4.rbf to PRIORITY_DEVICE
#define E_27                                 -127       //error Test_Clk_Sync(PRIORITY_DEVICE)
#define E_28                                 -128       //error downloading pi_sd1.rbf to PANEL_INTERFACE_DEVICE
#define E_29                                 -129       //error Test_SIN_SOUT(PANEL_INTERFACE_DEVICE
#define E_30                                 -130       //error downloading pi_run.rbf to PANEL_INTERFACE_DEVICE,
#define E_31                                 -131       //error Initialize_PI_Reference_Voltage()
#define E_32                                 -132       //error Test_Panel_Interface_Supply_Voltage()
#define E_33                                 -133       //error downloading pr_run.rbf to PRIORITY_DEVICE
#define E_34                                 -134       //error downloading pe_sd5.rbf to PRIORITY_END_DEVICE
#define E_35                                 -135       //error Test_Clk_Sync(PRIORITY_END_DEVICE)
#define E_36                                 -136       //error downloading pe_sd4.rbf to PRIORITY_END_DEVICE
#define E_37                             		-137
#define E_38                                 -138
#define E_39                                 -139
#define E_40                                 -140
#define E_41                                 -141
#define E_42                                 -142
#define E_DHI_TEMP_ERROR 							-143
#define E_PANEL_INTERFACE 							-144
#define E_LTI_EEPROM 								-145
#define E_DOWNLOAD_INTERFACE 						-146
#define E_DOWNLOAD_PRIORITY 						-147
#define E_DOWNLOAD_PRIORITY_END 					-148
#define E_DOWNLOAD_ANALOG 							-149
#define E_DOWNLOAD_PANEL 							-150
#define E_DOWNLOAD_POINTSOURCE 					-151
#define E_LOCAL_TEMP 								-152
#define E_REMOTE_TEMP 								-153
#define E_PSI_ADC                            -154
#define E_PSI_DAC                            -155
#define E_PSI_SOLENOID_HOME                  -156
#define E_PSI_SOLENOID_EXTEND                -157
#define E_PSI_SOLENOID_RETRACT               -158
#define MAXERRORCODE -159


static char *e_short_text[] =
{
	"OK - Operation Completed Correctly",
   "E_CHANNEL_OOR - Channel is out of range",
   "E_PARAMETER_OOR - Parameter is out of range",
   "E_UNKNOWN_PARAMETER - Unknown parameter",
   "E_NO_CONFIG - Configuration out of range (0-3)",
   "E_NO_MODE - Mode not recognized",
   "E_WRITE_FAILED - Write to flash disk failed (disk full?)",
   "E_OPEN_FAILED - Open failed, file not found",
   "E_BAD_FORMAT - Can't interpret data due to unrecognized formatting",
   "E_PRIORITY_FAILED - Download to priority device failed",
   "E_ANALOG_FAILED - Download to analog subsection failed",
   "E_ANALOG_COMM - Communication with analog subsection failed",
   "E_CHECKSUM - Checksum error occured during upload/download",
   "E_CMD_IN_PROGRESS - Background command is already in progress",
   "E_UNKNOWN_LOAD_TYPE - Data load type specified is not recognized",
   "E_SYNTAX_ERROR - Cannot interpret command, required parameters missing",
   "E_UNKNOWN_CMD - Unrecognized command",
   "E_TIMED_OUT - Command timed out",
   "E_E2POT_READ_FAILED - Cannot read EEPOT on HRRT detector head",
   "E_E2POT_WRITE_FAILED - Cannot write EEPOT on HRRT detector head",
   "E_E2POT_TRANSFER_FAILED - Cannot transfer temporary value to long term storage",
   "E_DIAG_TIMED_OUT - Diagnostic/Alignment routine did not complete in time",
   "E_XY_OFFSET_OOR - X or Y offset is not within range for proper function",
   "E_DELAY_OOR - CFD Delay is not within range for proper function",
   "E_ANALOG_DIAG_FAILED - Analog Processor diagnostics failed",
   "E_HIGH_VOLTAGE_ON - High Voltage is on when it should be off",
   "E_HIGH_VOLTAGE_OFF - High Voltage is off when it should be on",
   "E_VOLTAGE_LOW - Supply voltage is low",
   "E_PI_OVER_TEMP - Panel Interface is over the maximum permitted temperature",
   "E_DETECTOR_OVER_TEMP - Detector is over the maximum permitted temperature",
   "E_PI_REF_WRITE_FAILED - Panel reference save failed (Is write protection on?)",
   "E_PI_VOLTAGE_FAILED - Hardware failure reading Panel Interface voltage",
   "E_PI_TEMP_FAILED - Hardware failure reading Panel Interface temperature",
   "E_EOF - End of File",
   "E_DHI_LOW_TEMP - Detector Head Interface temperature too low",
   "E_DHI_HIGH_TEMP - Detector Head Interface temperature too high",
   "E_MEMORY - Unable to allocate enough memory to proceed with operation",
   "E_READ_FAILED - Read from flash disk failed",
   "E_CC_DOWNLOAD_ERROR - Error downloading to Coincidence Controller FPGAs",
   "E_CC_SINGLES_TIMEOUT - Coincidence Controller timed out reading singles rates from Detector Head Interface",
   "E_CC_TEMP_TIMEOUT - Coincidence Controller timed out reading temperature from Detector Head Interface",
   "E_PS_AXIAL_FAILURE - Unable to communicate with pointsource axial controller",
   "E_PS_TRANS_FAILURE - Unable to communicate with pointsource transaxial controller",
   "E_PS_AXIAL_JAMMED - Pointsource axial servo is not where it is supposed to be",
   "E_PS_TRANS_JAMMED - Pointsource transaxial servo is not where it is supposed to be",
   "E_PS_MOTION_NOT_AT_REST - Pointsource is currently moving and must be stopped before issuing another move command",
   "E_PS_GENERAL_FAILURE - Unable to do anything with pointsource motion control hardware",
   "E_PS_MOTION_ALREADY_AT_REST - Pointsource motion cannot be stopped if it is already stopped",
   "E_ORDER - Parameters are out of order",
   "E_LIMIT_OOR - Energy limits out of range (0-1000)",
   "E_FNF - File not found",
   "E_CC_REBOOT - Coincidence Controller has rebooted",
   "E_CC_BUCKET_COMM - Unable to communicate with detector head interface",
   "E_CC_HW_FAILURE - Coincidence Controller hardware not responding to commands",
   "E_ESTOP - System has been put in emergency stop",
   "E_NO_SCAN - A scan must be configured before it is started",
   "E_SCAN_IN_PR0GRESS - A scan is already running and must be stopped before it can be reconfigured",
   "E_PS_NO_EXTEND - Pointsource did not extend",
   "E_PS_NO_RETRACT - Pointsource did not retract",
   "E_COMM_ERROR - Unable to initialize UART to communicate serially",
   "E_RAMTEST_FAILED - Analog subsection RAM test failed for one or more channels",
   "E_SHUTDOWN - System is in shutdown state due to temperatures out of range",
   "E_PS_AXIAL_MISALIGNED - Pointsource is misaligned in the axial direction",  // -62
   "E_PS_TRANS_MISALIGNED - Pointsource is misaligned in the transaxial direction",  // -63
   "E_CMD_NOT_SUPPORTED - Command is not supported for this hardware",	// -64
   "E_DIAG_MODE - System is in diagnostic mode, reset to remove",		// -65
   "E_ADC_READ_FAILED - Pointsource interface HV adc read failed",		// -66
   "E_DAC_WRITE_FAILED - Pointsource interface HV dac write failed",	// -67
   "E_REBOOT - General system reboot code for all devices",				// -68
   "E_SERIALBUS - RS485 serial bus conflict",							// -69
   "E_HV_FAULT - High Voltage fault, HV shutdown initiated",			// -70
   "E_UNKNOWN_CODE_71 (RESERVED)",
   "E_UNKNOWN_CODE_72 (RESERVED)",
   "E_UNKNOWN_CODE_73 (RESERVED)",
   "E_UNKNOWN_CODE_74 (RESERVED)",
   "E_UNKNOWN_CODE_75 (RESERVED)",
   "E_UNKNOWN_CODE_76 (RESERVED)",
   "E_UNKNOWN_CODE_77 (RESERVED)",
   "E_UNKNOWN_CODE_78 (RESERVED)",
   "E_UNKNOWN_CODE_79 (RESERVED)",
   "E_UNKNOWN_CODE_80 (RESERVED)",
   "E_UNKNOWN_CODE_81 (RESERVED)",
   "E_UNKNOWN_CODE_82 (RESERVED)",
   "E_UNKNOWN_CODE_83 (RESERVED)",
   "E_UNKNOWN_CODE_84 (RESERVED)",
   "E_UNKNOWN_CODE_85 (RESERVED)",
   "E_UNKNOWN_CODE_86 (RESERVED)",
   "E_UNKNOWN_CODE_87 (RESERVED)",
   "E_UNKNOWN_CODE_88 (RESERVED)",
   "E_UNKNOWN_CODE_89 (RESERVED)",
   "E_UNKNOWN_CODE_90 (RESERVED)",
   "E_UNKNOWN_CODE_91 (RESERVED)",
   "E_UNKNOWN_CODE_92 (RESERVED)",
   "E_UNKNOWN_CODE_93 (RESERVED)",
   "E_UNKNOWN_CODE_94 (RESERVED)",
   "E_UNKNOWN_CODE_95 (RESERVED)",
   "E_UNKNOWN_CODE_96 (RESERVED)",
   "E_UNKNOWN_CODE_97 (RESERVED)",
   "E_UNKNOWN_CODE_98 (RESERVED)",
   "E_UNKNOWN_CODE_99 (RESERVED)",
   "E_UNKNOWN_CODE_100 (RESERVED)",
   "E_1 - Error downloading pc_sd1.rbf file to interface device",
	"E_2 - Error Test_SIN_SOUT(INTERFACE_DEVICE)",
	"E_3 - Error Test_PC104_Interface()",
	"E_4 - Error downloading pc_run.rbf to INTERFACE_DEVICE",
	"E_5 - Error Test_Temperature_Sensors()",
	"E_6 - Error downloading pe_sd1.rbf to PRIORITY_END_DEVICE",
	"E_7 - Error Test_SIN_SOUT(PRIORITY_END_DEVICE)",
	"E_8 - Error downloading pe_sd3.rbf to PRIORITY_END_DEVICE",
	"E_9 - Error Test_Priority_End_Device()",
	"E_10 - Error downloading pr_sd1.rbf to PRIORITY_DEVICE",
	"E_11 - Error Test_SIN_SOUT(PRIORITY_DEVICE)",
	"E_12 - Error downloading pr_sd3.rbf to PRIORITY_DEVICE",
	"E_13 - Error downloading pe_sd6.rbf PRIORITY_END_DEVICE",
	"E_14 - Error Test_Priority_Serial_Lines",
	"E_15 - Error downloading pe_sd2.rbf to PRIORITY_END_DEVICE",
	"E_16 - Error Perform_Priority_Diagnostics",
	"E_17 - Error Test_DIP_Switch()",
	"E_18 - Error downloading pr_ld.rbf to PRIORITY_DEVICE",
	"E_19 - Error downloading as_sd0.rbf to ANALOG_SUBSECTION_DEVICE",
	"E_20 - Error downloading as_sd1.rbf to ANALOG_SUBSECTION_DEVICE",
	"E_21 - Error downloading pr_sd2.rbf to PRIORITY_DEVICE",
	"E_22 - Error Perform_AS_Diagnostics",
	"E_23 - Error  downloading as_sd2.rbf to ANALOG_SUBSECTION_DEVICE",
   "E_24 - Error Test_Clk_Sync (ANALOG_SUBSECTION_DEVICE)",
	"E_25 - Error downloading as_clk.rbf to ANALOG_SUBSECTION_DEVICE",
	"E_26 - Error downloading pr_sd4.rbf to PRIORITY_DEVICE",
	"E_27 - Error Test_Clk_Sync(PRIORITY_DEVICE)",
	"E_28 - Error downloading pi_sd1.rbf to PANEL_INTERFACE_DEVICE",
	"E_29 - Error Test_SIN_SOUT(PANEL_INTERFACE_DEVICE",
	"E_30 - Error downloading pi_run.rbf to PANEL_INTERFACE_DEVICE",
	"E_31 - Error Initialize_PI_Reference_Voltage()",
	"E_32 - Error Test_Panel_Interface_Supply_Voltage()",
	"E_33 - Error downloading pr_run.rbf to PRIORITY_DEVICE",
	"E_34 - Error downloading pe_sd5.rbf to PRIORITY_END_DEVICE",
	"E_35 - Error Test_Clk_Sync(PRIORITY_END_DEVICE)",
	"E_36 - Error downloading pe_sd4.rbf to PRIORITY_END_DEVICE",
	"E_37 - Error downloading AS_rmtst.rbf to ANALOG_SUBSECTION_DEVICE",
	"E_38 - Error downloading pr_set.rbf to PRIORITY_DEVICE",
	"E_39 - Error performing BIST RAM test",
	"E_40 - Error downloading as_setup.rbf to ANALOG_SUBSECTION_DEVICE",
	"E_41 - Error downloading as_tmd.rbf to ANALOG_SUBSECTION_DEVICE",
	"E_42 - Error performing TMD test"
};

#endif
