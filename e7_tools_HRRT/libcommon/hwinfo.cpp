/*! \file hwinfo.cpp
    \brief This module is used to gather information about the hardware (CPUs).
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments

    This code uses some operating system functions and special CPU commands to
    gather some information about the used hardware.
 */

#include <iostream>
#include <fstream>
#include <string>
#ifdef __MACOSX__
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach-o/arch.h>
#endif
#if defined(__LINUX__) || defined(__SOLARIS__)
/* #include <sys/sysinfo.h> */
#endif
#ifdef WIN32
#include "exception.h"
#include "registry.h"
#include "str_tmpl.h"
#include "win_registry.h"
#endif
#include "logging.h"

/*---------------------------------------------------------------------------*/
/*! \brief Request number of logical CPUs.
    \return number of logical CPUs

    Request number of logical CPUs. If the environment variable
    SINGLE_THREADED_RECON is set under Windows, this function returns always 1.
    Otherwise it checks the environment variable NUMBER_OF_PROCESSORS.

    Under Linux the function counts the number of processors listed in the
    "/proc/cpuinfo" file.

    Under Mac OS X it requests the number of CPUs from the operating system.
 */
/*---------------------------------------------------------------------------*/
unsigned short int logicalCPUs()
 { unsigned short int num_log_cpus;

   num_log_cpus=0;
#ifdef __MACOSX__
   { mach_msg_type_number_t count=HOST_VM_INFO_COUNT;
     host_basic_info hinfo;

     host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hinfo, &count);
     num_log_cpus=hinfo.avail_cpus;
   }
#endif
#ifdef __SOLARIS__
   num_log_cpus=1;
