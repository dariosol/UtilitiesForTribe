#include "TH2.h"
#include "TH1.h"
#include "TGraph.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>
#include "TPrimitive.hh"
using namespace std;

bool sortfunction (int i,int j) {
  return (i<j);
}
//Algorithm to reconstruct the RAM address on the L0TP
long long L0address(unsigned int timestamp, unsigned int finetime, int bitfinetime){
 
  unsigned int ftMSBmask; 
  unsigned int timestampLSBmask;
  long long address;
  unsigned int timestampLSB;
  unsigned int ftMSB;

  if(bitfinetime==3) timestampLSBmask = 2047; //11111111111 (binary)
  if(bitfinetime==2) timestampLSBmask = 4095; 
  if(bitfinetime==1) timestampLSBmask = 8191; 
  if(bitfinetime==0) timestampLSBmask = 16383;
 
  if(bitfinetime==3) ftMSBmask=224;////11100000 (binary)
  if(bitfinetime==2) ftMSBmask=192;////11000000 (binary)
  if(bitfinetime==1) ftMSBmask=128;////10000000 (binary)
  if(bitfinetime==0) ftMSBmask=0;  ////00000000 (binary)
 
  timestampLSB = timestampLSBmask & timestamp;
  ftMSB = ftMSBmask & finetime;
  ftMSB = ftMSB >> ((unsigned int)8-(unsigned int)bitfinetime);
   
  address = timestampLSB;
  address = address <<(unsigned int)bitfinetime;
  address = address |  ftMSB;

  return address;

}


vector<unsigned long long> Merging(vector<unsigned long long> ReferenceFIFO,vector<unsigned long long> ControlFIFO){

  int MTPRef=ReferenceFIFO.size(); 
  int MTPCntrl=ControlFIFO.size();

  int indexC=0;//index of the FIFO
  int indexR=0;//index of the FIFO


  unsigned long long ValueR=0;//Register of the FIFO
  unsigned long long ValueC=0;//Register of the FIFO

  unsigned long long Datum=0;
  
  vector<unsigned long long> MergedFifo;

  //STATE: WaitForPackets: Both the boolean for the reading are set to 1
    
  bool ReadReference = 1;
  bool ReadControl   = 1;
  bool PrimitiveFinish=0;
  bool PrimitiveControlFinish=0;

  while(1){

    Datum=0;
    
    //STATE: ReadFIFO:
    if(indexC == MTPCntrl) PrimitiveControlFinish=1;
    if(indexR == MTPRef)   PrimitiveFinish=1;

    if(PrimitiveControlFinish==1 && PrimitiveFinish==1) break; //Come back to wait for packet
  
    //Read request. I cannot use Primitive Control Finish in the same state, because it is not yet set
    if(indexR < MTPRef && ReadReference == 1) {
      ValueR = ReferenceFIFO.at(indexR);
      indexR++;
    }
      
    if(indexC < MTPCntrl && ReadControl == 1) {
      ValueC = ControlFIFO.at(indexC);
      indexC++;
    }

    if(PrimitiveFinish==0 && PrimitiveControlFinish==0) {

      if((ValueR>>7) < (ValueC>>7)) { // comparing TSTMP + 1 bit of FT (rejecting last 7 bits)

	Datum = ((unsigned long long)0x1)<<42 | ValueR;
	MergedFifo.push_back(Datum);
	ReadReference=1;
	ReadControl=0;	  
      }

      else if((ValueC>>7) < (ValueR>>7)) {
	Datum = ((unsigned long long)0x10)<<42 | ValueC;	
	MergedFifo.push_back(Datum);
	ReadReference=0;
	ReadControl=1;	  
      }

      else if((ValueC>>7) == (ValueR>>7)) {
	Datum = ((unsigned long long)0x11)<<42 | ValueR;	
	MergedFifo.push_back(Datum);
	ReadReference=1;
	ReadControl=1;	  
      }
    }
	
    else if(PrimitiveFinish==0 && PrimitiveControlFinish==1) {
      Datum = ((unsigned long long)0x1)<<42 | ValueR;
      MergedFifo.push_back(Datum);
      ReadReference=1;
      ReadControl=0;	     
    }

    else if(PrimitiveFinish==1 && PrimitiveControlFinish==0) {
      Datum = ((unsigned long long)0x10)<<42 | ValueC;
      MergedFifo.push_back(Datum);
      ReadReference=0;
      ReadControl=1;	     
    }

	
  }//end while 1

  return MergedFifo;
}

unsigned int GetDataType(unsigned long long i){
  //1 is physics
  //16 is control
  //17 is both
  return i >> 42;
}

unsigned long long GetTime(unsigned long long i){
  //32 bit timestamp + 8 bit finetime
  return i & (unsigned long long)0xFFFFFFFFFF;
}


int TimeComparison(TString Data1, TString Data2, TString DetectorNewCHOD,TString DetectorLKr, TString OutputFile){
  TFile *f1 = new TFile(Data1);
  if(!f1){
    std::cout << " file not found" << std::endl ;
    return -1; 
  }

  TFile *f2 = new TFile(Data2);
  if(!f2){
    std::cout << " file not found" << std::endl ;
    return -1; 
  }

  
  //TPRIMITIVE NewCHOD
  TPrimitive *PrimitiveNewCHOD= new TPrimitive();
  TTree *treeNewCHOD = (TTree*)f1->Get(DetectorNewCHOD.Data());
  if(!treeNewCHOD) cout<<"tree of "<<DetectorNewCHOD.Data()<<" not found"<<endl;

  cout<<DetectorNewCHOD.Data()<<" tree opened"<<endl;

  //Branches treeNewCHOD
  TBranch *bNewCHOD;
 
  cout<<"branches:"<<endl;
  bNewCHOD=treeNewCHOD->GetBranch("fPrimitive");
     
  bNewCHOD->SetAddress(&PrimitiveNewCHOD);
  cout<<"branches set "<<endl;

  cout<<"Itializing variables:"<<endl;
  
  cout<<"entries from "<<DetectorNewCHOD.Data()<<": "<<bNewCHOD->GetEntries()<<endl;       
 
  cout<<"Start to analyze data from "<<DetectorNewCHOD.Data()<<endl;

  //TPRIMITIVE LKr
  TPrimitive *PrimitiveLKr= new TPrimitive();
  
  TTree *treeLKr = (TTree*)f2->Get(DetectorLKr.Data());
  if(!treeLKr) cout<<"tree of "<<DetectorLKr.Data()<<" not found"<<endl;

  cout<<DetectorLKr.Data()<<" tree opened"<<endl;

  //Branches tree LKr
  TBranch *bLKr;
 
  cout<<"branches:"<<endl;
  bLKr=treeLKr->GetBranch("fPrimitive");
     
  bLKr->SetAddress(&PrimitiveLKr);
  cout<<"branches set "<<endl;

 
  
  cout<<"entries from "<<DetectorLKr.Data()<<": "<<bLKr->GetEntries()<<endl;       
 
  cout<<"Start to analyze data from "<<DetectorLKr.Data()<<endl;
  
  vector<unsigned long long> ReferenceFIFO;
  vector<unsigned long long> ControlFIFO;
  vector<unsigned long long> MergedFIFO;

  cout<<"Itializing variables:"<<endl;
  Int_t iNewCHOD=0;
  Int_t iLKr=0;
  Int_t NewCHODEndFrame=0;
  Int_t LKrEndFrame=0;
  MergedFIFO.clear();
  ControlFIFO.clear();
  ReferenceFIFO.clear();
  
    
  cout<<"Start scanning the frames "<<endl;  

  // for(int iFrame=0; iFrame < 2.60000000000000000e+08; iFrame +=256) { //loop along the frames of the burst
  Int_t iFrame=0;
  
  while(1) {
    if(iNewCHOD==treeNewCHOD->GetEntries() && iLKr==treeLKr->GetEntries()) break;
     
    if(iFrame>47496704*2) {
      cout<<"finish"<<endl;
      cout<<"iNewCHOD "<<iNewCHOD<<" iLKr "<<iLKr<<endl;
      break;
    }
    if(NewCHODEndFrame==0) treeNewCHOD->GetEntry(iNewCHOD);
    if(LKrEndFrame==0) treeLKr->GetEntry(iLKr);
  
    if(PrimitiveNewCHOD->GetSendTimeStamp()==iFrame) {
      ReferenceFIFO.push_back(((unsigned long long)(PrimitiveNewCHOD->GetTimeStamp()))<<8 | ((unsigned long long)PrimitiveNewCHOD->GetFineTime()));
      iNewCHOD++;
    }
    else {
      NewCHODEndFrame=1;
    }
      
    if(PrimitiveLKr->GetSendTimeStamp()==iFrame) {
      ControlFIFO.push_back(((unsigned long long)PrimitiveLKr->GetTimeStamp()/*+400*/)<<8 | ((unsigned long long)PrimitiveLKr->GetFineTime()));
      iLKr++;
    }
    else {
      LKrEndFrame=1;
    }


    if(LKrEndFrame==1 && NewCHODEndFrame==1) {

      //if(ReferenceFIFO.size()>0)ReferenceFIFO.erase(ReferenceFIFO.end() - 1);
      MergedFIFO=Merging(ReferenceFIFO,ControlFIFO);
	
      if(ControlFIFO.size()>1 && ReferenceFIFO.size()>1) {
	cout<<"************"<<endl;
	cout<<"size ctrl "<<ControlFIFO.size()<<endl;
	cout<<"size ref "<<ReferenceFIFO.size()<<endl;

	for(int k=0;k<ControlFIFO.size();k++) {
	  cout<<"Control FIFO "<<hex<<ControlFIFO.at(k)<<endl;
	}
	  
	for(int k=0;k<ReferenceFIFO.size();k++) {
	  cout<<"Ref FIFO "<<hex<<ReferenceFIFO.at(k)<<endl;
	}	  
      }
	
      for(int j=0;j<MergedFIFO.size();j++) {
	  
	unsigned long long TotalTime=  GetTime(MergedFIFO.at(j));
	unsigned int       FineTime =  (TotalTime & 0xFF);
	unsigned long long TS       =  ((TotalTime & 0xFFFFFFFF00) >> 8);
	unsigned int DT = GetDataType(MergedFIFO.at(j));
	if(ControlFIFO.size()>1 && ReferenceFIFO.size()>1) {	    
	  cout<<"iFrame "<<iFrame<<" ("<<iFrame/256.<<")"<<endl;
	  cout<<"size "<<MergedFIFO.size()<<endl;	    
	  cout<<"Time: "<<hex<<TotalTime<<endl;  
	  cout<<"DataType: "<< DT  <<endl <<" TS " <<TS << " FT "<<FineTime<<endl;
	}  // end if
	      
      }// end for
      NewCHODEndFrame=0;
      LKrEndFrame=0;
      iFrame+=256;
      MergedFIFO.clear();
      ReferenceFIFO.clear();
      ControlFIFO.clear();
      if(iFrame > 0x1c00) break;
    }	

  }


  return 0;
}

