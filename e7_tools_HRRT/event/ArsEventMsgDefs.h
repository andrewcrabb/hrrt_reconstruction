//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: LIS_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define LIS_GENERIC_DEVELOPER_ERR        0xC8000001L

//
// MessageId: LIS_ERR_WITH_TEXT_AND_RAWDATA
//
// MessageText:
//
//  This LISTMODE msg contains free-formatted text = <%1>
//  It also contains binary data below
//
#define LIS_ERR_WITH_TEXT_AND_RAWDATA    0xCD000002L

//
// MessageId: LIS_ERR_WITH_ADDL_TEXT_ONLY
//
// MessageText:
//
//  This LISTMODE msg contains only addl text line = <%1>
//
#define LIS_ERR_WITH_ADDL_TEXT_ONLY      0xCD000003L

//
// MessageId: LIS_ERR_WITH_RAWDATA_ONLY
//
// MessageText:
//
//  This LISTMODE msg contains only raw binary data below
//
#define LIS_ERR_WITH_RAWDATA_ONLY        0xCD000004L

//
// MessageId: LIS_ERR_WITH_NO_TEXT_OR_RAWDATA
//
// MessageText:
//
//  This LISTMODE msg contains no free-form text or binary value
//
#define LIS_ERR_WITH_NO_TEXT_OR_RAWDATA  0xCD000005L

//
// MessageId: HST_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define HST_GENERIC_DEVELOPER_ERR        0xC80003E9L

//
// MessageId: HST_ERR_WITH_TEXT_AND_RAWDATA
//
// MessageText:
//
//  This HISTOGRAM msg contains free-formatted text = <%1>
//  It also contains binary data below
//
#define HST_ERR_WITH_TEXT_AND_RAWDATA    0xCD0003EAL

//
// MessageId: HST_ERR_WITH_ADDL_TEXT_ONLY
//
// MessageText:
//
//  This HISTOGRAM msg contains only addl text line = <%1>
//
#define HST_ERR_WITH_ADDL_TEXT_ONLY      0xCD0003EBL

//
// MessageId: HST_ERR_WITH_RAWDATA_ONLY
//
// MessageText:
//
//  This HISTOGRAM msg contains only raw binary data below
//
#define HST_ERR_WITH_RAWDATA_ONLY        0xCD0003ECL

//
// MessageId: HST_ERR_WITH_NO_TEXT_OR_RAWDATA
//
// MessageText:
//
//  This HISTOGRAM msg contains no free-form text or binary value
//
#define HST_ERR_WITH_NO_TEXT_OR_RAWDATA  0xCD0003EDL

//
// MessageId: CP_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define CP_GENERIC_DEVELOPER_ERR         0xC80007D1L

//
// MessageId: CP_PARM_OUT_OF_RANGE
//
// MessageText:
//
//  Out of range parameter = %1
//
#define CP_PARM_OUT_OF_RANGE             0x8D0007D2L

//
// MessageId: CP_UNDEFINED_PARM
//
// MessageText:
//
//  Undefined parameter = %1
//
#define CP_UNDEFINED_PARM                0x8D0007D3L

//
// MessageId: CP_UNDEFINED_CMD
//
// MessageText:
//
//  Undefined command = %1
//
#define CP_UNDEFINED_CMD                 0x8D0007D4L

//
// MessageId: CP_FPGA_LOAD_WARNING
//
// MessageText:
//
//  FPGA load warning, will retry, FPGA identifier = %1
//
#define CP_FPGA_LOAD_WARNING             0x8D0007D5L

//
// MessageId: CP_FPGA_LOAD_FAILURE
//
// MessageText:
//
//  FPGA load failure, FPGA identifier = %1
//
#define CP_FPGA_LOAD_FAILURE             0xCD0007D6L

//
// MessageId: CP_DIAGNOSTIC_ERR
//
// MessageText:
//
//  Diagnostic error, info = %1
//
#define CP_DIAGNOSTIC_ERR                0xCD0007D7L

//
// MessageId: CP_DISK_FULL
//
// MessageText:
//
//  Coincidence processor - disk is full
//
#define CP_DISK_FULL                     0x8D0007D8L

//
// MessageId: CP_DISK_ALMOST_FULL
//
// MessageText:
//
//  Coincidence processor - disk is 90% full
//
#define CP_DISK_ALMOST_FULL              0x8D0007D9L

//
// MessageId: CP_WARNING_FILE_NOT_FOUND
//
// MessageText:
//
//  Warning - File not found or unavailable for write, filename = %1
//
#define CP_WARNING_FILE_NOT_FOUND        0x8D0007DAL

//
// MessageId: CP_FATAL_FILE_NOT_FOUND
//
// MessageText:
//
//  Fatal - File not found or unavailable for write, filename = %1
//
#define CP_FATAL_FILE_NOT_FOUND          0xCD0007DBL

//
// MessageId: CP_BAD_LOAD_FILE_FORMAT
//
// MessageText:
//
//  Bad load file format
//
#define CP_BAD_LOAD_FILE_FORMAT          0xCD0007DCL

//
// MessageId: CP_WARNING_DOWNLOAD_CHKSUM
//
// MessageText:
//
//  Warning - download checksum failed
//
#define CP_WARNING_DOWNLOAD_CHKSUM       0x8D0007DDL

//
// MessageId: CP_FATAL_DOWNLOAD_CHKSUM
//
// MessageText:
//
//  Fatal - download checksum failed
//
#define CP_FATAL_DOWNLOAD_CHKSUM         0xCD0007DEL

//
// MessageId: CP_INFO_CMD_IN_PROGRESS
//
// MessageText:
//
//  New command cannot be executed, background command in progress, cmd = %1
//
#define CP_INFO_CMD_IN_PROGRESS          0x4D0007DFL

//
// MessageId: CP_FATAL_CMD_IN_PROGRESS
//
// MessageText:
//
//  New command cannot be executed, background command in progress, cmd = %1
//
#define CP_FATAL_CMD_IN_PROGRESS         0xCD0007E0L

//
// MessageId: CP_HI_VOLTAGE_ERR
//
// MessageText:
//
//  High voltage error occurred, operation cannot proceed, %1
//
#define CP_HI_VOLTAGE_ERR                0x8D0007E1L

//
// MessageId: CP_WARNING_FILE_FAILURE
//
// MessageText:
//
//  File failure (write?, read?, checksum?, filename?)
//
#define CP_WARNING_FILE_FAILURE          0x8D0007E2L

//
// MessageId: CP_FATAL_FILE_FAILURE
//
// MessageText:
//
//  File failure (write?, read?, checksum?, filename?)
//
#define CP_FATAL_FILE_FAILURE            0xCD0007E3L

//
// MessageId: CP_MEM_ALLOC_ERR
//
// MessageText:
//
//  Memory allocation attempt failed
//
#define CP_MEM_ALLOC_ERR                 0xCD0007E4L

//
// MessageId: CP_FPGA_NOT_LOADING
//
// MessageText:
//
//  FPGA does not load, will retry
//
#define CP_FPGA_NOT_LOADING              0x4D0007E5L

//
// MessageId: CP_FATAL_FPGA_NOT_LOADING
//
// MessageText:
//
//  FPGA does not load, forcing shutdown
//
#define CP_FATAL_FPGA_NOT_LOADING        0xCD0007E6L

//
// MessageId: CP_DEVICE_DRIVER_ERR
//
// MessageText:
//
//  Device driver does not open properly
//
#define CP_DEVICE_DRIVER_ERR             0xCD0007E7L

//
// MessageId: CP_COM_PORT_ERR
//
// MessageText:
//
//  COM port does not open, port = %1
//
#define CP_COM_PORT_ERR                  0xCD0007E8L

//
// MessageId: CP_FAN_FAILURE
//
// MessageText:
//
//  Fan failure occurred
//
#define CP_FAN_FAILURE                   0xCD0007E9L

//
// MessageId: CP_TEMPERATURE_OUT_OF_RANGE
//
// MessageText:
//
//  Temperature is out of range, %1
//
#define CP_TEMPERATURE_OUT_OF_RANGE      0xCD0007EAL

//
// MessageId: CP_HEADS_COMM_ERR
//
// MessageText:
//
//  Informational - Communications failure to heads occurred, %1
//
#define CP_HEADS_COMM_ERR                0x4D0007EBL

//
// MessageId: CP_FATAL_HEADS_COMM_ERR
//
// MessageText:
//
//  Fatal - Communications failure to heads occurred, %1
//
#define CP_FATAL_HEADS_COMM_ERR          0xCD0007ECL

//
// MessageId: CP_VOLTAGE_OUT_OF_RANGE
//
// MessageText:
//
//  Voltage is out of range, %1
//
#define CP_VOLTAGE_OUT_OF_RANGE          0xCD0007EDL

//
// MessageId: CP_GLINK_INIT_ERR
//
// MessageText:
//
//  Cannot initialize GLink, %1
//
#define CP_GLINK_INIT_ERR                0xCD0007EEL

//
// MessageId: CP_DRIVER_COMM_ERR
//
// MessageText:
//
//  Driver communications error occurred, function call = %1
//
#define CP_DRIVER_COMM_ERR               0xCD0007EFL

//
// MessageId: CP_DOWNLOAD_CHKSUM_ERR
//
// MessageText:
//
//  Download checksum occurred, will retry
//
#define CP_DOWNLOAD_CHKSUM_ERR           0x8D0007F0L

//
// MessageId: CP_FATAL_DOWNLOAD_CHKSUM_ERR
//
// MessageText:
//
//  Fatal - Download checksum occurred
//
#define CP_FATAL_DOWNLOAD_CHKSUM_ERR     0x8D0007F1L

//
// MessageId: CP_INFO_SYSTEM_ONLINE
//
// MessageText:
//
//  Coincidence Processor is successfully online
//
#define CP_INFO_SYSTEM_ONLINE            0x4D0007F2L

//
// MessageId: GAN_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define GAN_GENERIC_DEVELOPER_ERR        0xC8000BB9L

//
// MessageId: GAN_P39GC_DRIVER_ERR
//
// MessageText:
//
//  Cannot open the P39GC driver
//
#define GAN_P39GC_DRIVER_ERR             0xCD000BBAL

//
// MessageId: GAN_ALTERA_DEV_RETRY
//
// MessageText:
//
//  Retries were required to configure the Altera Device, number of retries = %1
//
#define GAN_ALTERA_DEV_RETRY             0x8D000BBBL

//
// MessageId: GAN_ALTERA_DEV_CFG_ERR
//
// MessageText:
//
//  Onboard Altera device did not configure
//
#define GAN_ALTERA_DEV_CFG_ERR           0xCD000BBCL

//
// MessageId: GAN_PORT_WRITE_ERR
//
// MessageText:
//
//  Port write error, port address = %1
//
#define GAN_PORT_WRITE_ERR               0xCD000BBDL

//
// MessageId: GAN_PORT_READ_ERR
//
// MessageText:
//
//  Port read error, port address = %1
//
#define GAN_PORT_READ_ERR                0xCD000BBEL

//
// MessageId: GAN_FLASH_DISK_FULL
//
// MessageText:
//
//  Flash Disk is full
//
#define GAN_FLASH_DISK_FULL              0xCD000BBFL

//
// MessageId: GAN_FLASH_DISK_ALMOST_FULL
//
// MessageText:
//
//  Flash Disk has 10% or less space remaining
//
#define GAN_FLASH_DISK_ALMOST_FULL       0x8D000BC0L

//
// MessageId: GAN_INTERFACE_ACCESS_ERR
//
// MessageText:
//
//  Access denied to a protected interface, method = %1
//
#define GAN_INTERFACE_ACCESS_ERR         0x4D000BC1L

//
// MessageId: GAN_ONBOARD_OVERTEMP
//
// MessageText:
//
//  Onboard temperature is above threshold, %1
//
#define GAN_ONBOARD_OVERTEMP             0xCD000BC2L

//
// MessageId: GAN_ONBOARD_UNDERTEMP
//
// MessageText:
//
//  Onboard temperature is below threshold, %1
//
#define GAN_ONBOARD_UNDERTEMP            0xCD000BC3L

//
// MessageId: GAN_GANTRY_OVERTEMP
//
// MessageText:
//
//  Gantry temperature is above threshold, %1
//
#define GAN_GANTRY_OVERTEMP              0xCD000BC4L

//
// MessageId: GAN_GANTRY_UNDERTEMP
//
// MessageText:
//
//  Gantry temperature is below threshold, %1
//
#define GAN_GANTRY_UNDERTEMP             0xCD000BC5L

//
// MessageId: GAN_GANTRY_HI_HUMIDITY
//
// MessageText:
//
//  Gantry humidity is above threshold, %1
//
#define GAN_GANTRY_HI_HUMIDITY           0xCD000BC6L

//
// MessageId: GAN_GANTRY_LO_HUMIDITY
//
// MessageText:
//
//  Gantry humidity is below threshold, %1
//
#define GAN_GANTRY_LO_HUMIDITY           0xCD000BC7L

//
// MessageId: GAN_GANTRY_HI_DEWPOINT
//
// MessageText:
//
//  Gantry dewpoint is above threshold, %1
//
#define GAN_GANTRY_HI_DEWPOINT           0xCD000BC8L

//
// MessageId: GAN_PARM_FILE_ERR
//
// MessageText:
//
//  Could not open parameter file = %1
//
#define GAN_PARM_FILE_ERR                0xCD000BC9L

//
// MessageId: GAN_INCORRECT_PARM_FILE
//
// MessageText:
//
//  Incorrect parameter file = %1
//
#define GAN_INCORRECT_PARM_FILE          0xCD000BCAL

//
// MessageId: GAN_SERIAL_PORT_OPEN_ERR
//
// MessageText:
//
//  Could not open serial port number = %1
//
#define GAN_SERIAL_PORT_OPEN_ERR         0xCD000BCBL

