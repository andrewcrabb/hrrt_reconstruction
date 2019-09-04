/*! \class ECAT7_DIRECTORY ecat7_directory.h "ecat7_directory.h"
    \brief This class deals with the directory structure of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2004/09/10 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class deals with the directory structure of ECAT7 files and provides
    methods to add entries and directory blocks and load and save directories.
    A directory is a doubly-linked circular list of directory blocks. Each
    directory block is 512 bytes long. The first 16 bytes of a block contain
    the following information:
    -  0- 3:  number of available entries
    -  4- 7:  pointer to next directory block
    -  8-11:  pointer to previous directory block
    - 12-15:  number of allocated entries in directory block

    The rest of the directory block contains entries for 31 matrices of the
    following form (16 byte each):
    -  0- 3:  matrix identifier
    -  4- 7:  record number of matrix subheader
    -  8-11:  last record number of matrix data
    - 12-15:  matrix status (1: matrix exists, 0: data not yet written,
                           -1: matrix deleted)

    The matrix identifier encodes the bed-, frame-, gate-, plane and dataset-
    number. The 32 bits are used as follows:
    -   0- 8:  frame number (0-511)
    -   9-10:  plane number (high bits)
    -  11   :  dataset number (high bit)
    -  12-15:  bed number (0-15)
    -  16-23:  plane number (low bits, 0-1023)
    -  24-29:  gate number (0-63)
    -  30-31:  dataset number (low bits, 0-7)
 */

