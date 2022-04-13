//
// Created by SERGEY SUVOROV on 13/04/2022.
//

#include "TTree.h"

#include "Output.hxx"

TString OutputBase::getFileName(std::string path, std::string file_in) {
    while (file_in.find('/') != string::npos)
        file_in = file_in.substr(file_in.find('/') + 1);
    file_in = file_in.substr(0, file_in.find('.'));
    return path + file_in + ".root";
}

void OutputArray::Initialise(const TString& fileName, bool useTracker) {
    _file = new TFile(fileName, "NEW");
    if (!_file->IsOpen()) {
        std::cerr << "ROOT file could not be opend." << std::endl;
        std::cerr << "File probably exists. Prevent overwriting" << std::endl;
        exit(1);
    }
    _t2k.loadMapping();
    _daq.loadDAQ();
    _tree = new TTree(treeName, "");
    _tree->Branch(branchName, &_padAmpl, Form("PadAmpl_[%i][%i][%i]/I", geom::nPadx, geom::nPady, n::samples));
    _tree->Branch("time_mid",    &_time_mid);
    _tree->Branch("time_msb",    &_time_msb);
    _tree->Branch("time_lsb",    &_time_lsb);

    if (useTracker)
        _tree->Branch("Tracker", &_trackerPos, Form("TrackerPos[8]/F"));
}

void OutputArray::AddEvent(TRawEvent* event) {
    _time_mid =  event->GetTimeMid();
    _time_msb =  event->GetTimeMsb();
    _time_lsb =  event->GetTimeLsb();
    memset(_padAmpl, 0, geom::nPadx * geom::nPady * n::samples * (sizeof(Int_t)));
    for (const auto& hit : event->GetHits()) {
        int x = _t2k.i(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
        int y = _t2k.j(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));

        auto v = hit->GetADCvector();
        for (auto t = 0; t < v.size(); ++t) {
            _padAmpl[x][y][hit->GetTime() + t] = v[t];
        }
    }
}

void OutputArray::Fill() {
    _tree->Fill();
}

void OutputArray::AddTrackerEvent(const std::vector<float>& TrackerPos) {
    for (float & data : _trackerPos)
        data = -999.;
    for (auto it = 0;  it < TrackerPos.size(); ++it)
        if (TrackerPos[it] > 0)
            _trackerPos[it] = TrackerPos[it];
}

void OutputArray::Finilise() {
    _tree->Write("", TObject::kOverwrite);
    _file->Write();
    _file->Close();
}



void OutputTRawEvent::Initialise(const TString& fileName, bool useTracker) {
    _file = new TFile(fileName, "NEW");
    if (!_file->IsOpen()) {
        std::cerr << "ROOT file could not be opend." << std::endl;
        std::cerr << "File probably exists. Prevent overwriting" << std::endl;
        exit(1);
    }
    _event = new TRawEvent();
    _tree = new TTree(treeName, "");
    _tree->Branch(branchName, &_event, 32000, 0);
    if (useTracker)
        _tree->Branch("Tracker", &_trackerPos, Form("TrackerPos[8]/F"));
}

void OutputTRawEvent::AddEvent(TRawEvent* event) {
    _event = event;
}

void OutputTRawEvent::Fill() {
    _tree->Fill();
    delete _event;
}

void OutputTRawEvent::AddTrackerEvent(const std::vector<float>& TrackerPos) {
    for (float & data : _trackerPos)
        data = -999.;
    for (auto it = 0;  it < TrackerPos.size(); ++it)
        if (TrackerPos[it] > 0)
            _trackerPos[it] = TrackerPos[it];
}

void OutputTRawEvent::Finilise() {
    _tree->Write("", TObject::kOverwrite);
    _file->Write();
    _file->Close();
}