//
// MessageId: GAN_SERIAL_PORT_WRITE_ERR
//
// MessageText:
//
//  Could not write to serial port number = %1
//
#define GAN_SERIAL_PORT_WRITE_ERR        0xCD000BCCL

//
// MessageId: GAN_SERIAL_PORT_NO_RESPONSE
//
// MessageText:
//
//  No response on serial port number = %1
//
#define GAN_SERIAL_PORT_NO_RESPONSE      0xCD000BCDL

//
// MessageId: GAN_SERIAL_PORT_RETRIES
//
// MessageText:
//
//  Serial port retries were required, %1
//
#define GAN_SERIAL_PORT_RETRIES          0x8D000BCEL

//
// MessageId: GAN_COMSERVER_ERR
//
// MessageText:
//
//  Bad HRESULT was returned by COM Server, %1
//
#define GAN_COMSERVER_ERR                0xCD000BCFL

//
// MessageId: GAN_FRONT_PANEL_BUTTON_ERR
//
// MessageText:
//
//  Front panel operator interface button was stuck, button location = %1
//
#define GAN_FRONT_PANEL_BUTTON_ERR       0xCD000BD0L

//
// MessageId: GAN_REAR_PANEL_BUTTON_ERR
//
// MessageText:
//
//  Rear panel operator interface button was stuck, button location = %1
//
#define GAN_REAR_PANEL_BUTTON_ERR        0xCD000BD1L

//
// MessageId: GAN_FRONT_PANEL_BUTTON_PRESSED
//
// MessageText:
//
//  Front panel operator interface button was pressed, button location = %1
//
#define GAN_FRONT_PANEL_BUTTON_PRESSED   0x4D000BD2L

//
// MessageId: GAN_REAR_PANEL_BUTTON_PRESSED
//
// MessageText:
//
//  Rear panel operator interface button was pressed, button location = %1
//
#define GAN_REAR_PANEL_BUTTON_PRESSED    0x4D000BD3L

//
// MessageId: GAN_COM_SERVER_OPEN_ERR
//
// MessageText:
//
//  Could not instantiate COM Server = %1
//
#define GAN_COM_SERVER_OPEN_ERR          0xCD000BD4L

//
// MessageId: GAN_COM_SERVER_IO_ERR
//
// MessageText:
//
//  Could not communicate with COM Server = %1
//
#define GAN_COM_SERVER_IO_ERR            0xCD000BD5L

//
// MessageId: GAN_OVERVIBRATION_WARNING
//
// MessageText:
//
//  Warning - Vibration warning threshold exceeded, spectral file = %1
//
#define GAN_OVERVIBRATION_WARNING        0x8D000BD6L

//
// MessageId: GAN_OVERVIBRATION_ERR
//
// MessageText:
//
//  Error - Vibration threshold exceeded, spectral file = %1
//
#define GAN_OVERVIBRATION_ERR            0xCD000BD7L

//
// MessageId: GAN_HI_INLET_WATER_TEMP
//
// MessageText:
//
//  Inlet water temperature is too high, temperature = %1
//
#define GAN_HI_INLET_WATER_TEMP          0xCD000BD8L

//
// MessageId: GAN_LO_INLET_WATER_TEMP
//
// MessageText:
//
//  Inlet water temperature is too low, temperature = %1
//
#define GAN_LO_INLET_WATER_TEMP          0xCD000BD9L

//
// MessageId: GAN_HI_OUTLET_WATER_TEMP
//
// MessageText:
//
//  Outlet water temperature is too high, temperature = %1
//
#define GAN_HI_OUTLET_WATER_TEMP         0xCD000BDAL

//
// MessageId: GAN_LO_OUTLET_WATER_TEMP
//
// MessageText:
//
//  Outlet water temperature is too low, temperature = %1
//
#define GAN_LO_OUTLET_WATER_TEMP         0xCD000BDBL

//
// MessageId: GAN_LO_COOLANT_FLOWRATE
//
// MessageText:
//
//  Coolant flow rate is too low
//
#define GAN_LO_COOLANT_FLOWRATE          0xCD000BDCL

//
// MessageId: GAN_SYS_IN_ESTOP
//
// MessageText:
//
//  The system is in Estop
//
#define GAN_SYS_IN_ESTOP                 0x4D000BDDL

//
// MessageId: GAN_LVDS_ERR
//
// MessageText:
//
//  LVDS 1-wire interface error occurred at panel location = %1
//
#define GAN_LVDS_ERR                     0x8D000BDEL

//
// MessageId: GAN_INTERCOM_AUDIO_ERR
//
// MessageText:
//
//  Intercom/Audio 1-wire interface error occurred
//
#define GAN_INTERCOM_AUDIO_ERR           0x8D000BDFL

//
// MessageId: GAN_LOCKPIN_SET
//
// MessageText:
//
//  Rotational locking pin is set
//
#define GAN_LOCKPIN_SET                  0x4D000BE0L

//
// MessageId: GAN_LOCKPIN_REMOVED
//
// MessageText:
//
//  Rotational locking pin is removed
//
#define GAN_LOCKPIN_REMOVED              0x4D000BE1L

//
// MessageId: GAN_SMOKE_ALARM_WARNING
//
// MessageText:
//
//  One smoke alarm has been activated
//
#define GAN_SMOKE_ALARM_WARNING          0x8D000BE2L

//
// MessageId: GAN_SMOKE_ALARM_ERR
//
// MessageText:
//
//  Both smoke alarms have been activated
//
#define GAN_SMOKE_ALARM_ERR              0xCD000BE3L

//
// MessageId: GAN_5V_SUPPLY_UPPER_WARNING
//
// MessageText:
//
//  +5V power supply upper warning range, voltage = %1
//
#define GAN_5V_SUPPLY_UPPER_WARNING      0x8D000BE4L

//
// MessageId: GAN_5V_SUPPLY_LOWER_WARNING
//
// MessageText:
//
//  +5V power supply lower warning range, voltage = %1
//
#define GAN_5V_SUPPLY_LOWER_WARNING      0x8D000BE5L

//
// MessageId: GAN_12V_SUPPLY_UPPER_WARNING
//
// MessageText:
//
//  +12V power supply upper warning range, voltage = %1
//
#define GAN_12V_SUPPLY_UPPER_WARNING     0x8D000BE6L

//
// MessageId: GAN_12V_SUPPLY_LOWER_WARNING
//
// MessageText:
//
//  +12V power supply lower warning range, voltage = %1
//
#define GAN_12V_SUPPLY_LOWER_WARNING     0x8D000BE7L

//
// MessageId: GAN_5V_SUPPLY_UPPER_ERR
//
// MessageText:
//
//  +5V power supply above allowed range, voltage = %1
//
#define GAN_5V_SUPPLY_UPPER_ERR          0xCD000BE8L

//
// MessageId: GAN_5V_SUPPLY_LOWER_ERR
//
// MessageText:
//
//  +5V power supply below allowed range, voltage = %1
//
#define GAN_5V_SUPPLY_LOWER_ERR          0xCD000BE9L

//
// MessageId: GAN_12V_SUPPLY_UPPER_ERR
//
// MessageText:
//
//  +12V power supply above allowed range, voltage = %1
//
#define GAN_12V_SUPPLY_UPPER_ERR         0xCD000BEAL

//
// MessageId: GAN_12V_SUPPLY_LOWER_ERR
//
// MessageText:
//
//  +12V power supply below allowed range, voltage = %1
//
#define GAN_12V_SUPPLY_LOWER_ERR         0xCD000BEBL

//
// MessageId: GAN_SLOW_CONTROLLER_FAN
//
// MessageText:
//
//  Gantry controller fan is running slow
//
#define GAN_SLOW_CONTROLLER_FAN          0x8D000BECL

//
// MessageId: GAN_CONTROLLER_FAN_NOT_ROTATING
//
// MessageText:
//
//  Gantry controller fan is not rotating
//
#define GAN_CONTROLLER_FAN_NOT_ROTATING  0xCD000BEDL

//
// MessageId: GAN_SLOW_HEATEX_FAN
//
// MessageText:
//
//  Heat exchanger fan is running slow
//
#define GAN_SLOW_HEATEX_FAN              0x8D000BEEL

//
// MessageId: GAN_HEATEX_FAN_NOT_ROTATING
//
// MessageText:
//
//  Heat exchanger fan is not rotating
//
#define GAN_HEATEX_FAN_NOT_ROTATING      0xCD000BEFL

//
// MessageId: GAN_ALTERA_DEV_CONFIGURED
//
// MessageText:
//
//  The onboard Altera device has been configured, rbf filename = %1
//
#define GAN_ALTERA_DEV_CONFIGURED        0x4D000BF0L

//
// MessageId: GAN_SENS_MON_THREAD_STARTED
//
// MessageText:
//
//  The Sensor Monitoring Thread has been started
//
#define GAN_SENS_MON_THREAD_STARTED      0x4D000BF1L

//
// MessageId: GAN_SENS_MON_THREAD_SHUTDOWN
//
// MessageText:
//
//  The Sensor Monitoring Thread has been shutdown
//
#define GAN_SENS_MON_THREAD_SHUTDOWN     0x4D000BF2L

//
// MessageId: GAN_FANTACH_MON_THREAD_STARTED
//
// MessageText:
//
//  The Fan Tach Monitoring Thread has been started
//
#define GAN_FANTACH_MON_THREAD_STARTED   0x4D000BF3L

//
// MessageId: GAN_FANTACH_MON_THREAD_SHUTDOWN
//
// MessageText:
//
//  The Fan Tach Monitoring Thread has been shutdown
//
#define GAN_FANTACH_MON_THREAD_SHUTDOWN  0x4D000BF4L

//
// MessageId: GAN_FLOWRATE_NORMAL
//
// MessageText:
//
//  Flow Rate has returned to normal
//
#define GAN_FLOWRATE_NORMAL              0x4D000BF5L

//
// MessageId: GAN_HI_ONBOARD_TEMP_WARNING
//
// MessageText:
//
//  High onboard temperature warning, %1
//
#define GAN_HI_ONBOARD_TEMP_WARNING      0x8D000BF6L

//
// MessageId: GAN_LO_ONBOARD_TEMP_WARNING
//
// MessageText:
//
//  Low onboard temperature warning, %1
//
#define GAN_LO_ONBOARD_TEMP_WARNING      0x8D000BF7L

//
// MessageId: GAN_HI_GANTRY_TEMP_WARNING
//
// MessageText:
//
//  High gantry temperature warning, %1
//
#define GAN_HI_GANTRY_TEMP_WARNING       0x8D000BF8L

//
// MessageId: GAN_LO_GANTRY_TEMP_WARNING
//
// MessageText:
//
//  Low gantry temperature warning, %1
//
#define GAN_LO_GANTRY_TEMP_WARNING       0x8D000BF9L

//
// MessageId: GAN_HI_GANTRY_HUMID_WARNING
//
// MessageText:
//
//  High gantry humidity warning, %1
//
#define GAN_HI_GANTRY_HUMID_WARNING      0x8D000BFAL

//
// MessageId: GAN_LO_GANTRY_HUMID_WARNING
//
// MessageText:
//
//  Low gantry humidity warning, %1
//
#define GAN_LO_GANTRY_HUMID_WARNING      0x8D000BFBL

//
// MessageId: GAN_TEMP_NORMAL
//
// MessageText:
//
//  Gantry temperature has returned to normal, %1
//
#define GAN_TEMP_NORMAL                  0x4D000BFCL

//
// MessageId: GAN_HUMIDITY_NORMAL
//
// MessageText:
//
//  Gantry humidity levels have returned to normal, %1
//
#define GAN_HUMIDITY_NORMAL              0x4D000BFDL

//
// MessageId: GAN_GANTRY_HI_DEWPOINT_WARN
//
// MessageText:
//
//  High gantry dewpoint is in warning range, %1
//
#define GAN_GANTRY_HI_DEWPOINT_WARN      0x8D000BFEL

//
// MessageId: GAN_DEWPOINT_NORMAL
//
// MessageText:
//
//  Gantry dewpoint level returned to normal, %1
//
#define GAN_DEWPOINT_NORMAL              0x4D000BFFL

//
// MessageId: GAN_ONBOARD_TEMP_NORMAL
//
// MessageText:
//
//  Onboard Gantry Controller temp has returned to normal, %1
//
#define GAN_ONBOARD_TEMP_NORMAL          0x4D000C00L

//
// MessageId: GAN_ALTERA_DEVICE_CONFIGURING
//
// MessageText:
//
//  The Altera device is configuring, %1
//
#define GAN_ALTERA_DEVICE_CONFIGURING    0x4D000C01L

//
// MessageId: GAN_ALTERA_DEVICE_CONFIG_ERR
//
// MessageText:
//
//  The Altera device did NOT configure correctly, %1
//
#define GAN_ALTERA_DEVICE_CONFIG_ERR     0xCD000C02L

//
// MessageId: GAN_ALTERA_DEVICE_CONFIGURED
//
// MessageText:
//
//  The Altera device has configured successfully, %1
//
#define GAN_ALTERA_DEVICE_CONFIGURED     0x4D000C03L

//
// MessageId: REC_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define REC_GENERIC_DEVELOPER_ERR        0xC8000FA1L

//
// MessageId: REC_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define REC_GENERIC_DEVELOPER_WARNING    0x88000FA2L

//
// MessageId: REC_FILE_DOESNT_EXIST
//
// MessageText:
//
//  %1
//
#define REC_FILE_DOESNT_EXIST            0xC8000FA3L

//
// MessageId: REC_FILE_CREATE
//
// MessageText:
//
//  %1
//
#define REC_FILE_CREATE                  0xC8000FA4L

//
// MessageId: REC_BABELFISH_UNSUPPORTED_LANG
//
// MessageText:
//
//  %1
//
#define REC_BABELFISH_UNSUPPORTED_LANG   0xC8000FA5L

//
// MessageId: REC_BABELFISH_FILE_DOESNT_EXIST
//
// MessageText:
//
//  %1
//
#define REC_BABELFISH_FILE_DOESNT_EXIST  0xC8000FA6L

