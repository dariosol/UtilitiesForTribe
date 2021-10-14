// @(#)root/fpga
// Author: Alberto Perro 10/11/2019
// Email: alberto.perro57@edu.unito.it
#include <stdio.h>
#include <vector>
#include <algorithm>
#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TString.h"
#include "TPrimitive.hh"
using namespace std;
//To start at the beginning, and all the primitives:
//startTime=0;
//nOfPrim=-1;
int rootToBin(TString file_path, TString branchName, uint32_t startTime, uint32_t nOfPrim, TString OutName_bin){

  cout<<"Did you compiled TPrimitive library? "<<endl;
  //  gSystem->CompileMacro("TPrimitive.cc","kg");

  TFile* file = new TFile(file_path);
  TTree* tree = 0;
  TH1D *primitiveID = new TH1D("PrimitiveID","PrimitiveID",65536,0,65535);
  TH1D *timestamp = new TH1D("timestamp","timestamp",6000,0,6); 

  cout<<"Looking for a tree"<<endl;
  file->GetObject(branchName,tree);
 
  if(tree){
    cout<<"Tree found"<<endl;

    FILE* binOut = fopen(OutName_bin.Data(),"wb"); //binary write

    TPrimitive* Primitive = new TPrimitive();
    TBranch* b = tree->GetBranch("fPrimitive");
    b->SetAddress(&Primitive);
    cout<<"branch found"<<endl;

    uint32_t global_time= 0;
 
    std::vector<uint32_t> data;
    uint32_t addr = 0;
    uint32_t startEntry = 0;
    uint32_t nentries;
    bool newPacket=0;
    uint32_t oldSendTimeStamp=0;
    uint32_t nprimitive=0;
    int unsorted=0;
    
    if(nOfPrim==-1) nentries = b -> GetEntries();
    else nentries = nOfPrim;
    printf("number of primitives: %u\n",nentries);
    tree->GetEntry(startEntry);

    //Looking at the first entry:
    //Removing primitives up to the first start time (in timestamp units)
    while(static_cast<uint32_t>(Primitive->GetTimeStamp()) <= startTime) {
      if(startEntry%1000000) cout<<"start entry: "<<startEntry<<endl;
      tree->GetEntry(++startEntry);
    }
    
    printf("timestamp at entry: %u\n",startEntry);
    
    for (uint32_t iEntry = startEntry; iEntry<startEntry+nentries; ++iEntry) {
      tree->GetEntry(iEntry);

      oldSendTimeStamp=Primitive->GetSendTimeStamp();

      uint32_t timestampH = (Primitive->GetTimeStamp()-startTime)>>8;

      if(newPacket) timestampH |= 1UL << 23; //MSB of new packet changes

      uint8_t timestampL = static_cast<uint8_t>((Primitive->GetTimeStamp()-startTime) & 0x00FF);

      //Sometime MUV3 and NewCHOD delivers unsorted primitives, but they cannot be handled by L0Tribe. They are not many (thousands).
      if(global_time != timestampH ) {
	if(global_time > timestampH) {
	  cout<<"Removing unsorted primitives"<<endl;
	  unsorted++;
	  continue;
	}
        uint32_t temp = (static_cast<uint16_t>(Primitive->GetPrimitiveID())<<16)+(timestampL<<8)+static_cast<uint8_t>(Primitive->GetFineTime());
        global_time = timestampH;
        data.push_back(timestampH);
        data.push_back(temp);
        addr += 8;
	nprimitive++;
       	primitiveID->Fill(temp>>16);
	timestamp->Fill(((timestampH << 8) | timestampL)*25e-9);
      }
      else {
        uint32_t temp = (static_cast<uint16_t>(Primitive->GetPrimitiveID())<<16)+(timestampL<<8)+static_cast<uint8_t>(Primitive->GetFineTime());
        data.push_back(temp);
        addr+=4;
	nprimitive++;
	primitiveID->Fill(temp>>16);
	timestamp->Fill(((timestampH << 8) |timestampL )*25e-9);
      }
    }
    printf("writing the output file");

    /////IN CASE AN OFFSET IS NEEDED BEFORE THE FIRST PRIMITIVE (FOR EXAMPLE TO ALIGN DONWSCALINGS)
      //    uint32_t zeros = 0;
    //uint32_t* zptr = &zeros;
//    //OFFSET:
//    fwrite(zptr,sizeof(uint32_t),1,binOut);
//    fwrite(zptr,sizeof(uint32_t),1,binOut);
//    fwrite(zptr,sizeof(uint32_t),1,binOut);
//    fwrite(zptr,sizeof(uint32_t),1,binOut);
//    fwrite(zptr,sizeof(uint32_t),1,binOut);
//    fwrite(zptr,sizeof(uint32_t),1,binOut);
    fwrite(&data[0],sizeof(uint32_t), data.size(), binOut);

    fclose(binOut);
    printf("Memory bin file has been written");
    TCanvas *c1 = new TCanvas();
    primitiveID->Draw();
    TCanvas *c2 = new TCanvas();
    timestamp->Draw();
    printf("\n%u 32-bit words\nTotal bin size %.2f MB\n", addr/4, (addr)/1e6);
    printf("Total number of primitives written in the file: %d\n",nprimitive);
    printf("number of unsorted primitives: %d\n: ", unsorted);
    return 0;
  }
  else {
    printf("Can't find %s tree",branchName.Data());
    return -1;
  }
}
