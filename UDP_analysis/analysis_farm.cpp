#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <iomanip>
#include <stdio.h>
#include "TH1F.h"
#include "TH2I.h"
#include "TFile.h"
#include <TF1.h>
#include "TRandom.h"

using namespace std;

const int MEPNumberDebug=0;

const int bitfinetime=1;

static const int NMASKS = 8;
static const int NDET = 7;


struct L0Masks_t {
  unsigned int Detector;
  unsigned int Reserved0;
  unsigned int Downscaling;
  unsigned int TriggerSent;
  unsigned int TriggerGenerated;
  unsigned int DetectorMask[NMASKS];
  unsigned int DetectorDontCare[NMASKS];
  unsigned int Reserved1;
} L0Masks;


struct MEPHeader_t {
  unsigned int SourceID;
  unsigned int FirstEventNumber;
  unsigned int TotalMEPLength;
  unsigned int NEventsInMEP;
  unsigned int SourceSubID;

} MEPHeader;

struct EventHeader_t {
  unsigned int FirstEventLength;
  unsigned int EventNLSB;
  unsigned int EventFlags;
  Long64_t Timestamp;
} EventHeader;

struct L0Data_t {
  unsigned int RefFinetime;
  unsigned int DataType;
  unsigned int Primitive[NDET];
  unsigned int PreviousTimestamp;
  unsigned int TriggerType;
  unsigned int PreviousTriggerType;
  unsigned int TriggerFlags;
  unsigned int ReservedFlags;
  Long64_t Finetime[NDET];
  unsigned int Reserved;
  //Adding slot N-1 and N+1
  unsigned int Reserved1;
  unsigned int Primitive1[NDET];
  unsigned int Finetime1[NDET];
  unsigned int Reserved2;
  unsigned int Primitive2[NDET];
  unsigned int Finetime2[NDET];
  unsigned int Reserved3;
  unsigned int Reserved4;  
} L0Data;


struct L0EOB_t {
  L0Masks_t L0Masks[NMASKS];
  unsigned int Reserved0;
  unsigned int DataType;
  unsigned int Latency;
  unsigned int FinetimeBits;
  unsigned int PriRefDetector;
  unsigned int SecRefDetector;
  unsigned int Reserved1;
  unsigned int Reserved2;
  unsigned int Reserved3;
  unsigned int PreviousTimestamp;
  unsigned int TriggerType;
  unsigned int PreviousTriggerType;
  unsigned int TriggerFlags;
  unsigned int NumberOfPrimitives[NDET];
  unsigned int NumberOfCHOKE;
  unsigned int NumberOfError;
  unsigned int NumberOfRandom;
  unsigned int NumberOfCalibration;
  unsigned int Reserved4;
} L0EOB;


void DebugMEPHeader(MEPHeader_t  MEPHeader);
void DebugEventHeader(EventHeader_t  EventHeader);
void DebugL0Data(L0Data_t  L0Data);
void DebugL0EOB(L0EOB_t  L0EOB);
void WriteL0EOB(L0EOB_t  L0EOB,EventHeader_t EventHeader);
long long GetSlotTime(long long TimeStamp, Int_t FineTime, Int_t ReferenceFineTime, Int_t iTrigSlot, Int_t BitFineTime);
  
int counterror=0;




Long64_t l0timecut( Long64_t RefTimestamp,Long64_t RefFinetime,Long64_t Finetime,int slot,int bitfinetime){
  
  Long64_t TimeSlot = (RefTimestamp<<bitfinetime) | ((RefFinetime & (unsigned int)pow(2,8-bitfinetime))>>(8-bitfinetime));
  Long64_t Time;

  if(slot==-1) Time=((TimeSlot-1) <<(8-bitfinetime)) | (Finetime & (unsigned int)(pow(2,8-bitfinetime)-1));

  if(slot==0) Time=((TimeSlot) <<(8-bitfinetime)) | (Finetime & (unsigned int)(pow(2,8-bitfinetime)-1));

  if(slot==1) Time=((TimeSlot+1) <<(8-bitfinetime)) | (Finetime & (unsigned int)(pow(2,8-bitfinetime)-1));

  return Time;
}

void hexdump(const void * buf, size_t size)
{
  const unsigned char * cbuf = (const unsigned char *) buf;
  const unsigned long BYTES_PER_LINE = 16;
  unsigned long offset, minioffset;

  for (offset = 0; offset < size; offset += BYTES_PER_LINE)
    {
     
      for (minioffset = offset;
	   minioffset < offset + BYTES_PER_LINE;
	   minioffset++)
	{
	  if (minioffset - offset == (BYTES_PER_LINE / 2)) {
	    printf(" ");
	  }

	  if (minioffset < size) {
	    printf("%02x", cbuf[minioffset]);
	  } else {
	    printf("   ");
	  }
	}
      printf("  ");

      for (minioffset = offset;
	   minioffset < offset + BYTES_PER_LINE;
	   minioffset++)
	{
	  if (minioffset >= size)
	    break;
	}
    }
}