//
// MessageId: REC_BABELFISH_FILEFORMAT
//
// MessageText:
//
//  %1
//
#define REC_BABELFISH_FILEFORMAT         0xC8000FA7L

//
// MessageId: REC_ITF_HEADER_ERROR_MAINHEADER_SET
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_MAINHEADER_SET 0xC8000FA8L

//
// MessageId: REC_ITF_HEADER_ERROR_MAINHEADER_REMOVE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_MAINHEADER_REMOVE 0xC8000FA9L

//
// MessageId: REC_ITF_HEADER_ERROR_SUBHEADER_SET
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_SUBHEADER_SET 0xC8000FAAL

//
// MessageId: REC_ITF_HEADER_ERROR_SUBHEADER_REMOVE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_SUBHEADER_REMOVE 0xC8000FABL

//
// MessageId: REC_ITF_HEADER_ERROR_DATE_VALUE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_DATE_VALUE  0xC8000FACL

//
// MessageId: REC_ITF_HEADER_ERROR_DATE_INVALID
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_DATE_INVALID 0xC8000FADL

//
// MessageId: REC_ITF_HEADER_ERROR_GREGORIAN
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_GREGORIAN   0xC8000FAEL

//
// MessageId: REC_ITF_HEADER_ERROR_TIME_VALUE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_TIME_VALUE  0xC8000FAFL

//
// MessageId: REC_ITF_HEADER_ERROR_TIME_INVALID
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_TIME_INVALID 0xC8000FB0L

//
// MessageId: REC_ITF_HEADER_ERROR_SEX_INVALID
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_SEX_INVALID 0xC8000FB1L

//
// MessageId: REC_ITF_HEADER_ERROR_ORIENTATION
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_ORIENTATION 0xC8000FB2L

//
// MessageId: REC_ITF_HEADER_ERROR_DIRECTION
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_DIRECTION   0xC8000FB3L

//
// MessageId: REC_ITF_HEADER_ERROR_PET_DATATYPE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_PET_DATATYPE 0xC8000FB4L

//
// MessageId: REC_ITF_HEADER_ERROR_NUMBER_FORMAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_NUMBER_FORMAT 0xC8000FB5L

//
// MessageId: REC_ITF_HEADER_ERROR_SEPTA_FORMAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_SEPTA_FORMAT 0xC8000FB6L

//
// MessageId: REC_ITF_HEADER_ERROR_DETMOT_FORMAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_DETMOT_FORMAT 0xC8000FB7L

//
// MessageId: REC_ITF_HEADER_ERROR_EXAMTYPE_FORMAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_EXAMTYPE_FORMAT 0xC8000FB8L

//
// MessageId: REC_ITF_HEADER_ERROR_ACQCOND_FORMAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_ACQCOND_FORMAT 0xC8000FB9L

//
// MessageId: REC_ITF_HEADER_ERROR_DATATYPE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_DATATYPE    0xC8000FBAL

//
// MessageId: REC_ITF_HEADER_ERROR_ENDIAN
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_ENDIAN      0xC8000FBBL

//
// MessageId: REC_ITF_HEADER_ERROR_KEY_INVALID
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_KEY_INVALID 0xC8000FBCL

//
// MessageId: REC_ITF_HEADER_ERROR_KEY_ORDER
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_KEY_ORDER   0xC8000FBDL

//
// MessageId: REC_ITF_HEADER_ERROR_KEY_MISSING
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_KEY_MISSING 0xC8000FBEL

//
// MessageId: REC_ITF_HEADER_ERROR_MULTIPLE_KEY
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_MULTIPLE_KEY 0xC8000FBFL

//
// MessageId: REC_ITF_HEADER_ERROR_SEPARATOR_MISSING
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_SEPARATOR_MISSING 0xC8000FC0L

//
// MessageId: REC_ITF_HEADER_ERROR_UNITFORMAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_UNITFORMAT  0xC8000FC1L

//
// MessageId: REC_ITF_HEADER_ERROR_UNIT_WRONG
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_UNIT_WRONG  0xC8000FC2L

//
// MessageId: REC_ITF_HEADER_ERROR_INDEX_TOO_LARGE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_INDEX_TOO_LARGE 0xC8000FC3L

//
// MessageId: REC_ITF_HEADER_ERROR_INDEX_TOO_SMALL
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_INDEX_TOO_SMALL 0xC8000FC4L

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE       0xC8000FC5L

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE_MISSING
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE_MISSING 0xC8000FC6L

//
// MessageId: REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING 0xC8000FC7L

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL 0xC8000FC8L

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE 0xC8000FC9L

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE_FORMAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE_FORMAT 0xC8000FCAL

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE_RANGE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE_RANGE 0xC8000FCBL

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG 0xC8000FCCL

//
// MessageId: REC_ITF_HEADER_ERROR_VALUE_TYPE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_VALUE_TYPE  0xC8000FCDL

//
// MessageId: REC_ITF_HEADER_ERROR_REMOVE_KEY
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_REMOVE_KEY  0xC8000FCEL

//
// MessageId: REC_ITF_HEADER_ERROR_SCANNERTYPE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_SCANNERTYPE 0xC8000FCFL

//
// MessageId: REC_ITF_HEADER_ERROR_DECAYCORR
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_DECAYCORR   0xC8000FD0L

//
// MessageId: REC_ITF_HEADER_ERROR_SL_ORIENTATION
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_SL_ORIENTATION 0xC8000FD1L

//
// MessageId: REC_ITF_HEADER_ERROR_RANDCORR
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_RANDCORR    0xC8000FD2L

//
// MessageId: REC_UNKNOWN_EXCEPTION
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_EXCEPTION            0xC8000FD3L

//
// MessageId: REC_PAR_IO_DATATYPE
//
// MessageText:
//
//  %1
//
#define REC_PAR_IO_DATATYPE              0xC8000FD4L

//
// MessageId: REC_CANT_START_PROCESS
//
// MessageText:
//
//  %1
//
#define REC_CANT_START_PROCESS           0xC8000FD5L

//
// MessageId: REC_CLIENT_DOESNT_CONNECT
//
// MessageText:
//
//  %1
//
#define REC_CLIENT_DOESNT_CONNECT        0xC8000FD6L

//
// MessageId: REC_FILE_TOO_SMALL
//
// MessageText:
//
//  %1
//
#define REC_FILE_TOO_SMALL               0xC8000FD7L

//
// MessageId: REC_BUFFER_OVERRUN
//
// MessageText:
//
//  %1
//
#define REC_BUFFER_OVERRUN               0xC8000FD8L

//
// MessageId: REC_VAX_DOUBLE
//
// MessageText:
//
//  %1
//
#define REC_VAX_DOUBLE                   0xC8000FD9L

//
// MessageId: REC_UNKNOWN_ECAT7_MATRIXTYPE
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_ECAT7_MATRIXTYPE     0xC8000FDAL

//
// MessageId: REC_ECAT7_MATRIXHEADER_MISSING
//
// MessageText:
//
//  %1
//
#define REC_ECAT7_MATRIXHEADER_MISSING   0xC8000FDBL

//
// MessageId: REC_FILESIZE_WRONG
//
// MessageText:
//
//  %1
//
#define REC_FILESIZE_WRONG               0xC8000FDCL

//
// MessageId: REC_EM_MATRIX_NUMBER_WRONG
//
// MessageText:
//
//  %1
//
#define REC_EM_MATRIX_NUMBER_WRONG       0xC8000FDDL

//
// MessageId: REC_TRA_MATRIX_NUMBER_WRONG
//
// MessageText:
//
//  %1
//
#define REC_TRA_MATRIX_NUMBER_WRONG      0xC8000FDEL

//
// MessageId: REC_INVALID_ECAT7_MATRIXTYPE
//
// MessageText:
//
//  %1
//
#define REC_INVALID_ECAT7_MATRIXTYPE     0xC8000FDFL

//
// MessageId: REC_UNKNOWN_ECAT7_DATATYPE
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_ECAT7_DATATYPE       0xC8000FE0L

//
// MessageId: REC_FBP_WIDTH_ODD
//
// MessageText:
//
//  %1
//
#define REC_FBP_WIDTH_ODD                0xC8000FE1L

//
// MessageId: REC_FBP_ANGLES_ODD
//
// MessageText:
//
//  %1
//
#define REC_FBP_ANGLES_ODD               0xC8000FE2L

//
// MessageId: REC_FBP_BINS_ODD
//
// MessageText:
//
//  %1
//
#define REC_FBP_BINS_ODD                 0xC8000FE3L

//
// MessageId: REC_FFT_DIM_TOO_SMALL
//
// MessageText:
//
//  %1
//
#define REC_FFT_DIM_TOO_SMALL            0xC8000FE4L

//
// MessageId: REC_FFT_INIT
//
// MessageText:
//
//  %1
//
#define REC_FFT_INIT                     0xC8000FE5L

//
// MessageId: REC_FHT_DIM_NOT_POWER2
//
// MessageText:
//
//  %1
//
#define REC_FHT_DIM_NOT_POWER2           0xC8000FE6L

//
// MessageId: REC_FHT_DIM_TOO_SMALL
//
// MessageText:
//
//  %1
//
#define REC_FHT_DIM_TOO_SMALL            0xC8000FE7L

//
// MessageId: REC_FHT_INIT
//
// MessageText:
//
//  %1
//
#define REC_FHT_INIT                     0xC8000FE8L

//
// MessageId: REC_FFT_RADIX_TOO_LARGE
//
// MessageText:
//
//  %1
//
#define REC_FFT_RADIX_TOO_LARGE          0xC8000FE9L

//
// MessageId: REC_FORE_BINS_TOO_SMALL
//
// MessageText:
//
//  %1
//
#define REC_FORE_BINS_TOO_SMALL          0xC8000FEAL

//
// MessageId: REC_FORE_ANGLES_TOO_SMALL
//
// MessageText:
//
//  %1
//
#define REC_FORE_ANGLES_TOO_SMALL        0xC8000FEBL

//
// MessageId: REC_QUEUE_OVERRUN
//
// MessageText:
//
//  %1
//
#define REC_QUEUE_OVERRUN                0xC8000FECL

//
// MessageId: REC_FILE_CREATE_ERROR
//
// MessageText:
//
//  %1
//
#define REC_FILE_CREATE_ERROR            0xC8000FEDL

//
// MessageId: REC_QUEUE_FULL
//
// MessageText:
//
//  %1
//
#define REC_QUEUE_FULL                   0xC8000FEEL

//
// MessageId: REC_PRIORITY_RANGE
//
// MessageText:
//
//  %1
//
#define REC_PRIORITY_RANGE               0xC8000FEFL

//
// MessageId: REC_MATRIX_NUMBER_RANGE
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_NUMBER_RANGE          0xC8000FF0L

//
// MessageId: REC_JOB_ID
//
// MessageText:
//
//  %1
//
#define REC_JOB_ID                       0xC8000FF1L

//
// MessageId: REC_MATRIX_ADD
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_ADD                   0xC8000FF2L

//
// MessageId: REC_MATRIX_SUB
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_SUB                   0xC8000FF3L

//
// MessageId: REC_MATRIX_MULT
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_MULT                  0xC8000FF4L

//
// MessageId: REC_MATRIX_INDEX
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_INDEX                 0xC8000FF5L

//
// MessageId: REC_MATRIX_IDENT
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_IDENT                 0xC8000FF6L

//
// MessageId: REC_MATRIX_INVERS
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_INVERS                0xC8000FF7L

//
// MessageId: REC_MATRIX_SINGULAR
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_SINGULAR              0xC8000FF8L

//
// MessageId: REC_BPJ_WIDTH_ODD
//
// MessageText:
//
//  %1
//
#define REC_BPJ_WIDTH_ODD                0xC8000FF9L

//
// MessageId: REC_QUEUE_OUT_OF_MEMORY
//
// MessageText:
//
//  %1
//
#define REC_QUEUE_OUT_OF_MEMORY          0xC8000FFAL

//
// MessageId: REC_RS_OUT_OF_MEMORY
//
// MessageText:
//
//  %1
//
#define REC_RS_OUT_OF_MEMORY             0xC8000FFBL

//
// MessageId: REC_STREAM_EMPTY
//
// MessageText:
//
//  %1
//
#define REC_STREAM_EMPTY                 0xC8000FFCL

//
// MessageId: REC_SOCKET_ERROR_INIT
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_INIT            0xC8000FFDL

//
// MessageId: REC_SOCKET_ERROR_CREATE_S
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_CREATE_S        0xC8000FFEL

//
// MessageId: REC_SOCKET_ERROR_CREATE_C
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_CREATE_C        0xC8000FFFL

//
// MessageId: REC_SOCKET_ERROR_CONNECT
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_CONNECT         0xC8001000L

//
// MessageId: REC_SOCKET_ERROR_CONF
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_CONF            0xC8001001L

//
// MessageId: REC_SOCKET_ERROR_BIND
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_BIND            0xC8001002L

//
// MessageId: REC_SOCKET_ERROR_GETPORT
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_GETPORT         0xC8001003L

//
// MessageId: REC_SOCKET_ERROR_LISTEN
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_LISTEN          0xC8001004L

//
// MessageId: REC_SOCKET_ERROR_ACCEPT
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_ACCEPT          0xC8001005L

//
// MessageId: REC_SOCKET_ERROR_BROKEN
//
// MessageText:
//
//  %1
//
#define REC_SOCKET_ERROR_BROKEN          0xC8001006L

//
// MessageId: REC_UNKNOWN_CRYSTAL_MATERIAL
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_CRYSTAL_MATERIAL     0xC8001007L

//
// MessageId: REC_IMAGESIZE_TOO_LARGE
//
// MessageText:
//
//  %1
//
#define REC_IMAGESIZE_TOO_LARGE          0xC8001008L

