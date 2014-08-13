//
// Author: Son Hoang 04/14/2013

// This macro scan frames generated from Pixelman library
// To run it do either:
// .x frameScan.C
// .x frameScan.C++

#include <TGClient.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TGLabel.h>
#include <TString.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <TSystem.h>
#include <TCanvas.h>


#include <TRootEmbeddedCanvas.h>
#include <TH2.h>
#include <TGTextEntry.h>

using namespace std;

class MyMainFrame : public TGMainFrame {

private:
	TRootEmbeddedCanvas *fCanvas;
	TH2I *fHistogram;
	TGTextEntry *fTextDir, *fTextFrame, *fTextAcq;
	TGCompositeFrame *fCframe, *fCframe1, *fCframe2;

	TGTextButton     *fDir, *fExit, *fCan, *fScan;

public:
	MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
	virtual ~MyMainFrame();
	// slots
	void ChangeDir();
	void Scan();
	void push_back_nbytes(unsigned int * val, char * bytes, Int_t nbytes);
	ClassDef(MyMainFrame, 0)
};

ClassImp(MyMainFrame)

void MyMainFrame::push_back_nbytes(unsigned int * val, char * bytes, Int_t nbytes) {    
	*val &= 0x00000000;
	unsigned int tempVal;

	for(int idx = nbytes-1 ; idx >= 0 ; idx--) {

		// Get the byte
		tempVal = 0x0;
		tempVal ^= bytes[idx];
		// Clean up to have info for only one byte
		tempVal &= 0x000000ff;
		// Switch it to the right place
		for(int sw = 0 ; sw < idx ; sw++){
			tempVal = tempVal << 8;
		}
		// XOR the value
		*val ^= tempVal;
	}
}

void MyMainFrame::ChangeDir()
{
	// Slot connected to the Clicked() signal.
	printf("change Dir");
}

void MyMainFrame::Scan()
{

	int frames=atoi(fTextFrame->GetText());
	float acq=atof(fTextAcq->GetText());
	printf("%d", frames);
	printf("%f",acq);
	TString dir=fTextDir->GetText(), file=dir;
	int i=0;
	TString strFrame=""; strFrame+=(frames-1);
	while(i<frames){
		TString strI=""; strI+=i;
		for(int ii=0; ii<strFrame.Length()-strI.Length(); ii++) strI="0"+strI;
		file=dir+strI;
		printf(file.Data());printf("\n");
		ifstream f(file.Data());
		fHistogram->Reset();
		//    for(int j=0; j<256; j++){
		//        for(int k=0; k<256; k++){
		//            int ToT;
		//            f>>ToT;
		//            if(ToT>0) fHistogram->Fill(j,k,ToT);
		//       }
		//   }

		unsigned int x = 0x0, y = 0x0, counts = 0x0;
		//       ofstream f("testbinary.txt");
		while (f.good()) {
			// read X, 32 bits
			char tempByte[8];
			x = 0x0;
			f.get(tempByte[0]);
			f.get(tempByte[1]);
			f.get(tempByte[2]);
			f.get(tempByte[3]);
			push_back_nbytes(&x, tempByte, 4);

			// read Y, 32 bits
			y = 0x0;
			f.get(tempByte[0]);
			f.get(tempByte[1]);
			f.get(tempByte[2]);
			f.get(tempByte[3]);
			push_back_nbytes(&y, tempByte, 4);

			// Read Counts, 32 bits
			counts = 0x0;
			f.get(tempByte[0]);
			f.get(tempByte[1]);
			//     f.get(tempByte[2]);
			//      f.get(tempByte[3]);
			push_back_nbytes(&counts, tempByte, 2);

			fHistogram->Fill(x,y,counts);
		}
		f.close();
		TCanvas *fc=fCanvas->GetCanvas();
		fc->cd();
		fHistogram->Draw("colz");
		fc->Update();
		gSystem->Sleep(acq);
		i++;
	}
}

MyMainFrame::MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h) :
		  TGMainFrame(p, w, h)
{
	fCanvas = new TRootEmbeddedCanvas("Dcanvas",this,500,500);
	AddFrame(fCanvas, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));
	fHistogram = new TH2I("", "", 256, 0, 256, 256, 0, 256);
	fHistogram->SetStats(false);
	// Create a horizontal frame containing buttons
	fCframe = new TGCompositeFrame(this, 800, 600, kHorizontalFrame|kFixedWidth);

	fDir = new TGTextButton(fCframe, "  &Dir  ");
	fDir->Connect("Clicked()", "MyMainFrame", this, "ChangeDir()");
	fDir->SetWidth(160);
	fCframe->AddFrame(fDir, new TGLayoutHints(kLHintsTop, 3, 2, 2, 2));
	fDir->SetToolTipText("Click to change directory");

	fTextDir = new TGTextEntry(fCframe,new TGTextBuffer(500));
	fTextDir->SetWidth(500);
	fCframe->AddFrame(fTextDir, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));
	AddFrame(fCframe, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

	fCframe1 = new TGCompositeFrame(this, 800, 600, kHorizontalFrame|kFixedWidth);
	fCframe1->AddFrame(new TGLabel(fCframe1, "NFrame"), new TGLayoutHints(kLHintsTop, 3, 2, 2, 2));
	fTextFrame = new TGTextEntry(fCframe1,new TGTextBuffer(500));
	fTextFrame->SetWidth(500);
	fCframe1->AddFrame(fTextFrame, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));
	AddFrame(fCframe1, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

	fCframe2 = new TGCompositeFrame(this, 800, 600, kHorizontalFrame|kFixedWidth);
	fCframe2->AddFrame(new TGLabel(fCframe2, "  Acq  "), new TGLayoutHints(kLHintsTop, 3, 2, 2, 2));
	fTextAcq = new TGTextEntry(fCframe2,new TGTextBuffer(500));
	fTextAcq->SetWidth(500);
	fCframe2->AddFrame(fTextAcq, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));
	AddFrame(fCframe2, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

	fScan = new TGTextButton(this, "&Scan ");
	fScan->Connect("Clicked()", "MyMainFrame", this, "Scan()");
	AddFrame(fScan, new TGLayoutHints(kLHintsTop | kLHintsExpandX,5,5,2,2));


	fExit = new TGTextButton(this, "&Exit ","gApplication->Terminate(0)");
	AddFrame(fExit, new TGLayoutHints(kLHintsTop | kLHintsExpandX,5,5,2,2));

	SetWindowName("Frame Scan");

	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
}


MyMainFrame::~MyMainFrame()
{
	// Clean up all widgets, frames and layouthints that were used
	fCframe->Cleanup();
	Cleanup();
}


void frameScan() {
//int main() {
	// Popup the GUI...
	new MyMainFrame(gClient->GetRoot(), 800, 600);
return 0;
}
