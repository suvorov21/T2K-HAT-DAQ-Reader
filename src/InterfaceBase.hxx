#ifndef Interface_hxx
#define Interface_hxx

#include <iostream>
#include <fstream>

#include "TTree.h"
#include "TFile.h"

#include "fdecoder.h"
#include "datum_decoder.h"
#include "T2KConstants.h"
#include "Mapping.h"
#include "DAQ.h"
#include "TRawEvent.hxx"
#include "midasio.h"

static int tmp = -1;


/// Base interface class
class InterfaceBase {
 public:
    InterfaceBase() : _verbose(0) {}
    virtual ~InterfaceBase() = default;

    /// Initialise the reader with file name
    virtual bool Initialise(const std::string &file_name, int verbose) = 0;
    //! Scan and define the number of events
    //! \param start the first event to e scan
    //! \param refresh whether to redo the scan from scratch
    //! \param Nevents_run update the number of events in the whole run
    //! \return
    virtual uint64_t Scan(int start, bool refresh, int &Nevents_run) = 0;
    /// Get the data for the particular event
    virtual TRawEvent *GetEvent(long int id) = 0;

    bool HasTracker() const { return _has_tracker; }
    virtual void GetTrackerEvent(long int id, Float_t pos[8]) = 0;

 protected:
    /// verbosity level
    int _verbose;
    bool _has_tracker{false};
};

class InterfaceRawEvent : public InterfaceBase {
 public:
    explicit InterfaceRawEvent() = default;;
    ~InterfaceRawEvent() override = default;
    bool Initialise(const std::string &file_name, int verbose) override;
    uint64_t Scan(int start, bool refresh, int &Nevents_run) override;
    TRawEvent *GetEvent(long int id) override;
    void GetTrackerEvent(long int id, Float_t pos[8]) override {
        throw std::logic_error("No tracker info in TRawEvent");
    }
 private:
    TFile *_file_in;
    TTree *_tree_in;
    TRawEvent *_event;
};

/// Silicon tracker file reader
class InterfaceTracker : public InterfaceBase {
 public:
    InterfaceTracker() = default;
    ~InterfaceTracker() override;

    bool Initialise(const std::string &file_name, int verbose) override;
    uint64_t Scan(int start, bool refresh, int &Nevents_run) override;
    bool GetEvent(long int id, std::vector<float> &data);
    TRawEvent *GetEvent(long int id) override {
        throw std::logic_error("not implemented");
    }

    void GetTrackerEvent(long int id, Float_t pos[8]) override {
        throw std::logic_error("not implemented");
    }

    void GotoEvent(unsigned int num);
    bool HasEvent(long int id);

 private:
    ifstream _file;
    std::map<long int, int> _eventPos;
};

#endif
