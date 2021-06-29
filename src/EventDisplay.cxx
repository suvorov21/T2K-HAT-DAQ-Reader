

#include <TApplication.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TRootEmbeddedCanvas.h>
#include <TGFileBrowser.h>
#include "EventDisplay.hxx"
#include <TGFileDialog.h>
#include <TRootCanvas.h>
#import "TStyle.h"

#include "TThread.h"
#include <unistd.h>

#include <iostream>

ClassImp(EventDisplay);

bool doMonitoring;

EventDisplay::EventDisplay(const TGWindow *p, UInt_t w, UInt_t h, TString name)
    : TGMainFrame(p, w, h) {
    SetCleanup(kDeepCleanup);

    Connect("CloseWindow()", "EventDisplay", this, "DoExit()");
    DontCallClose();
    doMonitoring = false;
    _use_root = _use_aqs = false;

    MM = new TH2F("h", "", 38, -1., 37., 34, -1., 33.);
  for (auto i = 0; i < 9; ++i)
    WF[i] = new TH1F(Form("WF_%i", i), "", 511, 0., 511);

  if (name.EndsWith(".root")) {
      _use_root = true;
      _interface_root = new InterfaceROOT();
   } else if (name.EndsWith(".aqs")) {
      _interface_aqs = new InterfaceAQS();
      // work_interface = static_cast<InterfaceAQS*>(work_interface);
      _use_aqs = true;
   } else {
      std:cerr << "ERROR in monitor. Unknown file type." << std::endl;
      exit(1);
   }

   if (_use_root) {
      _interface_root->Initialise(name);
      Nevents = _interface_root->Scan();
    } else if (_use_aqs) {
      _interface_aqs->Initialise(name);
      Nevents = _interface_aqs->Scan();
    }


  // ini GUI
  TGHorizontalFrame* fMain = new TGHorizontalFrame(this, w, h);
  fED = new TRootEmbeddedCanvas("glec1", fMain, 700, 500);
  fWF = new TRootEmbeddedCanvas("glec2", fMain, 700, 500);

  fMain->AddFrame(fED, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));
  fMain->AddFrame(fWF, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
  TGHorizontalFrame *hfrm = new TGHorizontalFrame(this, 10, 10);

  AddFrame(fMain, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

  // Exit
  fButtonExit = new TGTextButton(hfrm, "        &Exit...        ", 3);
  fButtonExit->Connect("Clicked()" , "TApplication", gApplication, "Terminate()");
  hfrm->AddFrame(fButtonExit, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
   10, 10, 10, 10));

  // next event
  fNextEvent = new TGTextButton(hfrm, "        &Next        ", 3);
  fNextEvent->Connect("Clicked()" , "EventDisplay", this, "NextEvent()");
  hfrm->AddFrame(fNextEvent, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
   10, 10, 10, 10));

  // previous event
  fPrevEvent = new TGTextButton(hfrm, "        &Previous        ", 3);
  fPrevEvent->Connect("Clicked()" , "EventDisplay", this, "PrevEvent()");
  hfrm->AddFrame(fPrevEvent, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
   10, 10, 10, 10));

  // do draw
  fButtonDraw = new TGTextButton(hfrm, "        &Draw        ", 3);
  fButtonDraw->Connect("Clicked()" , "EventDisplay", this, "UpdateNumber()");
  hfrm->AddFrame(fButtonDraw, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
   10, 10, 10, 10));

  // start monitoring
  fStartMon = new TGTextButton(hfrm, "        &Start monitoring        ", 3);
  fStartMon->Connect("Clicked()" , "EventDisplay", this, "StartMonitoring()");
  hfrm->AddFrame(fStartMon, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
   10, 10, 10, 10));


  // start monitoring
  fEndMon = new TGTextButton(hfrm, "        &End monitoring        ", 3);
  fEndMon->Connect("Clicked()" , "EventDisplay", this, "EndMonitoring()");
  hfrm->AddFrame(fEndMon, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
   10, 10, 10, 10));


  fNumber = new TGNumberEntry(hfrm, 0, 9,999, TGNumberFormat::kNESInteger,
                                               TGNumberFormat::kNEANonNegative,
                                               TGNumberFormat::kNELLimitMinMax,
                                               0, 99999);
  // fEntry = new TGTextEntry(this);
  // hfrm->AddFrame(fEntry, new TGLayoutHints(kLHintsCenterX | kLHintsRight, 10, 10, 10, 10));
  hfrm->AddFrame(fNumber, new TGLayoutHints(kLHintsBottom | kLHintsRight, 10, 10, 10, 10));

  AddFrame(hfrm, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 5, 5, 5, 5));

  // What to clean up in destructor
  SetCleanup(kDeepCleanup);

   // Set a name to the main frame
  SetWindowName("Event display");
  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();

  fMonitoringThread = new TThread("monitoring", Monitoring, (void *)this);

  _total_canv = new TCanvas("Accumulation", "Accumulation", 800, 800, 700, 400);
  _total_canv->Divide(2);
  _accum_time = new TH1F("time", "Time", 511, 0., 511.);
  _accum_ed = new TH2F("accum_ed", "Accumulation", 38, -1., 37., 34, -1., 33.);
  _total_canv->cd(1);
  _accum_ed->Draw("colz");
  _total_canv->cd(2);
  _accum_time->Draw();
  _total_canv->Update();
};

EventDisplay::~EventDisplay()
{
    // Destructor.

    Cleanup();
}


