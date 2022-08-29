//
// Created by SERGEY SUVOROV on 29/08/2022.
//

#ifndef DAQ_READER_SRC_INTERFACEROOT_HXX_
#define DAQ_READER_SRC_INTERFACEROOT_HXX_

#include "InterfaceBase.hxx"

/// ROOT file reader
class InterfaceROOT : public InterfaceBase {
 public:
    InterfaceROOT() {}
    ~InterfaceROOT() override = default;
    bool Initialise(const std::string &file_name, int verbose) override;
    uint64_t Scan(int start, bool refresh, int &Nevents_run) override;
    TRawEvent *GetEvent(long int id) override;
    void GetTrackerEvent(long int id, Float_t *pos) override;

 private:
    TFile *_file_in;
    TTree *_tree_in;
    int _padAmpl[geom::nPadx][geom::nPady][n::samples];
    int _padAmpl_511[geom::nPadx][geom::nPady][511];
    bool _use511;
    int _time_mid;
    int _time_msb;
    int _time_lsb;

    Float_t _pos[8];
    Mapping _t2k;
};

#endif //DAQ_READER_SRC_INTERFACEROOT_HXX_