#endif
#ifdef __LINUX__
   std::ifstream *file=NULL;

   try
   {                                            // count number of logical CPUs
     file=new std::ifstream("/proc/cpuinfo");
     for (;;)
      { std::string line;

        std::getline(*file, line);
                // eof is only false after trying to read after the end of file
        if (file->eof() && line.empty()) break;
        if (line.substr(0, 9) == "processor") num_log_cpus++;
      }
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
#endif
#ifdef WIN32
   if (getenv("SINGLE_THREADED_RECON") != NULL) num_log_cpus=1;
    else { char *env;

           if ((env=getenv("NUMBER_OF_PROCESSORS")) != NULL)
            num_log_cpus=atoi(env);
            else num_log_cpus=1;
           if (num_log_cpus < 1) num_log_cpus=1;
         }
#endif
   return(num_log_cpus);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print hardware information to Logging object.
    \param[in] loglevel   top level for logging

    Print hardware information to Logging object.

    Under Windows the procedure requests the amount of memory from the
    operating system. If the environment variable SINGLE_THREADED_RECON is set,
    it reports one CPU, otherwise the number stored in the environment variable
    NUMBER_OF_PROCESSORS. The CPU description is read from the registry key
    Hardware\\Description\\System\\CentralProcessor\\0\\ProcessorNameString
    which may not be available with older versions of Windows.

    Under Linux the procedure gets the number of CPUs and the CPU description
    from the file "/proc/cpuinfo" and requests the amount of memory from the
    operating system. It reports additional information about the CPU features
    and the cache sizes, which are retrieved by the "cpuid" assembler call
    (IA-32 Intel Architecture Software Developer's Manual, Vol 2: Instruction
     Set Reference, pp. 3-113, Order Number 245471-007, Intel).

    Under Mac OS X the procedure requests the amount of memory, number of CPUs
    and CPU description from the operating system.
 */
/*---------------------------------------------------------------------------*/
void printHWInfo(const unsigned short int loglevel)
 {
#ifndef __SOLARIS__
   std::string cpu_name="unknown";
   unsigned short int i=0;
   unsigned long int mbyte=0;
#endif
#ifdef __MACOSX__
   { mach_msg_type_number_t count=HOST_VM_INFO_COUNT;
     host_basic_info hinfo;
     const NXArchInfo *ai;

     host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hinfo, &count);
     mbyte=hinfo.memory_size/(1024*1024);
     i=hinfo.avail_cpus;
     ai=NXGetArchInfoFromCpuType(hinfo.cpu_type, hinfo.cpu_subtype);
     cpu_name=std::string(ai->description);
   }
#endif
#ifdef WIN32
   RegistryAccess *ra=NULL;
   std::string str;

   try
   { Logging::flog()->loggingOn(false);
     for (i=0; i < 16; i++)
      { ra=new RegistryAccess(cpukey+"\\"+toString(i), false);
        ra->getKeyValue("ProcessorNameString", &str);
        if (i == 0) cpu_name=str;
        delete ra;
        ra=NULL;
      }
   }
   catch (const Exception)
    { MEMORYSTATUS lp;

      Logging::flog()->loggingOn(true);
      while (cpu_name.at(0) == ' ') cpu_name.erase(0, 1);
      if (ra != NULL) delete ra;
      GlobalMemoryStatus(&lp);
      mbyte=lp.dwTotalPhys/(1024*1024);
    }
   if (getenv("SINGLE_THREADED_RECON") != NULL) i=1;
    else { char *env;

           if ((env=getenv("NUMBER_OF_PROCESSORS")) != NULL) i=atoi(env);
            else i=1;
           if (i < 1) i=1;
         }
#endif
#if defined(__LINUX__) && defined(__INTEL_COMPILER)
   std::ifstream *file=NULL;

   file=new std::ifstream("/proc/cpuinfo");
   for (;;)
    { std::string line;

      std::getline(*file, line);
                // eof is only false after trying to read after the end of file
      if (file->eof() && line.empty()) break;
      if (line.substr(0, 9) == "processor") i++;
      if (line.substr(0, 10) == "model name") cpu_name=line.substr(13);
    }
   file->close();
   delete file;
   { struct sysinfo si;

     sysinfo(&si);
     mbyte=si.totalram/1024*si.mem_unit/1024;
   }
   unsigned long int eax, ebx, ecx, edx, log_per_cpu, a=1;
   unsigned short int count, dv[15];
   bool hyperthreading_support, sse, sse2, mmx, cache2=false, cache3=false;
   std::string str;
                         // detect Hyper-Threading support
                         // IA-32 Intel Architecture Software Developer' Manual
                         // Volume 3: System Programming Guide
   asm("cpuid":"=b" (ebx), "=d" (edx):"a" (a));
   hyperthreading_support=((edx & 0x10000000) > 0);
   sse=((edx & 0x02000000) > 0);
   sse2=((edx & 0x04000000) > 0);
   mmx=((edx & 0x00800000) > 0);
   log_per_cpu=(ebx & 0xFF0000) >> 16;
#endif
#ifndef __SOLARIS__
   Logging::flog()->logMsg("#1 (#2x)    memory: #3 MByte", loglevel)->
    arg(cpu_name)->arg(i)->arg(mbyte);
#endif
#if defined(__LINUX__) && defined(__INTEL_COMPILER)

   if (mmx || sse || sse2)
    { str=" (";
      if (mmx) str+="MMX,";
      if (sse) str+="SSE,";
      if (sse2) str+="SSE2,";
      str=str.substr(0, str.length()-1)+")";
    }
   if (hyperthreading_support)
    Logging::flog()->logMsg("Hyper-Threading supported (#2 logical processors "
                            "per CPU) #1", loglevel)->arg(log_per_cpu)->
     arg(str);
    else Logging::flog()->logMsg("Hyper-Threading not supported", loglevel)->
          arg(str);
   Logging::flog()->logMsg("Cache information", loglevel);
   asm("cpuid":"=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx):"a" (2));
   count=eax & 0xF;
   for (unsigned short int i=1; i < count; i++)
    asm("cpuid":"=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx):"a" (2));
   count=0;
   if ((eax & 0x80000000) == 0) { dv[count++]=(eax & 0x0000FF00) >> 8;
                                  dv[count++]=(eax & 0x00FF0000) >> 16;
                                  dv[count++]=(eax & 0xFF000000) >> 24;
                                }
   if ((ebx & 0x80000000) == 0) { dv[count++]=(ebx & 0x000000FF);
                                  dv[count++]=(ebx & 0x0000FF00) >> 8;
                                  dv[count++]=(ebx & 0x00FF0000) >> 16;
                                  dv[count++]=(ebx & 0xFF000000) >> 24;
                                }
   if ((ecx & 0x80000000) == 0) { dv[count++]=(ecx & 0x000000FF);
                                  dv[count++]=(ecx & 0x0000FF00) >> 8;
                                  dv[count++]=(ecx & 0x00FF0000) >> 16;
                                  dv[count++]=(ecx & 0xFF000000) >> 24;
                                }
   if ((edx & 0x80000000) == 0) { dv[count++]=(edx & 0x000000FF);
                                  dv[count++]=(edx & 0x0000FF00) >> 8;
                                  dv[count++]=(edx & 0x00FF0000) >> 16;
                                  dv[count++]=(edx & 0xFF000000) >> 24;
                                }
   for (i=0; i < count; i++)
    { std::string s=std::string();

      switch (dv[i])
       { case 0x00:
          break;
         case 0x01:
          s="Instruction TLB: 4 KB Pages, 4-way set associative, 32 entries";
          break;
         case 0x02:
          s="Instruction TLB: 4 MB Pages, 4-way set associative, 2 entries";
          break;
         case 0x03:
          s="Data TLB: 4 KB Pages, 4-way set associative, 64 entries";
          break;
         case 0x04:
          s="Data TLB: 4 KB Pages, 4-way set associative, 8 entries";
          break;
         case 0x06:
          s="1st-level instruction cache: 8 KB, 4-way set associative, 32 byte"
            " line size";
          break;
         case 0x08:
          s="1st-level instruction cache: 16 KB, 4-way set associative, "
            "32 byte line size";
          break;
         case 0x0A:
          s="1st-level instruction cache: 8 KB, 2-way set associative, 32 byte"
            " line size";
          break;
         case 0x0C:
          s="1st-level instruction cache: 16 KB, 4-way set associative, "
            "32 byte line size";
          break;
         case 0x22:
          s="3rd-level cache: 512 KB, 4-way set associative, 64 byte line "
            "size";
          cache3=true;
          break;
         case 0x23:
          s="3rd-level cache: 1 MB, 8-way set associative, 64 byte line size";
          cache3=true;
          break;
         case 0x40:
          //s="No 2nd-level cache or, if processor contains a valid 2nd-level "
          //  "cache, no 3rd-level cache";
          break;
         case 0x41:
          s="2nd-level cache: 128 KB, 4-way set associative, 32 byte line "
            "size";
          cache2=true;
          break;
         case 0x42:
          s="2nd-level cache: 256 KB, 4-way set associative, 32 byte line "
            "size";
          cache2=true;
          break;
         case 0x43:
          s="2nd-level cache: 512 KB, 4-way set associative, 32 byte line "
            "size";
          cache2=true;
          break;
         case 0x44:
          s="2nd-level cache: 1 MB, 4-way set associative, 32 byte line size";
          cache2=true;
          break;
         case 0x45:
          s="2nd-level cache: 2 MB, 4-way set associative, 32 byte line size";
          cache2=true;
          break;
         case 0x50:
          s="Instruction TLB: 4 KB and 2 MB or 4 MB pages, 64 entries";
          break;
         case 0x51:
          s="Instruction TLB: 4 KB and 2 MB or 4 MB pages, 128 entries";
          break;
         case 0x52:
          s="Instruction TLB: 4 KB and 2 MB or 4 MB pages, 256 entries";
          break;
         case 0x5B:
          s="Data TLB: 4 KB and 4 MB pages, 64 entries";
          break;
         case 0x5C:
          s="Data TLB: 4 KB and 4 MB pages, 128 entries";
          break;
         case 0x5D:
          s="Data TLB: 4 KB and 4 MB pages, 256 entries";
          break;
         case 0x66:
          s="1st-level cache: 8 KB, 4-way set associative, 64 byte line size";
          break;
         case 0x67:
          s="1st-level cache: 16 KB, 4-way set associative, 64 byte line size";
          break;
         case 0x68:
          s="1st-level cache: 32 KB, 4-way set associative, 64 byte line size";
          break;
         case 0x70:
          s="Trace cache: 12 K-";
#ifdef WIN32
          s+=(char)230;
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
          s+="µ";
#endif
          s+="op, 8-way set associative";
          break;
         case 0x71:
          s="Trace cache: 16 K-";
#ifdef WIN32
          s+=(char)230;
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
          s+="µ";
#endif
          s+="op, 8-way set associative";
          break;
         case 0x72:
          s="Trace cache: 32 K-";
#ifdef WIN32
          s+=(char)230;
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
          s+="µ";
#endif
          s+="op, 8-way set associative";
          break;
         case 0x79:
          s="2nd-level cache: 128 KB, 8-way set associative, sectored, 64 byte"
            " line size";
          cache2=true;
          break;
         case 0x7A:
          s="2nd-level cache: 256 KB, 8-way set associative, sectored, 64 byte"
            " line size";
          cache2=true;
          break;
         case 0x7B:
          s="2nd-level cache: 512 KB, 8-way set associative, sectored, 64 byte"
            " line size";
          cache2=true;
          break;
         case 0x7C:
          s="2nd-level cache: 1 MB, 8-way set associative, sectored, 64 byte "
            "line size";
          cache2=true;
          break;
         case 0x82:
          s="2nd-level cache: 256 KB, 8-way set associative, sectored, 32 byte"
            " line size";
          cache2=true;
          break;
         case 0x83:
          s="2nd-level cache: 512 KB, 8-way set associative, sectored, 32 byte"
            " line size";
          cache2=true;
          break;
         case 0x84:
          s="2nd-level cache: 1 MB, 8-way set associative, sectored, 32 byte "
            "line size";
          cache2=true;
          break;
         case 0x85:
          s="2nd-level cache: 2 MB, 8-way set associative, sectored, 32 byte "
            "line size";
          cache2=true;
          break;
       }
      if (!s.empty()) Logging::flog()->logMsg(s, loglevel+1);
    }
   if (!cache2) Logging::flog()->logMsg("no 2nd-level cache", loglevel+1);
   if (!cache3) Logging::flog()->logMsg("no 3rd-level cache", loglevel+1);
#endif
 }