const bool fast=0;
int PrimitiveInPacket(TString Data,TString Detector, TString OutputFile) {
 
  TFile *f = new TFile(Data);
  if(!f){
    std::cout << " file not found" << std::endl ;
    return -1; 
  }
  //Masks for PrimitiveID selection.

  Int_t Granularity=2;

  Bool_t Axions=0;
  
  //----MUV3 MASKS:----//
  Int_t MaskMOQx = 0x200;
  Int_t MaskMO1 = 0x400;
  Int_t MaskMO2 = 0x1000;

  Int_t MaskM1 = 0x800;
  Int_t MaskM2 = 0x2000;
  Int_t MaskMUV = 0x4000;

  //---NewCHOD MASKS---//
  Int_t MaskQx  = 0x800;
  Int_t MaskUTMC= 0x1000;
  Int_t MaskQ1  = 0x4000;
  Int_t MaskQ2  = 0x400;

  //-----LKR MASKS----------
  Int_t MaskLKr2GeV    = 0x400;
  Int_t MaskLKrPiNuNu  = 0x4000;
  Int_t MaskLKr10GeV   = 0x2000;
  Int_t MaskLKr20GeV   = 0x1000;
  
  Int_t IsOverWritten = 0;
  Int_t IsRepeated    = 0;
  Int_t SameTS        = 0;

  Int_t NUTMC=0;
  Int_t NQ1  =0;
  Int_t NQ2  =0;
  Int_t NQx  =0;
 
  Int_t NMO1=0;
  Int_t NMO2=0;
  Int_t NM1=0;
  Int_t NM2=0;
  Int_t NMUV=0;
  Int_t NMOQx=0;

  Int_t NLKr2GeV    = 0;
  Int_t NLKrPiNuNu  = 0;
  Int_t NLKr10GeV   = 0;
  Int_t NLKr20GeV   = 0;
  

  Int_t IsMO1         = 0;
  Int_t IsMO2         = 0;
  Int_t WasMO1        = 0;
  Int_t WasMO2        = 0;
  Int_t MO2Diff       = 0;
  Int_t MO1Diff       = 0;

  Int_t IsQ1          = 0;
  Int_t IsQ2          = 0;
  Int_t IsQx          = 0;
  Int_t IsUTMC        = 0;
  Int_t WasQx         = 0;
  Int_t WasQ2         = 0;

  Int_t WasUTMC       = 0;
  Int_t QxDiff        = 0;
  Int_t Q2Diff        = 0;

  Int_t UTMCDiff      = 0;

  
  Int_t NewQx         = 0;
  Int_t NewQ2         = 0;

  Int_t NewUTMC       = 0;
  Int_t LostQx        = 0;
  Int_t LostQ2        = 0;
  
  Int_t LostUTMC      = 0;
  Int_t NewMO1        = 0;
  Int_t NewMO2        = 0;
  Int_t LostMO1       = 0;
  Int_t LostMO2       = 0;

  Int_t NLKrBit10    = 0;
  Int_t IsLKrBit10   = 0;
  Int_t WasLKrBit10  = 0;
  Int_t NewLKrBit10  = 0;
  Int_t LostLKrBit10 = 0;

  Int_t FirstTimeStamp=0;
  Int_t LastTimeStamp=0;

  Int_t OldRandomTS=0;
  
  TH1D *h_rawtimestamp = new TH1D(Form("RawTimeStamp%s",Detector.Data()),Form("TimeStamp %s; [ms]",Detector.Data()),7000,0,7000000);
  TH1D *h_foldedTS     = new TH1D(Form("FoldedTS%s",Detector.Data()),Form("foldedTS %s",Detector.Data()),256,-0.5,255.5);
  TH1D *h_foldedTS_DS  = new TH1D(Form("FoldedTS_DS%s",Detector.Data()),Form("foldedTS_DS %s",Detector.Data()),256,-0.5,255.5);

  TH1D *h_foldedTS_Qx  = new TH1D(Form("FoldedTS_Qx%s",Detector.Data()),Form("foldedTS_Qx %s",Detector.Data()),256,-0.5,255.5);
  TH1D *h_foldedTS_MO1 = new TH1D(Form("FoldedTS_MO1%s",Detector.Data()),Form("foldedTS_MO1 %s",Detector.Data()),256,-0.5,255.5);
  TH1D *h_foldedTS_MO2 = new TH1D(Form("FoldedTS_MO2%s",Detector.Data()),Form("foldedTS_MO2 %s",Detector.Data()),256,-0.5,255.5);

  TH1D *h_timestamp1_2sec    = new TH1D(Form("TimeStamp1_2%s",Detector.Data()),Form("TimeStamp %s; Time [s]; Rate [MHz]",Detector.Data()),1000,1,2);
 
  TH1D *h_timestamp2_3sec    = new TH1D(Form("TimeStamp2_3%s",Detector.Data()),Form("TimeStamp %s; Time [s]; Rate [MHz]",Detector.Data()),1000,2,3);

  TH1D *h_timestamp3_4sec    = new TH1D(Form("TimeStamp3_4%s",Detector.Data()),Form("TimeStamp %s; Time [s]; Rate [MHz]",Detector.Data()),1000,3,4);

  TH1D *h_timestamp4_5sec    = new TH1D(Form("TimeStamp4_5%s",Detector.Data()),Form("TimeStamp %s; Time [s]; Rate [MHz]",Detector.Data()),1000,4,5);
 
   
  TH2D *h_primitiveinpacket = new TH2D(Form("PrimitiveInPacket%sVsTimeStamp",Detector.Data()),Form("Primitives in %s MTP Vs TimeStamp;TimeStamp; # of primitives",Detector.Data()),256e3,0,256e6,256,-0.5,255.5); 

  TH1D *fHPrimTimeForEffSpill = new TH1D("fHPrimTimeForEffSpillRICH","RICH primitives rate", 1e5, 0., 1e10);

  TH1D *h_primitiveID = new TH1D(Form("PrimitiveID%s",Detector.Data()),"",16384,16384,16384*2); 
  TH1D *h_rate        = new TH1D(Form("Rate%s",Detector.Data()),"Rate; MHz",100,0,200);  
  TH1D *h_DT          = new TH1D(Form("DT%s",Detector.Data()),"#Delta T events; x 1 ns",20000,0,20000);
  TH1D *h_LKrDT       = new TH1D(Form("LKrDT%s",Detector.Data()),"#Delta T events; x 1 ns",2000,0,20000);

  TH1D *h_DTTimeStampDS         = new TH1D(Form("DTTimeStampDS%s",Detector.Data()),"#Delta Timestamp; x 1 ns",4000,0,40000);

  TH1D *h_DTTimeStamp         = new TH1D(Form("DTTimeStamp%s",Detector.Data()),"#Delta Timestamp; x 25 ns",200,0,200);
  
  TH1D * h_DTCloser           = new TH1D (Form("DTCloser%s",Detector.Data()),"#Delta T of primitives closer than 12.5 ns",500,-250,250);
  TH1D * h_LKr2GeVRate        = new TH1D ("h_LKr2GeVRate","",100,0,10); //In KHz
  TH1D *h_DTTimeStampInPacket = new TH1D(Form("DTTimeStampInPacket%s",Detector.Data()),Form("DTTimeStampInPacket %s; [s]",Detector.Data()),512,0,512);
     
  //TPRIMITIVE
  TPrimitive *Primitive= new TPrimitive();
  TPrimitive *oldPrimitive = new TPrimitive();
  TPrimitive *oldPrimitiveDS = new TPrimitive();
  long int Downscaling =0;
  
  TTree *tree = (TTree*)f -> Get(Detector.Data());
  if (!tree) cout<<"tree of "<<Detector.Data()<<" not found"<<endl;

  cout<<Detector.Data()<<" tree opened"<<endl;

  //Branches
  TBranch *b;
  cout<<"branches:"<<endl;
  b = tree -> GetBranch("fPrimitive");
     
  b -> SetAddress(&Primitive);
  cout<<"branches set "<<endl;

  cout<<"Itializing variables:"<<endl;

  int PrimitiveInPacket = 1;
  oldPrimitive -> SetTimeStamp(0);
  oldPrimitive -> SetFineTime(0);
  oldPrimitive -> SetSendTimeStamp(0);
  oldPrimitive -> SetPrimitiveID(0);

  cout<<"entries from "<<Detector.Data()<<": "<<b->GetEntries()<<endl;       
 
  cout<<"Start to analyze data from "<<Detector.Data()<<endl;

  long long Time1=0;

  long long Time = 0;
  long long OldTime=0;
  Double_t diff=0;
  Double_t Rate=0;
  int RepeatedPrimitive=0;
  int OverWrittenPrimitive=0;
  long long PrimitiveAddress=0;
  long long OldPrimitiveAddress=0;
  
  long long TimeLKr2GeV=0;
  long long oldTimeLKr2GeV=0;
  long long TimeLKr=0;
  long long TimeIRC=0;

  TH1F *DTRandom               = new TH1F("DTRandom","DTRandom",1000,0.0,0.00005);

  
  Int_t test=0;
  Int_t test_break=0;
  Int_t test_N=0;
  Int_t test_TS=0;
  Int_t test_level=90000000;
  Double_t fulltime=0;
  for (Long64_t iEntry=0; iEntry<tree->GetEntries(); iEntry++) {
    if(iEntry%1000000==0) cout<<iEntry<<" over "<<tree->GetEntries()<<" ("<<1.*iEntry/tree->GetEntries()<<")"<<endl;
    tree->GetEntry(iEntry);
    
    //--Clean variables
    if(IsOverWritten==0) {
      IsQ1       = 0;
      IsQ2       = 0;
      IsQx       = 0;
      IsUTMC     = 0;
      IsMO1      = 0;
      IsMO2      = 0;
      IsLKrBit10 = 0;
      
    }
    //--RepeatedPrimitive
    IsRepeated=0;
    if(Primitive->GetTime()==oldPrimitive->GetTime()) {
      RepeatedPrimitive++;
      IsRepeated=1;
    }
      
    if(Primitive->GetTimeStamp()==oldPrimitive->GetTimeStamp()) SameTS++;
       
    //------------------------------------------------------------  
    //--Fill histogram with number of primitives per ethernet packet:
    if(Primitive->GetSendTimeStamp() > oldPrimitive->GetSendTimeStamp()) {
      h_primitiveinpacket->Fill(Primitive->GetTimeStamp(),PrimitiveInPacket);
      PrimitiveInPacket=1;
      FirstTimeStamp=Primitive->GetTimeStamp();
      h_DTTimeStampInPacket->Fill(FirstTimeStamp-LastTimeStamp);
    }
    else {
      PrimitiveInPacket++;
      LastTimeStamp=Primitive->GetTimeStamp();
    }
    //------------------------------------------------------------  

    //Estremi per il run 7608: [1.2,2.2]
    
    h_rawtimestamp   -> Fill(Primitive->GetTimeStamp()*0.000000025*1000000);
    fulltime = (Double_t)Primitive->GetTime() ;
    fHPrimTimeForEffSpill->Fill(fulltime);

    if(Primitive->GetTimeStamp()<40000000*2 && Primitive->GetTimeStamp()>40000000*1) {
      h_timestamp1_2sec   -> Fill(Primitive->GetTimeStamp()*0.000000025);
    }

    if(Primitive->GetTimeStamp()<40000000*3 && Primitive->GetTimeStamp()>40000000*2) {
      h_timestamp2_3sec   -> Fill(Primitive->GetTimeStamp()*0.000000025);
    
    }

    if(Primitive->GetTimeStamp()<40000000*4 && Primitive->GetTimeStamp()>40000000*3) {
      h_timestamp3_4sec   -> Fill(Primitive->GetTimeStamp()*0.000000025);
    }

    if(Primitive->GetTimeStamp()<40000000*5 && Primitive->GetTimeStamp()>40000000*4) {
      h_timestamp4_5sec   -> Fill(Primitive->GetTimeStamp()*0.000000025);
    }
    
    
    h_DTTimeStamp ->Fill(Primitive->GetTimeStamp()-oldPrimitive->GetTimeStamp());

    if(Downscaling==100) {
      h_DTTimeStampDS ->Fill(Primitive->GetTimeStamp()*25-oldPrimitiveDS->GetTimeStamp()*25);      
      oldPrimitiveDS->SetTimeStamp(Primitive->GetTimeStamp());
      oldPrimitiveDS->SetFineTime(Primitive->GetFineTime());
      oldPrimitiveDS->SetSendTimeStamp(Primitive->GetSendTimeStamp());
      oldPrimitiveDS->SetPrimitiveID(Primitive->GetPrimitiveID());
      Downscaling=0;
    }
    else
      Downscaling++;
    
    Time= (long long)Primitive->GetTimeStamp()*0x100 + Primitive->GetFineTime();
    OldTime= (long long)oldPrimitive->GetTimeStamp()*0x100 + oldPrimitive->GetFineTime();
    diff=(Double_t)(Time-OldTime); //in 100 ps => 1 ns = 10*diff; 1000ns = 10000*diff
    
    //--Rates
    if(iEntry>0) {
      Rate=diff;
      Rate=1./Rate;
      h_rate->Fill(Rate*10000); //Rate in MHz
      h_DT->Fill(diff*0.1); //Diff in ns.
    }
    h_primitiveID->Fill(Primitive->GetPrimitiveID());

    if(fast==0) {
      if(Detector=="IRC") {
	IsQx    = Primitive->GetPrimitiveID()&MaskQx;
	IsUTMC  = Primitive->GetPrimitiveID()&MaskUTMC;
	IsQ1    = Primitive->GetPrimitiveID()&MaskQ1;
	IsQ2    = Primitive->GetPrimitiveID()&MaskQ2;
    
	if(IsQx)   NQx++;
	if(IsUTMC) NUTMC++;
	if(IsQ1)   NQ1++;
	if(IsQ2)   NQ2++;
      }
    
      if(Detector=="MUV3") {
	IsMO1    = Primitive->GetPrimitiveID()&MaskMO1;
	IsMO2    = Primitive->GetPrimitiveID()&MaskMO2;
	if(IsMO1) NMO1++;
	if(IsMO2) NMO2++;
	if((Primitive->GetPrimitiveID()&MaskM1   )!=0) NM1++;
	if((Primitive->GetPrimitiveID()&MaskM2   )!=0) NM2++;
	if((Primitive->GetPrimitiveID()&MaskMOQx )!=0) NMOQx++;
	if((Primitive->GetPrimitiveID()&MaskMUV  )!=0) NMUV++;
	
      }

      if(Detector=="LKr" and Axions==1) {
      
	IsLKrBit10    = Primitive->GetPrimitiveID()&MaskLKr2GeV;
	if(IsLKrBit10) NLKrBit10++;
	if(Primitive->GetPrimitiveID()&MaskLKr2GeV) {
	  oldTimeLKr2GeV=TimeLKr2GeV;

	  TimeLKr2GeV=(long long)Primitive->GetTimeStamp()*0x100 + Primitive->GetFineTime();
	
	  Double_t diff1=(Double_t)(TimeLKr2GeV-oldTimeLKr2GeV); //in 100 ps => 1 ns = 10*diff; 1000ns = 10000*diff

	  h_LKrDT->Fill(10*diff1);

	  diff1 = 1./diff1;

	  h_LKr2GeVRate -> Fill(diff1*1e7); //Rate in KHz

	}
      
      }

      if(Detector=="LKr" and Axions==0) {
      
	if((Primitive->GetPrimitiveID()&MaskLKr10GeV )!= 0) NLKr10GeV++;
	if((Primitive->GetPrimitiveID()&MaskLKr20GeV )!= 0) NLKr20GeV++;
	if((Primitive->GetPrimitiveID()&MaskLKrPiNuNu) != 0) NLKrPiNuNu++;
      }
    }
    h_foldedTS -> Fill(Primitive->GetTimeStamp()%256);
    if((iEntry%400) == 0) h_foldedTS_DS ->  Fill(Primitive->GetTimeStamp()%256);
    
    if(IsQx)h_foldedTS_Qx -> Fill(Primitive->GetTimeStamp()%256);
    if(IsMO1)h_foldedTS_MO1 -> Fill(Primitive->GetTimeStamp()%256);
    if(IsMO2)h_foldedTS_MO2 -> Fill(Primitive->GetTimeStamp()%256);
    
    //------------------------------------------------------------  
    //--Overwriting process:
    //--Look at primitive closer than the ram granulatity respect to the previous one: 
    //--Ram granularity in 2016 DataTaking: 12.5 ns; 
    //--How did the primitive ID change from the old primitive to the one?

    IsOverWritten=0;
    if(fast==0) {
      PrimitiveAddress=L0address(Primitive->GetTimeStamp(),Primitive->GetFineTime(),2);
      OldPrimitiveAddress=L0address(oldPrimitive->GetTimeStamp(),oldPrimitive->GetFineTime(),2);
      
      if(OldPrimitiveAddress==PrimitiveAddress  && Primitive->GetTimeStamp()>=oldPrimitive->GetTimeStamp()) {
	
	h_DTCloser->Fill(Primitive->GetTime()-oldPrimitive->GetTime());
	
	if(Detector=="LKr") {	
	  WasLKrBit10   = oldPrimitive->GetPrimitiveID()&MaskLKr2GeV;
	  if(IsLKrBit10!=0 && WasLKrBit10==0) NewLKrBit10++;
	  if(WasLKrBit10!=0 && IsLKrBit10==0) LostLKrBit10++;	
	}
		
	if(Detector=="IRC") {
	  
	  WasQx   = oldPrimitive->GetPrimitiveID()&MaskQx;
	  WasUTMC = oldPrimitive->GetPrimitiveID()&MaskUTMC;	
	  WasQ2   = oldPrimitive->GetPrimitiveID()&MaskQ2;

	  if(IsQ2!=WasQ2) Q2Diff++; //All Changing (in both direction)
	  if(IsQx!=WasQx) QxDiff++; //All Changing (in both direction)
	  if(IsUTMC!=WasUTMC) UTMCDiff++; //All Changing (in both direction)
	  if(IsQx!=0 && WasQx==0) NewQx++;
	  if(IsQ2!=0 && WasQ2==0) NewQ2++;
	  if(IsQx==0 && WasQx!=0) LostQx++;
	  if(IsUTMC!=0 && WasUTMC==0) NewUTMC++;
	  if(IsUTMC==0 && WasUTMC!=0) LostUTMC++;      
	}
      
	if(Detector=="MUV3") {
	  WasMO1   = oldPrimitive->GetPrimitiveID()&MaskMO1;
	  WasMO2   = oldPrimitive->GetPrimitiveID()&MaskMO2;
	
	  if(IsMO1!=WasMO1) MO1Diff++; //All Changing (in both direction)
	  if(IsMO2!=WasMO2) MO2Diff++; //All Changing (in both direction)
	  if(IsMO1!=0 && WasMO1==0) NewMO1++;
	  if(IsMO1==0 && WasMO1!=0) LostMO1++;
	  if(IsMO2!=0 && WasMO2==0) NewMO2++;
	  if(IsMO2==0 && WasMO2!=0) LostMO2++;
   
	}
	OverWrittenPrimitive++;
	IsOverWritten=1;
      }//same ts
    }//fast
    //  if(IsOverWritten==0){
    oldPrimitive->SetTimeStamp(Primitive->GetTimeStamp());
    oldPrimitive->SetFineTime(Primitive->GetFineTime());
    oldPrimitive->SetSendTimeStamp(Primitive->GetSendTimeStamp());
    oldPrimitive->SetPrimitiveID(Primitive->GetPrimitiveID());
    //  }
  }    
  
    cout<<"Number of primitives with timestamp equal to previous: "<<SameTS<<" ("<<(Double_t)SameTS/(Double_t)tree->GetEntries()<<")"<<endl;
    
    cout<<"Number of repeated primitives: "<<RepeatedPrimitive<<" ("<<(Double_t)RepeatedPrimitive/(Double_t)tree->GetEntries()<<")"<<endl;
    cout<<"Number of overwritten primitives "<<OverWrittenPrimitive<<"("<<(Double_t)OverWrittenPrimitive/(Double_t)tree->GetEntries()<<")"<<endl;
  
    TFile file(OutputFile,"recreate");
    if(Detector=="IRC") {
      cout<<"Entries "<<tree->GetEntries()<<endl;
      cout<<"Type "<<"Total Entries "<<"Ratio "<<endl;
      cout<<"  Qx     "<<NQx<<"     "<<  (double)NQx/tree->GetEntries()<<endl;
      cout<<"  Q1     "<<NQ1<<"     "<<  (double)NQ1/tree->GetEntries()<<endl;
      cout<<"  Q2     "<<NQ2<<"     "<<  (double)NQ2/tree->GetEntries()<<endl;
      cout<<"  UTMC   "<<NUTMC<<"   "<<(double)NUTMC/tree->GetEntries()<<endl;
    
      cout<<"Modification due to the overwriting process:"<<endl;
      cout<<"Q2Diff   "<<Q2Diff<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)Q2Diff/tree->GetEntries()<<")"<<endl;
      cout<<"QxDiff   "<<QxDiff<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)QxDiff/tree->GetEntries()<<")"<<endl;
      cout<<"UTMCDiff "<<UTMCDiff<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)UTMCDiff/tree->GetEntries()<<")"<<endl<<endl;
      cout<<"NewQ2    "<<NewQ2<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)NewQ2/tree->GetEntries()<<")"<<endl;
      cout<<"NewQx    "<<NewQx<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)NewQx/tree->GetEntries()<<")"<<endl;
      cout<<"NewUTMC  "<<NewUTMC<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)NewUTMC/tree->GetEntries()<<")"<<endl;
      cout<<"LostQx   "<<LostQx<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)LostQx/tree->GetEntries()<<")"<<endl;
      cout<<"LostUTMC "<<LostUTMC<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)LostUTMC/tree->GetEntries()<<")"<<endl;
      cout<<"LostQ2   "<<LostQ2<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)LostQ2/tree->GetEntries()<<")"<<endl;

    }


    if(Detector=="LKr") {
      cout<<"Entries "<<tree->GetEntries()<<endl;
      cout<<"Type "<<"Total Entries "<<"Ratio "<<endl;
      cout<<"  LKrBit10    "<<NLKrBit10<<"   "<<  (double)NLKrBit10/tree->GetEntries()<<endl;
      cout<<"  LKr10GeV    "<<NLKr10GeV<<"   "<<  (double)NLKr10GeV/tree->GetEntries()<<endl;
      cout<<"  LKr20GeV    "<<NLKr20GeV<<"   "<<  (double)NLKr20GeV/tree->GetEntries()<<endl;
      cout<<"  LKrPiNuNu   "<<NLKrPiNuNu<<"   "<<  (double)NLKrPiNuNu/tree->GetEntries()<<endl;

      cout<<"Modification due to the overwriting process:"<<endl;
      cout<<"NewLKrBit10    "<<NewLKrBit10<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)NewLKrBit10/tree->GetEntries()<<")"<<endl;
      cout<<"LostLKrBit10   "<<LostLKrBit10<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)LostLKrBit10/tree->GetEntries()<<")"<<endl;

    }



  
    if(Detector=="MUV3") {
      cout<<"Entries "<<tree->GetEntries()<<endl;
      cout<<"Type "<<"Total Entries "<<"Ratio "<<endl;
      cout<<"MUV   "<<NMUV<<" "<<(double)NMUV/tree->GetEntries()<<endl;
      cout<<"MO1   "<<NMO1<<" "<<(double)NMO1/tree->GetEntries()<<endl;
      cout<<"MO2   "<<NMO2<<" "<<(double)NMO2/tree->GetEntries()<<endl;
      cout<<"M1    "<<NM1<<" "<<(double)NM1/tree->GetEntries()<<endl;
      cout<<"M2    "<<NM2<<" "<<(double)NM2/tree->GetEntries()<<endl;
      cout<<"MOQx  "<<NMOQx<<" "<<(double)NMOQx/tree->GetEntries()<<endl;
   
      cout<<"Modification due to the overwriting process:"<<endl;
      cout<<"MO1Diff  "<<MO1Diff<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)MO1Diff/tree->GetEntries()<<")"<<endl;
      cout<<"MO2Diff  "<<MO2Diff<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)MO2Diff/tree->GetEntries()<<")"<<endl<<endl;
    
      cout<<"NewMO1   "<<NewMO1<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)NewMO1/tree->GetEntries()<<")"<<endl;
      cout<<"NewMO2   "<<NewMO2<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)NewMO2/tree->GetEntries()<<")"<<endl;
      cout<<"LostMO1  "<<LostMO1<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)LostMO1/tree->GetEntries()<<")"<<endl;
      cout<<"LostMO2  "<<LostMO2<<" over "<<tree->GetEntries()<<" entries "<<"("<<(double)LostMO2/tree->GetEntries()<<")"<<endl;
  
    }

    //FFT
    if(fast==0) {
      TH1 *hm =0;
      TVirtualFFT::SetTransform(0);
      hm = h_timestamp3_4sec->FFT(hm, "MAG");
      hm -> SetTitle("FFT");
      hm ->GetXaxis() -> SetRangeUser(1,350);
      hm ->GetXaxis() -> SetTitle("Hz");
      hm ->GetYaxis() -> SetRangeUser(-1000,200000);
      hm ->Write();

    }

    h_LKr2GeVRate->GetXaxis()->SetTitle("Rate [kHz]");

    DTRandom -> Write();
    h_rawtimestamp->Write();
    h_foldedTS           -> Write();
    h_foldedTS_Qx        -> Write();
    h_foldedTS_MO1       -> Write();
    h_foldedTS_MO2       -> Write();
    h_foldedTS_DS        -> Write();
    h_primitiveinpacket   -> Write();
    h_primitiveID         -> Write();
    h_timestamp1_2sec     -> Write();
    h_timestamp2_3sec     -> Write();
    h_timestamp3_4sec     -> Write();
    h_timestamp4_5sec     -> Write();
    h_LKr2GeVRate         -> Write();
    h_DTCloser            -> Write();
    h_DTTimeStamp         -> Write();
    h_rate                -> Write();
    h_DT                  -> Write();
    h_LKrDT               -> Write();
    h_DTTimeStampInPacket -> Write();
    h_DTTimeStampDS       ->Write();
    cout <<"finished"<<endl;

    // Eff spill
    Double_t Ndt = fHPrimTimeForEffSpill->Integral();
    TH1D* hist1 = (TH1D*)fHPrimTimeForEffSpill->Clone();
    fHPrimTimeForEffSpill->Multiply(hist1);
    Double_t N2dt = fHPrimTimeForEffSpill->Integral();
    std::cout.precision(5);
    Double_t effspill = (Double_t)(Ndt*Ndt/N2dt)*1e5/1e9; // *binwidth/units
    cout<<"eff spill: "<<effspill<<endl;
    file.Close();
    return 1;
  }


  void shiftefficiency(TString data){
    Int_t Granularity=2;
    TFile *f = new TFile(data);
    if(!f){
      std::cout << " file not found" << std::endl ;
      return; 
    }

     
    TPrimitive *CHODPrimitive= new TPrimitive();
    TPrimitive *MUV3Primitive= new TPrimitive();
    TPrimitive *oldCHODPrimitive = new TPrimitive();
    TPrimitive *oldMUV3Primitive = new TPrimitive();
  
    TTree *treeCHOD = (TTree*)f->Get("RICH");
    TTree *treeMUV = (TTree*)f->Get("MUV3");
    cout<<"trees opened"<<endl;

    //Branches
    TBranch *bCHOD;
    TBranch *bMUV;
    cout<<"branches:"<<endl;
    bCHOD=treeCHOD->GetBranch("fPrimitive");
    bMUV= treeMUV ->GetBranch("fPrimitive");
     
    bCHOD->SetAddress(&CHODPrimitive);
    bMUV ->SetAddress(&MUV3Primitive);
    cout<<"branches done "<<endl;
    int goodevent=0;
    int oldgoodevent=0;
    long long addressCHOD=0;
    long long addressMUV=0;
    long long oldaddressCHOD=0;
    long long oldaddressMUV=0;
    int isoverwritten=0;
    int countoverwritten=0;
    for (Int_t iEntry=0; iEntry<treeCHOD->GetEntries()/2; iEntry++) {
      if(iEntry%1000000==0) cout<<iEntry<<" over "<<treeCHOD->GetEntries()<<endl;
      treeCHOD->GetEntry(iEntry);
      if(CHODPrimitive->GetTimeStamp()<8000000) continue;
      if(iEntry%1000==0 && iEntry!=0){
	cout<<iEntry<<endl;
	cout<<"good events     "<<goodevent<<" "<<endl;
	cout<<"old good event  "<<oldgoodevent<<" "<<endl;
	cout<<"is overwtitten  "<<isoverwritten<<" "<<endl;
      }
      addressCHOD=L0address((unsigned int)CHODPrimitive->GetTimeStamp(), (unsigned int)CHODPrimitive->GetFineTime(), Granularity);

      for (Int_t iEntry2=0; iEntry2<treeMUV->GetEntries()/2; iEntry2++) {
	isoverwritten=0;
	treeMUV->GetEntry(iEntry2);
	if(MUV3Primitive->GetTimeStamp()<8000000) continue;
	if(MUV3Primitive->GetTimeStamp()-CHODPrimitive->GetTimeStamp()>25) break;
	addressMUV=L0address((unsigned int)MUV3Primitive->GetTimeStamp(), (unsigned int)MUV3Primitive->GetFineTime(),   Granularity);
	if(addressMUV==oldaddressMUV){
	  isoverwritten++;
	}
      
	if(addressMUV==addressCHOD && oldaddressMUV==addressCHOD){
	  if(TMath::Abs(CHODPrimitive->GetTime()-MUV3Primitive->GetTime())<100) goodevent++;
	  if(TMath::Abs(CHODPrimitive->GetTime()-oldMUV3Primitive->GetTime())<100) oldgoodevent++;
	  cout<<"****old*******"<<endl;
	  MUV3Primitive->Print();
	  cout<<"current"<<endl;
	  oldMUV3Primitive->Print();
	}
    
	if(isoverwritten==0)oldaddressMUV=addressMUV;
	if(isoverwritten==0)oldMUV3Primitive=MUV3Primitive;
      }
    }
    cout<<"good events     "<<goodevent<<endl;
    cout<<"old good event  "<<oldgoodevent<<endl;
  
  }






  void Coincidences(TString Data,TString OutputFile){
    TString DetectorIRC="IRC";
    TString DetectorLKr="LKr";
  
    TFile *f = new TFile(Data);
    if(!f){
      std::cout << " file not found" << std::endl ;
      return -1; 
    }
    //Masks for PrimitiveID selection.

    Int_t Granularity=1;

    Int_t MaskQx  = 0x800;
    Int_t MaskUTMC= 0x1000;
    Int_t MaskQ1  = 0x4000;
    Int_t MaskQ2  = 0x400;

    Int_t MaskLKr2GeV  = 0x400;

    Int_t IsOverWritten = 0;
    Int_t IsRepeated    = 0;

    Int_t NUTMC=0;
    Int_t NQ1  =0;
    Int_t NQ2  =0;
    Int_t NQx  =0;
 
    Int_t NMO1=0;
    Int_t NMO2=0;
 
    Int_t IsMO1         = 0;
    Int_t IsMO2         = 0;
    Int_t WasMO1        = 0;
    Int_t WasMO2        = 0;
    Int_t MO2Diff       = 0;
    Int_t MO1Diff       = 0;

    Int_t IsQ1          = 0;
    Int_t IsQ2          = 0;
    Int_t IsQx          = 0;
    Int_t IsUTMC        = 0;
    Int_t WasQx         = 0;
    Int_t WasUTMC       = 0;
    Int_t QxDiff        = 0;
    Int_t UTMCDiff      = 0;

  
  
    //TPRIMITIVE
    TPrimitive *PrimitiveLKr= new TPrimitive();
    TPrimitive *oldPrimitiveLKr = new TPrimitive();

    TPrimitive *PrimitiveIRC= new TPrimitive();
    TPrimitive *oldPrimitiveIRC = new TPrimitive();
  
  
    TTree *treeLKr = (TTree*)f->Get(DetectorLKr.Data());
    if(!treeLKr) cout<<"tree of "<<DetectorLKr.Data()<<" not found"<<endl;
    TTree *treeIRC = (TTree*)f->Get(DetectorIRC.Data());
    if(!treeIRC) cout<<"tree of "<<DetectorIRC.Data()<<" not found"<<endl;

    cout<<DetectorLKr.Data()<<" tree opened"<<endl;
    cout<<DetectorIRC.Data()<<" tree opened"<<endl;
  
  
    //Branches
    TBranch *bLKr;
    TBranch *bIRC;
    cout<<"branches:"<<endl;
    bLKr=treeLKr->GetBranch("fPrimitive");
    bLKr->SetAddress(&PrimitiveLKr);
    bIRC=treeIRC->GetBranch("fPrimitive");
    bIRC->SetAddress(&PrimitiveIRC);
    cout<<"branches set "<<endl;

    cout<<"Itializing variables:"<<endl;

  
    oldPrimitiveIRC ->SetTimeStamp(0);
    oldPrimitiveIRC ->SetFineTime(0);
    oldPrimitiveIRC ->SetSendTimeStamp(0);
    oldPrimitiveIRC ->SetPrimitiveID(0);

    oldPrimitiveLKr ->SetTimeStamp(0);
    oldPrimitiveLKr ->SetFineTime(0);
    oldPrimitiveLKr ->SetSendTimeStamp(0);
    oldPrimitiveLKr ->SetPrimitiveID(0);


  
    cout<<"entries from LKr "<<DetectorLKr.Data()<<": "<<bLKr->GetEntries()<<endl;       
    cout<<"entries from IRC "<<DetectorIRC.Data()<<": "<<bIRC->GetEntries()<<endl;       
 

  
  
  
    vector<unsigned int>  ftIRC_intV;
    vector<unsigned int>  tstmpIRC_intV;
    vector<unsigned int>  ftLKr_intV;
    vector<unsigned int>  tstmpLKr_intV;
    vector<unsigned int>  trigger;
    vector<unsigned int>  triggerword;
  
    vector<unsigned int>  primIdLKr_intV;
    vector<unsigned int>  primIdIRC_intV;
    int i=0;

    unsigned int  ftLKr_int;
    unsigned int  tstmpLKr_int;
    unsigned int  old_tstmpLKr_int=0;
    unsigned int IRCaddress=0;
    unsigned int previousAddressIRC=0;

    unsigned int  ftIRC_int;
    unsigned int  tstmpIRC_int;

    unsigned int LKraddress=0;
    unsigned int oldLKraddress=0;
    int oldLKraddresscount=0;
    unsigned int bitLKraddress=0;

    unsigned int coincidence=0;
    unsigned int coincidence_no_old=0;

    int sametimestamp=0;
    int j1=0;
  
    unsigned int previousAddressLKr=0;
    long long AddressLKr=0;
    long long AddressIRC=0;
    unsigned int previousIRCTimeStamp=0;
    unsigned int previousTS=0;
  
    for (Long64_t iEntryLKr=0; iEntryLKr<treeLKr->GetEntries(); ++iEntryLKr) {
      treeLKr->GetEntry(iEntryLKr);
      long long TimeLKr=(long long)PrimitiveLKr->GetTimeStamp()*0x100 + (int)PrimitiveLKr-> GetFineTime();    
      AddressLKr=L0address((unsigned int)PrimitiveLKr->GetTimeStamp(), (unsigned int)PrimitiveLKr->GetFineTime(),   Granularity);
    
      if(AddressLKr==previousAddressLKr) {
	sametimestamp++;
	continue;
      }   
      previousAddressLKr=AddressLKr;

      //MASK SELECTION!!!!*******************//
      if(((unsigned int)PrimitiveLKr->GetPrimitiveID()&0x400)==0x400){ // only bit 10
	ftLKr_intV.push_back((unsigned int)PrimitiveLKr-> GetFineTime());
	tstmpLKr_intV.push_back((unsigned int)PrimitiveLKr->GetTimeStamp());
	primIdLKr_intV.push_back((unsigned int)PrimitiveLKr->GetPrimitiveID());
      }
    
    }//end lkr

    cout<<"sametimestamp LKr "<<sametimestamp<<endl;
    sametimestamp=0;
    Int_t Downscaling=2;
    Int_t sent=0;
    Int_t sent2=0;
  
    for (Long64_t iEntryIRC=0; iEntryIRC<treeIRC->GetEntries(); ++iEntryIRC) {
      treeIRC->GetEntry(iEntryIRC);
      long long TimeIRC=(long long)PrimitiveIRC->GetTimeStamp()*0x100 + (int)PrimitiveIRC-> GetFineTime();
      AddressIRC=L0address((unsigned int)PrimitiveIRC->GetTimeStamp(), (unsigned int)PrimitiveIRC->GetFineTime(),   Granularity);

      if((long long)PrimitiveIRC->GetTimeStamp()==41364711){
	cout<<"Timestamp "<<dec<<PrimitiveIRC->GetTimeStamp()<<endl;
	cout<<"FineTime "<<dec<<PrimitiveIRC->GetFineTime()<<endl;
	cout<<"Address "<<dec<<AddressIRC<<endl;
	cout<<"previousIRCTimeStamp "<<dec<<previousIRCTimeStamp<<endl;
	cout<<"Downscaling "<<dec<<Downscaling<<endl;
	if(PrimitiveIRC->GetFineTime()==166) sent2=1;
     
      }
    
      if((long long)PrimitiveIRC->GetTimeStamp()==48386303){
	if(PrimitiveIRC->GetFineTime()==10) sent2=1;
      }

      if((long long)PrimitiveIRC->GetTimeStamp()==48782773){
	if(PrimitiveIRC->GetFineTime()==150) sent2=1;
      }

     
      if((long long)PrimitiveIRC->GetTimeStamp()==48826187){
	sent2=1;
      }

      if((long long)PrimitiveIRC->GetTimeStamp()==49249716){
	sent2=1;
      }

       
      if(AddressIRC==previousAddressIRC) {
	sametimestamp++;
	continue;
      }   
      previousAddressIRC=AddressIRC;

      if(PrimitiveIRC->GetTimeStamp()==previousIRCTimeStamp){
	sent=1; 
      }



    
      if(PrimitiveIRC->GetPrimitiveID()!=0){ //Q1
      
	if(Downscaling==4){
	  if(sent==0 && sent2==0)Downscaling=0;	
	  ftIRC_intV.push_back((unsigned int)PrimitiveIRC-> GetFineTime());
	  tstmpIRC_intV.push_back((unsigned int)PrimitiveIRC->GetTimeStamp());
	  primIdIRC_intV.push_back((unsigned int)PrimitiveIRC->GetPrimitiveID());
	  previousIRCTimeStamp=(unsigned int)PrimitiveIRC->GetTimeStamp();	    
	}
      
	else {//Increase the downscaling but if it is Q2, I send it.
	  if(sent==0 && sent2==0)Downscaling++;
	  if((PrimitiveIRC->GetPrimitiveID()&0x400)==0x400){//Q2
	    ftIRC_intV.push_back((unsigned int)PrimitiveIRC-> GetFineTime());
	    tstmpIRC_intV.push_back((unsigned int)PrimitiveIRC->GetTimeStamp());
	    primIdIRC_intV.push_back((unsigned int)PrimitiveIRC->GetPrimitiveID());
	    previousIRCTimeStamp=(unsigned int)PrimitiveIRC->GetTimeStamp();

	  }//Q2
	} //Downscaling
      }//Q1
      sent=0;
      sent2=0;
    }//end irc

    cout<<"sametimestamp IRC "<<sametimestamp<<endl;

    cout<<"Ho caricato i vettori: "<<endl;
    cout<<"timestamp IRC vector size: "<<dec<<tstmpIRC_intV.size()<<endl;
    cout<<"timestamp LKr vector size: "<<dec<<tstmpLKr_intV.size()<<endl;

  
    for(int i=0; i< tstmpIRC_intV.size();++i){
    
      if(i%5000==0)cout<<"primitive number "<<i<<endl;

      // time stamp 11 less significative bit (LSB) + fine time (ft) 1 more significative bit (MSB)      
      IRCaddress = L0address(tstmpIRC_intV.at(i),ftIRC_intV.at(i),1);
    

      //  ********************************************++    

      trigger.push_back(tstmpIRC_intV.at(i));
      triggerword.push_back(1);
    
      for(int j=0; j< tstmpLKr_intV.size();++j){
     
	//devono avere lo stesso timestamp(+/- 1):
     
	if(abs((int)tstmpLKr_intV.at(j) - (int)tstmpIRC_intV.at(i))>1) continue;

	// time stamp 11 less significative bit (LSB) + fine time (ft) 3 more significative bit (MSB)	  
	LKraddress = L0address(tstmpLKr_intV.at(j),ftLKr_intV.at(j),1);
    
	if (LKraddress==oldLKraddress) {
	  oldLKraddresscount++;
	  continue;}
          
	bitLKraddress = ftLKr_intV.at(j) & ((unsigned int) 1 << (unsigned int) 2);

	//**************COINCIDENCES:**********************************************************//		 
	if(IRCaddress==LKraddress || (bitLKraddress!=0 && LKraddress+1==IRCaddress) || (bitLKraddress==0 && LKraddress-1==IRCaddress)) {
	  coincidence_no_old++;
	  cout<<"Coincidence LKr x IRC: "<< tstmpIRC_intV.at(i)<<endl;
	  int old_tstmpIRC_int = tstmpIRC_intV.at(i);
	
	  oldLKraddress=LKraddress;
	
	  trigger.push_back(tstmpIRC_intV.at(i)); //COINCIDENCES
	  triggerword.push_back(11);

	  break;
	}
	else{
	  trigger.push_back(tstmpLKr_intV.at(j));
	  triggerword.push_back(10);
	  break;
	}
      } //end of IRC while
    }

    sort(trigger.begin(), trigger.end(),sortfunction);

    int oldtrigger=0;
    int deleted=0;
    int LKrTrigger=0;
    int LKrTriggerPure=0;

    cout<<"trigger.size() "<<trigger.size()<<endl;

    for (int i=0; i<trigger.size();i++){
      if(triggerword.at(i)==10 ||triggerword.at(i)==11) LKrTrigger++;
      if(triggerword.at(i)==11) LKrTriggerPure++;

  
      if((int)trigger.at(i)-oldtrigger <= 3) {
 
	deleted=trigger.at(i);
	trigger.erase(trigger.begin()+(i));
	i=i-1;
	continue;
      
      }
    
      if(trigger.at(i)-oldtrigger > 3) oldtrigger=trigger.at(i); //Dead Time
    }

    ofstream myfile;
    myfile.open ("Coincidences.txt");
    cout<<"coincidences between IRC and LKr before dead time "<<LKrTriggerPure<<endl;
    cout<<"All LKr Triggers "<<LKrTrigger<<endl;
  
  
    for(int i=0;i<trigger.size();i++){
      coincidence++;
      //    cout<<"trigger      "<<dec<<trigger.at(i)<<" coincidence "<<coincidence<<endl<<endl;
      myfile <<"trigger    "<<trigger.at(i)<<" coincidence "<<coincidence<<"\n";

    
    }
    myfile.close();

  }




  void ControlCheck(TString Data,TString OutputFile) {
    TString DetectorCHOD="CHOD";
  
    TFile *f = new TFile(Data);
    if(!f){
      std::cout << " file not found" << std::endl ;
      return -1; 
    }

    Int_t Granularity=2;

    //TPRIMITIVE
    TPrimitive *Primitive= new TPrimitive();  
    TTree *treeCHOD = (TTree*)f->Get(DetectorCHOD.Data());
    if(!treeCHOD) cout<<"tree of "<<DetectorCHOD.Data()<<" not found"<<endl;

    cout<<DetectorCHOD.Data()<<" tree opened"<<endl;
  
  
    //Branches
    TBranch *bCHOD;
    bCHOD=treeCHOD->GetBranch("fPrimitive");
    bCHOD->SetAddress(&Primitive);
    cout<<"branches set "<<endl;

    cout<<"Itializing variables:"<<endl;
    cout<<"entries from "<<DetectorCHOD.Data()<<": "<<bCHOD->GetEntries()<<endl;       
 
    unsigned int previousCHODTimeStamp=0;

    int sametimestamp=0;
  
    unsigned int previousAddressCHOD=0;

    long long AddressCHOD=0;

    Int_t Downscaling=0;

  
    unsigned int veryprevious=0;
    int i=0;
    TH1F *TotalRate = new TH1F("TotalRate","",500,0,100000);
    TH2I * DTTimeStampVSTimeStampControDetector= new TH2I("DTTimeStampVSTimeStampControDetector","DT Timestamp Vs Timestamp of Control Detector;x 25 ns",1000,0,256e6,500,0,50000);

    int TotalEntries=treeCHOD->GetEntries();
    for (Long64_t iEntryCHOD=0; iEntryCHOD<TotalEntries; ++iEntryCHOD) {
      if(iEntryCHOD%500000==0)cout<<"primitive number "<<1.*iEntryCHOD/TotalEntries<<endl;

      treeCHOD->GetEntry(iEntryCHOD);
      AddressCHOD=L0address((unsigned int)Primitive->GetTimeStamp(), (unsigned int)Primitive->GetFineTime(),   Granularity);
  
      if(AddressCHOD==previousAddressCHOD) {
	sametimestamp++;
	continue;
      }   

      previousAddressCHOD=AddressCHOD;

      if(Downscaling==400){
	Downscaling=0;
	if(Primitive->GetTimeStamp() == veryprevious) continue;
	TotalRate->Fill(Primitive->GetTimeStamp() - previousCHODTimeStamp);
	DTTimeStampVSTimeStampControDetector->Fill(Primitive->GetTimeStamp(),Primitive->GetTimeStamp() - previousCHODTimeStamp);
	previousCHODTimeStamp=(unsigned int)Primitive->GetTimeStamp();	    
      }
      else{
	Downscaling++;
      }
      veryprevious=Primitive->GetTimeStamp();
  
 
    }//end CHOD

    cout<<"same address CHOD "<<sametimestamp<<endl;
    sametimestamp=0;
    TFile * myfile = new TFile("ControlDownscalingCheck.root","RECREATE");
    TotalRate->Write();
    DTTimeStampVSTimeStampControDetector->Write();
  }







  //PACKET GENERATION FOR MODELSIM:
  //Generate .bin File for each ethernet packet to be analysed by L0TP ModelSim simulation
  unsigned int tohex(string line)
  {
    unsigned int intline=0;
    stringstream converter(line.c_str());
    converter >> std::hex >> intline;

    return intline;
  }





  void CheckFoldedTS(TString Data,TString OutputFile){
    TString Detector2="IRC";
    TString DetectorRef="RICH";
  
    TFile *f = new TFile(Data);
    if(!f){
      std::cout << " file not found" << std::endl ;
      return -1; 
    }
    //Masks for PrimitiveID selection.

    Int_t Granularity=2;

    Int_t MaskQx  = 0x800;
    Int_t MaskUTMC= 0x1000;
    Int_t MaskQ1  = 0x4000;
    Int_t MaskQ2  = 0x400;


    Int_t IsOverWritten = 0;
    Int_t IsRepeated    = 0;

 
  
  
    //TPRIMITIVE
    TPrimitive *PrimitiveRef= new TPrimitive();
    TPrimitive *oldPrimitiveRef = new TPrimitive();

    TPrimitive *Primitive2= new TPrimitive();
    TPrimitive *oldPrimitive2 = new TPrimitive();
  
  
    TTree *treeRef = (TTree*)f->Get(DetectorRef.Data());
    if(!treeRef) cout<<"tree of "<<DetectorRef.Data()<<" not found"<<endl;
    TTree *tree2 = (TTree*)f->Get(Detector2.Data());
    if(!tree2) cout<<"tree of "<<Detector2.Data()<<" not found"<<endl;

    cout<<DetectorRef.Data()<<" tree opened"<<endl;
    cout<<Detector2.Data()<<" tree opened"<<endl;
  
  
    //Branches
    TBranch *bRef;
    TBranch *b2;
    cout<<"branches:"<<endl;
    bRef=treeRef->GetBranch("fPrimitive");
    bRef->SetAddress(&PrimitiveRef);
    b2=tree2->GetBranch("fPrimitive");
    b2->SetAddress(&Primitive2);
    cout<<"branches set "<<endl;

    cout<<"Itializing variables:"<<endl;

  
    oldPrimitive2 ->SetTimeStamp(0);
    oldPrimitive2 ->SetFineTime(0);
    oldPrimitive2 ->SetSendTimeStamp(0);
    oldPrimitive2 ->SetPrimitiveID(0);

    oldPrimitiveRef ->SetTimeStamp(0);
    oldPrimitiveRef ->SetFineTime(0);
    oldPrimitiveRef ->SetSendTimeStamp(0);
    oldPrimitiveRef ->SetPrimitiveID(0);


  
    cout<<"entries from Ref "<<DetectorRef.Data()<<": "<<bRef->GetEntries()<<endl;       
    cout<<"entries from 2 "<<Detector2.Data()<<": "<<b2->GetEntries()<<endl;       
 

  
  
  
    vector<unsigned int>  ft2_intV;
    vector<unsigned int>  tstmp2_intV;
    vector<unsigned int>  ftRef_intV;
    vector<unsigned int>  tstmpRef_intV;
    vector<unsigned int>  trigger;
    vector<unsigned int>  triggerword;
  
    vector<unsigned int>  primIdRef_intV;
    vector<unsigned int>  primId2_intV;
    int i=0;

    unsigned int  ftRef_int;
    unsigned int  tstmpRef_int;
    unsigned int  old_tstmpRef_int=0;
    unsigned int previousAddress2=0;

    unsigned int  ft2_int;
    unsigned int  tstmp2_int;

    unsigned int oldaddressRef=0;
    int oldaddressRefcount=0;
    unsigned int bitaddressRef=0;

    unsigned int coincidence=0;
    unsigned int coincidence_no_old=0;

    int sametimestamp=0;
    int j1=0;
  
    unsigned int previousAddressRef=0;
    long long AddressRef=0;
    long long Address2=0;
    unsigned int previous2TimeStamp=0;
    unsigned int previousTS=0;
    TH1F * FoldedTS = new TH1F("foldedTS","foldedTS",256,-0.5,255.5);
    for (Long64_t iEntryRef=0; iEntryRef<treeRef->GetEntries(); ++iEntryRef) {
      if(iEntryRef>500000) break;
      treeRef->GetEntry(iEntryRef);
      long long TimeRef=(long long)PrimitiveRef->GetTimeStamp()*0x100 + (int)PrimitiveRef-> GetFineTime();    
      AddressRef=L0address((unsigned int)PrimitiveRef->GetTimeStamp(), (unsigned int)PrimitiveRef->GetFineTime(),   Granularity);
    
      if(AddressRef==previousAddressRef) {
	sametimestamp++;
	continue;
      }   
      previousAddressRef=AddressRef;

      //MASK SELECTION!!!!*******************//
      ftRef_intV.push_back((unsigned int)PrimitiveRef-> GetFineTime());
      tstmpRef_intV.push_back((unsigned int)PrimitiveRef->GetTimeStamp());
      primIdRef_intV.push_back((unsigned int)PrimitiveRef->GetPrimitiveID());
    
    }//end ref

    cout<<"sametimestamp Ref "<<sametimestamp<<endl;
    sametimestamp=0;
    Int_t Downscaling=0;
    Int_t sent=0;
    Int_t sent2=0;
    previousAddress2=0;
    previousAddressRef=0;
    for (Long64_t iEntry2=0; iEntry2<tree2->GetEntries(); ++iEntry2) {
      if(iEntry2>500000) break;
      tree2->GetEntry(iEntry2);
      if((Primitive2->GetPrimitiveID()&0x800)!=0x800) continue;
      long long Time2=(long long)Primitive2->GetTimeStamp()*0x100 + (int)Primitive2-> GetFineTime();
      Address2=L0address((unsigned int)Primitive2->GetTimeStamp(), (unsigned int)Primitive2->GetFineTime(),   Granularity);

      /*
	if((long long)Primitive2->GetTimeStamp()==41364711){
	cout<<"Timestamp "<<dec<<Primitive2->GetTimeStamp()<<endl;
	cout<<"FineTime "<<dec<<Primitive2->GetFineTime()<<endl;
	cout<<"Address "<<dec<<Address2<<endl;
	cout<<"previous2TimeStamp "<<dec<<previous2TimeStamp<<endl;
	cout<<"Downscaling "<<dec<<Downscaling<<endl;
	if(Primitive2->GetFineTime()==166) sent2=1;
     
	}
      */
       
      if(Address2==previousAddress2) {
	sametimestamp++;
	continue;
      }   
      previousAddress2=Address2;

    
      ft2_intV.push_back((unsigned int)Primitive2-> GetFineTime());
      tstmp2_intV.push_back((unsigned int)Primitive2->GetTimeStamp());
      primId2_intV.push_back((unsigned int)Primitive2->GetPrimitiveID());
      previous2TimeStamp=(unsigned int)Primitive2->GetTimeStamp();	    
     
     
    }//end irc

    cout<<"sametimestamp 2 "<<sametimestamp<<endl;

    cout<<"Ho caricato i vettori: "<<endl;
    cout<<"timestamp 2 vector size: "<<dec<<tstmp2_intV.size()<<endl;
    cout<<"timestamp Ref vector size: "<<dec<<tstmpRef_intV.size()<<endl;

  
    for(int i=0; i< tstmp2_intV.size();++i) {
    
      if(i%5000==0)cout<<"primitive number "<<i<<endl;

      // time stamp 11 less significative bit (LSB) + fine time (ft) 1 more significative bit (MSB)      
      Address2 = L0address(tstmp2_intV.at(i),ft2_intV.at(i),2);
    
   
      for(int j=0; j< tstmpRef_intV.size();++j) {
     
	//devono avere lo stesso timestamp(+/- 1):
     
	if(abs((int)tstmpRef_intV.at(j) - (int)tstmp2_intV.at(i))>1) continue;

	// time stamp 11 less significative bit (LSB) + fine time (ft) 3 more significative bit (MSB)	  
	AddressRef = L0address(tstmpRef_intV.at(j),ftRef_intV.at(j),2);
    
	if (AddressRef==oldaddressRef) {
	  oldaddressRefcount++;
	  continue;}
          
	bitaddressRef = ftRef_intV.at(j) & ((unsigned int) 1 << 5);

	//**************COINCIDENCES:**********************************************************//		 
	if(Address2==AddressRef || (bitaddressRef!=0 && AddressRef+1==Address2) || (bitaddressRef==0 && AddressRef-1==Address2)) {
	
	  coincidence_no_old++;
	  cout<<"Conicidence: Timestamp Ref: "<<tstmpRef_intV.at(j)<<" Timestamp 2: "<<tstmp2_intV.at(i)<<" Finetime Ref "<<ftRef_intV.at(j)<<"Finetime 2: "<<ft2_intV.at(i) <<" L0address Ref: "<<AddressRef<<" L0address2 "<<Address2<<endl;
	  int old_tstmp2_int = tstmp2_intV.at(i);
	
	  oldaddressRef=AddressRef;
	
	  trigger.push_back(tstmp2_intV.at(i)); //COINCIDENCES
	  break;
	}
      } //end of 2 while
    }

    sort(trigger.begin(), trigger.end(),sortfunction);

    int oldtrigger=0;
    int deleted=0;
    int RefTrigger=0;
    int RefTriggerPure=0;

    cout<<"trigger.size() "<<trigger.size()<<endl;

    for (int i=0; i<trigger.size();i++){
   
      if((int)trigger.at(i)-oldtrigger <= 3) {
 
	deleted=trigger.at(i);
	trigger.erase(trigger.begin()+(i));
	i=i-1;
	continue;
      
      }
    
      if(trigger.at(i)-oldtrigger > 3) oldtrigger=trigger.at(i); //Dead Time
    }

    ofstream myfile;
    myfile.open ("Coincidences.txt");
  
  
    for(int i=0;i<trigger.size();i++){
      coincidence++;
      //    cout<<"trigger      "<<dec<<trigger.at(i)<<" coincidence "<<coincidence<<endl<<endl;
      myfile <<"trigger    "<<trigger.at(i)<<" coincidence "<<coincidence<<"\n";
      FoldedTS -> Fill(trigger.at(i)%256);
	
    
    }
    FoldedTS -> Draw();
    myfile.close();

  }
