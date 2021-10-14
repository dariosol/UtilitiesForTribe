// --------------------------------------------------------------
// History:
//
// Created by Chris Parkinson (chris.parkinson@cern.ch) 2015-10-28
// Modified by Francesco Gonnella 2016-05-05 (added Run and burst ID)
//
// --------------------------------------------------------------
#include "Riostream.h"
#include "TPrimitive.hh"
#define ClockPeriod 24.951059536
ClassImp(TPrimitive)

TPrimitive::TPrimitive()
: fTimeStamp(0), fFineTime(0), fPrimitiveID(0), fSourceID(0),
  fSubID(0),  fMTP(0), fSendTimeStamp(0), fRunID(0), fBurstID(0) {}

void TPrimitive::Print(Option_t */*option*/) const {

  std::cout << "Primitive: TimeStamp= " << fTimeStamp
       << " FineTime= " << fFineTime << std::endl 
       << " PrimitiveID= " << fPrimitiveID << std::endl 
       << " SourceID= " << fSourceID << std::endl 
       << " SubID= " << fSubID << std::endl  
       << " MTP= " << fMTP << std::endl 
       << " SendTimeStamp= " << fSendTimeStamp  << std::endl 
       << " BurstID= " << fBurstID << std::endl  
       << " RunID= " << fBurstID
       << std::endl;
}

Double_t TPrimitive::GetTime() {
  return (fTimeStamp + fFineTime*(1./256.)) * ClockPeriod;
}

void TPrimitive::Clear(Option_t* /*option*/) {
  fTimeStamp     = 0;
  fFineTime      = 0;
  fPrimitiveID   = 0;
  fSourceID      = 0;
  fSubID         = 0;
  fMTP           = 0;
  fSendTimeStamp = 0;
  fRunID         = 0;
  fBurstID       = 0;
}
