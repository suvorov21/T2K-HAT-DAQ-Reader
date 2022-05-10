#include "Interface.hxx"

#include <cstdio>
#include <iostream>
#include <sstream>

#include <midasio.h>

//******************************************************************************
bool InterfaceAQS::Initialise(const std::string& file_namme, int verbose) {
//******************************************************************************
  _verbose = verbose;
  std::cout << "Initialise AQS interface" << std::endl;
  _fsrc = fopen(file_namme.c_str(), "rb");
  _daq.loadDAQ();
  cout << "...DAQ loaded successfully" << endl;

  _t2k.loadMapping();
  cout << "...Mapping loaded succesfully." << endl;
  _firstEv = -1;

  if (_fsrc == nullptr) {
    std::cerr << "Input file could not be read" << std::endl;
  }

  return true;
}

//******************************************************************************
uint64_t InterfaceAQS::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
  // Reset _eventPos vector
  // Scan the file
  DatumContext_Init(&_dc, _sample_index_offset_zs);
  unsigned short datum;
  int err;
  bool done = true;
  int prevEvnum = -1;
  int evnum;
  if (_verbose > 0 || refresh)
    std::cout << "\nScanning the file..." << std::endl;
  if (refresh) {
    _eventPos.clear();
    fseek(_fsrc, 0, SEEK_SET);
    lastRead = 0;
  }
  else {
    fseek(_fsrc, _eventPos[start].first, SEEK_SET);
  }

  _fea.TotalFileByteRead = lastRead;
  while (done) {
    // Read one short word
    if (fread(&datum, sizeof(unsigned short), 1, _fsrc) != 1) {
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
              if (_verbose > 1)
                std::cout << "First event id  " << evnum << "  at  " << _fea.TotalFileByteRead - 6*sizeof(unsigned short) << std::endl;
              _eventPos.emplace_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum);
              _firstEv = evnum;
              prevEvnum = evnum;
              lastRead = _fea.TotalFileByteRead - 6*sizeof(unsigned short);
            }
            else if (evnum != prevEvnum) {
              if (_fea.TotalFileByteRead - 6*sizeof(unsigned short) != _eventPos[_eventPos.size()-1].first) {
                _eventPos.emplace_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum);
                if (_verbose > 1) {
                  std::cout << "Event " << evnum << " at " << _fea.TotalFileByteRead - 6 * sizeof(unsigned short)
                            << std::endl;
                  std::cout << "time lsb:msb:mid : " << _dc.EventTimeStampLsb << " " << _dc.EventTimeStampMsb << " "
                            << _dc.EventTimeStampMid << std::endl;
                }
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
TRawEvent* InterfaceAQS::GetEvent(long int id) {
//******************************************************************************
  unsigned short datum;
  int err;
  bool done = true;
  if (_verbose > 1)
    std::cout << "\nGetting event #" << id << "  at pos  " << _eventPos[id].first << "  with id  " << _eventPos[id].second << std::endl;;

  fseek(_fsrc, _eventPos[id].first, SEEK_SET);

  // clean the padAmpl
  auto event = new TRawEvent(id);
  std::vector<TRawHit*> hitVector{};
  int eventNumber = -1;

  while (done) {
    if (fread(&datum, sizeof(unsigned short), 1, _fsrc) != 1) {
      done = false;
      if (ferror(_fsrc))
        cout << "\nERROR" << endl;
      if (feof(_fsrc))
        cout << "\nreach EOF" << endl;
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
          if (_verbose > 1)
            std::cout << "Found event with id " << (int)_dc.EventNumber << std::endl;
          event->SetTime(_dc.EventTimeStampMid,
                         _dc.EventTimeStampMsb,
                         _dc.EventTimeStampLsb);

          if ((int)_dc.EventNumber == _eventPos[id].second)
            eventNumber = (int)_dc.EventNumber;
        } else if (eventNumber == _eventPos[id].second && _dc.ItemType == IT_ADC_SAMPLE) {
          // std::cout << "ADC datum" << std::endl;
          if (_dc.ChannelIndex != 15 && _dc.ChannelIndex != 28 && _dc.ChannelIndex != 53 && _dc.ChannelIndex != 66 && _dc.ChannelIndex > 2 && _dc.ChannelIndex < 79) {
            auto hitCandidate = new TRawHit(_dc.CardIndex,
                                            _dc.ChipIndex,
                                            _dc.ChannelIndex);
            auto hitIt = std::find_if(hitVector.begin(),
                                   hitVector.end(),
                                   [hitCandidate](const TRawHit* ptr){return *hitCandidate == *ptr;});


            int a = (int)_dc.AbsoluteSampleIndex;
            int b = (int)_dc.AdcSample;

            if (hitIt != hitVector.end()) {
              (*hitIt)->SetADCunit(a, b);
              delete hitCandidate;
            } else {
              hitCandidate->ResetWF();
              hitCandidate->SetADCunit(a, b);
              hitVector.emplace_back(hitCandidate);
            }
          }
        }
        else if (_dc.ItemType == IT_END_OF_EVENT) {
          // go to the next event
          done = false;
        }

      } // end of if (_dc.isItemComplete)
    } // end of second loop inside while
  } // end of while(done) loop

  event->Reserve(hitVector.size());
  for (auto& hit : hitVector) {
    hit->ShrinkWF();
    event->AddHit(hit);
  }
  return event;
}

