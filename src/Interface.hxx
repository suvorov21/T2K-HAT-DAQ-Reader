#ifndef Interface_hxx
#define Interface_hxx

#include "TTree.h"
#include "TFile.h"

#include "fdecoder.h"
#include "datum_decoder.h"
#include "T2KConstants.h"
#include "Mapping.h"
#include "DAQ.h"

typedef struct _Param
{
  char* inp_file;
  char* out_path;
  FILE *fsrc;
  int has_no_run;
  int show_run;
  unsigned int vflag;
  int sample_index_offset_zs;
  bool test = false;
  bool use_root = false;
  bool use_aqs = false;
  int verbose = 0;
  int nevents = -1;
} Param;

class InterfaceBase {
public:
  InterfaceBase() {;}
  virtual ~InterfaceBase() {;}

  void Initialise(TString file_name) {};
  int Scan(int start=-1, bool refresh=true) {return 0;}
  void GetEvent(int i, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {};
};

class InterfaceAQS: public InterfaceBase {
public:
  InterfaceAQS() {};
  InterfaceAQS(InterfaceBase var) {};
  virtual ~InterfaceAQS() {;}
  void Initialise(TString file_name);
  int Scan(int start=-1, bool refresh=true);
  void GetEvent(int i, int padAmpl[geom::nPadx][geom::nPady][n::samples]);

private:
  Features _fea;
  std::vector<long int> _eventPos;
  __int64 lastRead;
  DatumContext _dc;
  Param _param;
  DAQ _daq;
  Mapping _T2K;

  int _firstEv;
  int _PadAmpl[geom::nPadx][geom::nPady][n::samples];
};

class InterfaceROOT: public InterfaceBase {
public:
  InterfaceROOT() {;}
  InterfaceROOT(InterfaceBase var) {;}
  virtual ~InterfaceROOT() {;}
  void Initialise(TString file_name);
  int Scan(int start=-1, bool refresh=true);
  void GetEvent(int i, int padAmpl[geom::nPadx][geom::nPady][n::samples]);

private:
  TFile *_file_in;
  TTree *_tree_in;
  int _PadAmpl[geom::nPadx][geom::nPady][n::samples];
};

#endif
