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
#define E_NO_CLK		-71		// No clock present

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
#endif
