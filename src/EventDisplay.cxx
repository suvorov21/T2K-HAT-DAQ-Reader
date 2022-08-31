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
  Nevents = _interface->Scan(-1, true, _nEvents_run);

  if (Nevents == 0) {
    std::cerr << "Empty file!" << std::endl;
    exit(1);
  }
  else
  {
    std::cout << "Found " << Nevents << " events in file" << std::endl;
  }

  auto *fMain = new TGVerticalFrame(this, w, h);

  // ####################################
  // Navigation
  // ####################################
  auto *navGroup = new TGGroupFrame(this, "Navigation", kVerticalFrame);

  // ****************************
  // Back - next line
  auto backNextFrame = new TGHorizontalFrame(this);

  // previous event
  fPrevEvent = new TGTextButton(backNextFrame, "  & < Back ", 3);
  fPrevEvent->Connect("Clicked()", "EventDisplay", this, "PrevEvent()");
  backNextFrame->AddFrame(fPrevEvent, new TGLayoutHints(kLHintsLeft,
                                                        5, 0, 10, 0));

  // next event
  fNextEvent = new TGTextButton(backNextFrame, "  &Next > ", 3);
  fNextEvent->Connect("Clicked()", "EventDisplay", this, "NextEvent()");
  backNextFrame->AddFrame(fNextEvent, new TGLayoutHints(kLHintsLeft,
                                                        5, 0, 10, 0));

  navGroup->AddFrame(backNextFrame, new TGLayoutHints(kLHintsLeft,
                                                      5, 0, 10, 0));

  // ****************************
  // GoTo frame
  auto *goToFrame = new TGHorizontalFrame(this);

  // event number display
  fNumber = new TGNumberEntry(goToFrame, 1, 10, 999, TGNumberFormat::kNESInteger,
                              TGNumberFormat::kNEANonNegative,
                              TGNumberFormat::kNELLimitMinMax,
                              0, 99999);

  goToFrame->AddFrame(fNumber, new TGLayoutHints(kLHintsLeft,
                                                 5, 0, 0, 0));

  // do draw
  fButtonDraw = new TGTextButton(goToFrame, "  &Draw ", 3);
  fButtonDraw->Connect("Clicked()", "EventDisplay", this, "UpdateNumber()");
  goToFrame->AddFrame(fButtonDraw, new TGLayoutHints(kLHintsLeft,
                                                     5, 0, 0, 0));

  navGroup->AddFrame(goToFrame, new TGLayoutHints(kLHintsLeft,
                                                  5, 0, 10, 0));

  // ****************************
  // Look through 50
  fLookThrough = new TGTextButton(navGroup, "  &Look through 50 ", 3);
  fLookThrough->Connect("Clicked()", "EventDisplay", this, "LookThroughClick()");
  navGroup->AddFrame(fLookThrough, new TGLayoutHints(kLHintsLeft,
                                                     0, 0, 0, 0));

  // Monitoring
  fStartMon = new TGTextButton(navGroup, "  &Start monitoring ", 3);
  fStartMon->Connect("Clicked()", "EventDisplay", this, "StartMonitoring()");
  navGroup->AddFrame(fStartMon, new TGLayoutHints(kLHintsLeft,
                                                  0, 0, 10, 0));

  fMain->AddFrame(navGroup, new TGLayoutHints(kLHintsLeft,
                                              10, 0, 10, 0));

  // ####################################
  // Canvas control
  // ####################################
  auto *windowGroup = new TGGroupFrame(this, "Canvases", kVerticalFrame);
  // Show time along Z axis
  fTimeMode = new TGTextButton(windowGroup, " &Time Mode  ", 3);
  fTimeMode->Connect("Clicked()" , "EventDisplay", this, "TimeModeClick()");
  windowGroup->AddFrame(fTimeMode, new TGLayoutHints(kLHintsLeft,
                                                     5, 0, 10, 0));

  // Show Z-X view
  fzxView = new TGTextButton(windowGroup, " &Z-X view  ", 3);
  fzxView->Connect("Clicked()" , "EventDisplay", this, "ZxModeClick()");
  windowGroup->AddFrame(fzxView, new TGLayoutHints(kLHintsLeft,
                                                   5, 0, 10, 0));

  // Show Z-Y view
  fzyView = new TGTextButton(windowGroup, " &Z-Y view  ", 3);
  fzyView->Connect("Clicked()", "EventDisplay", this, "ZyModeClick()");
  windowGroup->AddFrame(fzyView, new TGLayoutHints(kLHintsLeft,
                                                   5, 0, 10, 0));

  // Show 3D view
  ftdView = new TGTextButton(windowGroup, " &3D view  ", 3);
  ftdView->Connect("Clicked()", "EventDisplay", this, "TdModeClick()");
  windowGroup->AddFrame(ftdView, new TGLayoutHints(kLHintsLeft,
                                                   5, 0, 10, 0));

  // Show charge accumulation
  fQaccumMode = new TGTextButton(windowGroup, " &Charge accumulation  ", 3);
  fQaccumMode->Connect("Clicked()" , "EventDisplay", this, "ChargeAccumClicked()");
  windowGroup->AddFrame(fQaccumMode, new TGLayoutHints(kLHintsLeft,
                                                     5, 0, 10, 0));

  // Show time accumulation
  fTaccumMode = new TGTextButton(windowGroup, " &Time accumulation  ", 3);
  fTaccumMode->Connect("Clicked()" , "EventDisplay", this, "TimeAccumClicked()");
  windowGroup->AddFrame(fTaccumMode, new TGLayoutHints(kLHintsLeft,
                                                       5, 0, 10, 0));

  // WF explorer
  fWfExplorer = new TGTextButton(windowGroup, " &WF explorer  ", 3);
  fWfExplorer->Connect("Clicked()" , "EventDisplay", this, "WfExplorerClicked()");
  windowGroup->AddFrame(fWfExplorer, new TGLayoutHints(kLHintsLeft,
                                                       5, 0, 10, 0));

  fMain->AddFrame(windowGroup, new TGLayoutHints(kLHintsLeft,
                                                 10, 0, 10, 0));

  // ####################################
  // Exit
  // ####################################
  fButtonExit = new TGTextButton(fMain, "  &Exit... ", 3);
  fButtonExit->Connect("Clicked()", "TApplication", gApplication, "Terminate()");
  fMain->AddFrame(fButtonExit, new TGLayoutHints(kLHintsCenterX,
                                                0, 0, 10, 10));

  AddFrame(fMain);
