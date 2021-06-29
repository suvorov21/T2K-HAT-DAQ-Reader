/*******************************************************************************
                           T2K 280 m TPC readout
                           _____________________

 File:        platform_spec.h

 Description: Specific include file and definitions for Linux PC

 Author:      D. Calvet,        denis.calvetATcea.fr


 History:
  March 2006: created

*******************************************************************************/
#ifndef PLATFORM_SPEC_H
#define PLATFORM_SPEC_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
//#include <stropts.h>
#include <string.h>
#include <errno.h>
#include <sched.h>

extern int errno;

#define yield() sched_yield()
#define xil_printf printf
#define mySleep(ms) usleep(1000*(ms))

#define __int64 long long

#endif