//******************************************************************************
bool InterfaceROOT::Initialise(const std::string& file_name, int verbose) {
//******************************************************************************
  std::cout << "Initialise ROOT interface" << std::endl;
  _verbose = verbose;
  _file_in = new TFile(file_name.c_str());
  _tree_in = (TTree*)_file_in->Get("tree");
  _use511 = false;

  _t2k.loadMapping();

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

  if (_tree_in->GetBranch("Tracker")) {
    _has_tracker = true;
    std::cout << "Has tracker" << std::endl;
    _tree_in->SetBranchAddress("Tracker", _pos);
  }
  std::cout << "Input read" << std::endl;

  return true;
}

//******************************************************************************
uint64_t InterfaceROOT::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
  Nevents_run = -1;
  return _tree_in->GetEntries();
}

//******************************************************************************
TRawEvent* InterfaceROOT::GetEvent(long int id) {
//******************************************************************************
  _tree_in->GetEntry(id);
  auto event = new TRawEvent(id);
  event->SetTime(_time_mid, _time_msb, _time_lsb);

  for (int i = 0; i < geom::nPadx; ++i) {
    for (int j = 0; j < geom::nPady; ++j) {
      auto hit = new TRawHit();
      hit->SetCard(0);
      auto elec = _t2k.getElectronics(i, j);
      hit->SetChip(elec.first);
      hit->SetChannel(elec.second);
      hit->ResetWF();
      auto max = 0;
      for (int t = 0; t < n::samples; ++t) {
        auto ampl = _use511 ? _padAmpl_511[i][j][t] : _padAmpl[i][j][t];
        hit->SetADCunit(t, ampl);
        if (ampl > max)
          max = ampl;
      }
      if (max > 0) {
        hit->ShrinkWF();
        event->AddHit(hit);
      } else {
        delete hit;
      }
    }
  }
  return event;
}

void InterfaceROOT::GetTrackerEvent(long int id, Float_t* pos) {
  _tree_in->GetEntry(id);
  for (int i = 0; i < 8; ++i)
    pos[i] = _pos[i];
}

InterfaceTracker::~InterfaceTracker() {
  if (_file.is_open())
    _file.close();
}

//******************************************************************************
bool InterfaceRawEvent::Initialise(const std::string& file_name, int verbose) {
//******************************************************************************
  std::cout << "Initialise TRawEvent interface" << std::endl;
  _verbose = verbose;
  _file_in = new TFile(file_name.c_str());
  _tree_in = (TTree*)_file_in->Get("EventTree");
  _event = new TRawEvent();
  _tree_in->SetBranchAddress("TRawEvent", &_event);

  return true;
}

//******************************************************************************
uint64_t InterfaceRawEvent::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
  Nevents_run = -1;
  return _tree_in->GetEntries();
}

//******************************************************************************
TRawEvent* InterfaceRawEvent::GetEvent(long int id) {
//******************************************************************************
  _tree_in->GetEntry(id);
  return _event;
}

//******************************************************************************
// MIDAS INTERFACE
//******************************************************************************

