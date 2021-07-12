#include "Interface.hxx"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>

#include "fdecoder.h"
#include "platform_spec.h"
#include "frame.h"


//******************************************************************************
bool InterfaceAQS::Initialise(TString file_namme, int verbose) {
//******************************************************************************
  _verbose = verbose;
  std::cout << "Initialise AQS interface" << std::endl;
  _param.fsrc = fopen(file_namme, "rb");
  _daq.loadDAQ();
  cout << "...DAQ loaded successfully" << endl;

  _T2K.loadMapping();
  cout << "...Mapping loaded succesfully." << endl;
  _firstEv = -1;

  if (_param.fsrc == NULL) {
    std::cerr << "Input file could not be read" << std::endl;
  }

  return true;
}

//******************************************************************************
int InterfaceAQS::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
  // Reset _eventPos vector
  // Scan the file
  DatumContext_Init(&_dc, _param.sample_index_offset_zs);
  unsigned short datum;
  int err;
  bool done = true;
  int prevEvnum = -1;
  int evnum;
  if (_verbose > 0 || refresh)
    std::cout << "\nScanning the file..." << std::endl;
  if (refresh) {
    _eventPos.clear();
    fseek(_param.fsrc, 0, SEEK_SET);
    lastRead = 0;
  }
  else {
    fseek(_param.fsrc, _eventPos[start].first, SEEK_SET);
  }

  _fea.TotalFileByteRead = lastRead;
  while (done) {
    // Read one short word
    if (fread(&datum, sizeof(unsigned short), 1, _param.fsrc) != 1) {
      done = false;
    }
    else {
      _fea.TotalFileByteRead += sizeof(unsigned short);
      // Interpret datum
      if ((err = Datum_Decode(&_dc, datum)) < 0) {
        printf("%d Datum_Decode: %s\n", err, &_dc.ErrorString[0]);
        done = true;
      }
      else {
        if (_dc.isItemComplete) {
          if (_dc.ItemType == IT_START_OF_EVENT) {
            evnum = (int)_dc.EventNumber;
            if (_firstEv < 0) {
              if (_verbose > 0)
                std::cout << "First event id  " << evnum << "  at  " << _fea.TotalFileByteRead - 6*sizeof(unsigned short) << std::endl;
              _eventPos.push_back(std::make_pair(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum));
              _firstEv = evnum;
              prevEvnum = evnum;
              lastRead = _fea.TotalFileByteRead - 6*sizeof(unsigned short);
            }
            else if (evnum != prevEvnum) {
              if (_fea.TotalFileByteRead - 6*sizeof(unsigned short) != _eventPos[_eventPos.size()-1].first) {
                _eventPos.push_back(std::make_pair(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum));
                prevEvnum = evnum;
                lastRead = _fea.TotalFileByteRead - 6*sizeof(unsigned short);
              }
              else {
                prevEvnum = evnum;
              }
            }
          }
          else if (_dc.ItemType == IT_ADC_SAMPLE) {}
          else if (_dc.ItemType == IT_DATA_FRAME) {}
          else if (_dc.ItemType == IT_END_OF_FRAME) {}
          else if (_dc.ItemType == IT_MONITORING_FRAME) {}
          else if (_dc.ItemType == IT_CONFIGURATION_FRAME) {}
          else if (_dc.ItemType == IT_SHORT_MESSAGE) {}
          else if (_dc.ItemType == IT_LONG_MESSAGE) {}
          else if (_dc.ItemType == IT_TIME_BIN_INDEX) {}
          else if (_dc.ItemType == IT_CHANNEL_HIT_HEADER) {}
          else if (_dc.ItemType == IT_DATA_FRAME) {}
          else if (_dc.ItemType == IT_NULL_DATUM) {}
          else if (_dc.ItemType == IT_CHANNEL_HIT_COUNT) {}
          else if (_dc.ItemType == IT_LAST_CELL_READ) {}
          else if (_dc.ItemType == IT_END_OF_EVENT) {}
          else if (_dc.ItemType == IT_PED_HISTO_MD) {}
          else if (_dc.ItemType == IT_UNKNOWN) {}
          else if (_dc.ItemType == IT_CHAN_PED_CORRECTION) {}
          else if (_dc.ItemType == IT_CHAN_ZERO_SUPPRESS_THRESHOLD) {}
          else
          {
            cerr << "Interface.cxx: Unknow Item Type : " << _dc.ItemType << endl;
            cerr << "Program will exit to prevent the storing of the corrupted data" << endl;
            exit(1);
          }
        }
      }
    }
  }
  if (refresh || _verbose > 0) {
    cout << "Scan done." << std::endl;
    cout << _eventPos.size() << " events in the file." << std::endl;
  }

  Nevents_run = prevEvnum;
  return _eventPos.size();
}