//
// MessageId: REC_TOO_MANY_CRYSTAL_LAYERS
//
// MessageText:
//
//  %1
//
#define REC_TOO_MANY_CRYSTAL_LAYERS      0xC8001009L

//
// MessageId: REC_UMAP_ERROR_TL_OUTSIDE
//
// MessageText:
//
//  %1
//
#define REC_UMAP_ERROR_TL_OUTSIDE        0xC800100AL

//
// MessageId: REC_UMAP_ERROR_LL_OUTSIDE
//
// MessageText:
//
//  %1
//
#define REC_UMAP_ERROR_LL_OUTSIDE        0xC800100BL

//
// MessageId: REC_UMAP_ERROR_TR_OUTSIDE
//
// MessageText:
//
//  %1
//
#define REC_UMAP_ERROR_TR_OUTSIDE        0xC800100CL

//
// MessageId: REC_UMAP_ERROR_LR_OUTSIDE
//
// MessageText:
//
//  %1
//
#define REC_UMAP_ERROR_LR_OUTSIDE        0xC800100DL

//
// MessageId: REC_UMAP_ERROR_THRESHOLD
//
// MessageText:
//
//  %1
//
#define REC_UMAP_ERROR_THRESHOLD         0xC800100EL

//
// MessageId: REC_BSPLINE_ERROR
//
// MessageText:
//
//  %1
//
#define REC_BSPLINE_ERROR                0xC800100FL

//
// MessageId: REC_BINSIZE_TOO_LARGE
//
// MessageText:
//
//  %1
//
#define REC_BINSIZE_TOO_LARGE            0xC8001010L

//
// MessageId: REC_UNKNOWN_DCOM_COMMAND
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_DCOM_COMMAND         0xC8001011L

//
// MessageId: REC_UNKNOWN_QUEUE_COMMAND
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_QUEUE_COMMAND        0xC8001012L

//
// MessageId: REC_UNKNOWN_RS_COMMAND
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_RS_COMMAND           0xC8001013L

//
// MessageId: REC_CONFIGURATION_ERROR
//
// MessageText:
//
//  %1
//
#define REC_CONFIGURATION_ERROR          0xC8001014L

//
// MessageId: REC_TERMINATE_ERROR
//
// MessageText:
//
//  %1
//
#define REC_TERMINATE_ERROR              0xC8001015L

//
// MessageId: REC_IMAGESIZE_WRONG
//
// MessageText:
//
//  %1
//
#define REC_IMAGESIZE_WRONG              0xC8001016L

//
// MessageId: REC_NONNEG_CONSTRAINT
//
// MessageText:
//
//  %1
//
#define REC_NONNEG_CONSTRAINT            0xC8001017L

//
// MessageId: REC_ECAT7_MATRIX_NUMBER_WRONG
//
// MessageText:
//
//  %1
//
#define REC_ECAT7_MATRIX_NUMBER_WRONG    0xC8001018L

//
// MessageId: REC_IMAGE_FILE_INVALID
//
// MessageText:
//
//  %1
//
#define REC_IMAGE_FILE_INVALID           0xC8001019L

//
// MessageId: REC_IMAGE_FILE_EXISTS
//
// MessageText:
//
//  %1
//
#define REC_IMAGE_FILE_EXISTS            0xC800101AL

//
// MessageId: REC_TRA_FILE_INVALID
//
// MessageText:
//
//  %1
//
#define REC_TRA_FILE_INVALID             0xC800101BL

//
// MessageId: REC_TRA_FILE_MISSING
//
// MessageText:
//
//  %1
//
#define REC_TRA_FILE_MISSING             0xC800101CL

//
// MessageId: REC_BLANK_FILE_INVALID
//
// MessageText:
//
//  %1
//
#define REC_BLANK_FILE_INVALID           0xC800101DL

//
// MessageId: REC_BLANK_FILE_MISSING
//
// MessageText:
//
//  %1
//
#define REC_BLANK_FILE_MISSING           0xC800101EL

//
// MessageId: REC_EM_FILE_INVALID
//
// MessageText:
//
//  %1
//
#define REC_EM_FILE_INVALID              0xC800101FL

//
// MessageId: REC_NORM_FILE_INVALID
//
// MessageText:
//
//  %1
//
#define REC_NORM_FILE_INVALID            0xC8001020L

//
// MessageId: REC_NORM_FILE_MISSING
//
// MessageText:
//
//  %1
//
#define REC_NORM_FILE_MISSING            0xC8001021L

//
// MessageId: REC_DEBUG_PATH_INVALID
//
// MessageText:
//
//  %1
//
#define REC_DEBUG_PATH_INVALID           0xC8001022L

//
// MessageId: REC_QC_PATH_INVALID
//
// MessageText:
//
//  %1
//
#define REC_QC_PATH_INVALID              0xC8001023L

//
// MessageId: REC_OSEM_SUBSETS
//
// MessageText:
//
//  %1
//
#define REC_OSEM_SUBSETS                 0xC8001024L

//
// MessageId: REC_UMAP_RECON_OUT_OF_RANGE
//
// MessageText:
//
//  %1
//
#define REC_UMAP_RECON_OUT_OF_RANGE      0xC8001025L

//
// MessageId: REC_ITERATIONS
//
// MessageText:
//
//  %1
//
#define REC_ITERATIONS                   0xC8001026L

//
// MessageId: REC_SUBSETS
//
// MessageText:
//
//  %1
//
#define REC_SUBSETS                      0xC8001027L

//
// MessageId: REC_RADIAL_REBINNING
//
// MessageText:
//
//  %1
//
#define REC_RADIAL_REBINNING             0xC8001028L

//
// MessageId: REC_AZIMUTHAL_REBINNING
//
// MessageText:
//
//  %1
//
#define REC_AZIMUTHAL_REBINNING          0xC8001029L

//
// MessageId: REC_AXIAL_REBINNING
//
// MessageText:
//
//  %1
//
#define REC_AXIAL_REBINNING              0xC800102AL

//
// MessageId: REC_GANTRY_MODEL
//
// MessageText:
//
//  %1
//
#define REC_GANTRY_MODEL                 0xC800102BL

//
// MessageId: REC_OOR_FRAME
//
// MessageText:
//
//  %1
//
#define REC_OOR_FRAME                    0xC800102CL

//
// MessageId: REC_OOR_GATE
//
// MessageText:
//
//  %1
//
#define REC_OOR_GATE                     0xC800102DL

//
// MessageId: REC_OOR_BED
//
// MessageText:
//
//  %1
//
#define REC_OOR_BED                      0xC800102EL

//
// MessageId: REC_OOR_ENERGY
//
// MessageText:
//
//  %1
//
#define REC_OOR_ENERGY                   0xC800102FL

//
// MessageId: REC_OOR_TOF
//
// MessageText:
//
//  %1
//
#define REC_OOR_TOF                      0xC8001030L

//
// MessageId: REC_OOR_SCAN_TYPE
//
// MessageText:
//
//  %1
//
#define REC_OOR_SCAN_TYPE                0xC8001031L

//
// MessageId: REC_FILE_READ
//
// MessageText:
//
//  %1
//
#define REC_FILE_READ                    0xC8001032L

//
// MessageId: REC_FILE_WRITE
//
// MessageText:
//
//  %1
//
#define REC_FILE_WRITE                   0xC8001033L

//
// MessageId: REC_UNKNOWN_GANTRY
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_GANTRY               0xC8001034L

//
// MessageId: REC_REGKEY_ACCESS
//
// MessageText:
//
//  %1
//
#define REC_REGKEY_ACCESS                0xC8001035L

//
// MessageId: REC_REGKEY_TYPE
//
// MessageText:
//
//  %1
//
#define REC_REGKEY_TYPE                  0xC8001036L

//
// MessageId: REC_REGSUBKEY_ACCESS
//
// MessageText:
//
//  %1
//
#define REC_REGSUBKEY_ACCESS             0xC8001037L

//
// MessageId: REC_REGSUBKEY_WRITE
//
// MessageText:
//
//  %1
//
#define REC_REGSUBKEY_WRITE              0xC8001038L

//
// MessageId: REC_SPECTOR_NODE_MISSING
//
// MessageText:
//
//  %1
//
#define REC_SPECTOR_NODE_MISSING         0xC8001039L

//
// MessageId: REC_SPECTOR_ID_MISSING
//
// MessageText:
//
//  %1
//
#define REC_SPECTOR_ID_MISSING           0xC800103AL

//
// MessageId: REC_SPECTOR_FRAME_MISSING
//
// MessageText:
//
//  %1
//
#define REC_SPECTOR_FRAME_MISSING        0xC800103BL

//
// MessageId: REC_SPECTOR_STEP_MISSING
//
// MessageText:
//
//  %1
//
#define REC_SPECTOR_STEP_MISSING         0xC800103CL

//
// MessageId: REC_UNKNOWN_SPECTOR_COMMAND
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_SPECTOR_COMMAND      0xC800103DL

//
// MessageId: REC_UNKNOWN_QUEUE_SPECTOR_COMMAND
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_QUEUE_SPECTOR_COMMAND 0xC800103EL

//
// MessageId: REC_QUEUE_CONNECTED
//
// MessageText:
//
//  %1
//
#define REC_QUEUE_CONNECTED              0xC800103FL

//
// MessageId: REC_QUEUE_NOT_CONNECTED
//
// MessageText:
//
//  %1
//
#define REC_QUEUE_NOT_CONNECTED          0xC8001040L

//
// MessageId: REC_STUDY_RECONSTRUCTED
//
// MessageText:
//
//  %1
//
#define REC_STUDY_RECONSTRUCTED          0x48001041L

//
// MessageId: REC_MATRIX_RECONSTRUCTED
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_RECONSTRUCTED         0x48001042L

//
// MessageId: REC_ITF_HEADER_ERROR_NORMHEADER_SET
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_NORMHEADER_SET 0xC8001043L

//
// MessageId: REC_ITF_HEADER_ERROR_NORMHEADER_REMOVE
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_NORMHEADER_REMOVE 0xC8001044L

//
// MessageId: REC_FFT_DIM_NOT_EVEN
//
// MessageText:
//
//  %1
//
#define REC_FFT_DIM_NOT_EVEN             0xC8001045L

//
// MessageId: REC_RECONSTRUCTION_STEP
//
// MessageText:
//
//  %1
//
#define REC_RECONSTRUCTION_STEP          0x48001046L

//
// MessageId: REC_MATRIX_START
//
// MessageText:
//
//  %1
//
#define REC_MATRIX_START                 0x48001047L

//
// MessageId: REC_STUDY_START
//
// MessageText:
//
//  %1
//
#define REC_STUDY_START                  0x48001048L

//
// MessageId: REC_TERMINATE_SLAVE_ERROR
//
// MessageText:
//
//  %1
//
#define REC_TERMINATE_SLAVE_ERROR        0xC8001049L

//
// MessageId: REC_SEGMENTATION_PARAM_FORMAT
//
// MessageText:
//
//  %1
//
#define REC_SEGMENTATION_PARAM_FORMAT    0xC800104AL

//
// MessageId: REC_UNKNOWN_RED_COMMAND
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_RED_COMMAND          0xC800104BL

//
// MessageId: REC_UNKNOWN_OSEM_COMMAND
//
// MessageText:
//
//  %1
//
#define REC_UNKNOWN_OSEM_COMMAND         0xC800104CL

//
// MessageId: REC_SYNGO_CANCEL
//
// MessageText:
//
//  %1
//
#define REC_SYNGO_CANCEL                 0xC800104DL

//
// MessageId: REC_ITF_HEADER_ERROR_PROCSTAT
//
// MessageText:
//
//  %1
//
#define REC_ITF_HEADER_ERROR_PROCSTAT    0xC800104EL

//
// MessageId: REC_OFFSET_MATRIX
//
// MessageText:
//
//  %1
//
#define REC_OFFSET_MATRIX                0xC8001050L

//
// MessageId: REC_MEMORY_BLOCK
//
// MessageText:
//
//  %1
//
#define REC_MEMORY_BLOCK                 0xC8001051L

//
// MessageId: REC_SWAP_PATH_INVALID
//
// MessageText:
//
//  %1
//
#define REC_SWAP_PATH_INVALID            0xC8001052L

//
// MessageId: REC_HOME_UNKNOWN
//
// MessageText:
//
//  %1
//
#define REC_HOME_UNKNOWN                 0xC8001053L

//
// MessageId: REC_BED_NOT_FOUND
//
// MessageText:
//
//  %1
//
#define REC_BED_NOT_FOUND                0xC8001054L

//
// MessageId: REC_IDL_LIB
//
// MessageText:
//
//  %1
//
#define REC_IDL_LIB                      0xC8001055L

//
// MessageId: REC_IDL_SYMBOL
//
// MessageText:
//
//  %1
//
#define REC_IDL_SYMBOL                   0xC8001056L

//
// MessageId: REC_IDL_INIT
//
// MessageText:
//
//  %1
//
#define REC_IDL_INIT                     0xC8001057L

//
// MessageId: REC_IDL_RUN
//
// MessageText:
//
//  %1
//
#define REC_IDL_RUN                      0xC8001058L

//
// MessageId: REC_IDL_EXECUTE
//
// MessageText:
//
//  %1
//
#define REC_IDL_EXECUTE                  0xC8001059L

//
// MessageId: REC_IDL_SCALING
//
// MessageText:
//
//  %1
//
#define REC_IDL_SCALING                  0xC800105AL

//
// MessageId: REC_E7CONNECT
//
// MessageText:
//
//  %1
//
#define REC_E7CONNECT                    0xC800105BL

//
// MessageId: REC_GANTRY_MODEL_MATCH
//
// MessageText:
//
//  %1
//
#define REC_GANTRY_MODEL_MATCH           0xC800105CL

//
// MessageId: REC_GANTRY_MODEL_UNKNOWN_VALUE
//
// MessageText:
//
//  %1
//
#define REC_GANTRY_MODEL_UNKNOWN_VALUE   0xC800105DL

