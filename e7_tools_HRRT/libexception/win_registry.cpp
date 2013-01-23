/*! \class RegistryAccess win_registry.h "win_registry.h"
    \brief This class is used to read and write Windows registry keys.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version

    This class is used to read and write Windows registry keys. It's just a
    C++ wrapper for the Windows registry API.
*/

#include <cstring>
#include "win_registry.h"
#include "exception.h"

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Open or create a registry key.
    \param[in] ikey           name of registry key
    \param[in] write_access   write access to key ? (otherwise: read-only)

    Open or create a registry key under the top key HKEY_LOCAL_MACHINE.
 */
/*---------------------------------------------------------------------------*/
RegistryAccess::RegistryAccess(const std::string ikey, const bool write_access)
 { REGSAM flag;

   key=ikey;
   if (write_access) flag=KEY_ALL_ACCESS;
    else flag=KEY_READ;
   if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, NULL,
                      REG_OPTION_NON_VOLATILE, flag, NULL, &hkey,
                      NULL) != ERROR_SUCCESS)
    {                                                     // generate exception
      key_open=false;
      throw Exception(REC_REGKEY_ACCESS,
                      "Can't open registry key "
                      "'HKEY_LOCAL_MACHINE\\#1'.").arg(key);
    }
   key_open=true;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Close registry key.

    Close registry key.
 */
/*---------------------------------------------------------------------------*/
RegistryAccess::~RegistryAccess()
 { if (key_open) RegCloseKey(hkey);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request string value of sub key.
    \param[in]  sub_key   name of sub key
    \param[out] value     value of sub_key

    Request the string value of a sub key. The key is stored under
    HKEY_LOCAL_MACHINE\\key\\sub_key.
 */
/*---------------------------------------------------------------------------*/
void RegistryAccess::getKeyValue(const std::string sub_key,
                                 std::string * const value)
 { unsigned char *val=NULL;

   try
   { DWORD dwSize, dwType;

     *value=std::string();
                                                            // get registry key
     if (RegQueryValueEx(hkey, sub_key.c_str(), NULL, &dwType, NULL,
                         &dwSize) != ERROR_SUCCESS)
      throw Exception(REC_REGSUBKEY_ACCESS, "Can't read registry key "
                      "'HKEY_LOCAL_MACHINE\\#1\\#2'.").arg(key).arg(sub_key);
     if (dwType != REG_SZ)
      throw Exception(REC_REGKEY_TYPE, "Registry key has wrong datatype.");
                                                            // get value of key
     val=new unsigned char[dwSize];
     if (RegQueryValueEx(hkey, sub_key.c_str(), NULL, NULL, val,
                         &dwSize) != ERROR_SUCCESS)
      throw Exception(REC_REGSUBKEY_ACCESS, "Can't read registry key "
                      "'HKEY_LOCAL_MACHINE\\#1\\#2'.").arg(key).arg(sub_key);
     *value=std::string((char *)val);
     delete[] val;
     val=NULL;
   }
   catch (...)
    { if (val != NULL) delete[] val;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request integer value of sub key.
    \param[in]  sub_key   name of sub key
    \param[out] value     value of sub_key

    Request the integer value of a sub key. The key is stored under
    HKEY_LOCAL_MACHINE\\key\\sub_key.
 */
/*---------------------------------------------------------------------------*/
void RegistryAccess::getKeyValue(const std::string sub_key,
                                 unsigned long int * const value)
 { DWORD dwSize, dwType;

   *value=0;
                                                            // get registry key
   if (RegQueryValueEx(hkey, sub_key.c_str(), NULL, &dwType, NULL,
                       &dwSize) != ERROR_SUCCESS)
    throw Exception(REC_REGSUBKEY_ACCESS, "Can't read registry key "
                    "'HKEY_LOCAL_MACHINE\\#1\\#2'.").arg(key).arg(sub_key);
   if (dwType != REG_DWORD)
    throw Exception(REC_REGKEY_TYPE, "Registry key has wrong datatype.");
                                                            // get value of key
   if (RegQueryValueEx(hkey, sub_key.c_str(), NULL, NULL,
                       (unsigned char *)value, &dwSize) != ERROR_SUCCESS)
    throw Exception(REC_REGSUBKEY_ACCESS, "Can't read registry key "
                    "'HKEY_LOCAL_MACHINE\\#1\\#2'.").arg(key).arg(sub_key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set or change string value of sub key.
    \param[in] sub_key   name of sub key
    \param[in] value     value of sub_key

    Set or change the string value of a sub key. The sub key is created if
    necessary. The key is stored under HKEY_LOCAL_MACHINE\\key\\sub_key.
 */
/*---------------------------------------------------------------------------*/
void RegistryAccess::setKeyValue(const std::string sub_key,
                                 const std::string value)
 { if (RegSetValueEx(hkey, sub_key.c_str(), 0, REG_SZ,
                     (unsigned char *)value.c_str(),
                     (DWORD)(value.length()+1)) != ERROR_SUCCESS)
    throw Exception(REC_REGSUBKEY_WRITE, "Can't change value of registry key "
                    "'HKEY_LOCAL_MACHINE\\#1\\#2'.").arg(key).arg(sub_key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set or change integer value of sub key.
    \param[in] sub_key   name of sub key
    \param[in] value     value of sub_key

    Set or change the string value of a sub key. The sub key is created if
    necessary. The key is stored under HKEY_LOCAL_MACHINE\\key\\sub_key.
 */
/*---------------------------------------------------------------------------*/
void RegistryAccess::setKeyValue(const std::string sub_key,
                                 const unsigned long int value)
 { if (RegSetValueEx(hkey, sub_key.c_str(), 0, REG_DWORD,
                     (unsigned char *)&value,
                     sizeof(unsigned long int)) != ERROR_SUCCESS)
    throw Exception(REC_REGSUBKEY_WRITE, "Can't change value of registry key "
                    "'HKEY_LOCAL_MACHINE\\#1\\#2'.").arg(key).arg(sub_key);
 }