//******************************************************************************
void InterfaceAQS::GetEvent(int i, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {
//******************************************************************************
  unsigned short datum;
  int err;
  bool done = true;
  int ntot;
  if (_verbose > 0)
    std::cout << "\nGetting event #" << i << "  at pos  " << _eventPos[i].first << "  with id  " << _eventPos[i].second << std::endl;;
  // std::cout << "reading event " << i  << " id " << i - _firstEv << std::endl;
  fseek(_param.fsrc, _eventPos[i].first, SEEK_SET);
  // std::cout << "go to " << _eventPos[i - _firstEv] << std::endl;
  // clean the padAmpl
  memset(_PadAmpl, 0, geom::nPadx * geom::nPady * n::samples * (sizeof(Int_t)));
  int eventNumber = -1;

  while (done) {
    if (fread(&datum, sizeof(unsigned short), 1, _param.fsrc) != 1) {
      done = false;
      if (ferror(_param.fsrc))
        cout << "ERROR" << endl;
      if (feof(_param.fsrc))
        cout << "reach EOF" << endl;
    } else {

      _fea.TotalFileByteRead += sizeof(unsigned short);
      // Interpret datum
      if ((err = Datum_Decode(&_dc, datum)) < 0) {
        printf("%d Datum_Decode: %s\n", err, &_dc.ErrorString[0]);
        done = true;
      }
      // Decode
      if (_dc.isItemComplete) {
        // std::cout << "Datum complete, type " << _dc.ItemType << std::endl;

        if (_dc.ItemType == IT_START_OF_EVENT) {
          if (_verbose > 0)
            std::cout << "Found event with id " << (int)_dc.EventNumber << std::endl;
          if ((int)_dc.EventNumber == _eventPos[i].second)
            eventNumber = (int)_dc.EventNumber;
        } else if (eventNumber == _eventPos[i].second && _dc.ItemType == IT_ADC_SAMPLE) {
          // std::cout << "ADC datum" << std::endl;
          if (_dc.ChannelIndex != 15 && _dc.ChannelIndex != 28 && _dc.ChannelIndex != 53 && _dc.ChannelIndex != 66 && _dc.ChannelIndex > 2 && _dc.ChannelIndex < 79) {

            int a = 0;
            int b = 0;
            // histo and display
            int x = _T2K.i(_dc.ChipIndex/n::chips, _dc.ChipIndex%n::chips, _daq.connector(_dc.ChannelIndex));
            int y = _T2K.j(_dc.ChipIndex/n::chips, _dc.ChipIndex%n::chips, _daq.connector(_dc.ChannelIndex));

            a = (int)_dc.AbsoluteSampleIndex;
            b = (int)_dc.AdcSample;

            // safety check
            if ( x >= 0 && y >= 0) {
              _PadAmpl[x][y][a] = b;
            } else {
              std::cout << "card\t" << _dc.CardIndex << "\tChip\t" << _dc.ChipIndex << "\tch\t" << _dc.ChannelIndex << std::endl;
            }
          }
        }
        else if (_dc.ItemType == IT_DATA_FRAME) {}
        else if (_dc.ItemType == IT_END_OF_FRAME) {}
        else if (_dc.ItemType == IT_MONITORING_FRAME) {}
        else if (_dc.ItemType == IT_CONFIGURATION_FRAME) {}
        else if (_dc.ItemType == IT_SHORT_MESSAGE) {}
        else if (_dc.ItemType == IT_LONG_MESSAGE) {}
        else if (eventNumber == i && _dc.ItemType == IT_TIME_BIN_INDEX) {}
        else if (eventNumber == i && _dc.ItemType == IT_CHANNEL_HIT_HEADER) {}
        else if (_dc.ItemType == IT_DATA_FRAME) {}
        else if (_dc.ItemType == IT_NULL_DATUM) {}
        else if (_dc.ItemType == IT_CHANNEL_HIT_COUNT) {}
        else if (_dc.ItemType == IT_LAST_CELL_READ) {}
        else if (_dc.ItemType == IT_END_OF_EVENT) {
          // go to the next event
          done = false;
        }
        else if (_dc.ItemType == IT_PED_HISTO_MD) {}
        else if (_dc.ItemType == IT_UNKNOWN) {}
        else if (_dc.ItemType == IT_CHAN_PED_CORRECTION) {}
        else if (_dc.ItemType == IT_CHAN_ZERO_SUPPRESS_THRESHOLD) {}
        else {}
      } // end of if (_dc.isItemComplete)
    } // end of second loop inside while
  } // end of while(done) loop

  for (int i = 0; i < geom::nPadx; ++i)
         for (int j = 0; j < geom::nPady; ++j)
            for (int t = 0; t < n::samples; ++t)
               padAmpl[i][j][t] = _PadAmpl[i][j][t];
}

//******************************************************************************
bool InterfaceROOT::Initialise(TString file_namme, int verbose) {
//******************************************************************************
  std::cout << "Initialise ROOT interface" << std::endl;
  _verbose = verbose;
  _file_in = new TFile(file_namme.Data());
  _tree_in = (TTree*)_file_in->Get("tree");
  _use511 = false;

  TString branch_name = _tree_in->GetBranch("PadAmpl")->GetTitle();

  if (branch_name.Contains("[510]")) {
    _tree_in->SetBranchAddress("PadAmpl", _PadAmpl);
  } else if (branch_name.Contains("[511]")) {
    _use511 = true;
    _tree_in->SetBranchAddress("PadAmpl", _PadAmpl_511);
  } else {
    std::cerr << "ERROR in InterfaceROOT::Initialise()" << std::endl;
    exit(1);
  }
  std::cout << "Input read" << std::endl;

  return true;
}

//******************************************************************************
int InterfaceROOT::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
  Nevents_run = -1;
  return _tree_in->GetEntries();
}

//******************************************************************************
void InterfaceROOT::GetEvent(int i, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {
//******************************************************************************
  _tree_in->GetEntry(i);
  for (int i = 0; i < geom::nPadx; ++i)
         for (int j = 0; j < geom::nPady; ++j)
            for (int t = 0; t < n::samples; ++t)
              if (_use511)
                padAmpl[i][j][t] = _PadAmpl_511[i][j][t];
              else
                padAmpl[i][j][t] = _PadAmpl[i][j][t];
}