//  fWF_range = new TGTextButton(hfrm, "        &Apply       ", 3);
//  fWF_range->Connect("Clicked()" , "EventDisplay", this, "ChangeWFrange()");
//  hfrm->AddFrame(fWF_range, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
//                                              10, 10, 10, 10));
//
//  fWF_end = new TGNumberEntry(hfrm, 500, 15,999, TGNumberFormat::kNESInteger,
//                              TGNumberFormat::kNEANonNegative,
//                              TGNumberFormat::kNELLimitMinMax,
//                              0, 99999);
//  hfrm->AddFrame(fWF_end, new TGLayoutHints(kLHintsBottom | kLHintsRight, 10, 10, 10, 10));
//
//  fWF_start = new TGNumberEntry(hfrm, 0, 15,999, TGNumberFormat::kNESInteger,
//                              TGNumberFormat::kNEANonNegative,
//                              TGNumberFormat::kNELLimitMinMax,
//                              0, 99999);
//  hfrm->AddFrame(fWF_start, new TGLayoutHints(kLHintsBottom | kLHintsRight, 10, 10, 10, 10));
//
//  fLabel = new TGLabel(hfrm, "WF range");
//  hfrm->AddFrame(fLabel, new TGLayoutHints(kLHintsCenterX | kLHintsRight,
//                                              10, 10, 10, 10));
//
//
//  AddFrame(hfrm, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 5, 5, 5, 5));

  // What to clean up in destructor
  SetCleanup(kDeepCleanup);

   // Set a name to the main frame
  SetWindowName("Event display");
  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();

  // monitoring thread setup
  fMonitoringThread = new TThread("monitoring", Monitoring, (void *)this);
  fLookThread = new TThread("lookthrough", LookThrough, (void *)this);

  // Accumulation charge
  for (auto i = 0; i < 8; ++i) {
    _mmCharge[i] = new TH2F(Form("MMcharge_%i", i), Form("Charge accumulation MM %i", i), 38, -1., 37., 34, -1., 33.);
  }

  // Accumulation time
  for (auto i = 0; i < 8; ++i) {
    _mmTime[i] = new TH1F(Form("MMtime_%i", i), Form("Time accumulation MM %i", i), 511, 0., 511.);
  }

  // ZX charge
  for (auto i = 0; i < 8; ++i) {
    zsMm[i] = new TH2F(Form("MMzx_%i", i), Form("ZX MM %i", i), 511, 0., 511., 38, -1., 37.);
  }

  // ZY charge
  for (auto i = 0; i < 8; ++i) {
    zyMm[i] = new TH2F(Form("MMzy_%i", i), Form("ZY MM %i", i), 511, 0., 511., 34, -1., 33.);
  }
  // 3D
  for (auto i = 0; i < 8; ++i) {
    tdMm[i] = new TH3F(Form("MM3d_%i", i), Form("3D MM %i", i), 38, -1., 37., 34, -1., 33.,  511, 0., 511.);
  }

  // canvas for 8 MM view
  _mmm_canvas = new TCanvas("Multiple MM", "Multiple MM view", 300, 100, 1000, 600);
  _mmm_canvas->Divide(4, 2);
  for (auto i = 0; i < 8; ++i) {
    _mmm_canvas->cd(i + 1);
    _mm[i] = new TH2F(Form("MM_%i", i), Form("MM %i", i), 38, -1., 37., 34, -1., 33.);;
    _mm[i]->Draw("colz");
  }

