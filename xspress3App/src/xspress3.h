/**
 * Asyn port driver for the Xspress3 system.
 *
 * @author Matthew Pearson
 * @date Sept 2012
 *
 * @version 0.1
 *
 */


#ifndef XSPRESS3_H
#define XSPRESS3_H

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <cantProceed.h>
#include <epicsTypes.h>

#include <asynOctetSyncIO.h>

#include "asynNDArrayDriver.h"

/* These are the drvInfo strings that are used to identify the parameters.
 * They are used by asyn clients, including standard asyn device support */
#define xsp3ResetParamString              "RESET"
#define xsp3EraseParamString              "ERASE"
#define xsp3StartParamString              "START"
#define xsp3StopParamString               "STOP"
#define xsp3StatusParamString             "STATUS"
#define xsp3NumChannelsParamString        "NUM_CHANNELS"
#define xsp3TriggerModeParamString        "TRIGGER_MODE"
#define xsp3NumFramesParamString          "NUM_FRAMES"
#define xsp3ChanMcaParamString            "CHAN_MCA"
#define xsp3ChanMcaCorrParamString        "CHAN_MCA_CORR"
#define xsp3ChanSca1ParamString           "CHAN_SCA1"
#define xsp3ChanSca2ParamString           "CHAN_SCA2"
#define xsp3ChanSca3ParamString           "CHAN_SCA3"
#define xsp3ChanSca4ParamString           "CHAN_SCA4"
#define xsp3ChanSca1CorrParamString       "CHAN_SCA1_CORR"
#define xsp3ChanSca2CorrParamString       "CHAN_SCA2_CORR"
#define xsp3ChanSca3CorrParamString       "CHAN_SCA3_CORR"
#define xsp3ChanSca4CorrParamString       "CHAN_SCA4_CORR"
#define xsp3ChanSca1HlmParamString        "CHAN_SCA1_HLM"
#define xsp3ChanSca2HlmParamString        "CHAN_SCA2_HLM"
#define xsp3ChanSca3HlmParamString        "CHAN_SCA3_HLM"
#define xsp3ChanSca4HlmParamString        "CHAN_SCA4_HLM"
#define xsp3ChanSca1LlmParamString        "CHAN_SCA1_LLM"
#define xsp3ChanSca2LlmParamString        "CHAN_SCA2_LLM"
#define xsp3ChanSca3LlmParamString        "CHAN_SCA3_LLM"
#define xsp3ChanSca4LlmParamString        "CHAN_SCA4_LLM"
#define xsp3ChanTotalParamString          "CHAN_TOTAL"
#define xsp3ChanTotalCorrParamString      "CHAN_TOTAL_CORR"



extern "C" {
int xspress3Config(const char *portName, int numChannels, int maxBuffers, size_t maxMemory);
}


class Xspress3 : public asynNDArrayDriver {

 public:
  Xspress3(const char *portName, int numChannels, int maxBuffers, size_t maxMemory);
  virtual ~Xspress3();

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

  void dataTask(void);
  void statusTask(void);

 private:

  //Put private functions here
  void log(epicsUInt32 mask, const char *msg, const char *function);
  

  //Put private static data members here
  static const epicsUInt32 logFlow_;
  static const epicsUInt32 logError_;

  //Put private dynamic here
  epicsUInt32 acquiring_; //Data acquisition in progress
  epicsUInt32 running_; //driver is in good state and we can read data from device
  epicsUInt32 debug_;

  const epicsInt32 numChannels_; //The number of channels (this is a constructor param).

  epicsEventId statusEvent_;
  epicsEventId startEvent_;
  epicsEventId stopEvent_;
  float pollingPeriod_;
  float fastPollingPeriod_;

  //Values used for pasynUser->reason, and indexes into the parameter library.
  int xsp3ResetParam;
  #define FIRST_DRIVER_COMMAND xsp3ResetParam
  int xsp3EraseParam;
  int xsp3StartParam;
  int xsp3StopParam;
  int xsp3StatusParam;
  int xsp3NumChannelsParam;
  int xsp3TriggerModeParam;
  int xsp3NumFramesParam;
  int xsp3ChanMcaParam;             
  int xsp3ChanMcaCorrParam;         
  int xsp3ChanSca1Param;             
  int xsp3ChanSca2Param;             
  int xsp3ChanSca3Param;           
  int xsp3ChanSca4Param;             
  int xsp3ChanSca1CorrParam;         
  int xsp3ChanSca2CorrParam;         
  int xsp3ChanSca3CorrParam;         
  int xsp3ChanSca4CorrParam;         
  int xsp3ChanSca1HlmParam;          
  int xsp3ChanSca2HlmParam;          
  int xsp3ChanSca3HlmParam;          
  int xsp3ChanSca4HlmParam;          
  int xsp3ChanSca1LlmParam;          
  int xsp3ChanSca2LlmParam;          
  int xsp3ChanSca3LlmParam;          
  int xsp3ChanSca4LlmParam;          
  int xsp3ChanTotalParam;            
  int xsp3ChanTotalCorrParam;            
  #define LAST_DRIVER_COMMAND xsp3ChanTotalCorrParam

};

#define NUM_DRIVER_PARAMS (&LAST_DRIVER_COMMAND - &FIRST_DRIVER_COMMAND + 1)

static const char *driverName = "Xspress3";


#endif //ISC110BL_H