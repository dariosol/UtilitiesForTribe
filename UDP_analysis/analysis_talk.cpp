#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
using namespace std;

#define ClockPeriod 24.951059536
#define TdcCalib    (ClockPeriod/256.0)

struct MTPHeader_t {
  unsigned int SourceID;
  unsigned int MTPTimeStamp;
  unsigned int TotalMTPLength;
  unsigned int NEventsInMTP;
  unsigned int SourceSubID;
  
} MTPHeader;


struct L0Data_t {
  unsigned int PrimitiveID;
  unsigned int TimeStampL;
  unsigned int FineTime;
  unsigned int TimeStamp;
  unsigned int TimeStampH;
  unsigned int TimeStampWord;
  
} L0Data;



void DebugMTPHeader(MTPHeader_t  L0Data);
void DebugL0Data(L0Data_t  L0Data,int iEv);


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

long long L0address(unsigned int timestamp, unsigned int finetime, int bitfinetime) {

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

static void show_usage(std::string name)
{
  std::cerr << "Usage: \n"
	    << "Options:\n"
	    << "\t-h\t\tShow this help message\n"
	    << "\t-s\t\tSourceID\n"
	    << "\t-i\t\tinputfilename\n"
    	    << "\t-o\t\toutputfilename\n"
    	    << "\t-v\t\tverbose\n"
	    << std::endl;
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    show_usage(argv[0]);
    return 1;
  }
  bool debug=0;
  int par_SourceID=0x30;
  std::string inputFileName;
  std::string outputFileName;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "-h") || (arg == "--help")) {
      show_usage(argv[0]);
      return 0;
    }
    else if ((arg == "-v")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
	debug = 1; // Increment 'i' so we don't get the argument as the next argv[i].
	cout<<"debug: "<<debug<<endl;
      }
    }
    else if ((arg == "-s")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
	par_SourceID = atoi(argv[i+1]); // Increment 'i' so we don't get the argument as the next argv[i].
	cout<<"SourceID: "<<par_SourceID<<endl;
      }
    }

    else if ((arg == "-i")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
	inputFileName = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
	cout<<"input: "<<inputFileName<<endl;
      }
    }

    else if ((arg == "-o")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
	outputFileName = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
	cout<<"output: "<<outputFileName<<endl;
      }
    } 
  }
  
  ifstream  finbin (inputFileName.c_str(), ios::in | ios::binary);
  int MTPNumber=0;
  int AnalysedEvents=0;
  unsigned int temp1=0;
  unsigned int temp2=0;
  unsigned int offsetEndPacket=0;
  unsigned long long timestamp=0;
  TFile * outfile = new TFile(outputFileName.c_str(),"RECREATE");
  TH1F * SourceID = new TH1F("SourceID","SourceID",100,-0.5,99.5);
  TH1F * foldedTS = new TH1F("foldedTS","foldedTS",256,-0.5,255.5);
  TH1F * FineTime = new TH1F("FineTime","FineTime",256,-0.5,255.5);
  TH1F * TimeStamp = new TH1F("TimeStamp","TimeStamp; [s]",256*4, -0.5*ClockPeriod, 255.5*ClockPeriod);
  TH2F * MTPOffset = new TH2F("MTPOffset","MTPOffset; Timestamp [s]; [us]",256*4, -0.5*ClockPeriod, 255.5*ClockPeriod, 100, -5*(256*ClockPeriod)/1000.0, 20*(256*ClockPeriod)/1000.0);
  TH1F * DT = new TH1F("DT","#DeltaT Timestamp; [ns]",601, (-502-0.5)*TdcCalib, (2502+0.5)*TdcCalib);
  unsigned long long old_timestamp=0;
  unsigned int old_finetime=0;
  while(!finbin.eof()){    
    //MTP HEADER   
    finbin.read((char*)&MTPHeader.MTPTimeStamp,   sizeof(char)*3);
    finbin.read((char*)&MTPHeader.SourceID,       sizeof(char));
//
//    while(MTPHeader.SourceID!=30) {
//      finbin.seekg(-2, ios::cur);
//      cout<<"Before: MTPHeader.MTPTimeStamp "<<hex<<MTPHeader.MTPTimeStamp<<endl;
//      cout<<"Before: MTPHeader.SourceID "<<hex<<MTPHeader.SourceID<<endl;
//      finbin.read((char*)&MTPHeader.MTPTimeStamp,   sizeof(char)*3);
//      finbin.read((char*)&MTPHeader.SourceID,       sizeof(char));
//      cout<<"After: MTPHeader.MTPTimeStamp "<<hex<<MTPHeader.MTPTimeStamp<<endl;
//      cout<<"After: MTPHeader.SourceID "<<hex<<MTPHeader.SourceID<<endl;
//    }
//    
    if(MTPHeader.SourceID!=par_SourceID) {
      cout<<"ERROR IN READING SOURCE"<<endl;
      finbin.seekg(-2, ios::cur);
      cout<<"Before: MTPHeader.MTPTimeStamp "<<hex<<MTPHeader.MTPTimeStamp<<endl;
      cout<<"Before: MTPHeader.SourceID "<<hex<<MTPHeader.SourceID<<endl;
      finbin.read((char*)&MTPHeader.MTPTimeStamp,   sizeof(char)*3);
      finbin.read((char*)&MTPHeader.SourceID,       sizeof(char));
      cout<<"After: MTPHeader.MTPTimeStamp "<<hex<<MTPHeader.MTPTimeStamp<<endl;
      cout<<"After: MTPHeader.SourceID "<<hex<<MTPHeader.SourceID<<endl;
    }


    if(MTPHeader.SourceID!=par_SourceID) {
      cout<<"ERROR IN READING AGAIN SOURCE"<<endl;
      cout<<"Before: MTPHeader.MTPTimeStamp "<<hex<<MTPHeader.MTPTimeStamp<<endl;
      cout<<"Before: MTPHeader.SourceID "<<hex<<MTPHeader.SourceID<<endl;
      break;    
    }
      
    finbin.read((char*)&MTPHeader.TotalMTPLength, sizeof(char)*2);
    finbin.read((char*)&MTPHeader.NEventsInMTP,   sizeof(char));
    finbin.read((char*)&MTPHeader.SourceSubID,    sizeof(char));
    MTPHeader.NEventsInMTP=MTPHeader.NEventsInMTP;

    if(MTPHeader.SourceSubID!=par_SourceID) {
      finbin.seekg(-6, ios::cur);
      finbin.read((char*)&MTPHeader.MTPTimeStamp,   sizeof(char)*3);
      finbin.read((char*)&MTPHeader.SourceID,       sizeof(char));
      finbin.read((char*)&MTPHeader.TotalMTPLength, sizeof(char)*2);
      finbin.read((char*)&MTPHeader.NEventsInMTP,   sizeof(char));
      finbin.read((char*)&MTPHeader.SourceSubID,    sizeof(char));
      
    }
    SourceID->Fill(MTPHeader.SourceID);

    if(debug)DebugMTPHeader(MTPHeader);
    /*************************************************************/

    for(int iEv=0;iEv < MTPHeader.NEventsInMTP;iEv+=1) {
     
      /*************************************************************/
      
      finbin.read((char*)&temp1, sizeof(char)*3);
      finbin.read((char*)&temp2, sizeof(char));
      
      
      if(temp2 == 0) {
	L0Data.TimeStampH = temp1;
	L0Data.TimeStampWord=temp2;
	finbin.read((char*)&L0Data.FineTime, sizeof(char));
	finbin.read((char*)&L0Data.TimeStampL, sizeof(char));  
	finbin.read((char*)&L0Data.PrimitiveID, sizeof(char)*2);
	
      }
      
      else {
	L0Data.FineTime = temp1 & 0xFF;
	L0Data.TimeStampL = (temp1 & (0xFF<<8))>>8;
	L0Data.PrimitiveID = (temp2 <<24 | (temp1 & 0xFF<<16))>>16;
      }
      timestamp = L0Data.TimeStampH<<8 | L0Data.TimeStampL;

      foldedTS->Fill(timestamp%256);
      FineTime->Fill(L0Data.FineTime);

      Double_t TimeStamp_in_sec = timestamp*ClockPeriod/1E6;
      TimeStamp->Fill(TimeStamp_in_sec);
     
      MTPOffset->Fill(TimeStamp_in_sec, (MTPHeader.MTPTimeStamp*0x100*ClockPeriod - timestamp*ClockPeriod)/1000.0); // difference in us                                                           

      Long_t delta_long = (Long_t)timestamp - (Long_t)old_timestamp;
      Double_t delta = (delta_long*ClockPeriod + (L0Data.FineTime*TdcCalib - old_finetime*TdcCalib));
      DT->Fill(delta);
      old_timestamp=timestamp;
      old_finetime=L0Data.FineTime;
      if(debug) {
	DebugL0Data(L0Data,iEv);
      }
      AnalysedEvents+=1;
    }//end event
    //finbin.read((char*)&offsetEndPacket,   sizeof(char)*2);
    //if(offsetEndPacket!=0) finbin.seekg(-2, ios::cur);
    MTPNumber++;
  }//end file

  cout<<"event analyzed "<<dec<<AnalysedEvents<<endl;
  DT->Write();
  TimeStamp->Write();
  FineTime->Write();
  foldedTS->Write();
  SourceID->Write();
  MTPOffset->Write();
  cout<<"File Written"<<endl;
  //finbin.close();
  delete foldedTS;
  delete outfile;
  return 1;

}

