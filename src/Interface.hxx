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

/// AQS file reader
class InterfaceAQS : public InterfaceBase {
 public:
    explicit InterfaceAQS() = default;;
    ~InterfaceAQS() override = default;
    bool Initialise(const std::string &file_name, int verbose) override;
    uint64_t Scan(int start, bool refresh, int &Nevents_run) override;
    TRawEvent *GetEvent(long int id) override;
    void GetTrackerEvent(long int id, Float_t pos[8]) override {
        throw std::logic_error("No tracker info in AQS");
    }

 private:
    Features _fea;
    std::vector<std::pair<long int, int> > _eventPos;
    __int64 lastRead;
    DatumContext _dc;
    FILE* _fsrc{nullptr};
    int _sample_index_offset_zs;
    DAQ _daq;
    Mapping _t2k;

    int _firstEv;
};

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

/// Midas file reader

class InterfaceMidas : public InterfaceBase {
public:
    explicit InterfaceMidas() = default;
    ~InterfaceMidas() override = default;
    bool Initialise(const std::string &file_name, int verbose) override;
//    uint64_t Scan(int start, bool refresh, int &Nevents_run) override;
//    TRawEvent *GetEvent(long int id) override;
    void GetTrackerEvent(long int id, Float_t pos[8]) override {
        throw std::logic_error("No tracker info in TRawEvent");
    }
private:
    ifstream _file;
//    TTree *_tree_in;
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

/// Factory returns proper input interface
class InterfaceFactory {
 public:
    static std::shared_ptr<InterfaceBase> get(const TString &file_name) {
        if (file_name.EndsWith(".aqs")) {
            return std::make_shared<InterfaceAQS>();
        }

        if (file_name.EndsWith(".root")) {
            TFile file(file_name);
            if (file.Get<TTree>("EventTree")) {
                return std::make_shared<InterfaceRawEvent>();
            } else if (file.Get<TTree>("tree")) {
                return std::make_shared<InterfaceROOT>();
            } else {
                std::cerr << "ERROR in converter. Unknown ROOT file type." << std::endl;
            }
        };

        std::cerr << "ERROR in converter. Unknown file type." << std::endl;
        return nullptr;
    }
};

#endif
