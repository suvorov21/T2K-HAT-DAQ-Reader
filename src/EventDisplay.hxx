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
    /// Input interfaces
    std::shared_ptr<InterfaceBase> _interface;

    // WF plotter params
    TH2F* MM;
    TH1F* WF[9];
    /// Time bounds for WF plotter
    Int_t WFstart = 0;
    Int_t WFend = 500;
    /// amplitude bounds for the WF histoes
    Int_t fWF_ampl_min = -300;
    Int_t fWF_ampl_max = 4000;
    /// data representation
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
    TGTextButton* fGoToEnd;
    TGTextButton* fLookThrough;
    TGTextButton* fPallete;
    TGNumberEntry* fNumber;
    TGTextEntry* fEntry;
    TBox fbox;

    /// Special Paul's palette
    bool _rb_palette = false;

    /// Accumulation canvas and histoes
    TCanvas* _total_canv;
    TH2F* _accum_ed;
    TH1F* _accum_time;

    /// WF display vars
    bool _clicked = false;
    int _x_clicked;
    int _y_clicked;

    /// Thread for constant monitoring
    TThread *fMonitoringThread;

    /// Plot styking
    TStyle* _t2kstyle;

    /// verbosity level
    int _verbose;

public:
    EventDisplay(const TGWindow *p, UInt_t w, UInt_t h, TString name, int verbose);
    virtual ~EventDisplay();

public:
    /// Draw the particular event defined by eventID
    void DoDraw();
    /// Close the app
    void DoExit();
    /// Go to next event
    void NextEvent();
    /// Go to prev event
    void PrevEvent();
    /// Update the event number in the window
    void UpdateNumber();
    /// start the constant monitoring
    void StartMonitoring();
    /// Go to file end
    void GoToEnd();
    /// Show the WFs for the particular 3x3 pad region
    void ClickEventOnGraph(Int_t event, Int_t px, Int_t py, TObject *selected);
    /// Function that actually plots the WF
    void DrawWF();
    /// Do the constant monitoring each N microseconds
    static void *Monitoring(void * ptr);
    /// Scan through 100 events in a row
    static void *LookThrough(void *ptr);

    void LookThroughClick();

    void PaletteClick();

    TThread *fLookThread;

    /// total number of events in the file
    int Nevents;
    // Current event number
    Int_t eventID = 1;
    Int_t eventPrev = -1;

    ClassDef(EventDisplay, 1);
};

#endif