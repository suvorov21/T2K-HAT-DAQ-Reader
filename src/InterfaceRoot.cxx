//
// Created by SERGEY SUVOROV on 29/08/2022.
//

#include "InterfaceRoot.hxx"


//******************************************************************************
bool InterfaceROOT::Initialise(const std::string& file_name, int verbose) {
//******************************************************************************
    std::cout << "Initialise ROOT interface" << std::endl;
    _verbose = verbose;
    _file_in = new TFile(file_name.c_str());
    _tree_in = (TTree*)_file_in->Get("tree");
    _use511 = false;

    _t2k.loadMapping();

    TString branch_name = _tree_in->GetBranch("PadAmpl")->GetTitle();

    if (branch_name.Contains("[510]")) {
        _tree_in->SetBranchAddress("PadAmpl", _padAmpl);
    } else if (branch_name.Contains("[511]")) {
        _use511 = true;
        _tree_in->SetBranchAddress("PadAmpl", _padAmpl_511);
    } else {
        std::cerr << "ERROR in InterfaceROOT::Initialise()" << std::endl;
        exit(1);
    }

    if (_tree_in->GetBranch("Tracker")) {
        _has_tracker = true;
        std::cout << "Has tracker" << std::endl;
        _tree_in->SetBranchAddress("Tracker", _pos);
    }
    std::cout << "Input read" << std::endl;

    return true;
}

//******************************************************************************
uint64_t InterfaceROOT::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
    Nevents_run = _tree_in->GetEntries();
    return _tree_in->GetEntries();
}

//******************************************************************************
TRawEvent* InterfaceROOT::GetEvent(long int id) {
//******************************************************************************
    _tree_in->GetEntry(id);
    auto event = new TRawEvent(id);
    event->SetTime(_time_mid, _time_msb, _time_lsb);

    for (int i = 0; i < geom::nPadx; ++i) {
        for (int j = 0; j < geom::nPady; ++j) {
            auto hit = new TRawHit();
            hit->SetCard(0);
            auto elec = _t2k.getElectronics(i, j);
            hit->SetChip(elec.first);
            hit->SetChannel(elec.second);
            hit->ResetWF();
            auto max = 0;
            for (int t = 0; t < n::samples; ++t) {
                auto ampl = _use511 ? _padAmpl_511[i][j][t] : _padAmpl[i][j][t];
                hit->SetADCunit(t, ampl);
                if (ampl > max)
                    max = ampl;
            }
            if (max > 0) {
                hit->ShrinkWF();
                event->AddHit(hit);
            } else {
                delete hit;
            }
        }
    }
    return event;
}

void InterfaceROOT::GetTrackerEvent(long int id, Float_t* pos) {
    _tree_in->GetEntry(id);
    for (int i = 0; i < 8; ++i)
        pos[i] = _pos[i];
}