//
// MessageId: REC_STOPWATCH_INIT
//
// MessageText:
//
//  %1
//
#define REC_STOPWATCH_INIT               0xC800105EL

//
// MessageId: REC_STOPWATCH_RUN
//
// MessageText:
//
//  %1
//
#define REC_STOPWATCH_RUN                0xC800105FL

//
// MessageId: REC_DB_GETDATAACCESS
//
// MessageText:
//
//  %1
//
#define REC_DB_GETDATAACCESS             0xC8001060L

//
// MessageId: REC_DB_INITDATAACCESS
//
// MessageText:
//
//  %1
//
#define REC_DB_INITDATAACCESS            0xC8001061L

//
// MessageId: REC_DB_GET_DBPATIENT
//
// MessageText:
//
//  %1
//
#define REC_DB_GET_DBPATIENT             0xC8001062L

//
// MessageId: REC_DB_DBPATIENT_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_DBPATIENT_INTERF          0xC8001063L

//
// MessageId: REC_DB_GET_DBSTUDY
//
// MessageText:
//
//  %1
//
#define REC_DB_GET_DBSTUDY               0xC8001064L

//
// MessageId: REC_DB_DBSTUDY_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_DBSTUDY_INTERF            0xC8001065L

//
// MessageId: REC_DB_GET_DBSERIES
//
// MessageText:
//
//  %1
//
#define REC_DB_GET_DBSERIES              0xC8001066L

//
// MessageId: REC_DB_DBSERIES_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_DBSERIES_INTERF           0xC8001067L

//
// MessageId: REC_DB_GET_DBIMAGE
//
// MessageText:
//
//  %1
//
#define REC_DB_GET_DBIMAGE               0xC8001068L

//
// MessageId: REC_DB_DBIMAGE_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_DBIMAGE_INTERF            0xC8001069L

//
// MessageId: REC_DB_GET_DBEQUIPMENT
//
// MessageText:
//
//  %1
//
#define REC_DB_GET_DBEQUIPMENT           0xC800106AL

//
// MessageId: REC_DB_DBEQUIPMENT_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_DBEQUIPMENT_INTERF        0xC800106BL

//
// MessageId: REC_DB_GET_DBFOR
//
// MessageText:
//
//  %1
//
#define REC_DB_GET_DBFOR                 0xC800106CL

//
// MessageId: REC_DB_DBFOR_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_DBFOR_INTERF              0xC800106DL

//
// MessageId: REC_DB_GET_DBPETOBJECT
//
// MessageText:
//
//  %1
//
#define REC_DB_GET_DBPETOBJECT           0xC800106EL

//
// MessageId: REC_DB_DBPETOBJECT_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_DBPETOBJECT_INTERF        0xC800106FL

//
// MessageId: REC_DB_IMAGEPIXEL_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_IMAGEPIXEL_INTERF         0xC8001070L

//
// MessageId: REC_DB_IMAGEPLANE_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_IMAGEPLANE_INTERF         0xC8001071L

//
// MessageId: REC_DB_SOPCOMMON_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_SOPCOMMON_INTERF          0xC8001072L

//
// MessageId: REC_DB_PETIMAGE_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_PETIMAGE_INTERF           0xC8001073L

//
// MessageId: REC_DB_PETSERIES_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_PETSERIES_INTERF          0xC8001074L

//
// MessageId: REC_DB_PETISOTOPE_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_PETISOTOPE_INTERF         0xC8001075L

//
// MessageId: REC_DB_PATIENTSTUDY_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_PATIENTSTUDY_INTERF       0xC8001076L

//
// MessageId: REC_DB_NMPETPATIENTORIENTATION_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_NMPETPATIENTORIENTATION_INTERF 0xC8001077L

//
// MessageId: REC_DB_CTIMAGE_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_CTIMAGE_INTERF            0xC8001078L

//
// MessageId: REC_DB_UNKNOWN_PATGUID
//
// MessageText:
//
//  %1
//
#define REC_DB_UNKNOWN_PATGUID           0xC8001079L

//
// MessageId: REC_DB_UNKNOWN_STUDYGUID
//
// MessageText:
//
//  %1
//
#define REC_DB_UNKNOWN_STUDYGUID         0xC800107AL

//
// MessageId: REC_DB_UNKNOWN_SERIESGUID
//
// MessageText:
//
//  %1
//
#define REC_DB_UNKNOWN_SERIESGUID        0xC800107BL

//
// MessageId: REC_DB_UNKNOWN_PATID
//
// MessageText:
//
//  %1
//
#define REC_DB_UNKNOWN_PATID             0xC800107CL

//
// MessageId: REC_DB_UNKNOWN_PATNAME
//
// MessageText:
//
//  %1
//
#define REC_DB_UNKNOWN_PATNAME           0xC800107DL

//
// MessageId: REC_DB_UNKNOWN_FOR
//
// MessageText:
//
//  %1
//
#define REC_DB_UNKNOWN_FOR               0xC800107EL

//
// MessageId: REC_DB_UNKNOWN_EQUIPMENT
//
// MessageText:
//
//  %1
//
#define REC_DB_UNKNOWN_EQUIPMENT         0xC800107FL

//
// MessageId: REC_DB_RETRIEVE_SERIES
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_SERIES           0xC8001080L

//
// MessageId: REC_DB_RETRIEVE_IMAGES
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_IMAGES           0xC8001081L

//
// MessageId: REC_DB_RETRIEVE_PATIENTS
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_PATIENTS         0xC8001082L

//
// MessageId: REC_DB_RETRIEVE_STUDIES
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_STUDIES          0xC8001083L

//
// MessageId: REC_DB_RETRIEVE_ENERGYWINDOW
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_ENERGYWINDOW     0xC8001084L

//
// MessageId: REC_DB_RETRIEVE_CORRECTIONFLAGS
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_CORRECTIONFLAGS  0xC8001085L

//
// MessageId: REC_DB_RETRIEVE_RADIOINFO
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_RADIOINFO        0xC8001086L

//
// MessageId: REC_DB_RETRIEVE_IMAGEPOSITION
//
// MessageText:
//
//  %1
//
#define REC_DB_RETRIEVE_IMAGEPOSITION    0xC8001087L

//
// MessageId: REC_DB_CARS_INTERF
//
// MessageText:
//
//  %1
//
#define REC_DB_CARS_INTERF               0xC8001088L

//
// MessageId: REC_DB_MAKE_EQUIPMENT
//
// MessageText:
//
//  %1
//
#define REC_DB_MAKE_EQUIPMENT            0xC8001089L

//
// MessageId: REC_DB_MAKE_FOR
//
// MessageText:
//
//  %1
//
#define REC_DB_MAKE_FOR                  0xC800108AL

//
// MessageId: REC_DB_MAKE_IMAGE
//
// MessageText:
//
//  %1
//
#define REC_DB_MAKE_IMAGE                0xC800108BL

//
// MessageId: REC_DB_MAKE_PATIENT
//
// MessageText:
//
//  %1
//
#define REC_DB_MAKE_PATIENT              0xC800108CL

//
// MessageId: REC_DB_MAKE_SERIES
//
// MessageText:
//
//  %1
//
#define REC_DB_MAKE_SERIES               0xC800108DL

//
// MessageId: REC_DB_MAKE_STUDY
//
// MessageText:
//
//  %1
//
#define REC_DB_MAKE_STUDY                0xC800108EL

//
// MessageId: REC_DB_NOIMAGESLICES
//
// MessageText:
//
//  %1
//
#define REC_DB_NOIMAGESLICES             0xC800108FL

//
// MessageId: REC_DB_UNSUPPORTED_MODALITY
//
// MessageText:
//
//  %1
//
#define REC_DB_UNSUPPORTED_MODALITY      0xC8001090L

//
// MessageId: REC_OUT_OF_MEMORY
//
// MessageText:
//
//  %1
//
#define REC_OUT_OF_MEMORY                0xC8001091L

//
// MessageId: REC_INVALID_NORM
//
// MessageText:
//
//  %1
//
#define REC_INVALID_NORM                 0xC8001092L

//
// MessageId: REC_SPAN_RD
//
// MessageText:
//
//  %1
//
#define REC_SPAN_RD                      0xC8001093L

//
// MessageId: REC_COINSAMPMODE
//
// MessageText:
//
//  %1
//
#define REC_COINSAMPMODE                 0xC8001094L

//
// MessageId: REC_NEG_ZOOM
//
// MessageText:
//
//  %1
//
#define REC_NEG_ZOOM                     0xC8001095L

//
// MessageId: REC_CRS_QUEUE_CONNECT
//
// MessageText:
//
//  %1
//
#define REC_CRS_QUEUE_CONNECT            0xC8001096L

//
// MessageId: REC_CRS_QUEUE_DISCONNECT
//
// MessageText:
//
//  %1
//
#define REC_CRS_QUEUE_DISCONNECT         0xC8001097L

//
// MessageId: REC_IDL_SCATTER
//
// MessageText:
//
//  %1
//
#define REC_IDL_SCATTER                  0xC800109AL

//
// MessageId: REC_IDL_SCATTER_RUN
//
// MessageText:
//
//  %1
//
#define REC_IDL_SCATTER_RUN              0xC800109BL

//
// MessageId: REC_GANTRY_MODEL_UNKNOWN
//
// MessageText:
//
//  %1
//
#define REC_GANTRY_MODEL_UNKNOWN         0xC800109CL

#define REC_START_TIME_WRONG             0xC800109DL
#define REC_NODE_UNKNOWN                 0xC800109EL
#define REC_CANT_CREATE_GUID             0xC800109FL
#define REC_SELECT_CLUSTER               0xC80010A0L
#define REC_ALLOCATE_CLUSTER             0xC80010A1L
#define REC_FREE_CLUSTER                 0xC80010A2L
#define REC_START_CLUSTERJOB             0xC80010A3L
#define REC_ITF_HEADER_ERROR_BOOL        0xC80010A4L
//
// MessageId: REC_GETHOSTBYNAME_FAILS
// MessageText: gethostbyname() fails 
//
#define REC_GETHOSTBYNAME_FAILS          0xC80010A6L
//
// MessageId: REC_INFO
//
// MessageText:
//
//  %1
//
#define REC_INFO                         0x4800109DL

//
// MessageId: PHS_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define PHS_GENERIC_DEVELOPER_ERR        0xC8001389L

//
// MessageId: PHS_CMD_NOT_IMPL
//
// MessageText:
//
//  Command not implemented (U0), cmd = %1
//
#define PHS_CMD_NOT_IMPL                 0x8D00138AL

//
// MessageId: PHS_INV_CMD_DATA
//
// MessageText:
//
//  Invalid command data (E0), cmd = %1
//
#define PHS_INV_CMD_DATA                 0x8D00138BL

//
// MessageId: PHS_BED_TRAV_POS_OUT_OF_RANGE
//
// MessageText:
//
//  Cannot perform requested move - the desired position is out of range of the bed travel, move range = %1
//
#define PHS_BED_TRAV_POS_OUT_OF_RANGE    0x8D00138CL

//
// MessageId: PHS_BED_VELOCITY_OUT_OF_RANGE
//
// MessageText:
//
//  Cannot perform requested move - the desired velocity is out of range, move velocity = %1
//
#define PHS_BED_VELOCITY_OUT_OF_RANGE    0x8D00138DL

//
// MessageId: PHS_DISABLED_CLUTCH
//
// MessageText:
//
//  Cannot move horizontally - the clutch is disabled
//
#define PHS_DISABLED_CLUTCH              0x8D00138EL

//
// MessageId: PHS_ACTIVE_EXTEND_LIMIT_SW
//
// MessageText:
//
//  Cannot move in extend direction - the extend limit switch is active, PHS horizontal position = %1
//
#define PHS_ACTIVE_EXTEND_LIMIT_SW       0x8D00138FL

//
// MessageId: PHS_ACTIVE_RETRACT_LIMIT_SW
//
// MessageText:
//
//  Cannot move in retract direction - the retract limit switch is active, PHS horizontal position = %1
//
#define PHS_ACTIVE_RETRACT_LIMIT_SW      0x8D001390L

//
// MessageId: PHS_NO_MOVE_EMERGENCY_STOP
//
// MessageText:
//
//  Cannot move, system is in Emergency Stop
//
#define PHS_NO_MOVE_EMERGENCY_STOP       0x4D001391L

//
// MessageId: PHS_PALLET_BELOW_INTERMED_HT
//
// MessageText:
//
//  Cannot move absolute - pallet is below the intermediate height, %1
//
#define PHS_PALLET_BELOW_INTERMED_HT     0x4D001392L

//
// MessageId: PHS_UNKNOWN_BED_POSN
//
// MessageText:
//
//  Cannot perform the requested move command - the bed position is unknown
//
#define PHS_UNKNOWN_BED_POSN             0x4D001393L

//
// MessageId: PHS_BED_ALREADY_MOVING
//
// MessageText:
//
//  Cannot perform move - the bed is already moving
//
#define PHS_BED_ALREADY_MOVING           0x4D001394L

//
// MessageId: PHS_UNKNOWN_VERT_POSN
//
// MessageText:
//
//  Cannot jog up - the vertical position is not known and not on the retract limit switch
//
#define PHS_UNKNOWN_VERT_POSN            0x4D001395L

//
// MessageId: PHS_BED_JOGGING_DOWN
//
// MessageText:
//
//  Cannot jog up - the bed is jogging down
//
#define PHS_BED_JOGGING_DOWN             0x8D001396L

//
// MessageId: PHS_BED_JOGGING_UP
//
// MessageText:
//
//  Cannot jog up - the bed is jogging up
//
#define PHS_BED_JOGGING_UP               0x8D001397L

//
// MessageId: PHS_VERT_MOTION_NOT_ENABLED
//
// MessageText:
//
//  Cannot jog up - vertical motion is not enabled
//
#define PHS_VERT_MOTION_NOT_ENABLED      0x8D001398L

