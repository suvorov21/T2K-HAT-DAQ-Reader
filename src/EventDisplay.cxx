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
#include "TStyle.h"
#include "SetT2KStyle.hxx"

#include "TThread.h"
#include <unistd.h>

#include <iostream>

ClassImp(EventDisplay);

bool doMonitoring;

//******************************************************************************
EventDisplay::EventDisplay(const TGWindow *p,
                           UInt_t w,
                           UInt_t h,
                           std::string name,
                           int verbose
                           ) : TGMainFrame(p, w, h) {
//******************************************************************************
  SetCleanup(kDeepCleanup);
  _verbose = verbose;

  TString localStyleName = "T2K";
  int localWhichStyle = 1;
  _t2kstyle = T2K().SetT2KStyle(localWhichStyle, localStyleName);
  gROOT->SetStyle(_t2kstyle->GetName());
  gROOT->ForceStyle();

  _daq.loadDAQ();
  _t2k.loadMapping();

  Connect("CloseWindow()", "EventDisplay", this, "DoExit()");
  DontCallClose();
  doMonitoring = false;

  MM = new TH2F("h", "", 38, -1., 37., 34, -1., 33.);
  for (auto i = 0; i < 9; ++i)
    WF[i] = new TH1F(Form("WF_%i", i), "", 511, 0., 511);

  // define the file type
  _interface = InterfaceFactory::get(name);

  std::cout << "Opening file " << name << std::endl;

  // read the events number
  if (!_interface->Initialise(name, verbose)) {
      std::cerr << "Interface initialisation fails. Exit" << std::endl;
      exit(1);
  }
  Nevents = _interface->Scan(-1, true, tmp);

  if (Nevents == 0) {
    std::cerr << "Empty file!" << std::endl;
    exit(1);
  }

  // ini GUI
  auto* fMain = new TGHorizontalFrame(this, w, h);
  fED = new TRootEmbeddedCanvas("glec1", fMain, 700, 500);
  fWF = new TRootEmbeddedCanvas("glec2", fMain, 700, 500);

  fMain->AddFrame(fED, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));
  fMain->AddFrame(fWF, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
  auto *hfrm = new TGHorizontalFrame(this, 10, 10);

  f_WF_canvas = fWF->GetCanvas();
  f_WF_canvas->Clear();
  f_WF_canvas->Divide(3, 3);

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

  fNumber = new TGNumberEntry(hfrm, 1, 15,999, TGNumberFormat::kNESInteger,
                              TGNumberFormat::kNEANonNegative,
                              TGNumberFormat::kNELLimitMinMax,
                              0, 99999);

  hfrm->AddFrame(fNumber, new TGLayoutHints(kLHintsBottom | kLHintsRight, 10, 10, 10, 10));

  // look through the events
  fLookThrough = new TGTextButton(hfrm, "        &Look through 50       ", 3);
  fLookThrough->Connect("Clicked()" , "EventDisplay", this, "LookThroughClick()");
  hfrm->AddFrame(fLookThrough, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
                                              10, 10, 10, 10));

  // shange the palette to Paul's favourite
  fTimeMode = new TGTextButton(hfrm, "        &Time Mode       ", 3);
  fTimeMode->Connect("Clicked()" , "EventDisplay", this, "TimeModeClick()");
  hfrm->AddFrame(fTimeMode, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
                                              10, 10, 10, 10));

  // shange the palette to Paul's favourite
  fPallete = new TGTextButton(hfrm, "        &Paul's button       ", 3);
  fPallete->Connect("Clicked()" , "EventDisplay", this, "PaletteClick()");
  hfrm->AddFrame(fPallete, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
                                              10, 10, 10, 10));
  // fGoToEnd = new TGTextButton(hfrm, "        &Go to file end        ", 3);
  // fGoToEnd->Connect("Clicked()" , "EventDisplay", this, "GoToEnd()");
  // hfrm->AddFrame(fGoToEnd, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
  //                                             10, 10, 10, 10));

  fWF_range = new TGTextButton(hfrm, "        &Apply       ", 3);
  fWF_range->Connect("Clicked()" , "EventDisplay", this, "ChangeWFrange()");
  hfrm->AddFrame(fWF_range, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
                                              10, 10, 10, 10));

  fWF_end = new TGNumberEntry(hfrm, 500, 15,999, TGNumberFormat::kNESInteger,
                              TGNumberFormat::kNEANonNegative,
                              TGNumberFormat::kNELLimitMinMax,
                              0, 99999);
  hfrm->AddFrame(fWF_end, new TGLayoutHints(kLHintsBottom | kLHintsRight, 10, 10, 10, 10));

  fWF_start = new TGNumberEntry(hfrm, 0, 15,999, TGNumberFormat::kNESInteger,
                              TGNumberFormat::kNEANonNegative,
                              TGNumberFormat::kNELLimitMinMax,
                              0, 99999);
  hfrm->AddFrame(fWF_start, new TGLayoutHints(kLHintsBottom | kLHintsRight, 10, 10, 10, 10));

  fLabel = new TGLabel(hfrm, "WF range");
  hfrm->AddFrame(fLabel, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
                                              10, 10, 10, 10));


  AddFrame(hfrm, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 5, 5, 5, 5));

  // What to clean up in destructor
  SetCleanup(kDeepCleanup);

   // Set a name to the main frame
  SetWindowName("Event display");
  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();

  fMonitoringThread = new TThread("monitoring", Monitoring, (void *)this);
  fLookThread = new TThread("lookthrough", LookThrough, (void *)this);

  _total_canv = new TCanvas("Accumulation", "Accumulation", 800, 800, 700, 400);
  _total_canv->Divide(2);
  _accum_time = new TH1F("time", "Time", 511, 0., 511.);
  _accum_ed = new TH2F("accum_ed", "Accumulation", 38, -1., 37., 34, -1., 33.);
  _total_canv->cd(1);
  _accum_ed->Draw("colz");
  _total_canv->cd(2);
  _accum_time->Draw();
  _total_canv->Update();

  _mmm_canvas = new TCanvas("Multiple MM", "Multiple MM", 1000, 600);
  _mmm_canvas->Divide(4, 2);
  for (auto i = 0; i < 8; ++i) {
    _mmm_canvas->cd(i + 1);
    _mm[i] = new TH2F(Form("MM_%i", i), Form("MM_%i", i), 38, -1., 37., 34, -1., 33.);;
    _mm[i]->Draw("colz");
  }

  _tracker_canv = new TCanvas("Tracker", "Tracker", 0, 800, 400, 400);
  _tracker_canv->Divide(2, 2);
  for (auto & tr : _tracker)
    tr = new TGraphErrors();

  std::cout << std::endl;
};