void DebugMTPHeader(MTPHeader_t  MTPHeader) {
  cout<<"MTP HEADER: "<<endl;
  cout<<hex<<"Source        "<< MTPHeader.SourceID         <<endl;
  cout<<hex<<"MTP TimeStamp "<< MTPHeader.MTPTimeStamp     <<endl; 
  cout<<hex<<"TotalLength   "<< MTPHeader.TotalMTPLength   <<endl;  
  cout<<hex<<"NEventsInMTP  "<< MTPHeader.NEventsInMTP     <<endl; 
  cout<<hex<<"SubSource     "<< MTPHeader.SourceSubID      <<endl;
  cout<<"********************************************************"<<endl;
}


void DebugL0Data(L0Data_t  L0Data, int iEv){
  cout<<"EVENT DATA: "<<iEv+1<<endl;    
  cout<<"PrimitiveID          "<<hex<<L0Data.PrimitiveID<<endl;
  cout<<"FineTime             "<<hex<<L0Data.FineTime<<endl;
  cout<<"TimeStampWord        "<<hex<<L0Data.TimeStampWord<<endl;
  cout<<"TimeStampH           "<<hex<<L0Data.TimeStampH<<endl;
  cout<<"TimeStampL           "<<hex<<L0Data.TimeStampL<<endl;
  unsigned int TimeStampFull = L0Data.TimeStampH<<8 | L0Data.TimeStampL;
  cout<<"TimeStamp            "<<hex<<TimeStampFull<<"["<<TimeStampFull*0.000000025<<" sec]"<<endl;

  
  cout<<"********************************************************"<<endl;

}