long long L0address(unsigned int timestamp, unsigned int finetime, int bitfinetime){

  unsigned int ftMSBmask; //11100000 (binary)
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


//Put -v to have the printout
int main(int argc, char *argv[]) {

  bool debug=0;
  if (argc>1 && strcmp(argv[1], "-v") == 0)  debug=1;
  
  cout<<"Creating histos"<<endl;
  double* Dati = new double[12000000];
  double* Delta = new double[12000000];

  long long int posizione = 0;
  long long int posizione2 = 0;
  ofstream myfile;
  
  //Was used to dump data into files
  //  myfile.open ("Run8764_Burst500_OFFSET_OK_ALSO_LKr.txt");

  bool finish=0;

  //****ROOT****//
  TFile *data = new TFile("dumpL0TP_plus.root","recreate");

  TH1D *h_timestamp   = new TH1D("timestamp","timestamp",1200, 0, 6 );
  h_timestamp->GetXaxis()->SetTitle("Time [s]");
  h_timestamp->GetYaxis()->SetTitle("Counts");

  //Must be flat:
  TH1D *h_foldedTS    = new TH1D("fodedTS","foldedTS",256, -0.5, 255.5 );
  TH1D *h_foldedTS_Control = new TH1D("fodedTS_Control","foldedTS_Control",256, -0.5, 255.5 );
  
  TH1F *h_DTtimestamp  = new TH1F("DTtimestampDATA","DTtimestampDATA",500,0,0.00005);  
  TH1F *h_data_type    = new TH1F("data_type","data_type",51,0,50);
  TH1F *h_triggerflags = new TH1F("triggerflags","triggerflags",51,0,50);

  TH1F *h_triggertype = new TH1F("triggertype","triggertype",51,0,50);


  TH1F *h_reffinetime          = new TH1F("reffinetime","reffinetime",256*100,0,255*100);   
  TH1F *h_DTTime               = new TH1F("DTTime","DTTime",1000,0.0,0.00005);
  h_DTTime->GetXaxis()->SetTitle("Time [s]");
  h_DTTime->GetYaxis()->SetTitle("Counts");    

  TH1F *h_timestamp_periodic   = new TH1F("timestamp_periodic","timestamp_periodic",100000,0,100000);
  TH2I *h_data_typeVStimestamp = new TH2I("data_typeVStimestamp","data_typeVStimestamp",60000,0,6,60,0,60);
  h_data_typeVStimestamp->GetXaxis()->SetTitle("Time [s]");
  h_data_typeVStimestamp->GetYaxis()->SetTitle("Data Type");

  TH2I *h_DeltaTVStimestamp     = new TH2I("DeltaTVStimestamp","DeltaTVStimestamp",6000,0,6,800,0.0,0.00005);
  h_DeltaTVStimestamp->GetXaxis()->SetTitle("Time [s]");
  h_DeltaTVStimestamp->GetYaxis()->SetTitle("Delta Time [s]");

  //For each primitive composing the trigger I do an histo:
  TH1F *h_primitive[NDET];
  TH1F *h_finetime[NDET];
  TH2F *h_slotVSaddress[NDET];
  TH1F  *h_primitiveDiffVsRICH[NDET];

  for(int i=0; i<NDET; i++){
    h_primitive[i]     = new TH1F(Form("primitive_%d",i),Form("primitive_%d",i),16384,16384,16384*2);
    h_finetime[i]      = new TH1F(Form("finetime_%d",i),Form("finetime_%d",i),256,0,255);
    h_slotVSaddress[i] = new TH2F(Form("slotVSaddress_%d",i),Form("slotVSaddress_%d",i),10,-5,5,10,-5,5);
    h_primitiveDiffVsRICH[i] = new TH1F(Form("primitive_%d_VsRICH",i),Form("primitive_%d_VsRICH",i),500,-250,250);
  }

  //***********//
  //File I want to read:
  //  ifstream  finbin ("CHOD_Primitives_30_09_2019.bin", ios::in | ios::binary);
  ifstream  finbin ("L0TP_TRIBE_RUN8764_500_OFFSET_OK_ALSO_LKr.bin", ios::in | ios::binary);
//  ifstream  finbin ("/home/na62torino/Data/l0tribeinitandanalysis/ethreader/L0TPv35_Run9386_burst100_Otto_CHOD6.bin", ios::in | ios::binary);
  
  //ifstream  finbin ("dump.bin", ios::in | ios::binary);
  
  int MEPNumber=0;
  int hexdump_shift=96;
  char event[1601] = ""; //(hexdump * 2) + 1
  char eventbin[800] = ""; //(hexdump) 
  long int AnalysedEvents=0;
  unsigned int old_timestamp=0;
  unsigned int oldFirstEventNumber=0;

  cout<<"Start parsing file..."<<endl;
  while(!finbin.eof()) {    
    //MEP HEADER   
    finbin.read((char*)&MEPHeader.FirstEventNumber, sizeof(char)*3);

    while(MEPHeader.FirstEventNumber<oldFirstEventNumber) {
      finbin.read((char*)&MEPHeader.FirstEventNumber, sizeof(char)*3);
    }
    oldFirstEventNumber =MEPHeader.FirstEventNumber;
    finbin.read((char*)&MEPHeader.SourceID, sizeof(char));

    finbin.read((char*)&MEPHeader.TotalMEPLength, sizeof(char)*2);

    finbin.read((char*)&MEPHeader.NEventsInMEP, sizeof(char));
    finbin.read((char*)&MEPHeader.SourceSubID, sizeof(char));
    if(debug)DebugMEPHeader(MEPHeader);
    
    /*************************************************************/
    
    for(int iEv=0;iEv<MEPHeader.NEventsInMEP;iEv+=1) {
     
      /*************************************************************/
      //Event Header
      finbin.read((char*)&EventHeader.FirstEventLength, sizeof(char)*2);
      finbin.read((char*)&EventHeader.EventNLSB, sizeof(char));  
      finbin.read((char*)&EventHeader.EventFlags, sizeof(char));       
      finbin.read((char*)&EventHeader.Timestamp, sizeof(char)*4);       
      if(debug) DebugEventHeader(EventHeader);
      /*************************************************************/

      Dati[posizione] = EventHeader.Timestamp*0.000000025;

      posizione = posizione +1;

      old_timestamp=EventHeader.Timestamp;
      
      //cout<<setfill('0')<<setw(8)<<hex<<EventHeader.Timestamp<<endl;
    
      if(EventHeader.EventFlags == 0) {
	//Data
	finbin.read((char*)&L0Data.RefFinetime, sizeof(char));       
	finbin.read((char*)&L0Data.DataType, sizeof(char));       
	for(int i=0;i<NDET;i++){
	  finbin.read((char*)&L0Data.Primitive[i], sizeof(char)*2);   
	}
	finbin.read((char*)&L0Data.PreviousTimestamp, sizeof(char)*4);   
	finbin.read((char*)&L0Data.TriggerType, sizeof(char));   
	finbin.read((char*)&L0Data.PreviousTriggerType, sizeof(char));   
	finbin.read((char*)&L0Data.TriggerFlags, sizeof(char)*2);   
	finbin.read((char*)&L0Data.ReservedFlags, sizeof(char)*8);  

	h_timestamp->Fill(EventHeader.Timestamp*0.000000025);
	h_foldedTS->Fill(EventHeader.Timestamp%256);
	if((L0Data.DataType&0x10)==0x10)h_foldedTS_Control->Fill(EventHeader.Timestamp%256);
	if(L0Data.DataType==0xA1) cout<<"CHOKE ON: "<<EventHeader.Timestamp<<endl;
	if(L0Data.DataType==0xA0) cout<<"CHOKE OFF: "<<EventHeader.Timestamp<<endl;
	
	for(int i=0;i < NDET;i++){
	  finbin.read((char*)&L0Data.Finetime[i], sizeof(char));   
	}
	
	finbin.read((char*)&L0Data.Reserved, sizeof(char));   

	finbin.read((char*)&L0Data.Reserved1,sizeof(char)*2);

	for(int i=0;i < NDET;i++){
	  finbin.read((char*)&L0Data.Primitive1[i], sizeof(char)*2);   
	}
	
	for(int i=0;i < NDET;i++){
	  finbin.read((char*)&L0Data.Finetime1[i], sizeof(char));   
	}  
	finbin.read((char*)&L0Data.Reserved2, sizeof(char));   

	finbin.read((char*)&L0Data.Reserved3,sizeof(char)*2);

	for(int i=0;i < NDET;i++){
	  finbin.read((char*)&L0Data.Primitive2[i], sizeof(char)*2);   
	}  

	for(int i=0;i < NDET;i++){
	  finbin.read((char*)&L0Data.Finetime2[i], sizeof(char));   
	}  
	finbin.read((char*)&L0Data.Reserved4, sizeof(char));   

	//***************************************

	long long time[7][3]; //40 bit time: timestamp + finetime
	Int_t fBinFineTime=2;
	//Default time: 0
	for(UInt_t iTrigDet = 0; iTrigDet < 7; iTrigDet++){
	  for(UInt_t iTrigSlot = 0; iTrigSlot < 3; iTrigSlot++){
	    time[iTrigDet][iTrigSlot]=0;

	  }
	}
	
	//For all detectors, I correct the timestamp roll-over due to the ram slot with the function: GetSlotTime
	for(UInt_t iTrigDet = 0; iTrigDet < 7; iTrigDet++) {
	  if(L0Data.Primitive[iTrigDet] & 0x1<<14) {
	    time[iTrigDet][0] = GetSlotTime((long long)EventHeader.Timestamp,(Int_t)L0Data.Finetime[iTrigDet],(Int_t)L0Data.RefFinetime,0,fBinFineTime);
	  }
	}

	for(UInt_t iTrigDet = 0; iTrigDet < 7; iTrigDet++) {
	  if(L0Data.Primitive1[iTrigDet] & 0x1<<14) {
	    time[iTrigDet][1] = GetSlotTime((long long)EventHeader.Timestamp,(Int_t)L0Data.Finetime1[iTrigDet],(Int_t)L0Data.RefFinetime,1,fBinFineTime);
	  }
	}
	
	for(UInt_t iTrigDet = 0; iTrigDet < 7; iTrigDet++) {
	  if(L0Data.Primitive2[iTrigDet] & 0x1<<14) {
	    time[iTrigDet][2] = GetSlotTime((long long)EventHeader.Timestamp,(Int_t)L0Data.Finetime2[iTrigDet],(Int_t)L0Data.RefFinetime,2,fBinFineTime);
	  }
	}
	

	//Take control triggers with the RICH centered
	//	if(L0Data.DataType & 0x10==0x10) {
	  if((Int_t)L0Data.Primitive[1]!=0) {
	    for(UInt_t iTrigDet = 0; iTrigDet < 7; iTrigDet++) {
	      if((Int_t)(L0Data.Primitive[iTrigDet])!=0) {
		h_primitiveDiffVsRICH[iTrigDet]->Fill(time[1][0] - time[iTrigDet][0]);
		h_primitiveDiffVsRICH[iTrigDet]->Fill(time[1][0] - time[iTrigDet][1]);
		h_primitiveDiffVsRICH[iTrigDet]->Fill(time[1][0] - time[iTrigDet][2]);
		
	      }
	    
	    }
	  }
	  //}
 
	
       
	  if(debug) DebugL0Data(L0Data);

	//ANALYSIS
	/*if(L0Data.DataType!=2 && L0Data.DataType!=0x22) {
	  cout<<"EventID: "<<dec<<MEPHeader.FirstEventNumber<<" DataType 0x"<<hex<<L0Data.DataType<<" TriggerFlag 0x"<<L0Data.TriggerFlags<<" TimeStamp 0x"<<EventHeader.Timestamp<<endl;

	  myfile<<"EventID: "<<dec<<MEPHeader.FirstEventNumber<<" DataType 0x"<<hex<<L0Data.DataType<<" TriggerFlag 0x"<<L0Data.TriggerFlags<<" TimeStamp 0x"<<EventHeader.Timestamp<<endl;
	  }*/

	//	if(L0Data.DataType ==0x10) break;

	int slot0[NDET]={0,0,0,0,0,0,0};
	int slot1[NDET]={0,0,0,0,0,0,0};
	int slot2[NDET]={0,0,0,0,0,0,0};
	
	
	int slot[NDET]={0,0,0,0,0,0,0};
	int a =  0;
	int b =  1;
	int c = -1;
	double DT=0.;
	Long64_t TimeSlot;
	Long64_t TimeSlot1;
	Long64_t TimeSlot2;
	 
	
	TimeSlot=(EventHeader.Timestamp<<bitfinetime) | ((L0Data.RefFinetime & (unsigned int)pow(2,8-bitfinetime))>>(8-bitfinetime));
	TimeSlot1=TimeSlot-1;
	TimeSlot2=TimeSlot+1;
	
    
	h_reffinetime->Fill(L0Data.RefFinetime*100);
	
	//	if((L0Data.DataType & 0x11)==0x11) DebugL0Data(L0Data);

	if(L0Data.DataType==2) h_timestamp_periodic->Fill(EventHeader.Timestamp-L0Data.PreviousTimestamp);
	h_DTtimestamp->Fill(EventHeader.Timestamp*0.000000025-L0Data.PreviousTimestamp*0.000000025);
	h_triggerflags->Fill(L0Data.TriggerFlags);
	//	if(L0Data.TriggerFlags==2) DebugL0Data(L0Data);
	h_data_type->Fill(L0Data.DataType);
	h_triggertype->Fill(L0Data.TriggerType);
	h_data_typeVStimestamp ->Fill(EventHeader.Timestamp*0.000000025,L0Data.DataType);

	//Stefan
	h_DeltaTVStimestamp ->Fill(EventHeader.Timestamp*0.000000025,EventHeader.Timestamp*0.000000025-L0Data.PreviousTimestamp*0.000000025);


      }//EOB flag ==0

      if(EventHeader.EventFlags != 0) { //EOB flag==1

	finbin.read((char*)&L0EOB.Reserved0,           sizeof(char));       
	finbin.read((char*)&L0EOB.DataType,            sizeof(char));       
	finbin.read((char*)&L0EOB.Latency,             sizeof(char)*2);       
	finbin.read((char*)&L0EOB.FinetimeBits,        sizeof(char));       
	finbin.read((char*)&L0EOB.PriRefDetector,      sizeof(char));       
	finbin.read((char*)&L0EOB.SecRefDetector,      sizeof(char));       
	finbin.read((char*)&L0EOB.Reserved1,           sizeof(char));       
	finbin.read((char*)&L0EOB.Reserved2,           sizeof(char)*4);       
	finbin.read((char*)&L0EOB.Reserved3,           sizeof(char)*4);       
	finbin.read((char*)&L0EOB.PreviousTimestamp,   sizeof(char)*4);       
	finbin.read((char*)&L0EOB.TriggerType,         sizeof(char));       
	finbin.read((char*)&L0EOB.PreviousTriggerType, sizeof(char));       
	finbin.read((char*)&L0EOB.TriggerFlags,        sizeof(char)*2);       

	for(int i=0;i < NDET;i++) {
	  finbin.read((char*)&L0EOB.NumberOfPrimitives[i], sizeof(char)*4);       
	}
	
	finbin.read((char*)&L0EOB.NumberOfCHOKE,       sizeof(char)*4);       
	finbin.read((char*)&L0EOB.NumberOfError,       sizeof(char)*4);       
	finbin.read((char*)&L0EOB.NumberOfRandom,      sizeof(char)*4);       
	finbin.read((char*)&L0EOB.NumberOfCalibration, sizeof(char)*4);       
	finbin.read((char*)&L0EOB.Reserved4,           sizeof(char)*4);       

	for(int i=0; i<NMASKS;i++){//numero di maschere
	  finbin.read((char*)&L0EOB.L0Masks[i].Detector,         sizeof(char));       
	  finbin.read((char*)&L0EOB.L0Masks[i].Reserved0,        sizeof(char)*3);       
	  finbin.read((char*)&L0EOB.L0Masks[i].Downscaling,      sizeof(char)*4);       
	  finbin.read((char*)&L0EOB.L0Masks[i].TriggerSent,      sizeof(char)*4);       
	  finbin.read((char*)&L0EOB.L0Masks[i].TriggerGenerated, sizeof(char)*4);       
	  int f=0;
	  
	  for(int j=0;j < NDET;j++){//numero di detector
	    finbin.read((char*)&L0EOB.L0Masks[i].DetectorMask[j],     sizeof(char)*2);       
	    finbin.read((char*)&L0EOB.L0Masks[i].DetectorDontCare[j], sizeof(char)*2);       
	  }	  
	  finbin.read((char*)&L0EOB.L0Masks[i].Reserved1, sizeof(char)*4);
	
	}//end masks

	if(debug) DebugL0EOB(L0EOB); 
	WriteL0EOB(L0EOB,EventHeader);
	finish=1;
	cout<<"HO FINITO"<<endl;
	break;
      } //eob 
      AnalysedEvents+=1;
      if(EventHeader.EventFlags != 0 || finish==1) break;
    }//end event

    //fine ciclo for stefan


    
    MEPNumber++;
    if(finish==1) break;
  } //end file

  long int ContatoreDelta = 0;

  for (int i = 0; i < AnalysedEvents-1; i++)
    {
      Delta[i]=Dati[i+1]-Dati[i];
      h_DTTime->Fill(Delta[i]);
      ContatoreDelta++;
    }


  
  cout<<"event analyzed: "<<dec<<AnalysedEvents<<endl;
  cout<<"Numeri di Delta: "<<ContatoreDelta<<endl;
  cout<<"ERRORS: "<<counterror<<endl;


  //inizio Montecarlo

  const float frequency= 8;    //mean DT  (clock hits) //4: uno ogni 0.1 us => 10.000 kHz
  double* timestampMc  = new double[59000000];
  double* timestampM   = new double[59000000];
  double* DatiM        = new double[12000000];
  double* DeltaM       = new double[12000000];
  long k               = 0;
  int contatore        = 0;
  TRandom *ran         = new TRandom();
  TH1D *h_timestampM   = new TH1D("timestampM","timestampM",6000, 0, 6 );
  TH1F *h_DTTimeM      = new TH1F("DTTimeM","DTTimeM",80000,-0.00004,0.00004);

  h_DTTimeM->GetXaxis()->SetTitle("Time [s]");
  h_DTTimeM->GetYaxis()->SetTitle("Counts");
  h_DTTimeM->SetTitle("Delta Time Montecarlo n=50");

  h_timestampM->GetXaxis()->SetTitle("Time [s]");
  h_timestampM->GetYaxis()->SetTitle("Counts");
  h_timestampM->SetTitle("TimeStamp Montecarlo n=50");

  timestampMc[0]      = 0;

  //L0EOB.NumberOfPrimitives[i]

  //Montecarlo del tesista:
  for (long long i =1; i<58187197; i++ ){ 
    double finetime = ran->Rndm();   //256 divisions da 100ps          // uniform in ]0,1]
    timestampM[i] += -frequency * TMath::Log(1-(finetime)); //exponential distro
    timestampMc[i]=timestampMc[i-1]+timestampM[i];
    contatore++;
    if (contatore==50)
      { 
	h_timestampM->Fill(timestampMc[i]*0.000000025+0.000069);
	contatore=0;
	DatiM[k]=timestampMc[i]*0.000000025+0.000069;
	k++;
      }   
  }

  for (int j = 0; j < k-1; j++)
    {
      DeltaM[j]=DatiM[j+1]-DatiM[j];
      //cout<<DeltaM[j]<<endl;
      h_DTTimeM->Fill(DeltaM[j]);
    }





  cout<<"Writing histos"<<endl;
  h_timestamp              -> Write();
  h_foldedTS               -> Write();
  h_foldedTS_Control       -> Write();
  h_reffinetime            -> Write();
  h_DTtimestamp            -> Write();
  h_triggerflags           -> Write();
  h_DTTime                 -> Write();
  h_data_type              -> Write();
  h_timestamp_periodic     -> Write();
  h_triggertype            -> Write();
  h_data_typeVStimestamp   -> Write();
  h_DeltaTVStimestamp      -> Write();
  h_timestampM             -> Write();
  h_DTTimeM                -> Write();

  for(int i =0; i<7;i++) h_primitiveDiffVsRICH[i]->Write();  


  //questo forse non deve essere commentato

  //cout<<"Writing data for each detector"<<endl;
  //for(int i =0; i<NDET;i++){
  //  h_primitive[i]->Write();
  //  h_finetime[i]->Write();
  //  h_slotVSaddress[i]->Write();
  //}
  cout<<"Closing the file "<<endl;
  finbin.close();
  //myfile.close();
  delete [] Dati; 
  delete [] Delta; 
  return 0;
  
}

void DebugMEPHeader(MEPHeader_t  MEPHeader){
  cout<<"MEP HEADER: "<<endl;
  cout<<hex<< "firstev "    << MEPHeader.FirstEventNumber <<endl;
  cout<<hex<<"Source "      << MEPHeader.SourceID         <<endl;
  cout<<hex<<"TotalLength " << MEPHeader.TotalMEPLength   <<endl;  
  cout<<hex<<"EventsINMEP "  << MEPHeader.NEventsInMEP     <<endl; 
  cout<<hex<<"SubSource "   << MEPHeader.SourceSubID      <<endl;
  cout<<"********************************************************"<<endl;
}


void DebugEventHeader(EventHeader_t  EventHeader){
  cout<<"EVENT HEADER: "<<endl;  
  cout<<hex<<"firstevlength  "<<EventHeader.FirstEventLength <<endl;
  cout<<hex<<"eventNLSB      "<< EventHeader.EventNLSB       <<endl;
  cout<<hex<<"EventFlags     "<<EventHeader.EventFlags       <<endl;  
  cout<<hex<<"timestamp      " <<hex<<EventHeader.Timestamp       <<endl;
  cout<<"********************************************************"<<endl;

}


void DebugL0Data(L0Data_t  L0Data){
  cout<<"EVENT DATA: "<<endl;    
  cout<<"Ref finetime         "<<hex<<L0Data.RefFinetime<<endl;
  cout<<"DataType             "<<hex<<L0Data.DataType<<endl;  
  cout<<" PreviousTimestamp   "<<hex<<L0Data.PreviousTimestamp<<endl;
  cout<<" TriggerType         "<<hex<<L0Data.TriggerType<<endl;
  cout<<" PreviousTriggerType "<<hex<<L0Data.PreviousTriggerType<<endl;
  cout<<" TriggerFlags        "<<hex<<L0Data.TriggerFlags<<endl;
  cout<<"ReservedFlags        "<<hex<<L0Data.ReservedFlags<<endl;    
  cout<<"Primitive Data:"<<endl;
  for (int i=0;i<NDET;i++){
    cout<<"Detector "<<i<<endl;
    cout<<"Primitive N   "<<"  "<<hex<<L0Data.Primitive[i]<<endl;
    cout<<"Finetime  N   "<<"  "<<hex<<L0Data.Finetime[i]<<endl<<endl;
    cout<<"Primitive N-1 "<<"  "<<hex<<L0Data.Primitive1[i]<<endl;
    cout<<"Finetime  N-1 "<<"  "<<hex<<L0Data.Finetime1[i]<<endl<<endl;
    cout<<"Primitive N+1 "<<"  "<<hex<<L0Data.Primitive2[i]<<endl;
    cout<<"Finetime  N+1 "<<"  "<<hex<<L0Data.Finetime2[i]<<endl<<endl;
  }   
  cout<<"********************************************************"<<endl;

}



void DebugL0EOB(L0EOB_t  L0EOB){
  cout<<"EOB "<<endl;    
  
  cout<<"L0EOB.Reserved0           "<< L0EOB.Reserved0            <<endl;
  cout<<"L0EOB.DataType            "<< L0EOB.DataType             <<endl;   
  cout<<"L0EOB.Latency             "<< L0EOB.Latency              <<endl; 
  cout<<"L0EOB.FineTimeBits        "<< L0EOB.FinetimeBits         <<endl;     
  cout<<"L0EOB.PriRefDetector      "<< L0EOB.PriRefDetector       <<endl;       
  cout<<"L0EOB.SecRefDetector      "<< L0EOB.SecRefDetector       <<endl;      
  cout<<"L0EOB.Reserved1           "<< L0EOB.Reserved1            <<endl; 
  cout<<"L0EOB.Reserved2           "<< L0EOB.Reserved2            <<endl;  
  cout<<"L0EOB.Reserved3           "<< L0EOB.Reserved3            <<endl;
  cout<<"L0EOB.PreviousTimestamp   "<< L0EOB.PreviousTimestamp    <<endl;       
  cout<<"L0EOB.TriggerType         "<< L0EOB.TriggerType          <<endl;       
  cout<<"L0EOB.PreviousTriggerType "<< L0EOB.PreviousTriggerType  <<endl;             
  cout<<"L0EOB.TriggerFlags        "<< L0EOB.TriggerFlags         <<endl;     

  for(int i=0;i<NDET;i++){
    cout<<" L0EOB.NumberOfPrimitives[" <<i <<"] "<< L0EOB.NumberOfPrimitives[i]<<endl;
  }
	
  cout<<"L0EOB.NumberOfCHOKE       "<<L0EOB.NumberOfCHOKE       <<endl;  
  cout<<"L0EOB.NumberOfError       "<<L0EOB.NumberOfError       <<endl;        
  cout<<"L0EOB.NumberOfRandom      "<<L0EOB.NumberOfRandom      <<endl;         
  cout<<"L0EOB.NumberOfCalibration "<<L0EOB.NumberOfCalibration <<endl;              
  cout<<"L0EOB.Reserved4           "<<L0EOB.Reserved4           <<endl;         

  for(int i=0; i<8;i++){
    cout<<"i "<<i<<endl;
    cout<<"L0EOB.L0Mask["<<i <<"].Detector        "<<L0EOB.L0Masks[i].Detector<<endl; 
    cout<<"L0EOB.L0Mask["<<i<<"].Reserved0        "<<L0EOB.L0Masks[i].Reserved0<<endl;
    cout<<"L0EOB.L0Mask["<<i<<"].Downscaling      "<<L0EOB.L0Masks[i].Downscaling<<endl;
    cout<<"L0EOB.L0Mask["<<i<<"].TriggerSent      "<<L0EOB.L0Masks[i].TriggerSent<<endl;
    cout<<"L0EOB.L0Mask["<<i<<"].TriggerGenerated "<<L0EOB.L0Masks[i].TriggerGenerated<<endl;
	 
    for(int j=0;j<NDET;j++){
      cout<< "L0EOB.L0Mask["<<i<<"].DetectorMask["<<j<<"] "<<L0EOB.L0Masks[i].DetectorMask[j]<<endl;
      cout<<"L0EOB.L0Mask["<<i<<"].DetectorDontCare["<<j<<"] "<<L0EOB.L0Masks[i].DetectorDontCare[j] <<endl;
    }

    cout<<"L0EOB.l0Mask["<<i<<"].Reserved1 "<<L0EOB.L0Masks[i].Reserved1<<endl;

  }//end masks
  cout<<"********************************************************"<<endl;
   
}



void WriteL0EOB(L0EOB_t  L0EOB,EventHeader_t EventHeader){
  ofstream ofs ("EOB.txt",std::ofstream::out);
        
  ofs<<"timestamp EOB:            "<<hex<<EventHeader.Timestamp<<endl<<endl;
  ofs<<"L0EOB.Reserved0           "<< L0EOB.Reserved0            <<endl;
  ofs<<"L0EOB.DataType            "<< L0EOB.DataType             <<endl;   
  ofs<<"L0EOB.Latency             "<< L0EOB.Latency              <<endl; 
  ofs<<"L0EOB.FineTimeBits        "<< L0EOB.FinetimeBits         <<endl;     
  ofs<<"L0EOB.PriRefDetector      "<< L0EOB.PriRefDetector       <<endl;       
  ofs<<"L0EOB.SecRefDetector      "<< L0EOB.SecRefDetector       <<endl;      
  ofs<<"L0EOB.Reserved1           "<< L0EOB.Reserved1            <<endl; 
  ofs<<"L0EOB.Reserved2           "<< L0EOB.Reserved2            <<endl;  
  ofs<<"L0EOB.Reserved3           "<< L0EOB.Reserved3            <<endl;
  ofs<<"L0EOB.PreviousTimestamp   "<< L0EOB.PreviousTimestamp    <<endl;       
  ofs<<"L0EOB.TriggerType         "<< L0EOB.TriggerType          <<endl;       
  ofs<<"L0EOB.PreviousTriggerType "<< L0EOB.PreviousTriggerType  <<endl;             
  ofs<<"L0EOB.TriggerFlags        "<< L0EOB.TriggerFlags         <<endl;     

  for(int i=0;i<NDET;i++){
    ofs<<" L0EOB.NumberOfPrimitives[" <<i <<"] "<< L0EOB.NumberOfPrimitives[i]<<endl;
  }
	
  ofs<<"L0EOB.NumberOfCHOKE       "<<L0EOB.NumberOfCHOKE       <<endl;  
  ofs<<"L0EOB.NumberOfError       "<<L0EOB.NumberOfError       <<endl;        
  ofs<<"L0EOB.NumberOfRandom      "<<L0EOB.NumberOfRandom      <<endl;         
  ofs<<"L0EOB.NumberOfCalibration "<<L0EOB.NumberOfCalibration <<endl;              
  ofs<<"L0EOB.Reserved4           "<<L0EOB.Reserved4           <<endl;         

  for(int i=0; i<NMASKS;i++){
    ofs<<"i "<<i<<endl;
    ofs<<"L0EOB.L0Mask["<<i <<"].Detector        "<<L0EOB.L0Masks[i].Detector<<endl; 
    ofs<<"L0EOB.L0Mask["<<i<<"].Reserved0        "<<L0EOB.L0Masks[i].Reserved0<<endl;
    ofs<<"L0EOB.L0Mask["<<i<<"].Downscaling      "<<L0EOB.L0Masks[i].Downscaling<<endl;
    ofs<<"L0EOB.L0Mask["<<i<<"].TriggerSent      "<<L0EOB.L0Masks[i].TriggerSent<<endl;
    ofs<<"L0EOB.L0Mask["<<i<<"].TriggerGenerated "<<L0EOB.L0Masks[i].TriggerGenerated<<endl;
	 
    for(int j=0;j<NDET;j++){
      ofs<< "L0EOB.L0Mask["<<i<<"].DetectorMask["<<j<<"] "<<L0EOB.L0Masks[i].DetectorMask[j]<<endl;
      ofs<<"L0EOB.L0Mask["<<i<<"].DetectorDontCare["<<j<<"] "<<L0EOB.L0Masks[i].DetectorDontCare[j] <<endl;
    }

    ofs<<"L0EOB.l0Mask["<<i<<"].Reserved1 "<<L0EOB.L0Masks[i].Reserved1<<endl;

  }//end masks

  ofs.close();
}


long long GetSlotTime(long long TimeStamp, Int_t FineTime, Int_t ReferenceFineTime, Int_t iTrigSlot, Int_t BitFineTime)
{

  long long time=0;
  if(BitFineTime==0){
    if(iTrigSlot==0){
      time = (long long)(TimeStamp)   * 0x100  + (Int_t)FineTime;
      return time;
    }
    if(iTrigSlot==1){
      time = (long long)(TimeStamp-1) * 0x100  + (Int_t)FineTime;
      return time;
    }
    if(iTrigSlot==2){
      time = (long long)(TimeStamp+1) * 0x100  + (Int_t)FineTime;
      return time;
    }
  }



  if(BitFineTime==1){
    if(iTrigSlot==0){
      time = (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
      return time;
    }

    if(iTrigSlot==1){
      if(ReferenceFineTime>127) {
	time= (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
	return time;

      }
      else{
	time= (long long)(TimeStamp-1) * 0x100  + (Int_t)FineTime;
	return time;
      }
    }

    if(iTrigSlot==2){
      if(ReferenceFineTime>127) {
	time= (long long)(TimeStamp+1) * 0x100  + (Int_t)FineTime;
	return time;
      }
      else{
	time= (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
	return time;
      }
    }
  }

  if(BitFineTime==2){

    if(iTrigSlot==0){
      time = (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
      return time;
    }

    if(iTrigSlot==1){
      if(ReferenceFineTime>63) {
	time= (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
	return time;
      }
      else{
	time= (long long)(TimeStamp-1) * 0x100  + (Int_t)FineTime;
	return time;
      }
    }

    if(iTrigSlot==2){
      if(ReferenceFineTime>191){
	time= (long long)(TimeStamp+1) * 0x100  + (Int_t)FineTime;
	return time;
      }
      else {
	time= (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
	return time;
      }
    }
  }


  if(BitFineTime==3){

    if(iTrigSlot==0){
      time = (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
      return time;
    }

    if(iTrigSlot==1){
      if(ReferenceFineTime>223) {
	time= (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
	return time;
      }
      else{
	time= (long long)(TimeStamp-1) * 0x100  + (Int_t)FineTime;
	return time;
      }
    }

    if(iTrigSlot==2){
      if(ReferenceFineTime>223){
	time= (long long)(TimeStamp+1) * 0x100  + (Int_t)FineTime;
	return time;
      }
      else{
	time= (long long)(TimeStamp) * 0x100  + (Int_t)FineTime;
	return time;
      }
    }
  }

  return -1;

}