EventDisplay::~EventDisplay() {
  Cleanup();
}


//******************************************************************************
void EventDisplay::DoExit() {
//******************************************************************************
  // Close application window.
  std::cout << "Exiting" << std::endl;
  gSystem->Unlink(fName.Data());
  gApplication->Terminate();
}

//******************************************************************************
void EventDisplay::DoDraw() {
//******************************************************************************
  if (eventID >= Nevents) {
    std::cout << "EOF" << std::endl;
    eventID = Nevents - 1;
    fNumber->SetIntNumber(eventID);
    return;
  }
  // get canvas and connect to monitor
  f_ED_canvas = fED->GetCanvas();
  f_ED_canvas->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)","EventDisplay",this, "ClickEventOnGraph(Int_t,Int_t,Int_t,TObject*)");

  bool fill_gloabl = (eventPrev != eventID);

  // read event
  // WARNING due to some bug events MAY BE skipped qt the first read
  _event = _interface->GetEvent(eventID);

  std::cout << "\rEvent\t" << eventID << " from " << Nevents;
  std::cout << " in the file (" << _Nevents_run << " in run in total)" << std::flush;
  MM->Reset();
  for (auto i = 0; i < 8; ++i)
    _mm[i]->Reset();

  for (const auto& hit : _event->GetHits()) {
    auto wf = hit->GetADCvector();
    auto max = std::max_element(wf.cbegin(), wf.cend());
    if (*max == 0)
      continue;
    auto qMax = *max;
    auto maxt = hit->GetTime() + (max - wf.begin());
    auto x = _t2k.i(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
    auto y = _t2k.j(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
    if (hit->GetCard() == 0) {
      if (fIsTimeModeOn){
        MM->Fill(x, y, maxt);
      } else {
        MM->Fill(x, y, qMax);
      }
    }
    _mm[hit->GetCard()]->Fill(x, y, qMax);

    if (fill_gloabl) {
      _accum_ed->Fill(x, y, qMax);
      _accum_time->Fill(maxt);
      eventPrev = eventID;
    }
  }

	double minZ{std::nan("unset")}, maxZ{std::nan("unset")};
	for(auto& mmHist : _mm){
		if(minZ != minZ or minZ > mmHist->GetMinimum()) minZ = mmHist->GetMinimum();
		if(maxZ != maxZ or maxZ < mmHist->GetMaximum()) maxZ = mmHist->GetMaximum();
	}
	for(auto& mmHist : _mm){ mmHist->GetZaxis()->SetRangeUser(minZ, maxZ); }

  f_ED_canvas->cd();
  gStyle->SetOptStat(0);

  //gROOT->ForceStyle();
  _t2kstyle->SetPalette(fPaletteMM);
  gROOT->SetStyle(_t2kstyle->GetName());
  MM->Draw("colz");
  /** Whether to draw the pad borders */
  MM->GetXaxis()->SetNdivisions(38);
  MM->GetXaxis()->SetLabelSize(0.025);
  MM->GetYaxis()->SetLabelSize(0.025);
  MM->GetYaxis()->SetNdivisions(36);
  /** end of block*/

  _t2kstyle->SetPalette(fPaletteMM);
  gROOT->SetStyle(_t2kstyle->GetName());
  gPad->SetGrid();
  f_ED_canvas->Update();

  for (auto i = 0; i < 8; ++i) {
    _mmm_canvas->cd(i+1);
    _mm[i]->Draw("colz");
  }
  _mmm_canvas->Update();

  auto oldStyle = _rb_palette ? 1 : kBird;
  _t2kstyle->SetPalette(oldStyle);
  gROOT->SetStyle(_t2kstyle->GetName());
  _total_canv->cd(1);
  _accum_ed->Draw("colz");
  _total_canv->cd(2);
  _accum_time->Draw();
  _total_canv->Update();

  if (_interface->HasTracker()) {
    Float_t pos[8];
    _interface->GetTrackerEvent(eventID, pos);
    for (auto& tr : _tracker)
      tr->Set(0);

    for (auto i = 0; i < 4; ++i) {
      _tracker[i]->Set(0);
      _tracker[i]->SetPoint(0, pos[i*2], pos[i*2 + 1]);
      _tracker_canv->cd(i+1);
      gPad->DrawFrame(0, 0, 10, 10);
      _tracker[i]->Draw("p");
    }
    _tracker_canv->Update();
  }

  if (_clicked)
    DrawWF();

}

//******************************************************************************
void EventDisplay::NextEvent() {
//******************************************************************************
  ++eventID;
  // scan the rest of the file for new events
  if (doMonitoring)
    Nevents = _interface->Scan(Nevents-1, false, _Nevents_run);

  fNumber->SetIntNumber(eventID);
  DoDraw();
}

void EventDisplay::PrevEvent() {
  --eventID;
  if (eventID < 0)
    return;
  fNumber->SetIntNumber(eventID);
  DoDraw();
}

void EventDisplay::UpdateNumber() {
  eventID = fNumber->GetNumberEntry()->GetIntNumber();
  DoDraw();
}

void EventDisplay::StartMonitoring() {
  if (doMonitoring) {
    doMonitoring = false;
    fStartMon->SetText("        &Start monitoring        ");
  } else {
    doMonitoring = true;
    fMonitoringThread->Run();
    fStartMon->SetText("        &Stop monitoring        ");
  }
}

//******************************************************************************
void EventDisplay::ClickEventOnGraph(Int_t event,
                                     Int_t px,
                                     Int_t py,
                                     TObject *selected
                                     ) {
//******************************************************************************
  auto *f_WF_canvas = (TCanvas *)gTQSender;
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

  _x_clicked = x;
  _y_clicked = y;
  _clicked = true;

  f_WF_canvas = fWF->GetCanvas();
  // f_WF_canvas->Clear();
  // f_WF_canvas->Divide(3, 3);

  for (const auto& hit : _event->GetHits()) {
    if (x+1 > 35 || x-1 < 0 || y+1 > 31 || y-1 < 0)
      continue;

    auto x_i = _t2k.i(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
    auto y_i = _t2k.j(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));

    if (abs(x_i - _x_clicked) > 1 || abs(y_i - _y_clicked) > 1)
      continue;

    int i = (x_i - _x_clicked + 1) % 3 + (- y_i + _y_clicked + 1) * 3;
    if (i >= 0 && i < 9) {
      WF[i]->Reset();
      for (auto t = 0; t < hit->GetADCvector().size(); ++t) {
        auto ampl = hit->GetADCvector()[t]-250;
        WF[i]->SetBinContent(hit->GetTime() + t,  ampl > -249 ? ampl : 0);
      }
      f_WF_canvas->cd(i+1);
      WF[i]->GetYaxis()->SetRangeUser(fWF_ampl_min, fWF_ampl_max);
      WF[i]->GetXaxis()->SetRangeUser(WFstart, WFend);
      WF[i]->Draw("hist");
    }
  }

  f_WF_canvas->Modified();
  f_WF_canvas->Update();

  f_ED_canvas = fED->GetCanvas();
  f_ED_canvas->cd();

  fbox = TBox(x-1, y-1, x+2, y+2);
  fbox.SetFillStyle(0);
  fbox.SetLineColor(kRed);
  fbox.SetLineWidth(3);
  fbox.Draw();

  _t2kstyle->SetPalette(fPaletteMM);
  gROOT->SetStyle(_t2kstyle->GetName());
  f_ED_canvas->Modified();
  f_ED_canvas->Update();


}

//******************************************************************************
void EventDisplay::DrawWF() {
//******************************************************************************
  f_WF_canvas = fWF->GetCanvas();
  for (const auto& hit : _event->GetHits()) {
    if (_x_clicked+1 > 35 || _x_clicked-1 < 0 || _y_clicked+1 > 31 || _y_clicked-1 < 0)
      continue;

    auto x_i = _t2k.i(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
    auto y_i = _t2k.j(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));

    if (abs(x_i - _x_clicked) > 1 || abs(y_i - _y_clicked) > 1)
      continue;

    int i = (x_i - _x_clicked + 1) % 3 + (- y_i + _y_clicked + 1) * 3;
    if (i >= 0 && i < 9) {
      WF[i]->Reset();
      for (auto t = 0; t < hit->GetADCvector().size(); ++t) {
        auto ampl = hit->GetADCvector()[t]-250;
        WF[i]->SetBinContent(hit->GetTime() + t, ampl > -249 ? ampl : 0);
      }
      f_WF_canvas->cd(i + 1);
      WF[i]->GetYaxis()->SetRangeUser(fWF_ampl_min, fWF_ampl_max);
      WF[i]->GetXaxis()->SetRangeUser(WFstart, WFend);
      WF[i]->Draw("hist");
      f_WF_canvas->Modified();
      f_WF_canvas->Update();
    }
  }

  f_ED_canvas = fED->GetCanvas();
  f_ED_canvas->cd();

  fbox = TBox(_x_clicked-1, _y_clicked-1, _x_clicked+2, _y_clicked+2);
  fbox.SetFillStyle(0);
  fbox.SetLineColor(kRed);
  fbox.SetLineWidth(3);
  fbox.Draw();

  _t2kstyle->SetPalette(fPaletteMM);
  gROOT->SetStyle(_t2kstyle->GetName());
  f_ED_canvas->Modified();
  f_ED_canvas->Update();

}


//******************************************************************************
void EventDisplay::GoToEnd() {
//******************************************************************************
  eventID = Nevents - 3;
  NextEvent();
}

void EventDisplay::TimeModeClick() {
  if (!fIsTimeModeOn){
    std::cout << "Setting to time mode" << std::endl;
    fPaletteMM = kInvertedDarkBodyRadiator;
    //_t2kstyle->SetPalette(kInvertedDarkBodyRadiator);
    //gROOT->SetStyle(_t2kstyle->GetName());
    //gROOT->ForceStyle();
    fIsTimeModeOn = true;
  }
  else {
    std::cout << "Setting to charge mode" << std::endl;
    fPaletteMM = kBird;
    _t2kstyle->SetPalette(kBird);
    gROOT->SetStyle(_t2kstyle->GetName());
    gROOT->ForceStyle();
    fIsTimeModeOn = false;
  }
}


//******************************************************************************
void EventDisplay::PaletteClick() {
//******************************************************************************
  if (!_rb_palette) {
    _t2kstyle->SetPalette(1);
    gROOT->SetStyle(_t2kstyle->GetName());
    gROOT->ForceStyle();
    _rb_palette = true;
  } else {
    _t2kstyle->SetPalette(kBird);
    gROOT->SetStyle(_t2kstyle->GetName());
    gROOT->ForceStyle();
    _rb_palette = false;
  }
}

//******************************************************************************
void EventDisplay::ChangeWFrange() {
//******************************************************************************
  WFstart = int(fWF_start->GetNumber());
  WFend = int(fWF_end->GetNumber());
}

//******************************************************************************
void *EventDisplay::Monitoring(void *ptr) {
//******************************************************************************
  auto *ED = (EventDisplay *)ptr;
  while (doMonitoring) {
    usleep(400000);
    ED->eventID = ED->Nevents - 7;
    ED->NextEvent();
  }
  return nullptr;
}

void EventDisplay::LookThroughClick(){
  fLookThread->Run();
}

//******************************************************************************
void *EventDisplay::LookThrough(void *ptr) {
//******************************************************************************
  auto *ED = (EventDisplay *)ptr;
    for (auto i = 0; i < 50; ++i) {
      ED->NextEvent();
      usleep(100000);
    }
  ED->fLookThread->Kill();
  return nullptr;
}
