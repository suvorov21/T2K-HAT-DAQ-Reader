//
// Created by SERGEY SUVOROV on 13/04/2022.
//

#include "TTree.h"

#include "Output.hxx"

TString OutputBase::getFileName(const std::string& path, const std::string& file_in) {
    auto fileName = file_in;
    while (fileName.find('/') != string::npos)
        fileName = fileName.substr(fileName.find('/') + 1);
    fileName = fileName.substr(0, fileName.find('.'));
    return path + fileName + ".root";
}

void OutputBase::SetCard(int card) {
    _card = card;
}

void OutputBase::Fill() {
    _tree->Fill();
    delete _event;
}

void OutputArray::Initialise(const TString& fileName, bool useTracker) {
    _file = TFile::Open(fileName, "NEW");
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
    _event = event;
    _time_mid =  event->GetTimeMid();
    _time_msb =  event->GetTimeMsb();
    _time_lsb =  event->GetTimeLsb();
    memset(_padAmpl, 0, geom::nPadx * geom::nPady * n::samples * (sizeof(Int_t)));
    for (const auto& hit : event->GetHits()) {
        // doesn't fill the array if the particular card is required
        if (_card >= 0 && _card != hit->GetCard()) {
            continue;
        }
        int x = _t2k.i(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
        int y = _t2k.j(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));

        auto v = hit->GetADCvector();
        for (auto t = 0; t < v.size(); ++t) {
            _padAmpl[x][y][hit->GetTime() + t] = v[t];
        }
    }
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
    _file = TFile::Open(fileName, "NEW");
    if (!_file->IsOpen()) {
        std::cerr << "ROOT file could not be opened." << std::endl;
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


///////////////////////////


void OutputText::Initialise(const TString& fileName, bool useTracker) {

    _t2k.loadMapping();
    _daq.loadDAQ();

    _fOutputFile.open(fileName);
    if (!_fOutputFile.is_open()) {
        std::cerr << "Text file could not be opened." << std::endl;
        std::cerr << "File probably exists. Prevent overwriting" << std::endl;
        exit(1);
    }
    _event = new TRawEvent();
}

void OutputText::AddEvent(TRawEvent* event) {
    _event = event;
}

void OutputText::AddTrackerEvent(const std::vector<float>& TrackerPos) {
//    for (float & data : _trackerPos)
//        data = -999.;
//    for (auto it = 0;  it < TrackerPos.size(); ++it)
//        if (TrackerPos[it] > 0)
//            _trackerPos[it] = TrackerPos[it];
}

void OutputText::Fill(){
    int eventID = _event->GetID();

    for (const auto& hit : _event->GetHits()) {
        // doesn't fill the array if the particular card is required
//        if (_card >= 0 && _card != hit->GetCard()) {
//            continue;
//        }
        int x = _t2k.i(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));
        int y = _t2k.j(hit->GetChip() / n::chips, hit->GetChip() % n::chips, _daq.connector(hit->GetChannel()));

        auto v = hit->GetADCvector();
        _fOutputFile << "i " << eventID << " card " << hit->GetCard() << " px " << x << " py " << y << " t " << hit->GetTime() << " nslot " << v.size() << "\n";
        for (auto t = 0; t < v.size(); ++t) {
            _fOutputFile << v[t] << "\n";
        }
    }
}

void OutputText::Finilise() {
    _fOutputFile.close();
}