#include <iostream>
#include "ecat7_directory.h"
#include "data_changer.h"
#include "ecat7_global.h"
#include "exception.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
ECAT7_DIRECTORY::ECAT7_DIRECTORY()
 { id_vals.clear();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator for object.
    \param[in] e7   source object
    \return destination object

    Copy operator for object.
 */
/*---------------------------------------------------------------------------*/
ECAT7_DIRECTORY& ECAT7_DIRECTORY::operator = (const ECAT7_DIRECTORY &e7)
 { if (this != &e7) id_vals=e7.id_vals;
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create new entries in array of matrix information.
    \param[in] num   number of new entries

    Create new entries in array of matrix information.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::AddEntries(const unsigned short int num)
 { unsigned short int old_size;

   old_size=id_vals.size();
   id_vals.resize(old_size+num);
   memset(&id_vals[old_size], 0, num*sizeof(tid_val7));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Append directory entry for matrix.
    \param[in] filename     name of ECAT7 file
    \param[in] matrix_rec   number of records needed by the matrix
    \param[in] mat_nr       number of matrix

    Append directory entry for matrix with given number. If the last directory
    block in the chain is full, a new block is created and appended to the
    file. All changes are made directly to the file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::AppendEntry(const std::string filename,
                                  const unsigned long int matrix_rec,
                                  const unsigned long int mat_nr)
 { unsigned long int recnr;
   std::ifstream *file_in=NULL;
   std::ofstream *file_out=NULL;

   recnr=2;
   try
   { do { tdir_list dir_list;

          file_in=new std::ifstream(filename.c_str(),
                                    std::ios::in|std::ios::binary);
          if (!*file_in)
           throw Exception(REC_FILE_DOESNT_EXIST,
                           "The file '#1' doesn't exist.").arg(filename);
          LoadDirBlock(file_in, recnr, &dir_list);      // load directory block
          file_in->close();
          delete file_in;
          file_in=NULL;
          if (dir_list.num_avail_entries == 0)                  // block full ?
           { if (dir_list.next_dir_rec == 2)       // last block of directory ?
              {                              // connect full block to new block
                dir_list.next_dir_rec=dir_list.entry[30][2];
                file_out=new std::ofstream(filename.c_str(),
                                           std::ios::out|std::ios::in|
                                           std::ios::binary);
                SaveDirBlock(file_out, recnr, &dir_list);    // save full block
                                          // create new (empty) directory block
                CreateDirBlock(file_out, recnr, dir_list.next_dir_rec);
                file_out->close();
                delete file_out;
                file_out=NULL;
              }
           }
           else { signed long int nr;
                                   // block not full -> insert entry for matrix
                  nr=dir_list.num_alloc_entries;
                                        // store matrix information as ID value
                  dir_list.entry[nr][0]=MatrixID(id_vals[mat_nr].bed,
                                                 id_vals[mat_nr].frame,
                                                 id_vals[mat_nr].gate,
                                                 id_vals[mat_nr].plane,
                                                 id_vals[mat_nr].data);
                                           // record number of matrix subheader
                  if (nr > 0) dir_list.entry[nr][1]=dir_list.entry[nr-1][2]+1;
                   else dir_list.entry[nr][1]=recnr+1;
                                                // last record number of matrix
                  dir_list.entry[nr][2]=dir_list.entry[nr][1]+matrix_rec;
                  dir_list.entry[nr][3]=1;                     // matrix exists
                  dir_list.num_avail_entries--;
                  dir_list.num_alloc_entries++;
                                      // store numbers of first and last record
                  id_vals[mat_nr].first_record=dir_list.entry[nr][1]-1;
                  id_vals[mat_nr].last_record=dir_list.entry[nr][2]-1;
                                                // save changed directory block
                  file_out=new std::ofstream(filename.c_str(),
                                             std::ios::out|std::ios::in|
                                             std::ios::binary);
                  SaveDirBlock(file_out, recnr, &dir_list);
                  file_out->close();
                  delete file_out;
                  file_out=NULL;
                }
          recnr=dir_list.next_dir_rec;
        } while (recnr != 2);                // loop to last block of directory
   }
   catch (...)                                             // handle exceptions
    { if (file_in != NULL) { file_in->close();
                             delete file_in;
                           }
      if (file_out != NULL) { file_out->close();
                              delete file_out;
                            }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create empty directory block.
    \param [in,out]   file           handle of ECAT7 file
     \param [in]      last_dir_rec   number of last directory record
     \param [in]      this_dir_rec   number of record for new directory block

    Create empty directory block.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::CreateDirBlock(std::ofstream * const file,
                                     const signed long int last_dir_rec,
                                     const signed long int this_dir_rec) const
 { tdir_list dir_list;
                                                // create empty directory block
   dir_list.num_avail_entries=31;
   dir_list.next_dir_rec=2;
   dir_list.last_dir_rec=last_dir_rec;
   dir_list.num_alloc_entries=0;
   memset(&dir_list.entry, 0, 31*4*sizeof(signed long int));
   SaveDirBlock(file, this_dir_rec, &dir_list);                   // save block
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete entry from array of matrix information.
    \param[in] num   number of matrix to delete

    Delete entry from array of matrix information.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::DeleteEntry(const unsigned short int num)
 { if (num < id_vals.size())
    { if (num < id_vals.size()-1)
       memcpy(&id_vals[num], &id_vals[num+1],
              (id_vals.size()-num-1)*sizeof(tid_val7));
      id_vals.resize(id_vals.size()-1);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to array with matrix information.
    \return pointer to array with matrix information
=
    Request pointer to array with matrix information.
 */
/*---------------------------------------------------------------------------*/
ECAT7_DIRECTORY::tid_val7 *ECAT7_DIRECTORY::HeaderPtr() const
 { return((ECAT7_DIRECTORY::tid_val7 *)&id_vals[0]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load directory block.
    \param[in,out] file       handle of ECAT7 file
    \param[in]     dir_rec    record number of directory block
    \param[in,out] dir_list   directory data structure

    Load directory block.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::LoadDirBlock(std::ifstream * const file,
                                   const signed long int dir_rec,
                                   tdir_list * const dir_list) const
 { DataChanger *dc=NULL;

   file->seekg((dir_rec-1)*E7_RECLEN);
   try
   {                   // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&dir_list->num_avail_entries);
     dc->Value(&dir_list->next_dir_rec);
     dc->Value(&dir_list->last_dir_rec);
     dc->Value(&dir_list->num_alloc_entries);
     for (unsigned short int i=0; i < 31; i++)
      for (unsigned short int j=0; j < 4; j++)
       dc->Value(&dir_list->entry[i][j]);
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load complete directory and create array of matrix information.
    \param[in,out] file   handle of ECAT7 file

    Load complete directory and create array of matrix information.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::LoadDirectory(std::ifstream * const file)
 { unsigned long int recnr=2;                    // pointer to directory record

   id_vals.clear();
   do { tdir_list dir_list;

        LoadDirBlock(file, recnr, &dir_list);           // load directory block
                                                     // analyze directory block
        for (signed long int i=0; i < dir_list.num_alloc_entries; i++)
         { unsigned long int matrix_id;
           tid_val7 tv;
                             // extract matrix information from directory entry
           matrix_id=dir_list.entry[i][0];
           tv.bed=(matrix_id >> 12) & 0xF;
           tv.frame=matrix_id & 0x1FF;
           tv.gate=(matrix_id >> 24) & 0x3F;
           tv.plane=((matrix_id >> 16) & 0xFF)+((matrix_id >> 1) & 0x300);
           tv.data=((matrix_id >> 30) & 0x3)+((matrix_id >> 9) & 0x4);
           tv.first_record=dir_list.entry[i][1]-1;
           tv.last_record=dir_list.entry[i][2]-1;
           id_vals.push_back(tv);
         }
        recnr=dir_list.next_dir_rec;
      } while (recnr != 2);                      // until last block is reached
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert matrix information into ID value.
    \param[in] bed     bed number (0-15 = 4 Bit)
    \param[in] frame   frame number (0-511 = 9 Bit)
    \param[in] gate    gate number (0-63 = 6 Bit)
    \param[in] plane   plane number (0-1023 = 10 Bit)
    \param[in] data    (0-7 = 3 Bit)
    \return ID value

    Convert matrix information into ID value. The structure of the bits in the
    ID value is as follows: d2 d1 g6 g5 g4 g3 g2 g1 p8 p7 p6 p5 p4 p3 p2 p1 b4
    b3 b2 b1 d3 p10 p9 f9 f8 f7 f6 f5 f4 f3 f2 f1.
 */
/*---------------------------------------------------------------------------*/
signed long int ECAT7_DIRECTORY::MatrixID(const signed long int bed,
                                          const signed long int frame,
                                          const signed long int gate,
                                          const signed long int plane,
                                          const signed long int data) const
 { return(((bed & 0xF) << 12) | (frame & 0x1FF) | ((gate & 0x3F) << 24) |
          ((plane & 0xFF) << 16) | ((plane & 0x300) << 1) |
          ((data & 0x3) << 30) | ((data & 0x4) << 9));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of records of a matrix (incl. header).
    \param[in] num   number of matrix
    \return number of records

    Request number of records of a matrix (incl. header).
 */
/*---------------------------------------------------------------------------*/
unsigned long int ECAT7_DIRECTORY::MatrixRecords(
                                            const unsigned short int num) const
 { if (num >= NumberOfMatrices()) return(0);
   return(id_vals[num].last_record-id_vals[num].first_record+1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of matrices.
    \return number of matrices

    Request number of matrices.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_DIRECTORY::NumberOfMatrices() const
 { return(id_vals.size());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print directory information into string list.
    \param[in,out] sl   string list

    Print directory information into string list.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::PrintDirectory(std::list <std::string> * const sl) const
 { sl->push_back("******************* ID-values *****************");
   for (unsigned long int i=0; i < id_vals.size(); i++)
    { sl->push_back(" "+toString(i+1)+": bed:   "+toString(id_vals[i].bed));
      sl->push_back(" "+toString(i+1)+": frame: "+toString(id_vals[i].frame));
      sl->push_back(" "+toString(i+1)+": gate:  "+toString(id_vals[i].gate));
      sl->push_back(" "+toString(i+1)+": plane: "+toString(id_vals[i].plane));
      sl->push_back(" "+toString(i+1)+": data:  "+toString(id_vals[i].data));
      sl->push_back(" "+toString(i+1)+": first: "+
                    toString(id_vals[i].first_record));
      sl->push_back(" "+toString(i+1)+": last:  "+
                    toString(id_vals[i].last_record));
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store directory block.
    \param[in,out] file       handle of ECAT7 file
    \param[in]     dir_rec    record number of directory block
    \param[in,out] dir_list   data structure of directory block

    Store directory block.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::SaveDirBlock(std::ofstream * const file,
                                   const signed long int dir_rec,
                                   const tdir_list * const dir_list) const
 { DataChanger *dc=NULL;

   file->seekp((dir_rec-1)*E7_RECLEN);
   try
   {                  // DataChanger is used to store data system independently
     dc = new DataChanger(E7_RECLEN);
                                                       // fill data into buffer
     dc->Value(dir_list->num_avail_entries);
     dc->Value(dir_list->next_dir_rec);
     dc->Value(dir_list->last_dir_rec);
     dc->Value(dir_list->num_alloc_entries);
     for (unsigned short int i=0; i < 31; i++)
      for (unsigned short int j=0; j < 4; j++)
       dc->Value(dir_list->entry[i][j]);
     dc->SaveBuffer(file);                                       // store block
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set file pointer to start of matrix.
    \param[in,out] file   handle of ECAT7 file
    \param[in]     num    number of matrix

    Set file pointer to start of matrix to read from file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::SeekMatrixStart(std::ifstream * const file,
                                      const unsigned short int num) const
 { if (num >= NumberOfMatrices())
    throw Exception(REC_ECAT7_MATRIX_NUMBER_WRONG,
                    "The matrix '#1' does not exist in the ECAT7 file"
                    ".").arg(num);
   file->seekg(id_vals[num].first_record*E7_RECLEN);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set file pointer to start of matrix.
    \param[in,out] file   handle of ECAT7 file
    \param[in]     num    number of matrix

    Set file pointer to start of matrix to write to file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_DIRECTORY::SeekMatrixStart(std::ofstream * const file,
                                      const unsigned short int num) const
 { if (num >= NumberOfMatrices())
    throw Exception(REC_ECAT7_MATRIX_NUMBER_WRONG,
                    "The matrix '#1' does not exist in the ECAT7 file"
                    ".").arg(num);
   file->seekp(id_vals[num].first_record*E7_RECLEN);
 }
