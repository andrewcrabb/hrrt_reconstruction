/*! \file interfile_reader.c
    \copyright HRRT User Community
    \brief Implement access Interfile header.
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2008/03/07 initial version
    \date 2009/07/20 Clean \r when loading 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "interfile_reader.h"


#define MAX_LINE_LENGTH 1024
#define MAX_TABLE_SIZE 2048

/*! \brief Decode a key:=value line and insert in memory table.
    \param[in] IFH_table      Interfile (key,value) table
    \param[in] buffer         Interfile (key:=value) string
    \return 1 if inserted  or 0 if invalid string or max table size reached

 * Decode a key:=value line and insert in memory table.
 * Returns 1 if inserted and 0 if invalid string or max table size reached
 */
static int 
insert_tag(IFH_Table *table, char *buffer)
{
	char *cptr,*cptr1;

    if (table->size >= MAX_TABLE_SIZE-1) return 0;
	// remove trailing newline and carriage return
	if ((cptr = strchr(buffer,'\n')))
		*cptr = '\0';
	if ((cptr = strchr(buffer,'\r')))
		*cptr = '\0';

	// if this is not a comment
	if ((cptr = strstr(buffer,":=")))
	{
		cptr1 = cptr - 1;
		while (*cptr1 == ' ')
			cptr1--;
		cptr1++;
		*cptr1 = '\0';
		if (strcasecmp(buffer,"!Interfile") != 0)
		{
			table->tags[table->size] = strdup(buffer);
			cptr += 2;
			while (*cptr == ' ')
				cptr++;
			table->data[table->size] = strdup(cptr);
			return 1;
		}
	}
	return 0;
}

/**
 * Do load open filename in memory table.
 * Returns number of loaded elements.
 */
static int
do_load(FILE *fp, IFH_Table *table)
{
	char buffer[MAX_LINE_LENGTH];
	while (fgets(buffer,MAX_LINE_LENGTH,fp))
		table->size += insert_tag(table, buffer);
	return table->size;
}

/*! \brief Load specified Interfile header in memory table.
    \param[in] filename      Interfile header filename
    \param[in] IFH_table      Interfile (key,value) memory table
    \return 0 on success  or error code on failure

 * Load specified filename in memory table.
 * Returns 0 on success  or error code on failure
 */
int 
interfile_load(char *filename, IFH_Table *table)
{
    FILE *fp=NULL;

    if (table->size != 0) return IFH_TABLE_INVALID;
    if ((fp=fopen(filename,"r")) == NULL) return IFH_FILE_OPEN_ERROR;
    table->tags = (char**)calloc(MAX_TABLE_SIZE, sizeof(char*));
    table->data = (char**)calloc(MAX_TABLE_SIZE, sizeof(char*));
    do_load(fp,table);
    fclose(fp);
    return 0;
}

/*! \brief Search specified key value from memory table
    \param[in] IFH_table      Interfile (key,value) memory table
    \param[in] key            key string
    \param[out] val           pointer to value string
    \param[in] val            max value string size
    \return 1 if found  or 0 otherwise

    Find specified key from memory table.
    Returns 1 if found or 0 otherwise.
 */
int 
interfile_find(IFH_Table *table, char *key, char* val, int len)
{
  size_t i;
	for (i = 0; i < table->size; i++)
	{
		if (strstr(table->tags[i],key))
		{
			strncpy(val,table->data[i],len);
			val[len-1] = '\0';
			return 1;
		}
	}
	return 0;
}

/*! \brief Clear memory table
    \param[in] IFH_table      Interfile (key,value) memory table
    \return 0 on success and error code on failure
    
    Clear memory table allocated in interfile_load.
    Returns 0 if OK or error code otherwise.
 */
int 
interfile_clear(IFH_Table *table)
{
    size_t i;
    if (table->size<=0 || table->size>=MAX_TABLE_SIZE)
        return IFH_TABLE_INVALID;
    for (i = 0; i < table->size; i++)
    {
        free(table->tags[i]); 
        free(table->data[i]);
    }
    free(table->tags);  
    free(table->data);
    table->tags = table->data = NULL;
    table->size = 0;
    return 0;
}