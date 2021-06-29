#include "Interface.hxx"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>

#include "fdecoder.h"
#include "platform_spec.h"
#include "frame.h"


void InterfaceAQS::Initialise(TString file_namme) {
  _param.fsrc = fopen(file_namme, "rb");
  _daq.loadDAQ();
  cout << "... DAQ loaded successfully" << endl;

  _T2K.loadMapping();
  cout << "...Mapping loaded succesfully." << endl;
  _firstEv = -1;
}

int InterfaceAQS::Scan(int start, bool refresh) {
  // Reset _eventPos vector
  // Scan the file
  DatumContext_Init(&_dc, _param.sample_index_offset_zs);
  unsigned short datum;
  int err;
  bool done = true;
  int prevEvnum = -1;
  int evnum;
  if (refresh) {
    std::cout << "Scanning the file..." << std::endl;
    _eventPos.clear();
    fseek(_param.fsrc, 0, SEEK_SET);
    lastRead = 0;
  }
  else {
    fseek(_param.fsrc, _eventPos[start], SEEK_SET);
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
              _eventPos.push_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short));
              _firstEv = evnum;
              prevEvnum = evnum;
              lastRead = _fea.TotalFileByteRead - 6*sizeof(unsigned short);
            }
            else if (evnum != prevEvnum) {
              if (_fea.TotalFileByteRead - 6*sizeof(unsigned short) != _eventPos[_eventPos.size()-1]) {
                _eventPos.push_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short));
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
  if (refresh) {
    cout << "Scan done." << std::endl;
  }
    cout << _eventPos.size() << " events in the file..." << endl;

  return _eventPos.size();
}

void InterfaceAQS::GetEvent(int i, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {

  unsigned short datum;
  int err;
  bool done = true;
  int ntot;
  // std::cout << "reading event " << i  << " id " << i - _firstEv << std::endl;
  fseek(_param.fsrc, _eventPos[i - _firstEv], SEEK_SET);
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

        if (_dc.ItemType == IT_START_OF_EVENT && (int)_dc.EventNumber == i) {
          eventNumber = (int)_dc.EventNumber;
          // std::cout << "Event num " << eventNumber << std::endl;
        } else if (eventNumber == i && _dc.ItemType == IT_ADC_SAMPLE) {
          // std::cout << "ADC datum" << std::endl;
          if (_dc.ChannelIndex != 15 && _dc.ChannelIndex != 28 && _dc.ChannelIndex != 53 && _dc.ChannelIndex != 66 && _dc.ChannelIndex > 2 && _dc.ChannelIndex < 79) {

            int a = 0;
            int b = 0;
            // histo and display
            int x = _T2K.i(_dc.ChipIndex/n::chips, _dc.ChipIndex%n::chips, _daq.connector(_dc.ChannelIndex));
            int y = _T2K.j(_dc.ChipIndex/n::chips, _dc.ChipIndex%n::chips, _daq.connector(_dc.ChannelIndex));

            a = (int)_dc.AbsoluteSampleIndex;
            b = (int)_dc.AdcSample;

            // std::cout << x << "\t" << y << "\t" << a << "\t" << b << std::endl;
            // std::cout << "card\t" << _dc.CardIndex << "\tChip\t" << _dc.ChipIndex << "\tch\t" << _dc.ChannelIndex << std::endl;

            if ( x >= 0 && y >= 0) {

              // std::cout << "card\t" << _dc.CardIndex << "\tChip\t" << _dc.ChipIndex << "\tch\t" << _dc.ChannelIndex << "\tx\t" << x << "\ty\t" << y << std::endl;
              _PadAmpl[x][y][a] = b;
              // std::cout << "charge " << b << std::endl;
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
        else if (_dc.ItemType == IT_CHAN_PED_CORRECTION) {} //printf("Type : 0x%x \n", _dc.ItemType);}
        else if (_dc.ItemType == IT_CHAN_ZERO_SUPPRESS_THRESHOLD) {} //printf("Type : 0x%x \n", _dc.ItemType);}
        else {} //}&& _dc.ItemType==IT_START_OF_EVENT
      } // end of if (_dc.isItemComplete)
    } // end of second loop inside while
  } // end of while(done) loop

  for (int i = 0; i < geom::nPadx; ++i)
         for (int j = 0; j < geom::nPady; ++j)
            for (int t = 0; t < n::samples; ++t)
               padAmpl[i][j][t] = _PadAmpl[i][j][t];
}

void InterfaceROOT::Initialise(TString file_namme) {
  // TFile* test = new TFile(file_namme.Data());
  _file_in = new TFile(file_namme.Data());
  _tree_in = (TTree*)_file_in->Get("tree");

  _tree_in->SetBranchAddress("PadAmpl", _PadAmpl);
  std::cout << "Input read" << std::endl;
}

int InterfaceROOT::Scan(int start, bool refresh) {
  return _tree_in->GetEntries();
}

void InterfaceROOT::GetEvent(int i, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {
  _tree_in->GetEntry(i);
  padAmpl = _PadAmpl;
}