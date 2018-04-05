/*! \file ecat7_directory.h
    \brief This class deals with the directory structure of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2004/09/10 added Doxygen style comments
 */

#pragma once

#include <fstream>
#include <list>
#include <string>
#include <vector>

/*- class definitions -------------------------------------------------------*/

class ECAT7_DIRECTORY
 { public:
                       /*! data structure for ID values from the entry field */
    typedef struct { signed long int bed,                    /*!< bed number */
                                     frame,                /*!< frame number */
                                     gate,                  /*!< gate number */
                                     plane,                /*!< plane number */
                                     data,               /*!< dataset number */
                                                  /*! first record of matrix */
                                     first_record,
                                     last_record; /*!< last record of matrix */
                   } tid_val7;
   private:
                         /*! structure of a directory block of an ECAT7 file */
    typedef struct {                    /*! number of free directory entries */
                     signed long int num_avail_entries,
                                         /*! pointer to next directory block */
                                     next_dir_rec,
                                     /*! pointer to previous directory block */
                                     last_dir_rec,
                                        /*! number of used directory entries */
                                     num_alloc_entries,
                                               /*! ID values of the matrices */
                                     entry[31][4];
                   } tdir_list;
                                 /*! array of information about the matrices */
    std::vector <ECAT7_DIRECTORY::tid_val7> id_vals;
                                                        // load directory block
    void LoadDirBlock(std::ifstream * const, const signed long int,
                      tdir_list * const) const;
                                    // convert matrix information into ID value
    signed long int MatrixID(const signed long int, const signed long int,
                             const signed long int, const signed long int,
                             const signed long int) const;
                                                       // store directory block
    void SaveDirBlock(std::ofstream * const, const signed long int,
                      const tdir_list * const) const;
   public:
    ECAT7_DIRECTORY();                                     // initialize object
    ECAT7_DIRECTORY& operator = (const ECAT7_DIRECTORY &);      // '='-operator
                           // create new entries in array of matrix information
    void AddEntries(const unsigned short int);
                                           // append directory entry for matrix
    void AppendEntry(const std::string, const unsigned long int,
                     const unsigned long int);
                                                // create empty directory block
    void CreateDirBlock(std::ofstream * const, const signed long int,
                        const signed long int) const;
                               // delete entry from array of matrix information
    void DeleteEntry(const unsigned short int);
                                    // pointer to array with matrix information
    ECAT7_DIRECTORY::tid_val7 *HeaderPtr() const;
    void LoadDirectory(std::ifstream * const);                // load directory
                                // request number of records needed by a matrix
    unsigned long int MatrixRecords(const unsigned short int) const;
    unsigned short int NumberOfMatrices() const;  // request number of matrices
                                // print directory information into string list
    void PrintDirectory(std::list <std::string> * const) const;
                                         // set file pointer to start of matrix
    void SeekMatrixStart(std::ifstream * const,
                         const unsigned short int) const;
                                         // set file pointer to start of matrix
    void SeekMatrixStart(std::ofstream * const,
                         const unsigned short int) const;
 };

