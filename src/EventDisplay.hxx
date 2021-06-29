#ifndef MyGuiApplication_h_
#define MyGuiApplication_h_

#include <iostream>
#include <TApplication.h>
#include <TRint.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TGTextEntry.h>
#include <TGTextView.h>
#include <TGClient.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TGLayout.h>
#include <TGWindow.h>
#include <TGLabel.h>
#include <TString.h>
#include <TGComboBox.h>
#include <Getline.h>
#include <TFile.h>
#include <TString.h>
#include "TBox.h"
#include "TGNumberEntry.h"
#include "TGTextEntry.h"
#include <TRootEmbeddedCanvas.h>
#include "TH1F.h"
#include "TH2F.h"
#include "Interface.hxx"
#include "TThread.h"


class IDList
{

private:
    Int_t fID; //create widget Id(s)

public:
    IDList() : fID(0) {}
    ~IDList() {}
    Int_t GetUnID(void) { return ++fID; }
};

class EventDisplay : public TGMainFrame
{

private:
    /// I/O params
    // file
    Int_t eventID = 0;

    // WF plotter params
    TH2F* MM;
    TH1F* WF[9];
    Int_t WFstart = 100;
    Int_t WFend = 260;
    Int_t fWF_ampl_min = -300;
    Int_t fWF_ampl_max = 4000;
    int Nevents;
    Int_t _padAmpl[geom::nPadx][geom::nPady][n::samples];

    // GUI
    TCanvas *f_ED_canvas;
    TCanvas *f_WF_canvas;
    TGHorizontalFrame *fhf;
    TRootEmbeddedCanvas  *fED;
    TRootEmbeddedCanvas  *fWF;

    TGTextButton* fButtonExit;
    TGTextButton* fButtonDraw;
    TGTextButton* fNextEvent;
    TGTextButton* fPrevEvent;
    TGTextButton* fStartMon;
    TGTextButton* fEndMon;

    TGNumberEntry* fNumber;
    TGTextEntry* fEntry;

    bool _use_root;
    bool _use_aqs;

    // bool doMonitoring;

    InterfaceROOT* _interface_root;
    InterfaceAQS* _interface_aqs;

    TThread *fMonitoringThread;

    TBox fbox;
public:
    EventDisplay(const TGWindow *p, UInt_t w, UInt_t h, TString name);
    virtual ~EventDisplay();

public:
    void SelectFile();
    void DoDraw();
    void DoExit();
    void DoEnteredCommand();
    void NextEvent();
    void PrevEvent();
    void UpdateNumber();
    void StartMonitoring();
    void EndMonitoring();
    void ClickEventOnGraph(Int_t event, Int_t px, Int_t py, TObject *selected);
    static void *Monitoring(void * ptr);

    ClassDef(EventDisplay, 1);
};

#endif