// --------------------------------------------------------------
// History:
//
// Created by Chris Parkinson (chris.parkinson@cern.ch) 2015-10-28
// Modified by Francesco Gonnella 2016-05-05 (added Run and burst ID)
//
// --------------------------------------------------------------
#ifndef TPRIMITIVE_H
#define TPRIMITIVE_H

#include "TObject.h"

class TPrimitive : public TObject {

public:

  TPrimitive();
  virtual ~TPrimitive() {}

  void Clear(Option_t* = "");

  Int_t   GetTimeStamp()     { return fTimeStamp; }
  Short_t GetFineTime()      { return fFineTime; }
  Int_t   GetPrimitiveID()   { return fPrimitiveID; }
  Short_t GetSourceID()      { return fSourceID; }
  Short_t GetSubID()         { return fSubID; }
  Int_t   GetMTP()           { return fMTP; }
  Int_t   GetSendTimeStamp() { return fSendTimeStamp; }
  Int_t   GetRunID()         { return fRunID; }
  Int_t   GetBurstID()       { return fBurstID; }

  void SetTimeStamp     (Int_t   timestamp)     { fTimeStamp = timestamp; }
  void SetFineTime      (Short_t finetime)      { fFineTime = finetime; }
  void SetPrimitiveID   (Int_t   primitiveID)   { fPrimitiveID = primitiveID; }
  void SetSourceID      (Short_t sourceID)      { fSourceID = sourceID; }
  void SetSubID         (Short_t subID)         { fSubID = subID; }
  void SetMTP           (Int_t   MTP)           { fMTP = MTP; }
  void SetSendTimeStamp (Int_t   sendTimeStamp) { fSendTimeStamp = sendTimeStamp; }
  void SetRunID         (Int_t   RunID)         { fRunID = RunID; }
  void SetBurstID       (Int_t   BurstID)       { fBurstID = BurstID; }


  void Print(Option_t *option="") const;
  Double_t GetTime();

private:

  Int_t   fTimeStamp;
  Short_t fFineTime;
  Int_t   fPrimitiveID;
  Short_t fSourceID;
  Short_t fSubID;
  Int_t   fMTP;
  Int_t   fSendTimeStamp;
  Int_t   fRunID;
  Int_t   fBurstID;

  ClassDef(TPrimitive,1);

};

#endif
