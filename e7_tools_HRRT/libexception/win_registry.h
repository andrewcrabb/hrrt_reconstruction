/*! \file win_registry.h
    \brief This class is used to read and write Windows registry keys.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
 */

#pragma once

#include <string>
#include <windows.h>

/*- class definitions -------------------------------------------------------*/

class RegistryAccess
 { private:
    HKEY hkey;                                             /*!< registry key */
    bool key_open;                                /*!< registry key opened ? */
    std::string key;                               /*!< name of registry key */
   public:
    RegistryAccess(const std::string, const bool);  // open/create registry key
    ~RegistryAccess();                                    // close registry key
                                                    // request value of sub key
    void getKeyValue(const std::string, std::string * const);
    void getKeyValue(const std::string, unsigned long int * const);
                                                        // set value of sub key
    void setKeyValue(const std::string, const std::string);
    void setKeyValue(const std::string, const unsigned long int);
 };
