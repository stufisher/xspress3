#include <iostream>
#include <string>
#include <stdexcept>

//Epics headers
#include <iocsh.h>
#include <drvSup.h>
#include <registryFunction.h>
#include <epicsExport.h>


//Xspress3 SDK header
extern "C" {
  //#include "xspress3sdk.h"
}

#include "xspress3.h"

using std::cout;
using std::endl;

//Definitions of static class data members
const epicsUInt32 Xspress3::logFlow_ = 1;
const epicsUInt32 Xspress3::logError_ = 2;

//C Function prototypes to tie in with EPICS
static void xsp3StatusTaskC(void *drvPvt);
static void xsp3DataTaskC(void *drvPvt);


/**
 * Constructor for Xspress3::Xspress3. 
 * This must be called in the Epics IOC startup file.
 * @param portName The Asyn port name to use
 * @param maxChannels The number of channels to use (eg. 8)
 */
Xspress3::Xspress3(const char *portName, int numChannels)
  : asynPortDriver(portName,
                   1, /* maxAddr */ 
                   NUM_DRIVER_PARAMS,
                   asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynDrvUserMask | asynOctetMask, /* Interface mask */
                   asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
                   0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
                   1, /* Autoconnect */
                   0, /* Default priority */
                   0), /* Default stack size*/
  numChannels_(numChannels)
{
  int status = asynSuccess;
  const char *functionName = "Xspress3::Xspress3";

  log(logFlow_, "Start of constructor", functionName); 

  //Create the epicsEvent for signaling to the status task when parameters should have changed.
  //This will cause it to do a poll immediately, rather than wait for the poll time period.
  statusEvent_ = epicsEventMustCreate(epicsEventEmpty);
  if (!statusEvent_) {
    printf("%s:%s epicsEventCreate failure for status event\n", driverName, functionName);
    return;
  }
  dataEvent_ = epicsEventMustCreate(epicsEventEmpty);
  if (!dataEvent_) {
    printf("%s:%s epicsEventCreate failure for data event\n", driverName, functionName);
    return;
  }

  //Add the params to the paramLib
  createParam(xsp3ResetParamString,   asynParamInt32,       &xsp3ResetParam);
  createParam(xsp3Chan1ArrayParamString,   asynParamInt32Array,  &xsp3Chan1ArrayParam);

 //Initialize non static data members
  acquiring_ = 0;

  pollingPeriod_ = 0.2; //seconds
  fastPollingPeriod_ = 0.01; //seconds

  /* Create the thread that updates the Xspress3 status */
  status = (epicsThreadCreate("Xsp3StatusTask",
                              epicsThreadPriorityMedium,
                              epicsThreadGetStackSize(epicsThreadStackMedium),
                              (EPICSTHREADFUNC)xsp3StatusTaskC,
                              this) == NULL);
  if (status) {
    printf("%s:%s epicsThreadCreate failure for status task\n",driverName, functionName);    
    return;
  }


    /* Create the thread that readouts the data */
  status = (epicsThreadCreate("GeDataTask",
                              epicsThreadPriorityMedium,
                              epicsThreadGetStackSize(epicsThreadStackMedium),
                              (EPICSTHREADFUNC)xsp3DataTaskC,
                              this) == NULL);
  if (status) {
    printf("%s:%s epicsThreadCreate failure for data task\n",
           driverName, functionName);
    return;
  }

  running_ = 1;

  //Set any paramLib parameters that need passing up to device support
  setIntegerParam(xsp3NumChannelsParam, numChannels_);

  //statusRead();

  callParamCallbacks();

  log(logFlow_, "End of constructor", functionName); 

}

/**
 * Wrapper for asynPrint and local debug prints. If the debug_ data member
 * is on, then it simply prints the message. Otherwise if uses asynPrint statements.
 * @param mask Use either logFlow_ or logError_
 * @param msg The message to print
 * @param function The name of the calling function
 */
void Xspress3::log(epicsUInt32 mask, const char *msg, const char *function)
{
  if (debug_ == 1) {
    printf("  %s %s\n", function, msg);
  } else {
    if (mask == logFlow_) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s %s.\n", function, msg);
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s %s.\n", function, msg);
    }
  }
}



asynStatus Xspress3::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *functionName = "Xspress3::writeInt32";
  
  log(logFlow_, "Calling ", functionName);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s asynUser->reason: &d.\n", functionName, function);

  if (function == xsp3ResetParam) {
    //Do a system reset
    log(logFlow_, "System reset", functionName);
  } else {
    log(logError_, "No matching parameter.", functionName);
  }

  //Set in param lib so the user sees a readback straight away. We might overwrite this in the 
  //status task, depending on the parameter.
  status = (asynStatus) setIntegerParam(function, value);

  //Do callbacks so higher layers see any changes 
  callParamCallbacks();

  return status;
}


asynStatus Xspress3::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *functionName = "Xspress3::writeFloat64";
  
  log(logFlow_, "Calling ", functionName);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s asynUser->reason: &d.\n", functionName, function);

  

  //Set in param lib so the user sees a readback straight away. We might overwrite this in the 
  //status task, depending on the parameter.
  status = (asynStatus) setDoubleParam(function, value);

  //Do callbacks so higher layers see any changes 
  callParamCallbacks();

  return status;
}


//Global C utility functions to tie in with EPICS

static void xsp3StatusTaskC(void *drvPvt)
{
  Xspress3 *pPvt = (Xspress3 *)drvPvt;

  pPvt->statusTask();
}


static void xsp3DataTaskC(void *drvPvt)
{
  Xspress3 *pPvt = (Xspress3 *)drvPvt;
  
  pPvt->dataTask();
}

/*************************************************************************************/
/** The following functions have C linkage, and can be called directly or from iocsh */


/**
 * Config function for IOC shell. It instantiates an instance of the driver.
 * @param portName The Asyn port name to use
 * @param numChannels The number of channels (eg. 8)
 */
extern "C" {

  int xspress3Config(const char *portName, int numChannels)
  {
    asynStatus status = asynSuccess;
    
    /*Instantiate class.*/
    try {
      new Xspress3(portName, numChannels);
    } catch (...) {
      cout << "Unknown exception caught when trying to construct Xspress3." << endl;
      status = asynError;
    }
    
    return(status);
  }

  /* Code for iocsh registration */

  /* xspress3Config */
  static const iocshArg xspress3ConfigArg0 = {"Controller port name", iocshArgString};
  static const iocshArg xspress3ConfigArg1 = {"Number of channels", iocshArgInt};
  static const iocshArg * const xspress3ConfigArgs[] = {&xspress3ConfigArg0,
							&xspress3ConfigArg1};
  static const iocshFuncDef configxspress3Config = {"xspress3Config", 2, xspress3ConfigArgs};
  static void configxspress3ConfigCallFunc(const iocshArgBuf *args)
  {
    xspress3Config(args[0].sval, args[1].ival);
  }
  

} // extern "C"


/****************************************************************************************/