//
// MessageId: PHS_BED_OUT_OF_GANTRY
//
// MessageText:
//
//  Cannot jog into the gantry, the bed is moving out of the gantry
//
#define PHS_BED_OUT_OF_GANTRY            0x8D001399L

//
// MessageId: PHS_BED_IN_GANTRY
//
// MessageText:
//
//  Cannot jog out of the gantry, the bed is moving into the gantry
//
#define PHS_BED_IN_GANTRY                0x8D00139AL

//
// MessageId: PHS_HARDWARE_ERR
//
// MessageText:
//
//  Cannot move - system in shutdown state due to hardware error (voltage or current), shutdown cause = %1
//
#define PHS_HARDWARE_ERR                 0xCD00139BL

//
// MessageId: PHS_BED_BELOW_INTERMED_HT
//
// MessageText:
//
//  Cannot jog down, the bed is below intermediate height = %1
//
#define PHS_BED_BELOW_INTERMED_HT        0x4D00139CL

//
// MessageId: PHS_BED_AT_LOWER_LIMIT_SW
//
// MessageText:
//
//  Cannot jog down, on the lower limit switch, PHS vertical position = %1
//
#define PHS_BED_AT_LOWER_LIMIT_SW        0x8D00139DL

//
// MessageId: PHS_BED_AT_UPPER_LIMIT_SW
//
// MessageText:
//
//  Cannot jog down, on the upper limit switch, PHS vertical position = %1
//
#define PHS_BED_AT_UPPER_LIMIT_SW        0x8D00139EL

//
// MessageId: PHS_NEURO_INSERT_PRESENT
//
// MessageText:
//
//  Cannot perform move - neuro insert is present
//
#define PHS_NEURO_INSERT_PRESENT         0xCD00139FL

//
// MessageId: PHS_HORIZ_CMD_NOT_SENT
//
// MessageText:
//
//  Horizontal jog command was not sent, motion is stopped (normal stop)
//
#define PHS_HORIZ_CMD_NOT_SENT           0x8D0013A0L

//
// MessageId: PHS_VERT_CMD_NOT_SENT
//
// MessageText:
//
//  Vertical jog command was not sent, motion is stopped (normal stop)
//
#define PHS_VERT_CMD_NOT_SENT            0x8D0013A1L

//
// MessageId: PHS_HORIZ_ENCODER_UNCHANGED
//
// MessageText:
//
//  Horizontal encoder does not change, the horizontal position is changed to unknown and the motion is stopped during a move absolute (continues if in jog mode)
//
#define PHS_HORIZ_ENCODER_UNCHANGED      0xCD0013A2L

//
// MessageId: PHS_VERT_ENCODER_UNCHANGED
//
// MessageText:
//
//  Vertical encoder does not change, the vertical position status is changed to unknown (motion is continued)
//
#define PHS_VERT_ENCODER_UNCHANGED       0xCD0013A3L

//
// MessageId: PHS_NO_ABS_MOVE_EMERGENCY_STOP
//
// MessageText:
//
//  Move absolute command failed to complete, emergency stop was activated, System goes into emergency stop state
//
#define PHS_NO_ABS_MOVE_EMERGENCY_STOP   0x4D0013A4L

//
// MessageId: PHS_PALLET_REL_SW_PRESSED
//
// MessageText:
//
//  Move absolute command failed to complete, pallet release switch was pressed, move is stopped and the brake and clutch are disabled
//
#define PHS_PALLET_REL_SW_PRESSED        0x4D0013A5L

//
// MessageId: PHS_EXTEND_LIMIT_SW_REACHED
//
// MessageText:
//
//  Move absolute command failed to complete, extend limit switch was reached, move is stopped
//
#define PHS_EXTEND_LIMIT_SW_REACHED      0x8D0013A6L

//
// MessageId: PHS_HORIZ_RETRACT_SWITCHES_ACTIVE
//
// MessageText:
//
//  Both the horizontal extend and retract limit switches are active, the horizontal position status is changed to unknown, motion is stopped due to limit switch active
//
#define PHS_HORIZ_RETRACT_SWITCHES_ACTIVE 0xCD0013A7L

//
// MessageId: PHS_VERT_UP_DOWN_SWITCHES_ACTIVE
//
// MessageText:
//
//  Both the vertical up and down switches are active, the vertical position status is changed to unknown, motion is stopped due to limit switch active
//
#define PHS_VERT_UP_DOWN_SWITCHES_ACTIVE 0xCD0013A8L

//
// MessageId: PHS_MIN_HT_SW_ACTIVATED
//
// MessageText:
//
//  On the vertical down limit switch and the minimum height switch is activated, the vertical position status is changed to unknown, motion is continued
//
#define PHS_MIN_HT_SW_ACTIVATED          0xCD0013A9L

//
// MessageId: PHS_MIN_HT_SW_NOT_ACTIVATED
//
// MessageText:
//
//  On the vertical up limit switch and the minimum height switch is not activated, the vertical position status is changed to unknown, motion is continued
//
#define PHS_MIN_HT_SW_NOT_ACTIVATED      0xCD0013AAL

//
// MessageId: PHS_INTERMED_SW_30_MM_OUT
//
// MessageText:
//
//  The intermediate switch transitioned 30 mm too high or 30 mm too low, the vertical position status is changed to unknown, motion is continued
//
#define PHS_INTERMED_SW_30_MM_OUT        0xCD0013ABL

//
// MessageId: PHS_INTERMED_SW_20_MM_OUT
//
// MessageText:
//
//  The intermediate switch transitioned 20 mm too high or 20 mm too low, the vertical position status is changed to unknown, motion is continued
//
#define PHS_INTERMED_SW_20_MM_OUT        0x8D0013ACL

//
// MessageId: PHS_INTERMED_SW_ACTIVATED
//
// MessageText:
//
//  The intermediate switch activated during a move absolute, activation due to load variation, the move is stopped
//
#define PHS_INTERMED_SW_ACTIVATED        0x8D0013ADL

//
// MessageId: PHS_DOWN_LIMIT_SW_ACTIVATED
//
// MessageText:
//
//  The down limit switch activated 20 mm too high or 20 mm too low
//
#define PHS_DOWN_LIMIT_SW_ACTIVATED      0x8D0013AEL

//
// MessageId: PHS_MAX_HORIZ_MOTOR_CURRENT
//
// MessageText:
//
//  The horizontal motor is over current (greater than 1.25 amps), max horizontal motor current = %1
//
#define PHS_MAX_HORIZ_MOTOR_CURRENT      0xCD0013AFL

//
// MessageId: PHS_MAX_VERT_MOTOR_CURRENT
//
// MessageText:
//
//  The vertical motor is over current (greater than 1.25 amps), max vertical motor current = %1
//
#define PHS_MAX_VERT_MOTOR_CURRENT       0xCD0013B0L

//
// MessageId: PHS_MAX_BRAKE_CURRENT
//
// MessageText:
//
//  The brake is over current (greater than 250 mA), max brake current = %1
//
#define PHS_MAX_BRAKE_CURRENT            0xCD0013B1L

//
// MessageId: PHS_MAX_CLUTCH_CURRENT
//
// MessageText:
//
//  The clutch is over current (greater than 150 mA), max clutch current = %1
//
#define PHS_MAX_CLUTCH_CURRENT           0xCD0013B2L

//
// MessageId: PHS_POWER_SUPPLY_OUT_OF_TOL
//
// MessageText:
//
//  The +5 volt or +15 volt power supply is out of tolerance
//
#define PHS_POWER_SUPPLY_OUT_OF_TOL      0xCD0013B3L

//
// MessageId: PHS_NO_PROC_OK_INPUT
//
// MessageText:
//
//  No processor OK input, hardware latch = %1
//
#define PHS_NO_PROC_OK_INPUT             0xCD0013B4L

//
// MessageId: PHS_HI_VOLTAGE_LOW
//
// MessageText:
//
//  The high voltage is too low, less than 40 volts 5 seconds after the main voltage is enabled
//
#define PHS_HI_VOLTAGE_LOW               0xCD0013B5L

//
// MessageId: PHS_HI_VOLTAGE_HI
//
// MessageText:
//
//  The high voltage is too high, greater than 115 volts
//
#define PHS_HI_VOLTAGE_HI                0xCD0013B6L

//
// MessageId: PHS_OVER_TEMP
//
// MessageText:
//
//  The temperature is too hot, greater than 140 F (60 C)
//
#define PHS_OVER_TEMP                    0xCD0013B7L

//
// MessageId: PHS_VERT_MOTOR_STUCK
//
// MessageText:
//
//  The vertical motor voltage is stuck on, greater than 15 volts 35 seconds after the vertical motor voltage is disabled
//
#define PHS_VERT_MOTOR_STUCK             0xCD0013B8L

//
// MessageId: PHS_HI_VOLTAGE_STUCK
//
// MessageText:
//
//  The high voltage is stuck on, greater than 15 volts 35 seconds after the main voltage is disabled
//
#define PHS_HI_VOLTAGE_STUCK             0xCD0013B9L

//
// MessageId: PHS_VERT_MOTOR_TOO_HIGH
//
// MessageText:
//
//  The vertical motor voltage is too high, greater than 115 volts
//
#define PHS_VERT_MOTOR_TOO_HIGH          0xCD0013BAL

//
// MessageId: PHS_VERT_MOTOR_TOO_LOW
//
// MessageText:
//
//  The vertical motor voltage is too low, less than 40 volts 5 seconds after the vertical motor voltage is enabled
//
#define PHS_VERT_MOTOR_TOO_LOW           0xCD0013BBL

//
// MessageId: PHS_VERT_MOVE_DETECTED
//
// MessageText:
//
//  The vertical encoder detected a move when not in the enabled state (greater than +/-25.0 mm)
//
#define PHS_VERT_MOVE_DETECTED           0xCD0013BCL

//
// MessageId: PHS_HORIZ_MOVE_DETECTED
//
// MessageText:
//
//  The horizontal encoder detected a move when not in the enabled state (greater than +/-15.0 mm)
//
#define PHS_HORIZ_MOVE_DETECTED          0xCD0013BDL

//
// MessageId: PHS_INVALID_SYSTEM_STATE
//
// MessageText:
//
//  Firmware error - the System_State is invalid
//
#define PHS_INVALID_SYSTEM_STATE         0xCD0013BEL

//
// MessageId: PHS_INVALID_HORIZ_STATE
//
// MessageText:
//
//  Firmware error - the Horizontal_State is invalid
//
#define PHS_INVALID_HORIZ_STATE          0xCD0013BFL

//
// MessageId: PHS_INVALID_VERT_STATE
//
// MessageText:
//
//  Firmware error - the Vertical_State is invalid
//
#define PHS_INVALID_VERT_STATE           0xCD0013C0L

//
// MessageId: PHS_INVALID_H_MOTION_STATE
//
// MessageText:
//
//  Firmware error - the H_Motion_State is invalid
//
#define PHS_INVALID_H_MOTION_STATE       0xCD0013C1L

//
// MessageId: PHS_INVALID_V_MOTION_STATE
//
// MessageText:
//
//  Firmware error - the V_Motion_State is invalid
//
#define PHS_INVALID_V_MOTION_STATE       0xCD0013C2L

//
// MessageId: PHS_INVALID_CHK_SYS_STATE
//
// MessageText:
//
//  Firmware error - the System_State is not valid in the check_system_state() function
//
#define PHS_INVALID_CHK_SYS_STATE        0xCD0013C3L

//
// MessageId: PHS_START_MOVE_ABS_ERR
//
// MessageText:
//
//  Firmware error - the Start_Move_Absolute function returned an error
//
#define PHS_START_MOVE_ABS_ERR           0xCD0013C4L

//
// MessageId: PHS_PARM_FILE_OPEN_ERR
//
// MessageText:
//
//  Could not open parm file = %1
//
#define PHS_PARM_FILE_OPEN_ERR           0xCD0013C5L

//
// MessageId: PHS_INCORRECT_PARM_FILE
//
// MessageText:
//
//  Incorrect parm file = %1
//
#define PHS_INCORRECT_PARM_FILE          0xCD0013C6L

//
// MessageId: PHS_SERIAL_PORT_OPEN_ERR
//
// MessageText:
//
//  Could not open serial port, port number = %1
//
#define PHS_SERIAL_PORT_OPEN_ERR         0xCD0013C7L

//
// MessageId: PHS_SERIAL_PORT_WRITE_ERR
//
// MessageText:
//
//  Could not write to serial port, port number = %1
//
#define PHS_SERIAL_PORT_WRITE_ERR        0xCD0013C8L

//
// MessageId: PHS_SERIAL_PORT_NO_RESPONSE
//
// MessageText:
//
//  No response at serial port, port number = %1
//
#define PHS_SERIAL_PORT_NO_RESPONSE      0xCD0013C9L

//
// MessageId: PHS_SERIAL_PORT_RETRIES
//
// MessageText:
//
//  Serial port retries required, port number = %1
//
#define PHS_SERIAL_PORT_RETRIES          0x8D0013CAL

//
// MessageId: PHS_GENERIC_ERROR
//
// MessageText:
//
//  PHS generic error, text message = %1
//
#define PHS_GENERIC_ERROR                0xCD0013CBL

//
// MessageId: MTR_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define MTR_GENERIC_DEVELOPER_ERR        0xC8001771L

//
// MessageId: MTR_PWR_STAGE_OVERTEMP
//
// MessageText:
//
//  Power stage overtemp: %1
//
#define MTR_PWR_STAGE_OVERTEMP           0xCD001772L

//
// MessageId: MTR_OVER_VOLTAGE
//
// MessageText:
//
//  OverVoltage: %1
//
#define MTR_OVER_VOLTAGE                 0xCD001773L

//
// MessageId: MTR_OVER_CURRENT
//
// MessageText:
//
//  OverCurrent: %1
//
#define MTR_OVER_CURRENT                 0xCD001774L