//  _tracker_canv = new TCanvas("Tracker", "Tracker", 0, 800, 400, 400);
//  _tracker_canv->Divide(2, 2);
//  for (auto & tr : _tracker)
//    tr = new TGraphErrors();
//
//  std::cout << std::endl;
};

EventDisplay::~EventDisplay() {
  Cleanup();
}


//******************************************************************************
void EventDisplay::DoExit() {
//******************************************************************************
  // Close application window.
  std::cout << "Bye!" << std::endl;
  gSystem->Unlink(fName.Data());
  gApplication->Terminate();
}

//******************************************************************************
void EventDisplay::DoDraw() {
//******************************************************************************
//  _daq.printDAQ2Fec();
  if (eventID >= Nevents) {
    std::cout << "EOF" << std::endl;
    eventID = Nevents - 1;
    fNumber->SetIntNumber(eventID);
    return;
  }
   bool fill_gloabl = (eventPrev != eventID);

  // read event
  // WARNING due to some bug events MAY BE skipped qt the first read
  _event = _interface->GetEvent(eventID);

  std::cout << "\rEvent\t" << eventID << " from " << Nevents;
  std::cout << " in the file (" << _nEvents_run << " in run in total)" << std::flush;
  MM->Reset();
  for (auto i =  0; i < 8; ++i) {
    _mm[i]->Reset();
    zsMm[i]->Reset();
    zyMm[i]->Reset();
    tdMm[i]->Reset();
  }

  for (const auto& hit : _event->GetHits()) {
    auto wf = hit->GetADCvector();
    auto max = std::max_element(wf.cbegin(), wf.cend());
    if (*max == 0)
    {
        continue;
    }
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

    zsMm[hit->GetCard()]->Fill(maxt, x, qMax);
    zyMm[hit->GetCard()]->Fill(maxt, y, qMax);
    tdMm[hit->GetCard()]->Fill(x, y, maxt, qMax);

    if (fIsTimeModeOn) {
        _mm[hit->GetCard()]->Fill(x, y, maxt);
    } else {
        _mm[hit->GetCard()]->Fill(x, y, qMax);
    };

    // accumulation plots in case event is updated
    if (fill_gloabl) {
      eventPrev = eventID;
      _mmCharge[hit->GetCard()]->Fill(x, y, qMax);
      _mmTime[hit->GetCard()]->Fill(maxt);
    }
  }

	double minZ{std::nan("unset")}, maxZ{std::nan("unset")};
	for(auto& mmHist : _mm){
		if(minZ != minZ or minZ > mmHist->GetBinContent(mmHist->GetMinimumBin())) minZ = mmHist->GetBinContent(mmHist->GetMinimumBin());
		if(maxZ != maxZ or maxZ < mmHist->GetBinContent(mmHist->GetMaximumBin())) maxZ = mmHist->GetBinContent(mmHist->GetMaximumBin());
	}
	for(auto& mmHist : _mm){ mmHist->GetZaxis()->SetRangeUser(minZ, maxZ); }

  _t2kstyle->SetPalette(fPaletteMM);
  gROOT->SetStyle(_t2kstyle->GetName());

  for (auto i = 0; i < 8; ++i) {
    _mmm_canvas->cd(i+1);
    _mm[i]->Draw("colz");
  }
  _mmm_canvas->Update();

  auto oldStyle = _rb_palette ? 1 : kBird;
  _t2kstyle->SetPalette(oldStyle);
  gROOT->SetStyle(_t2kstyle->GetName());

  if (_chargeAccum) {
    for (auto i = 0; i < 8; ++i) {
      _chargeAccum->cd(i+1);
      _mmCharge[i]->Draw("colz");
    }
    _chargeAccum->Update();
  }

  if (_timeAccum) {
    for (auto i = 0; i < 8; ++i) {
      _timeAccum->cd(i+1);
      _mmTime[i]->Draw();
    }
    _timeAccum->Update();
  }

  if (zxView) {
    for (auto i = 0; i < 8; ++i) {
      zxView->cd(i+1);
      zsMm[i]->Draw("colz");
    }
    zxView->Update();
  }

  if (zyView) {
    for (auto i = 0; i < 8; ++i) {
      zyView->cd(i+1);
      zyMm[i]->Draw("colz");
    }
    zyView->Update();
  }

  if (tdView) {
    for (auto i = 0; i < 8; ++i) {
      tdView->cd(i+1);
      tdMm[i]->Draw("BOX");
    }
    tdView->Update();
  }

  if (f_ED_canvas) {
    edPad->cd();
    MM->Draw("colz");
    edPad->Update();
  }

//  if (_interface->HasTracker()) {
//    Float_t pos[8];
//    _interface->GetTrackerEvent(eventID, pos);
//    for (auto& tr : _tracker)
//      tr->Set(0);
//
//    for (auto i = 0; i < 4; ++i) {
//      _tracker[i]->Set(0);
//      _tracker[i]->SetPoint(0, pos[i*2], pos[i*2 + 1]);
//      _tracker_canv->cd(i+1);
//      gPad->DrawFrame(0, 0, 10, 10);
//      _tracker[i]->Draw("p");
//    }
//    _tracker_canv->Update();
//  }

  if (_clicked)
    DrawWF();
  if (_verbose > 0)
    std::cout << "\nDone here" << std::endl;
}

