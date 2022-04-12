#include "Interface.hxx"

#include <iostream>

#include "TString.h"

/*******************************************************************************
help() to display usage
*******************************************************************************/
void help()
{
  printf("fdecoder <options>\n");
  printf("   -h                   : print usage\n");
  printf("   -i <input_file>      : input file name with a path\n");
  printf("   -o <output_path>     : output files path (default='.')\n");
  printf("   -p <Value>           : number of pre-samples below threshold in zero-suppressed mode\n");
  printf("   -n <Value>           : spesify the number of events");
  printf("   -v <level>           : verbose\n");
  printf("   -t                   : test converter workability - run iver 30 first events\n");
  printf("   -s <tracker_file>    : embed info from the external tracker");
  printf("   -f <0xFlags>         : flags to determine printed items\n");
  exit(1);
}

// ******************************************************************************
// parse_cmd_args() to parse command line arguments
// ******************************************************************************
int parse_cmd_args(int argc, char **argv, Param *p) {
  for (;;) {
    int c = getopt(argc, argv, "i:o:v:p:n:hts:");
    if (c < 0) break;
    switch (c) {
      case 'i' : p->inp_file                = optarg;       break;
      case 'o' : p->out_path                = optarg;       break;
      case 'v' : p->verbose                 = atoi(optarg); break;
      case 't' : p->test                    = true;         break;
      case 'f' : p->vflag                   = atoi(optarg); break;
      case 'p' : p->sample_index_offset_zs  = atoi(optarg); break;
      case 's' : p->tracker_file            = optarg;       break;
      case 'n' : p->nevents                 = atoi(optarg); break;
      default : help();
    }
  }
  if (argc == 1)
    help();

  return 0;
}

int main(int argc, char **argv) {
   Param param;

   if (parse_cmd_args(argc, argv, &param) < 0)
    return (-1);

   TString file_name = TString(param.inp_file);
   std::shared_ptr<InterfaceBase> interface;

   if (file_name.EndsWith(".root")) {
      interface = std::make_shared<InterfaceROOT>();
   } else if (file_name.EndsWith(".aqs")) {
      interface = std::make_shared<InterfaceAQS>();
   } else {
      std::cerr << "ERROR in converter. Unknown file type." << std::endl;
      exit(1);
   }

   if (!interface->Initialise(file_name, param.verbose)) {
      std::cerr << "Interface initialisation fails. Exit" << std::endl;
      exit(1);
   }

   bool read_tracker = false;
   auto tracker = std::make_shared<InterfaceTracker>();
   if (strcmp(param.tracker_file, "") != 0) {
     TString tracker_name = TString(param.tracker_file);
     read_tracker = tracker->Initialise(tracker_name, param.verbose);
     if (!read_tracker)
       std::cerr << "Tracker file is specified, but could not be opened" << std::endl;
   }

   // extract the file name from the input
   std::string file_in = param.inp_file;
   while (file_in.find('/') != string::npos)
      file_in = file_in.substr(file_in.find('/') + 1);
   file_in = file_in.substr(0, file_in.find('.'));

  string out_file = param.out_path + file_in + ".root";

  // create output file
  TFile file_out(out_file.c_str(), "NEW");
   if (!file_out.IsOpen()) {
      std::cerr << "ROOT file could not be opend." << std::endl;
      std::cerr << "File probably exists. Prevent overwriting" << std::endl;
      exit(1);
   }
   TTree tree_out("tree", "");
//    int PadAmpl[geom::nPadx][geom::nPady][n::samples];
//    tree_out.Branch("PadAmpl", &PadAmpl, Form("PadAmpl_[%i][%i][%i]/I", geom::nPadx, geom::nPady, n::samples));

    auto event = new TRawEvent();
    tree_out.Branch("TRawEvent", &event, 32000, 0);

   int time_mid, time_msb, time_lsb;
   float TrackerPos[8];

   if (read_tracker)
       tree_out.Branch("Tracker", &TrackerPos, Form("TrackerPos[8]/F"));

   // define the output events number
   long int Nevents;
   Nevents = interface->Scan(-1, true, tmp);

   if (read_tracker) {
     long int N_tracker = tracker->Scan(-1, true, tmp);
     std::cout << N_tracker << " events in the tracker file" << std::endl;
     // overflow by 1 is allowed by trigger design
     if (N_tracker - 1 > Nevents)
       std::cerr << "Number of events in tracker is larger then in TPC" << std::endl;
   }
  if (param.nevents > 0) {
    Nevents = std::min(Nevents, (long int)param.nevents);
  }
  if (!param.verbose)
    std::cout << "Doing conversion" << "\n[                     ]\r[" << std::flush;

  std::vector<float> tracker_data;
  for (long int i = 0; i < Nevents; ++i) {
    if (param.verbose)
      std::cout << "Working on " << i << std::endl;
    else if (Nevents/20 > 0) {
      if (i % (Nevents / 20) == 0)
      std::cout << "#" << std::flush;
    }
    event = interface->GetEvent(i);
//    int time[3];
//    interface->GetEvent(i, PadAmpl, time);
//    time_mid = time[0];
//    time_msb = time[1];
//    time_lsb = time[2];

    tracker_data.clear();
    for (float & data : TrackerPos)
      data = -999.;
    if (tracker->GetEvent(i, tracker_data)) {
      for (auto it = 0;  it < tracker_data.size(); ++it)
        if (tracker_data[it] > 0)
          TrackerPos[it] = tracker_data[it];
    }
    tree_out.Fill();
    delete event;
  } // loop over events

  tree_out.Write("", TObject::kOverwrite);
  file_out.Write();
  file_out.Close();
  std::cout << "\nConversion done" << std::endl;

  return 0;
}