//
// MessageId: MTR_EXT_FEEDBACK_FAULT
//
// MessageText:
//
//  External feedback fault: %1
//
#define MTR_EXT_FEEDBACK_FAULT           0xCD001775L

//
// MessageId: MTR_RESOLVER_LINE_BREAK
//
// MessageText:
//
//  Resolver line break: %1
//
#define MTR_RESOLVER_LINE_BREAK          0xCD001776L

//
// MessageId: MTR_RDC_ERR
//
// MessageText:
//
//  RDC error: %1
//
#define MTR_RDC_ERR                      0xCD001777L

//
// MessageId: MTR_SINE_ENCODER_INIT_ERR
//
// MessageText:
//
//  Sine encoder init failure: %1
//
#define MTR_SINE_ENCODER_INIT_ERR        0xCD001778L

//
// MessageId: MTR_AB_LINE_BREAK
//
// MessageText:
//
//  A/B line break: %1
//
#define MTR_AB_LINE_BREAK                0xCD001779L

//
// MessageId: MTR_IDX_LINE_BREAK
//
// MessageText:
//
//  Index line break: %1
//
#define MTR_IDX_LINE_BREAK               0xCD00177AL

//
// MessageId: MTR_ILLEGAL_HALLS
//
// MessageText:
//
//  Illegal halls: %1
//
#define MTR_ILLEGAL_HALLS                0xCD00177BL

//
// MessageId: MTR_CD_LINE_BREAK
//
// MessageText:
//
//  C/D line break: %1
//
#define MTR_CD_LINE_BREAK                0xCD00177CL

//
// MessageId: MTR_AB_OUT_OF_RANGE
//
// MessageText:
//
//  A/B out of range: %1
//
#define MTR_AB_OUT_OF_RANGE              0xCD00177DL

//
// MessageId: MTR_BURST_PULSE_OVRFLW
//
// MessageText:
//
//  Burst pulse overflow: %1
//
#define MTR_BURST_PULSE_OVRFLW           0xCD00177EL

//
// MessageId: MTR_UNDER_VOLTAGE
//
// MessageText:
//
//  Under voltage: %1
//
#define MTR_UNDER_VOLTAGE                0xCD00177FL

//
// MessageId: MTR_MOTOR_OVERTEMP
//
// MessageText:
//
//  Motor over temperature: %1
//
#define MTR_MOTOR_OVERTEMP               0xCD001780L

//
// MessageId: MTR_POS_ALOG_SUPPLY_ERR
//
// MessageText:
//
//  Positive analog supply fail: %1
//
#define MTR_POS_ALOG_SUPPLY_ERR          0xCD001781L

//
// MessageId: MTR_NEG_ALOG_SUPPLY_ERR
//
// MessageText:
//
//  Negative analog supply fail: %1
//
#define MTR_NEG_ALOG_SUPPLY_ERR          0xCD001782L

//
// MessageId: MTR_OVERSPEED
//
// MessageText:
//
//  Overspeed: %1
//
#define MTR_OVERSPEED                    0xCD001783L

//
// MessageId: MTR_EEPROM_ERR
//
// MessageText:
//
//  EEPROM failure: %1
//
#define MTR_EEPROM_ERR                   0xCD001784L

//
// MessageId: MTR_EEPROM_CHKSUM_ERR
//
// MessageText:
//
//  EEPROM checksum failure: %1
//
#define MTR_EEPROM_CHKSUM_ERR            0xCD001785L

//
// MessageId: MTR_FOLDBACK
//
// MessageText:
//
//  Foldback: %1
//
#define MTR_FOLDBACK                     0xCD001786L

//
// MessageId: MTR_POS_OVER_TRAVEL_ERR
//
// MessageText:
//
//  Positive over travel fault: %1
//
#define MTR_POS_OVER_TRAVEL_ERR          0xCD001787L

//
// MessageId: MTR_NEG_OVER_TRAVEL_ERR
//
// MessageText:
//
//  Negative over travel fault: %1
//
#define MTR_NEG_OVER_TRAVEL_ERR          0xCD001788L

//
// MessageId: MTR_NUM_POS_DEVIATION
//
// MessageText:
//
//  Numeric position deviation: %1
//
#define MTR_NUM_POS_DEVIATION            0xCD001789L

//
// MessageId: MTR_EXCESS_POS_DEVIATION
//
// MessageText:
//
//  Excessive position deviation: %1
//
#define MTR_EXCESS_POS_DEVIATION         0xCD00178AL

//
// MessageId: MTR_COMM_INTERFACE
//
// MessageText:
//
//  Communications interface: %1
//
#define MTR_COMM_INTERFACE               0xCD00178BL

//
// MessageId: MTR_UNKNOWN_CMD
//
// MessageText:
//
//  Unknown command or variable: %1
//
#define MTR_UNKNOWN_CMD                  0x4D00178CL

//
// MessageId: MTR_CHKSUM_ERR
//
// MessageText:
//
//  Checksum error: %1
//
#define MTR_CHKSUM_ERR                   0x8D00178DL

//
// MessageId: MTR_DRIVE_ACTIVE
//
// MessageText:
//
//  Drive active: %1
//
#define MTR_DRIVE_ACTIVE                 0x4D00178EL

//
// MessageId: MTR_DRIVE_INACTIVE
//
// MessageText:
//
//  Drive inactive: %1
//
#define MTR_DRIVE_INACTIVE               0x4D00178FL

//
// MessageId: MTR_VAL_OUT_OF_RANGE
//
// MessageText:
//
//  Value is out of range: %1
//
#define MTR_VAL_OUT_OF_RANGE             0x8D001790L

//
// MessageId: MTR_NEG_NUMBER
//
// MessageText:
//
//  Negative number: %1
//
#define MTR_NEG_NUMBER                   0x8D001791L

//
// MessageId: MTR_BAD_OPMODE
//
// MessageText:
//
//  Not in proper OpMode: %1
//
#define MTR_BAD_OPMODE                   0x8D001792L

//
// MessageId: MTR_SYNTAX_ERR
//
// MessageText:
//
//  Syntax error: %1
//
#define MTR_SYNTAX_ERR                   0x8D001793L

//
// MessageId: MTR_TUNE_ERR
//
// MessageText:
//
//  Tune failed: %1
//
#define MTR_TUNE_ERR                     0x8D001794L

//
// MessageId: MTR_BAD_BANDWIDTH
//
// MessageText:
//
//  Bad bandwidth: %1
//
#define MTR_BAD_BANDWIDTH                0x8D001795L

//
// MessageId: MTR_BAD_STABILITY
//
// MessageText:
//
//  Bad stability: %1
//
#define MTR_BAD_STABILITY                0x8D001796L

//
// MessageId: MTR_NOT_PROGRAMMABLE
//
// MessageText:
//
//  Not programmable: %1
//
#define MTR_NOT_PROGRAMMABLE             0x8D001797L

//
// MessageId: MTR_MOTOR_CFG_ERR
//
// MessageText:
//
//  Motor config failed: %1
//
#define MTR_MOTOR_CFG_ERR                0x8D001798L

//
// MessageId: MTR_WRONG_COMPMODE
//
// MessageText:
//
//  Not in proper CompMode: %1
//
#define MTR_WRONG_COMPMODE               0x8D001799L

//
// MessageId: MTR_EXT_VELOC_PARM
//
// MessageText:
//
//  EXT velocity param warning: %1
//
#define MTR_EXT_VELOC_PARM               0x8D00179AL

//
// MessageId: MTR_VELOC_LOOP_ERR
//
// MessageText:
//
//  Velocity loop design failed: %1
//
#define MTR_VELOC_LOOP_ERR               0x8D00179BL

//
// MessageId: MTR_BAD_EEPROM
//
// MessageText:
//
//  Invalid EEPROM: %1
//
#define MTR_BAD_EEPROM                   0x8D00179CL

//
// MessageId: MTR_RECORDING_ACTIVE
//
// MessageText:
//
//  Recording active: %1
//
#define MTR_RECORDING_ACTIVE             0x8D00179DL

//
// MessageId: MTR_REC_DATA_UNAVAILABLE
//
// MessageText:
//
//  Recorded data not available: %1
//
#define MTR_REC_DATA_UNAVAILABLE         0x8D00179EL

//
// MessageId: MTR_EMPTY_EEPROM
//
// MessageText:
//
//  EEPROM is empty: %1
//
#define MTR_EMPTY_EEPROM                 0x8D00179FL

//
// MessageId: MTR_ARG_NOT_BINARY
//
// MessageText:
//
//  Argument must be binary: %1
//
#define MTR_ARG_NOT_BINARY               0x8D0017A0L

//
// MessageId: MTR_ENCOUT_CONFLICT
//
// MessageText:
//
//  Conflicts with ENCOUT: %1
//
#define MTR_ENCOUT_CONFLICT              0x8D0017A1L

//
// MessageId: MTR_VLIM_CONFLICT
//
// MessageText:
//
//  Conflicts with VLIM: %1
//
#define MTR_VLIM_CONFLICT                0x8D0017A2L

//
// MessageId: MTR_VAR_VAL_UNAVAILABLE
//
// MessageText:
//
//  Variable value not available: %1
//
#define MTR_VAR_VAL_UNAVAILABLE          0x8D0017A3L

//
// MessageId: MTR_DRV_IN_HOLD
//
// MessageText:
//
//  Drive in hold mode: %1
//
#define MTR_DRV_IN_HOLD                  0x8D0017A4L

//
// MessageId: MTR_LIMIT_SW_HOLD
//
// MessageText:
//
//  Limit switch hold: %1
//
#define MTR_LIMIT_SW_HOLD                0x8D0017A5L

//
// MessageId: MTR_CMD_INTO_LIMIT
//
// MessageText:
//
//  Command into limit: %1
//
#define MTR_CMD_INTO_LIMIT               0x8D0017A6L

//
// MessageId: MTR_DRV_IN_ZERO_MODE
//
// MessageText:
//
//  Drive in zero mode: %1
//
#define MTR_DRV_IN_ZERO_MODE             0x8D0017A7L

//
// MessageId: MTR_MOTOR_JOGGING
//
// MessageText:
//
//  Motor is jogging: %1
//
#define MTR_MOTOR_JOGGING                0x8D0017A8L

//
// MessageId: MTR_INDIVISIBLE_ARG
//
// MessageText:
//
//  Argument not divisible by 20: %1
//
#define MTR_INDIVISIBLE_ARG              0x8D0017A9L

//
// MessageId: MTR_ENCODER_PROC_ACTIVE
//
// MessageText:
//
//  Encoder init process active: %1
//
#define MTR_ENCODER_PROC_ACTIVE          0x8D0017AAL

//
// MessageId: MTR_NO_ROTATION
//
// MessageText:
//
//  Tune failed - no rotation: %1
//
#define MTR_NO_ROTATION                  0x8D0017ABL

//
// MessageId: MTR_CURR_SATURATION
//
// MessageText:
//
//  Tune failed - current saturation: %1
//
#define MTR_CURR_SATURATION              0x8D0017ACL

//
// MessageId: MTR_NO_VELOCITY
//
// MessageText:
//
//  Tune failed - no velocity: %1
//
#define MTR_NO_VELOCITY                  0x8D0017ADL

//
// MessageId: MTR_DISABLE_DURING_TUNE
//
// MessageText:
//
//  Disable during tune: %1
//
#define MTR_DISABLE_DURING_TUNE          0x8D0017AEL

//
// MessageId: MTR_HOLD_DURING_TIME
//
// MessageText:
//
//  Tune failed - hold during time: %1
//
#define MTR_HOLD_DURING_TIME             0x8D0017AFL

//
// MessageId: MTR_LO_VELOC_LIM
//
// MessageText:
//
//  Low velocity limits: %1
//
#define MTR_LO_VELOC_LIM                 0x8D0017B0L

//
// MessageId: MTR_LOWER_BANDWIDTH
//
// MessageText:
//
//  Use lower bandwidth: %1
//
#define MTR_LOWER_BANDWIDTH              0x8D0017B1L

//
// MessageId: MTR_DUAL_FEEDBACK
//
// MessageText:
//
//  Drive is in dual feedback mode: %1
//
#define MTR_DUAL_FEEDBACK                0x8D0017B2L

//
// MessageId: MTR_GEAR_MODE
//
// MessageText:
//
//  Drive is in gear mode: %1
//
#define MTR_GEAR_MODE                    0x8D0017B3L

//
// MessageId: MTR_OCC_FUNCTIONALITY
//
// MessageText:
//
//  Functionality is occupied: %1
//
#define MTR_OCC_FUNCTIONALITY            0x8D0017B4L

//
// MessageId: MTR_AB_NOT_ROUTED
//
// MessageText:
//
//  A/B line is not routed: %1
//
#define MTR_AB_NOT_ROUTED                0x8D0017B5L

//
// MessageId: MTR_LIM_SW_NOT_ROUTED
//
// MessageText:
//
//  Limit switch is not routed: %1
//
#define MTR_LIM_SW_NOT_ROUTED            0x8D0017B6L

//
// MessageId: MTR_MOVE_PENDING
//
// MessageText:
//
//  Move is pending: %1
//
#define MTR_MOVE_PENDING                 0x8D0017B7L

//
// MessageId: MTR_INCORRECT_PWD
//
// MessageText:
//
//  Incorrect password: %1
//
#define MTR_INCORRECT_PWD                0x8D0017B8L

//
// MessageId: MTR_PWD_PROTECTED
//
// MessageText:
//
//  Password protected: %1
//
#define MTR_PWD_PROTECTED                0x8D0017B9L

//
// MessageId: MTR_CAPTURE_DURING_HOMING
//
// MessageText:
//
//  Capture during homing: %1
//
#define MTR_CAPTURE_DURING_HOMING        0x8D0017BAL

//
// MessageId: MTR_HOMING_DURING_CAPTURE
//
// MessageText:
//
//  Homing during capture: %1
//
#define MTR_HOMING_DURING_CAPTURE        0x8D0017BBL