void EventDisplay::DoExit()
{
    // Close application window.
    std::cout << "Exiting" << std::endl;
    gSystem->Unlink(fName.Data());
    gApplication->Terminate();
}

void EventDisplay::DoDraw()
{
    if (eventID >= Nevents) {
    std::cout << "EOF" << std::endl;
    --eventID;
    return;
  }
  // get canvas and connect to monitor
  f_ED_canvas = fED->GetCanvas();
  f_ED_canvas->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)","EventDisplay",this, "ClickEventOnGraph(Int_t,Int_t,Int_t,TObject*)");

  // f_ED_canvas->Connect("Pick(Int_t, Int_t, TObject*)", "MyMainFrame", this, "HandleTest(Int_t, Int_t, TObject*)");
  // f_ED_canvas->Connect("Pick(Int_t, Int_t, TObject*)", "MyMainFrame", this, "HandleTest(Int_t, Int_t, TObject*)");

  // read event
  if (_use_root)
    _interface_root->GetEvent(eventID, _padAmpl);
  else if (_use_aqs)
    _interface_aqs->GetEvent(eventID, _padAmpl);

  std::cout << "Event\t" << eventID << std::endl;
  MM->Reset();
  for (auto x = 0; x < geom::nPadx; ++x) {
    for (auto y = 0; y < geom::nPady; ++y) {
      auto max = 0;
      auto maxt = -1;
      for (auto t = 0; t < n::samples; ++t) {
        int Q = 0;
          Q = _padAmpl[x][y][t] - 250;

        if (Q > max) {
          max = Q;
          maxt = t;
        }
      } // over t
      if (max) {
        MM->Fill(x, y, max);
        _accum_ed->Fill(x, y, max);
        _accum_time->Fill(maxt);
      }

    }
  }
  f_ED_canvas->cd();
  gStyle->SetOptStat(0);
  MM->Draw("colz");
  // MM->GetXaxis()->SetNdivisions(38);
  // MM->GetXaxis()->SetLabelSize(0.025);
  // MM->GetYaxis()->SetNdivisions(36);
  gPad->SetGrid();
  f_ED_canvas->Update();

  _total_canv->cd(1);
  _accum_ed->Draw("colz");
  _total_canv->cd(2);
  _accum_time->Draw();
  _total_canv->Update();

}

void EventDisplay::NextEvent() {
  ++eventID;
  if (doMonitoring) {
    if (_use_root) {
        Nevents = _interface_root->Scan(Nevents-1, false);
      } else if (_use_aqs) {
        Nevents = _interface_aqs->Scan(Nevents-1, false);
      }
  }
  fNumber->SetIntNumber(eventID);
  DoDraw();
}

void EventDisplay::PrevEvent() {
  --eventID;
  fNumber->SetIntNumber(eventID);
  DoDraw();
}

void EventDisplay::UpdateNumber() {
  eventID = fNumber->GetNumberEntry()->GetIntNumber();
  DoDraw();
}

void EventDisplay::StartMonitoring() {
  doMonitoring = true;
  fMonitoringThread->Run();
}

void EventDisplay::EndMonitoring() {
  doMonitoring = false;
}




void EventDisplay::ClickEventOnGraph(Int_t event, Int_t px, Int_t py, TObject *selected)
{
  TCanvas *f_WF_canvas = (TCanvas *)gTQSender;
  if (event != 1)
    return;
  TString s = selected->GetObjectInfo(px,py);
  Ssiz_t first = 0, last;

  first = s.Index("=", 0);
  last  = s.Index(",", first);

  if (first == kNPOS || last == kNPOS)
    return;

  Int_t x = int(TString(s(first+1, last-1)).Atof());

  first = s.Index("=", last);
  last  = s.Index(",", first);

  if (first == kNPOS)
    return;

  Int_t y = int(TString(s(first+1, last-1)).Atof());

  //std::cout << x << "  " << y << std::endl;

  f_WF_canvas = fWF->GetCanvas();
  f_WF_canvas->Clear();
  f_WF_canvas->Divide(3, 3);
  for (auto i = 0; i < 9; ++i) {
    WF[i]->Reset();
    for (auto t_id = 0; t_id < 510; ++t_id) {
      if (x+1 > 35 || x-1 < 0 || y+1 > 31 || y-1 < 0)
        continue;
      int WF_signal = 0;
        WF_signal = _padAmpl[x-1+i%3][y+1-i/3][t_id] - 250;
      if (WF_signal > -250)
        WF[i]->SetBinContent(t_id, WF_signal);
      else
        WF[i]->SetBinContent(t_id, 0);
    }

    f_WF_canvas->cd(i+1);
    WF[i]->GetYaxis()->SetRangeUser(fWF_ampl_min, fWF_ampl_max);
    WF[i]->GetXaxis()->SetRangeUser(WFstart, WFend);
    WF[i]->Draw("hist");
    f_WF_canvas->Modified();
    f_WF_canvas->Update();
  }

  f_ED_canvas = fED->GetCanvas();
  f_ED_canvas->cd();

  fbox = TBox(x-1, y-1, x+2, y+2);
  fbox.SetFillStyle(0);
  fbox.SetLineColor(kRed);
  fbox.SetLineWidth(3);
  fbox.Draw();
  f_ED_canvas->Modified();
  f_ED_canvas->Update();

}

void *EventDisplay::Monitoring(void *ptr) {
    EventDisplay *ED = (EventDisplay *)ptr;
    while (doMonitoring) {
        usleep(400000);
        ED->NextEvent();
    }
}
