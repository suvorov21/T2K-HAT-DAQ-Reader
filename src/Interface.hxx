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

typedef struct _Param
{
  char* inp_file;
  char* out_path;
  FILE *fsrc = nullptr;
  int has_no_run;
  int show_run;
  unsigned int vflag;
  int sample_index_offset_zs;
  bool test = false;
  int verbose = 0;
  int nevents = -1;
  char* tracker_file = new char('\0');
} Param;

/// Base interface class
class InterfaceBase {
public:
  InterfaceBase(): _verbose(0) {}
  virtual ~InterfaceBase() = default;

  /// Initialise the reader with file name
  virtual bool Initialise(TString& file_name, int verbose) {return false;}
  //! Scan and define the number of events
  //! \param start the first event to e scan
  //! \param refresh whether to redo the scan from scratch
  //! \param Nevents_run update the number of events in the whole run
  //! \return
  virtual long int Scan(int start, bool refresh, int& Nevents_run) = 0;
  /// Get the data for the particular event
  virtual TRawEvent* GetEvent(long int id) = 0;

  bool HasTracker() const {return _has_tracker;}
  virtual void GetTrackerEvent(long int id, Float_t pos[8]) = 0;

protected:
  /// verbosity level
  int _verbose;
  bool _has_tracker{false};
};

/// AQS file reader
class InterfaceAQS: public InterfaceBase {
public:
  explicit InterfaceAQS() = default;;
  ~InterfaceAQS() override = default;
  bool Initialise(TString& file_name, int verbose) override;
  long int Scan(int start, bool refresh, int& Nevents_run) override;
  TRawEvent* GetEvent(long int id) override;
  void GetTrackerEvent(long int id, Float_t pos[8]) override {
    throw std::logic_error("No tracker info in AQS");
  }

private:
  Features _fea;
  std::vector<std::pair<long int, int> > _eventPos;
  __int64 lastRead;
  DatumContext _dc;
  Param _param;
  DAQ _daq;
  Mapping _t2k;

  int _firstEv;
};

/// ROOT file reader
class InterfaceROOT: public InterfaceBase {
public:
  InterfaceROOT() {}
  ~InterfaceROOT() override = default;
  bool Initialise(TString& file_name, int verbose) override;
  long int Scan(int start, bool refresh, int& Nevents_run) override;
  TRawEvent* GetEvent(long int id) override;
  void GetTrackerEvent(long int id, Float_t* pos) override;

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
};

/// Silicon tracker file reader
class InterfaceTracker: public InterfaceBase {
public:
    InterfaceTracker() = default;
    ~InterfaceTracker() override;

    bool Initialise(TString& file_name, int verbose) override;
    long int Scan(int start, bool refresh, int& Nevents_run) override;
    bool GetEvent(long int id, std::vector<float>& data);
    TRawEvent* GetEvent(long int id) override {
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
