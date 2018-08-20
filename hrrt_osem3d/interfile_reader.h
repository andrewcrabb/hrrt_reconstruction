/*! \file interfile_reader.h
    \copyright HRRT User Community
    \brief Provide interface to access Interfile header.
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2008/03/07 initial version
 */
#pragma once

#include <sys/types.h>

#define RETURN_MSG_SIZE 256
#define ERROR_MSG_SIZE 300


// Error codes
#define IFH_TABLE_INVALID 1001          /*!< table size must be 0 for loading */
#define IFH_FILE_OPEN_ERROR 1002        /*!< Error opening specified filename */
#define IFH_FILE_INVALID 1003            /*!< file not valid Interfile header */

typedef
struct _IFH_TAble{
    char *filename;
    char **tags;
    char **data;
    size_t size;
}
IFH_Table;

extern "C" {
    /*! Load filename in the table */
    int interfile_load(char* in_filename, IFH_Table *out_table);

    /*! Find the value corresponding to a tag */
    int interfile_find(IFH_Table *in_table,  const char *in_tag, char *out_str, int in_str_len);

    /*! free table memory allocated by interfile_load */
    int interfile_clear(IFH_Table *in_table);

}