//
// MessageId: MTR_AB_INCOMPLETE_CAPTURE
//
// MessageText:
//
//  Capture process not done: %1
//
#define MTR_AB_INCOMPLETE_CAPTURE        0x8D0017BCL

//
// MessageId: MTR_INACTIVE_CAPTURE
//
// MessageText:
//
//  Capture process not active: %1
//
#define MTR_INACTIVE_CAPTURE             0x8D0017BDL

//
// MessageId: MTR_DISABLED_CAPTURE
//
// MessageText:
//
//  Capture process not enabled: %1
//
#define MTR_DISABLED_CAPTURE             0x8D0017BEL

//
// MessageId: MTR_ENCSTART_WHILE_ACFG
//
// MessageText:
//
//  ENCSTART while ACONFIG: %1
//
#define MTR_ENCSTART_WHILE_ACFG          0x8D0017BFL

//
// MessageId: MTR_SERCOS_TEST_ERR
//
// MessageText:
//
//  SERCOS test failure: %1
//
#define MTR_SERCOS_TEST_ERR              0x8D0017C0L

//
// MessageId: MTR_PARM_FILE_OPEN_ERR
//
// MessageText:
//
//  Could not open parameter file = %1
//
#define MTR_PARM_FILE_OPEN_ERR           0xCD0017C1L

//
// MessageId: MTR_INCORRECT_PARM_FILE
//
// MessageText:
//
//  Incorrect parameter file = %1
//
#define MTR_INCORRECT_PARM_FILE          0xCD0017C2L

//
// MessageId: MTR_SERIAL_PORT_OPEN_ERR
//
// MessageText:
//
//  Serial port open error, port number = %1
//
#define MTR_SERIAL_PORT_OPEN_ERR         0xCD0017C3L

//
// MessageId: MTR_SERIAL_PORT_WRITE_ERR
//
// MessageText:
//
//  Serial port write error, port number = %1
//
#define MTR_SERIAL_PORT_WRITE_ERR        0xCD0017C4L

//
// MessageId: MTR_SERIAL_PORT_NO_RESPONSE
//
// MessageText:
//
//  No response on the serial port, port number = %1
//
#define MTR_SERIAL_PORT_NO_RESPONSE      0xCD0017C5L

//
// MessageId: MTR_SERIAL_PORT_RETRIES
//
// MessageText:
//
//  Retries were required on the serial port, port number = %1
//
#define MTR_SERIAL_PORT_RETRIES          0x8D0017C6L

//
// MessageId: MTR_GENERIC_MC_ERROR
//
// MessageText:
//
//  MC generic error, text message = %1
//
#define MTR_GENERIC_MC_ERROR             0x8D0017C7L

//
// MessageId: LHS_INIT_INFORMATION
//
// MessageText:
//
//  PET acq initialized
//
#define LHS_INIT_INFORMATION             0x48001B59L

//
// MessageId: LHS_INIT_ERROR
//
// MessageText:
//
//  PET acq initialization error: %1
//
#define LHS_INIT_ERROR                   0xC8001B5AL

//
// MessageId: LHS_STATE_OPERATION_INFORMATION
//
// MessageText:
//
//  %1 request rejected by PET acq
//
#define LHS_STATE_OPERATION_INFORMATION  0x48001B5BL

//
// MessageId: LHS_UNCONFIG_ERROR
//
// MessageText:
//
//  PET acq unconfig error: %1
//
#define LHS_UNCONFIG_ERROR               0xCD001B5CL

//
// MessageId: LHS_UNCONFIG_INFORMATION
//
// MessageText:
//
//  PET acq unconfigured
//
#define LHS_UNCONFIG_INFORMATION         0x4D001B5DL

//
// MessageId: LHS_CONFIG_ERROR
//
// MessageText:
//
//  PET acq config error: %1
//
#define LHS_CONFIG_ERROR                 0xCD001B5EL

//
// MessageId: LHS_CONFIG_INFORMATION
//
// MessageText:
//
//  PET acq configured %1
//
#define LHS_CONFIG_INFORMATION           0x4D001B5FL

//
// MessageId: LHS_START_ERROR
//
// MessageText:
//
//  PET acquisition start error: %1
//
#define LHS_START_ERROR                  0xCD001B60L

//
// MessageId: LHS_START_INFORMATION
//
// MessageText:
//
//  PET acquisition started (blocking %1)
//
#define LHS_START_INFORMATION            0x4D001B61L

//
// MessageId: LHS_STOP_ERROR
//
// MessageText:
//
//  PET acquisition stop error: %1
//
#define LHS_STOP_ERROR                   0xCD001B62L

//
// MessageId: LHS_STOP_INFORMATION
//
// MessageText:
//
//  PET acquisition stopping on request
//
#define LHS_STOP_INFORMATION             0x4D001B63L

//
// MessageId: LHS_ABORT_ERROR
//
// MessageText:
//
//  PET acquisition abort error: %1
//
#define LHS_ABORT_ERROR                  0xCD001B64L

//
// MessageId: LHS_ABORT_INFORMATION
//
// MessageText:
//
//  PET acquisition aborting on request
//
#define LHS_ABORT_INFORMATION            0x4D001B65L

//
// MessageId: LHS_SETSINK_WARNING
//
// MessageText:
//
//  PET acq failed to release previous sink interface pointer
//
#define LHS_SETSINK_WARNING              0x88001B66L

//
// MessageId: LHS_SETSINK_ERROR
//
// MessageText:
//
//  PET acq setSink error: %1
//
#define LHS_SETSINK_ERROR                0xC8001B67L

//
// MessageId: LHS_SETSINK_QI_ERROR
//
// MessageText:
//
//  PET acq failed to query client for INotification
//
#define LHS_SETSINK_QI_ERROR             0xC8001B68L

//
// MessageId: LHS_SETSINK_MARSHALL_ERROR
//
// MessageText:
//
//  PET acq failed to marshall client INotification
//
#define LHS_SETSINK_MARSHALL_ERROR       0xC8001B69L

//
// MessageId: LHS_ACQCOMPLETE_PRESET_INFORMATION
//
// MessageText:
//
//  PET acquisition finished on preset
//
#define LHS_ACQCOMPLETE_PRESET_INFORMATION 0x4D001B6AL

//
// MessageId: LHS_ACQCOMPLETE_STOP_INFORMATION
//
// MessageText:
//
//  PET acquisition stopped by request
//
#define LHS_ACQCOMPLETE_STOP_INFORMATION 0x4D001B6BL

//
// MessageId: LHS_ACQCOMPLETE_ABORT_INFORMATION
//
// MessageText:
//
//  PET acquisition aborted by request
//
#define LHS_ACQCOMPLETE_ABORT_INFORMATION 0x4D001B6CL

//
// MessageId: LHS_ACQCOMPLETE_ERROR_INFORMATION
//
// MessageText:
//
//  PET acquisition stopped due to error
//
#define LHS_ACQCOMPLETE_ERROR_INFORMATION 0x4D001B6DL

//
// MessageId: LHS_ACQPOSTCOMPLETE_INFORMATION
//
// MessageText:
//
//  PET raw data file(s) saved
//
#define LHS_ACQPOSTCOMPLETE_INFORMATION  0x4D001B6EL

//
// MessageId: LHS_ACQPOSTCOMPLETE_ERROR
//
// MessageText:
//
//  PET acq failed to save PET raw data file(s)
//
#define LHS_ACQPOSTCOMPLETE_ERROR        0xCD001B6FL

//
// MessageId: LHS_APC_ERROR
//
// MessageText:
//
//  PET acq failed to dispatch function %1
//
#define LHS_APC_ERROR                    0xC8001B70L

//
// MessageId: CHL_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define CHL_GENERIC_DEVELOPER_ERR        0xC8001F41L

//
// MessageId: CHL_PARM_FILE_OPEN_ERR
//
// MessageText:
//
//  Could not open the parm file = %1
//
#define CHL_PARM_FILE_OPEN_ERR           0xCD001F42L

//
// MessageId: CHL_INCORRECT_PARM_FILE
//
// MessageText:
//
//  Incorrect parm file = %1
//
#define CHL_INCORRECT_PARM_FILE          0xCD001F43L

//
// MessageId: CHL_SERIAL_PORT_OPEN_ERR
//
// MessageText:
//
//  Could not open serial port, port number = %1
//
#define CHL_SERIAL_PORT_OPEN_ERR         0xCD001F44L

//
// MessageId: CHL_SERIAL_PORT_WRITE_ERR
//
// MessageText:
//
//  Could not write to serial port, port number = %1
//
#define CHL_SERIAL_PORT_WRITE_ERR        0xCD001F45L

//
// MessageId: CHL_SERIAL_PORT_NO_RESPONSE
//
// MessageText:
//
//  No response at serial port, port number = %1
//
#define CHL_SERIAL_PORT_NO_RESPONSE      0xCD001F46L

//
// MessageId: CHL_SERIAL_PORT_RETRIES
//
// MessageText:
//
//  Serial port retries were required, port number = %1
//
#define CHL_SERIAL_PORT_RETRIES          0x8D001F47L

//
// MessageId: DSP_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define DSP_GENERIC_DEVELOPER_ERR        0xC8002329L

//
// MessageId: DSP_FILE_OPEN_ERR
//
// MessageText:
//
//  Could not open file = %1
//
#define DSP_FILE_OPEN_ERR                0xCD00232AL

//
// MessageId: DSP_GRAPHICS_INIT_ERR
//
// MessageText:
//
//  Graphics init failed, error message = %1
//
#define DSP_GRAPHICS_INIT_ERR            0xCD00232BL

//
// MessageId: SIM_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define SIM_GENERIC_DEVELOPER_ERR        0xC8002711L

//
// MessageId: SIM_ERR_1
//
// MessageText:
//
//  Test error 1
//
#define SIM_ERR_1                        0xCD002712L

//
// MessageId: LCK_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define LCK_GENERIC_DEVELOPER_ERR        0xC8002AF9L

//
// MessageId: LCK_ALREADY_LOCKED
//
// MessageText:
//
//  The ACS servers are already locked, LockId = %1
//
#define LCK_ALREADY_LOCKED               0xCD002AFAL

//
// MessageId: LCK_INVALID_ID_TO_UNLOCK
//
// MessageText:
//
//  Bad LockId provided for unlock request, LockId = %1
//
#define LCK_INVALID_ID_TO_UNLOCK         0xCD002AFBL

//
// MessageId: LCK_COM_ERROR
//
// MessageText:
//
//  COM error occurred during lock/unlock request to an ACS server, HResult = %1
//
#define LCK_COM_ERROR                    0xCD002AFCL

//
// MessageId: DAL_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define DAL_GENERIC_DEVELOPER_ERR        0xC8002EE1L

//
// MessageId: DAL_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define DAL_GENERIC_DEVELOPER_WARNING    0x88002EE2L

//
// MessageId: DAL_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define DAL_GENERIC_DEVELOPER_INFO       0x48002EE3L

//
// MessageId: DALCP_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define DALCP_GENERIC_DEVELOPER_ERR      0xC8002EE4L

//
// MessageId: DALCP_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define DALCP_GENERIC_DEVELOPER_WARNING  0x88002EE5L

//
// MessageId: DALCP_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define DALCP_GENERIC_DEVELOPER_INFO     0x48002EE6L

//
// MessageId: DALDHI_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define DALDHI_GENERIC_DEVELOPER_ERR     0xC8002EE7L

//
// MessageId: DALDHI_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define DALDHI_GENERIC_DEVELOPER_WARNING 0x88002EE8L

//
// MessageId: DALDHI_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define DALDHI_GENERIC_DEVELOPER_INFO    0x48002EE9L

//
// MessageId: PDS_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define PDS_GENERIC_DEVELOPER_ERR        0xC80032C9L

//
// MessageId: PDS_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define PDS_GENERIC_DEVELOPER_WARNING    0x880032CAL

//
// MessageId: PDS_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define PDS_GENERIC_DEVELOPER_INFO       0x480032CBL

//
// MessageId: DSU_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define DSU_GENERIC_DEVELOPER_ERR        0xC80036B1L

//
// MessageId: DSU_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define DSU_GENERIC_DEVELOPER_WARNING    0x880036B2L

//
// MessageId: DSU_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define DSU_GENERIC_DEVELOPER_INFO       0x480036B3L

//
// MessageId: SAQ_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define SAQ_GENERIC_DEVELOPER_ERR        0xC8003A99L

//
// MessageId: SAQ_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define SAQ_GENERIC_DEVELOPER_WARNING    0x88003A9AL

//
// MessageId: SAQ_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define SAQ_GENERIC_DEVELOPER_INFO       0x48003A9BL

//
// MessageId: DSG_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define DSG_GENERIC_DEVELOPER_ERR        0xC8003E81L

//
// MessageId: DSG_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define DSG_GENERIC_DEVELOPER_WARNING    0x88003E82L

//
// MessageId: DSG_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define DSG_GENERIC_DEVELOPER_INFO       0x48003E83L

//
// MessageId: DUG_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define DUG_GENERIC_DEVELOPER_ERR        0xC8004269L

//
// MessageId: DUG_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define DUG_GENERIC_DEVELOPER_WARNING    0x8800426AL

//
// MessageId: DUG_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define DUG_GENERIC_DEVELOPER_INFO       0x4800426BL

//
// MessageId: QCG_GENERIC_DEVELOPER_ERR
//
// MessageText:
//
//  %1
//
#define QCG_GENERIC_DEVELOPER_ERR        0xC8004651L

//
// MessageId: QCG_GENERIC_DEVELOPER_WARNING
//
// MessageText:
//
//  %1
//
#define QCG_GENERIC_DEVELOPER_WARNING    0x88004652L

//
// MessageId: QCG_GENERIC_DEVELOPER_INFO
//
// MessageText:
//
//  %1
//
#define QCG_GENERIC_DEVELOPER_INFO       0x48004653L