//******************************************************************************
bool InterfaceMidas::Initialise(const std::string& file_name, int verbose) {
//******************************************************************************
    if (file_name == "")
        return false;
    _reader = TMNewReader(file_name.c_str());
    _verbose = verbose;
    if (_reader->fError) {
        std::cout << "Cannot open input file " <<  file_name << std::endl;
        delete _reader;
        return false;
    }
    std::cout << "Opened " <<  file_name << std::endl;
    _filename = file_name;
    return true;
}

//******************************************************************************
uint64_t InterfaceMidas::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************

    unsigned int events_number = 0;
    if (start < _currentEventIndex){
        delete _reader;
        _reader = TMNewReader(_filename.c_str());
        _currentEventIndex = 0;
    }
//    uint64_t number = 0;
    while (true) {
        _currentEvent = TMReadEvent(_reader);

        if (!_currentEvent) {
            // EOF
            delete _currentEvent;
            break;
        }

        if (_currentEvent->error) {
            // broken event
            printf("event with error, bye!\n");
            delete _currentEvent;
            break;
        }
//        e->FindAllBanks();
//        if (e->banks.size() == 0){
//            std::cout << "skipping" << std::endl;
//            continue;
//        }
//
//
//        auto bankTDCM = e->FindBank("TDCM");
////        std::cout << bankTDCM ->type << "\t" << bankTDCM->data_size << std::endl;
//
//        auto bankCOUN = e->FindBank("COUN");
////        std::cout << bankCOUN ->type << "\t" << bankCOUN->data_size << std::endl;
//
//        auto bankFEMC = e->FindBank("FEMC");
////        std::cout << bankFEMC ->type << "\t" << bankFEMC->data_size << std::endl;
//
//        auto bankChip = e->FindBank("CHIP");
////        std::cout << bankChip ->type << "\t" << bankChip->data_size << std::endl;
//
//        auto bankChan = e->FindBank("CHAN");
////        std::cout << bankChan ->type << "\t" << bankChan->data_size << std::endl;
//
//        auto bankNWAV = e->FindBank("NWAV");
////        std::cout << bankNWAV ->type << "\t" << bankNWAV->data_size << std::endl;
//
//        auto bankNADC = e->FindBank("NADC");
//        auto bankWAVE = e->FindBank("WAVE");
//        auto bankTBIN = e->FindBank("TBIN");
//
//
//        auto tdcm = static_cast<unsigned int>(e->GetBankData(bankTDCM)[0]);
//        auto count = static_cast<unsigned int>(e->GetBankData(bankCOUN)[0]);
//        auto vwav = static_cast<unsigned int>(e->GetBankData(bankNWAV)[0]);
//        std::cout << "Event number: " << count << std::endl;
//        std::cout << "-> TDCM: " << tdcm << "->" << vwav << std::endl;
//        unsigned int iterCharge = 0;
//        for (int i =0; i < bankFEMC->data_size; i++) {
//            auto femc = static_cast<unsigned int>(e->GetBankData(bankFEMC)[i]);
//            auto chip = static_cast<unsigned int>(e->GetBankData(bankChip)[i]);
//            auto chan = static_cast<unsigned int>(e->GetBankData(bankChan)[i]);
//            auto nadc = static_cast<unsigned int>(e->GetBankData(bankNADC)[i]);
//            std::cout << "--> ID: " << femc << ";" << chip << ";" << chan << ";" << nadc << std::endl;
//            for (int iterAdc = 0; iterAdc < nadc; iterAdc+=2){
//                std::cout << "---> " <<static_cast<unsigned int>(e->GetBankData(bankTBIN)[iterCharge]) << "\t" << static_cast<unsigned int>(e->GetBankData(bankWAVE)[iterCharge]) << std::endl;
//                iterCharge+=2;
////                std::cout << "---> " <<static_cast<unsigned int>(e->GetBankData(bankTBIN)[iterAdc/2]) << "\t" << static_cast<unsigned int>(static_cast<unsigned short>(e->GetBankData(bankNWAV)[iterAdc]) | static_cast<unsigned short>(e->GetBankData(bankNWAV)[iterAdc+1]) << 8) << std::endl;
//            }
//        }
        _currentEventIndex++;
        events_number++;
    }
    return events_number;
}

