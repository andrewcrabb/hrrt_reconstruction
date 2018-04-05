/*! \file data_changer.h
    \brief This class implements system independent code to read from big- or
           little-endian files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
 */

#pragma once

#include <fstream>

/*- class definitions -------------------------------------------------------*/

class DataChanger
 { private:
    unsigned char *buffer,                              /*!< buffer for data */
                  *bufptr,        /*!< pointer to current position in buffer */
                  *bufend;                     /*!< pointer to end of buffer */
    unsigned long int buffer_size;                   /*!< size of the buffer */
    bool vax_format,          /*!< does system use VAX format from Digital ? */
             /*!< is endianess of data different from endianess of machine ? */
         swap_data;
   public:
                                                               // create buffer
    DataChanger(const unsigned long int, const bool, const bool);
    ~DataChanger();                                           // destroy buffer
    void LoadBuffer(std::ifstream * const);    // load buffer content from file
    void SaveBuffer(std::ofstream * const) const;// save buffer content to file
    void ResetBufferPtr();                  // reset pointer to start of buffer
    void Value(unsigned char * const);                  // read "unsigned char"
    void Value(const unsigned char);                   // write "unsigned char"
    void Value(signed short int * const);            // read "signed short int"
    void Value(const signed short int);             // write "signed short int"
    void Value(unsigned short int * const);        // read "unsigned short int"
    void Value(const unsigned short int);         // write "unsigned short int"
    void Value(signed long int * const);                   // read "signed-int"
    void Value(const signed long int);                    // write "signed-int"
    void Value(unsigned long int * const);               // read "unsigned-int"
    void Value(const unsigned long int);                // write "unsigned-int"
    void Value(float * const);                                  // read "float"
    void Value(const float);                                   // write "float"
    void Value(double * const);                                // read "double"
    void Value(const double);                                 // write "double"
    void Value(unsigned char * const, const unsigned short int); // read string
                                                                // write string
    void Value(const unsigned short int, const unsigned char * const);
 };

