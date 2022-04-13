//
// Created by SERGEY SUVOROV on 13/04/2022.
//

#ifndef DAQ_READER_SRC_OUTPUT_HXX_
#define DAQ_READER_SRC_OUTPUT_HXX_

#include "TFile.h"

#include "TRawEvent.hxx"
#include "T2KConstants.h"
#include "Mapping.h"
#include "DAQ.h"

/// Output converter interface
class OutputBase {
 public:
    virtual void Initialise(const TString& fileName, bool useTracker) = 0;
    virtual void AddEvent(TRawEvent* event) = 0;
    virtual void AddTrackerEvent(const std::vector<float>& TrackerPos) = 0;
    virtual void Fill() = 0;
    virtual void Finilise() = 0;
};

/// Store output as a 3-D array
class OutputArray : public OutputBase{
    TFile* _file;
    TTree* _tree;
    DAQ _daq;
    Mapping _t2k;
    int _time_mid, _time_msb, _time_lsb;
    int _padAmpl[geom::nPadx][geom::nPady][n::samples];
    float _trackerPos[8];
    const TString treeName = "tree";
    const TString branchName = "PadAmpl";
 public:
    void Initialise(const TString& fileName, bool useTracker) override;
    void AddEvent(TRawEvent* event) override;
    void AddTrackerEvent(const std::vector<float>& TrackerPos) override;
    void Fill() override;
    void Finilise() override;
};

/// Store a TTree with TRawEvent
class OutputTRawEvent : public OutputBase {
    TFile* _file;
    TTree* _tree;
    TRawEvent* _event;
    float _trackerPos[8];
    const TString treeName = "EventTree";
    const TString branchName = "TRawEvent";
 public:
    void Initialise(const TString& fileName, bool useTracker) override;
    void AddEvent(TRawEvent* event) override;
    void AddTrackerEvent(const std::vector<float>& TrackerPos) override;
    void Fill() override;
    void Finilise() override;
};

#endif //DAQ_READER_SRC_OUTPUT_HXX_