//******************************************************************************
void EventDisplay::NextEvent() {
//******************************************************************************
  ++eventID;
  // scan the rest of the file for new events
  if (doMonitoring)
    Nevents = _interface->Scan(Nevents-1, false, _nEvents_run);

  fNumber->SetIntNumber(eventID);
  DoDraw();
}

void EventDisplay::PrevEvent() {
  --eventID;
  if (eventID < 0) {
    eventID = 0;
    return;
  }
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

  auto *f_WF_canvas = (TPad *)gTQSender;
  if (event != 1)
    return;

  double xd = edPad->AbsPixeltoX(px);
  double yd = edPad->AbsPixeltoY(py);
  double x = edPad->PadtoX(xd);
  double y = edPad->PadtoY(yd);

  _x_clicked = x;
  _y_clicked = y;
  _clicked = true;

  if (_verbose > 0)
    std::cout << "\nClick on " << _x_clicked << "\t" << _y_clicked << std::endl;

  DrawWF();
}

//******************************************************************************
void EventDisplay::DrawWF() {
//******************************************************************************
  for (auto & i : WF) {
      i->Reset();
  }
  for (const auto& hit : _event->GetHits()) {
    if (_x_clicked+1 > 35 || _x_clicked-1 < 0 || _y_clicked+1 > 31 || _y_clicked-1 < 0)
      continue;

    auto x_i = _t2k.i(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
    auto y_i = _t2k.j(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));

    if (abs(x_i - _x_clicked) > 1 || abs(y_i - _y_clicked) > 1)
      continue;

    int i = (x_i - _x_clicked + 1) % 3 + (- y_i + _y_clicked + 1) * 3;
    if (i >= 0 && i < 9) {
      for (auto t = 0; t < hit->GetADCvector().size(); ++t) {
        auto ampl = hit->GetADCvector()[t]-250;
        WF[i]->SetBinContent(hit->GetTime() + t, ampl > -249 ? ampl : 0);
      }
      WF[i]->GetYaxis()->SetRangeUser(fWF_ampl_min, fWF_ampl_max);
      WF[i]->GetXaxis()->SetRangeUser(WFstart, WFend);
    }
  }

  for (auto i = 0; i < 9; ++i) {
    individualWf[i]->cd();
    WF[i]->Draw("hist");
    individualWf[i]->Update();
  }

  edPad->cd();

  fbox = TBox(_x_clicked-1.4, _y_clicked-1.4, _x_clicked+1.6, _y_clicked+1.6);
  fbox.SetFillStyle(0);
  fbox.SetLineColor(kRed);
  fbox.SetLineWidth(3);
  fbox.Draw();

  _t2kstyle->SetPalette(fPaletteMM);
  gROOT->SetStyle(_t2kstyle->GetName());
  edPad->Modified();
  edPad->Update();

}