TRawEvent* InterfaceMidas::GetEvent(long id) {
    if (id >= 150 or id <= 80){
        return nullptr;
    }
    TMEvent* midas_event = GoToEvent(id);
    if (midas_event == nullptr){
        std::cerr << "Cannot go to event id " << id << std::endl;
        exit(1);
    }
    midas_event->FindAllBanks();
    if (midas_event->error){
//        std::cerr << "Error with banks of event " << id << std::endl;
        return nullptr;
    }


    /// Get event number
    auto bank_count = midas_event->FindBank("COUN");
//    std::cout << bank_count->data_size << std::endl;
    unsigned int event_number = 0;
//    event_number = (unsigned int)((unsigned char)(midas_event->GetBankData(bank_count)[3] << 24) +
//            ((unsigned char)midas_event->GetBankData(bank_count)[2] << 16) +
//            ((unsigned char)midas_event->GetBankData(bank_count)[1] << 8) +
//            ((unsigned char)midas_event->GetBankData(bank_count)[0] << 0));

    event_number = GetUIntFromBank(midas_event->GetBankData(bank_count));
//    event_number |= midas_event->GetBankData(bank_count)[3] << 24;
//    event_number |= midas_event->GetBankData(bank_count)[2] << 16;
//    event_number |= midas_event->GetBankData(bank_count)[1] << 8;
//    event_number |= midas_event->GetBankData(bank_count)[0];
    if (event_number != id){std::cout << "Error " << event_number << "\t" << id << "\t" << std::bitset<32>(event_number) << std::endl;}
    std::cout << "Event number: " << event_number << std::endl;

    /// Define TRawEvent
    auto event = new TRawEvent(event_number);

    /// Get timing of the event
    auto bank_tmsb = midas_event->FindBank("TMSB");
    auto tmsb = GetUShortFromBank(midas_event->GetBankData(bank_tmsb));
    auto bank_tmid = midas_event->FindBank("TMID");
    auto tmid = GetUShortFromBank(midas_event->GetBankData(bank_tmid));
    auto bank_tlsb = midas_event->FindBank("TLSB");
    auto tlsb = GetUShortFromBank(midas_event->GetBankData(bank_tlsb));
    event->SetTime(tmid,tmsb,tlsb);
    std::cout << "-> Times: " << tmsb << "\t" << tmid << "\t" << tlsb << std::endl;

    /// Get number of waveforms
    auto bank_nwav = midas_event->FindBank("NWAV");
    auto waveformsNumber = GetUShortFromBank(midas_event->GetBankData(bank_nwav));
    if (waveformsNumber <= 0){
        std::cout << "No waveform in event " << id << std::endl;
        return nullptr;
    }
    std::cout << "-> Number of waveforms: " << waveformsNumber << std::endl;

    unsigned short version = 0;
    /// version 1
    auto bank_femc = midas_event->FindBank("FEMC");
    auto bank_chip = midas_event->FindBank("CHIP");
    auto bank_chan = midas_event->FindBank("CHAN");
    auto bank_tbin = midas_event->FindBank("TBIN");
    auto bank_wave = midas_event->FindBank("WAVE");
    if (bank_femc){
        version = 1;
    }

    /// version 2
    auto bank_chid = midas_event->FindBank("CHID");
    auto bank_tmin = midas_event->FindBank("TMIN");
    auto bank_tmax = midas_event->FindBank("TMAX");
    if (bank_chid and version == 0) {
        version = 2;
    }

    std::cout << "-> Data format version: " << version << std::endl;

    event->Reserve(waveformsNumber);
    TRawHit* hit;
    auto femc_vector = GetUCharVectorFromBank(midas_event->GetBankData(bank_femc), waveformsNumber);
    auto chip_vector = GetUCharVectorFromBank(midas_event->GetBankData(bank_chip), waveformsNumber);
    auto chan_vector = GetUCharVectorFromBank(midas_event->GetBankData(bank_chan), waveformsNumber);
    for (int i =0; i < waveformsNumber; i++) {
        if (version == 1){
//            auto femc = static_cast<unsigned int>(midas_event->GetBankData(bank_femc)[i]);
//            auto chip = static_cast<unsigned int>(midas_event->GetBankData(bank_chip)[i]);
//            auto chan = static_cast<unsigned int>(midas_event->GetBankData(bank_chan)[i]);
            std::cout << "--> " << femc_vector[i] << "\t" << chip_vector[i] << "\t" << chan_vector[i] << std::endl;
            hit = new TRawHit(femc_vector[i], chip_vector[i], chan_vector[i]);

        }
        event->AddHit(hit);
//        auto chip = static_cast<unsigned int>(midas_event->GetBankData(bankChip)[i]);
//        auto chan = static_cast<unsigned int>(midas_event->GetBankData(bankChan)[i]);
    }
    return event;

}

