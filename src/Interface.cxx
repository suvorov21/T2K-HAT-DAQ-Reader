#include "Interface.hxx"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

//******************************************************************************
bool InterfaceAQS::Initialise(TString& file_namme, int verbose) {
//******************************************************************************
  _verbose = verbose;
  std::cout << "Initialise AQS interface" << std::endl;
  _param.fsrc = fopen(file_namme, "rb");
  _daq.loadDAQ();
  cout << "...DAQ loaded successfully" << endl;

  _t2k.loadMapping();
  cout << "...Mapping loaded succesfully." << endl;
  _firstEv = -1;

  if (_param.fsrc == nullptr) {
    std::cerr << "Input file could not be read" << std::endl;
  }

  return true;
}

//******************************************************************************
long int InterfaceAQS::Scan(int start, bool refresh, int& Nevents_run) {
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
              _eventPos.emplace_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum);
              _firstEv = evnum;
              prevEvnum = evnum;
              lastRead = _fea.TotalFileByteRead - 6*sizeof(unsigned short);
            }
            else if (evnum != prevEvnum) {
              if (_fea.TotalFileByteRead - 6*sizeof(unsigned short) != _eventPos[_eventPos.size()-1].first) {
                _eventPos.emplace_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum);
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
            cerr << "Interface.cxx: Unknown Item Type : " << _dc.ItemType << endl;
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
void InterfaceAQS::GetEvent(long int id, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {
//******************************************************************************
  unsigned short datum;
  int err;
  bool done = true;
  if (_verbose > 0)
    std::cout << "\nGetting event #" << id << "  at pos  " << _eventPos[id].first << "  with id  " << _eventPos[id].second << std::endl;;

  fseek(_param.fsrc, _eventPos[id].first, SEEK_SET);

  // clean the padAmpl
  memset(_padAmpl, 0, geom::nPadx * geom::nPady * n::samples * (sizeof(Int_t)));
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

        if (_dc.ItemType == IT_START_OF_EVENT) {
          if (_verbose > 0)
            std::cout << "Found event with id " << (int)_dc.EventNumber << std::endl;
          if ((int)_dc.EventNumber == _eventPos[id].second)
            eventNumber = (int)_dc.EventNumber;
        } else if (eventNumber == _eventPos[id].second && _dc.ItemType == IT_ADC_SAMPLE) {
          // std::cout << "ADC datum" << std::endl;
          if (_dc.ChannelIndex != 15 && _dc.ChannelIndex != 28 && _dc.ChannelIndex != 53 && _dc.ChannelIndex != 66 && _dc.ChannelIndex > 2 && _dc.ChannelIndex < 79) {
            // histo and display
            int x = _t2k.i(_dc.ChipIndex / n::chips, _dc.ChipIndex % n::chips, _daq.connector(_dc.ChannelIndex));
            int y = _t2k.j(_dc.ChipIndex / n::chips, _dc.ChipIndex % n::chips, _daq.connector(_dc.ChannelIndex));

            int a = (int)_dc.AbsoluteSampleIndex;
            int b = (int)_dc.AdcSample;

            // safety check
            if ( x >= 0 && y >= 0) {
              _padAmpl[x][y][a] = b;
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
        else if (eventNumber == id && _dc.ItemType == IT_TIME_BIN_INDEX) {}
        else if (eventNumber == id && _dc.ItemType == IT_CHANNEL_HIT_HEADER) {}
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

  for (int x = 0; x < geom::nPadx; ++x)
     for (int j = 0; j < geom::nPady; ++j)
        for (int t = 0; t < n::samples; ++t)
           padAmpl[x][j][t] = _padAmpl[x][j][t];
}

//******************************************************************************
bool InterfaceROOT::Initialise(TString& file_name, int verbose) {
//******************************************************************************
  std::cout << "Initialise ROOT interface" << std::endl;
  _verbose = verbose;
  _file_in = new TFile(file_name.Data());
  _tree_in = (TTree*)_file_in->Get("tree");
  _use511 = false;

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
  std::cout << "Input read" << std::endl;

  return true;
}

//******************************************************************************
long int InterfaceROOT::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
  Nevents_run = -1;
  return _tree_in->GetEntries();
}

//******************************************************************************
void InterfaceROOT::GetEvent(long int id, int padAmpl[geom::nPadx][geom::nPady][n::samples]) {
//******************************************************************************
  _tree_in->GetEntry(id);
  for (int i = 0; i < geom::nPadx; ++i)
         for (int j = 0; j < geom::nPady; ++j)
            for (int t = 0; t < n::samples; ++t)
              if (_use511)
                padAmpl[i][j][t] = _padAmpl_511[i][j][t];
              else
                padAmpl[i][j][t] = _padAmpl[i][j][t];
}

InterfaceTracker::~InterfaceTracker() {
  if (_file.is_open())
    _file.close();
}

//******************************************************************************
bool InterfaceTracker::Initialise(TString &file_name, int verbose) {
//******************************************************************************
  _file.open(file_name);
  _verbose = verbose;
  if (!_file.is_open())
    return false;
  return true;
}

//******************************************************************************
long int InterfaceTracker::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
//  return std::count(std::istreambuf_iterator<char>(_file),
//                    std::istreambuf_iterator<char>(), '\n');
  std::string line_str;
  int line = -1;
  while(getline(_file, line_str)) {
    ++line;
    istringstream ss(line_str);
    float x1, y1, x2, y2;
    int fake;
    int event;
    ss >> x1 >> y1 >> x2 >> y2;
    for (auto i = 0; i < 66; ++i)
      ss >> fake;
    ss >> event;
    // thanks to fortran, event number starts from 1;
    --event;
    _eventPos[event] = line;
    if (_verbose) {
      std::cout << "Event " << event << " found at line " << line << std::endl;
      std::cout << x1 << "\t" << y1 << "\t" << x2 << "\t" << y2 << std::endl;
    }
  }
  return line;
}

void InterfaceTracker::GotoEvent(unsigned int num) {
  _file.clear();
  _file.seekg(std::ios::beg);
  for (int i = 0; i < num; ++i){
//    _file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    std::string line_str;
    getline(_file, line_str);
  }
}

bool InterfaceTracker::HasEvent(long int id) {
  return _eventPos.find(id) != _eventPos.end();
}

//******************************************************************************
bool InterfaceTracker::GetEvent(long int id, std::vector<float>& data) {
//******************************************************************************
  if (!HasEvent(id))
    return false;

  GotoEvent(_eventPos[id]);
  if (_verbose)
    std::cout << "Reading tracker data for event " << id << std::endl;
  std::string line_str;
  getline(_file, line_str);
  istringstream ss(line_str);
  float x1{-1}, y1{-1}, x2{-1}, y2{-1}, x3{-1}, y3{-1}, x4{-1}, y4{-1};
  int fake;
  int event;
  ss >> x1 >> y1 >> x2 >> y2 >> x3 >> y3 >> x4 >> y4;
  for (auto i = 0; i < 62; ++i)
    ss >> fake;
  ss >> event;
  if (event - 1 != id)
    throw std::logic_error("Event number mismatch " +  \
                            std::to_string(event-1) + " " + \
                            std::to_string(id)
                            );
  data.insert(data.end(), {x1, y1, x2, y2, x3, y3, x4, y4});

  return true;
}