//******************************************************************************
void EventDisplay::GoToEnd() {
//******************************************************************************
  eventID = Nevents - 3;
  NextEvent();
}

void EventDisplay::TimeModeClick() {
  if (!fIsTimeModeOn){
    if (_verbose > 0)
      std::cout << "Setting to time mode" << std::endl;
    fPaletteMM = kInvertedDarkBodyRadiator;
    //_t2kstyle->SetPalette(kInvertedDarkBodyRadiator);
    //gROOT->SetStyle(_t2kstyle->GetName());
    //gROOT->ForceStyle();
    fIsTimeModeOn = true;
  }
  else {
    if (_verbose > 0)
        std::cout << "Setting to charge mode" << std::endl;
    fPaletteMM = kBird;
    _t2kstyle->SetPalette(kBird);
    gROOT->SetStyle(_t2kstyle->GetName());
    gROOT->ForceStyle();
    fIsTimeModeOn = false;
  }
  DoDraw();
}

void EventDisplay::ZxModeClick() {
  if (zxView) {
    delete zxView;
    zxView = nullptr;
    return;
  }

  zxView = new TCanvas("Multiple MM ZX", "Z-X view", 600, 300, 1000, 600);
  zxView->Divide(4, 2);
  zxView->Draw();

  DoDraw();
}

void EventDisplay::ZyModeClick() {
  if (zyView) {
    delete zyView;
    zyView = nullptr;
    return;
  }

  zyView = new TCanvas("Multiple MM ZY", "Z-Y view", 600, 300, 1000, 600);
  zyView->Divide(4, 2);
  zyView->Draw();

  DoDraw();
}

void EventDisplay::TdModeClick() {
  if (tdView) {
    delete tdView;
    tdView = nullptr;
    return;
  }

  tdView = new TCanvas("Multiple MM 3D", "3D view", 600, 300, 1000, 600);
  tdView->Divide(4, 2);
  tdView->Draw();

  DoDraw();
}

void EventDisplay::ChargeAccumClicked() {
  if (_chargeAccum) {
    delete _chargeAccum;
    _chargeAccum = nullptr;
    return;
  }

  _chargeAccum = new TCanvas("Multiple MM charge", "Charge accumulation", 600, 300, 1000, 600);
  _chargeAccum->Divide(4, 2);
  _chargeAccum->Draw();

  DoDraw();
}

void EventDisplay::TimeAccumClicked() {
  if (_timeAccum) {
    delete _timeAccum;
    _timeAccum = nullptr;
    return;
  }

  _timeAccum = new TCanvas("Multiple MM time", "Time accumulation", 600, 300, 1000, 600);
  _timeAccum->Divide(4, 2);
  _timeAccum->Draw();

  DoDraw();
}

void EventDisplay::WfExplorerClicked() {
  if (f_ED_canvas) {
    _clicked = false;
    f_ED_canvas->Disconnect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)");
    delete f_ED_canvas;
    f_ED_canvas = nullptr;
    return;
  }

  f_ED_canvas = new TCanvas("WF", "WF explorer", 300, 300,1000, 600);

  edPad = new TPad("edPad", "edPad", 0., 0.2, 0.5, 0.80);
  edPad->Draw();
  edPad->cd();
  MM->Draw("colz");

  f_ED_canvas->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)",
                       "EventDisplay",
                       this,
                       "ClickEventOnGraph(Int_t,Int_t,Int_t,TObject*)");


  f_ED_canvas->cd();
  wfPad = new TPad("wfPad", "wfPad", 0.5, 0., 1., 1.);
  wfPad->Draw();

  for (auto i = 0; i < 9; ++i) {
    wfPad->cd();
    individualWf[i] = new TPad(Form("pad_%i", i), Form("pad_%i", i),
                               i % 3 * 0.33,
                               0.66 - 0.33*(i/3),
                               i % 3 * 0.33 + 0.33,
                               0.66 - 0.33*(i/3) + 0.33);

    individualWf[i]->Draw();
    individualWf[i]->cd();
    WF[i]->Reset();
  }

  DoDraw();
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
  DoDraw();
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