TMEvent* InterfaceMidas::GoToEvent(long id) {

    // if the index is too far away, we need to move to the right location
    if (id < _currentEventIndex){
        delete _reader;
        _reader = TMNewReader(_filename.c_str());
        _currentEvent = TMReadEvent(_reader);
        if (!IsValid(_currentEvent)) {
            // EOF
            _currentEvent = nullptr;
            std::cerr << "End of file was reached during initialization" << std::endl;
            return nullptr;
        }
        _currentEventIndex = 0;
    }

    /// TODO there might be a better way to go to an event, but the midasio lib doesn't allow this with a user-friendly method
//    TMEvent *e = nullptr;
    while (id > _currentEventIndex){
        _currentEvent = TMReadEvent(_reader); // read but drops the event
        if (!IsValid(_currentEvent)) {
            // EOF
            std::cerr << "End of file was reached while " << id << " was requested!" << std::endl;
            return nullptr;
        }
        _currentEventIndex++;
    }
    // if the current index is the right one, nothing to do
    if (id == _currentEventIndex){
        return _currentEvent;
    }
    else {
        std::cerr << "Event id " << _currentEventIndex << " was found while " << id << " was requested!" << std::endl;
        return nullptr;
    }
    return nullptr;
}


bool InterfaceMidas::IsValid(TMEvent* event) {
    if (!event) {
        printf("nulltptr event, bye!\n");
        return false;
    }

    if (event->error) {
        // broken event
        printf("event with error, bye!\n");
        delete event;
        return false;
    }
    return true;
}

unsigned int InterfaceMidas::GetUIntFromBank(char * data) {
    unsigned int value = 0;
    value = (unsigned int)((unsigned char)(data[3] << 24) +
                                  (unsigned char)(data[2] << 16) +
                                  (unsigned char)(data[1] << 8) +
                                  (unsigned char)(data[0] << 0));
    return value;
}

unsigned short InterfaceMidas::GetUShortFromBank(char * data) {
    unsigned short value = 0;
    value = (unsigned short)(((unsigned char)data[1] << 8) +
                           ((unsigned char)data[0] << 0));
    return value;
    return 0;
}

std::vector<unsigned short> InterfaceMidas::GetUShortVectorFromBank(char * data, unsigned int nvalues) {
    std::vector<unsigned short> vector;
    vector.resize(nvalues);
    for (int i = 0; i < nvalues; i++){
        vector[i] = (unsigned short)(((unsigned char)data[2*i+1] << 8) +
                                     ((unsigned char)data[2*i] << 0));
    }
    return vector;
}

std::vector<unsigned short> InterfaceMidas::GetUCharVectorFromBank(char * data, unsigned int nvalues) {
    std::vector<unsigned short> vector;
//    unsigned int len_vector = nvalues % 2;
    vector.resize(nvalues);
    for (int i = 0; i < nvalues; i++){
        vector[i] = (unsigned short)(((unsigned char)data[i] << 0));
    }
    return vector;
}

//******************************************************************************
// TRACKER INTERFACE
//******************************************************************************

//******************************************************************************
bool InterfaceTracker::Initialise(const std::string& file_name, int verbose) {
//******************************************************************************
  if (file_name == "")
    return false;
  _file.open(file_name);
  _verbose = verbose;
  if (!_file.is_open()) {
    std::cerr << "Tracker file is specified, but could not be opened" << std::endl;
    return false;
  }
  return true;
}

//******************************************************************************
uint64_t InterfaceTracker::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
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
    if (_verbose > 1) {
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
  if (_verbose > 1)
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