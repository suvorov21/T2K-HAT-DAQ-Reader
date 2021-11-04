#ifndef Interface_hxx
#define Interface_hxx

#include <iostream>

#include "TTree.h"
#include "TFile.h"

#include "fdecoder.h"
#include "datum_decoder.h"
#include "T2KConstants.h"
#include "Mapping.h"
#include "DAQ.h"

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
} Param;

/// Base interface class
class InterfaceBase {
public:
  InterfaceBase(): _verbose(0) {}
  virtual ~InterfaceBase() = default;

  /// Initialise the reader with file name
  virtual bool Initialise(TString& file_name, int verbose) {return false;}
  /// Scan and define the number of events
  virtual long int Scan(int start, bool refresh, int& Nevents_run) {return 0;}
  /// Get the data for the particular event
  virtual void GetEvent(int id, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {};

protected:
  /// verbosity level
  int _verbose;
};

/// AQS file reader
class InterfaceAQS: public InterfaceBase {
public:
  InterfaceAQS() {};
//  InterfaceAQS(const InterfaceBase& var) {};
  ~InterfaceAQS() override = default;
  bool Initialise(TString& file_name, int verbose) override;
  long int Scan(int start, bool refresh, int& Nevents_run) override;
  void GetEvent(int id, int padAmpl[geom::nPadx][geom::nPady][n::samples]) override;

private:
  Features _fea;
  std::vector<std::pair<long int, int> > _eventPos;
  __int64 lastRead;
  DatumContext _dc;
  Param _param;
  DAQ _daq;
  Mapping _t2k;

  int _firstEv;
  int _padAmpl[geom::nPadx][geom::nPady][n::samples];
};

/// ROOT file reader
class InterfaceROOT: public InterfaceBase {
public:
  InterfaceROOT() {}
//  InterfaceROOT(const InterfaceBase& var) {}
  ~InterfaceROOT() override = default;
  bool Initialise(TString& file_name, int verbose) override;
  long int Scan(int start, bool refresh, int& Nevents_run) override;
  void GetEvent(int id, int padAmpl[geom::nPadx][geom::nPady][n::samples]) override;

private:
  TFile *_file_in;
  TTree *_tree_in;
  int _padAmpl[geom::nPadx][geom::nPady][n::samples];
  int _padAmpl_511[geom::nPadx][geom::nPady][511];
  bool _use511;
};

#endif
