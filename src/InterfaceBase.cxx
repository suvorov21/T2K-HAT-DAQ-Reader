#include "InterfaceBase.hxx"

#include <cstdio>
#include <iostream>
#include <sstream>

#include <midasio.h>

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