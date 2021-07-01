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
  printf("   -o <output_path>     : output files path\n");
  printf("   -p <Value>           : number of pre-samples below threshold in zero-suppressed mode\n");
  printf("   -n <Value>           : spesify the number of events");
  printf("   -v <level>           : verbose\n");
  printf("   -t                   : test converter workability - run iver 30 first events\n");
  printf("   -f <0xFlags>         : flags to determine printed items\n");
  exit(1);
}

// ******************************************************************************
// parse_cmd_args() to parse command line arguments
// ******************************************************************************
int parse_cmd_args(int argc, char **argv, Param *p)
{
  /*int i;
  int match = 0;
  int err = 0;*/

  for (;;) {
    int c = getopt(argc, argv, "i:o:p:n:ht");
    if (c < 0) break;
    switch (c) {
      case 'i' : p->inp_file          = optarg;       break;
      case 'o' : p->out_path          = optarg;       break;
      case 'v' : p->verbose              = atoi(optarg); break;
      case 't' : p->test              = true;         break;
      case 'f' : p->vflag                   = atoi(optarg); break;
      case 'p' : p->sample_index_offset_zs  = atoi(optarg); break;
      case 'n' : p->nevents              = atoi(optarg); break;
      case '?' : help();
    }
  }
  if (argc == 1)
    help();

  return (0);
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
      std:cerr << "ERROR in converter. Unknown file type." << std::endl;
      exit(1);
   }

   if (!interface->Initialise(file_name, param.verbose)) {
      std::cerr << "Interface initialisation fails. Exit" << std::endl;
      exit(1);
   }

   // extract the file name from the input
   std::string file_in = param.inp_file;
   while (file_in.find("/") != string::npos)
      file_in = file_in.substr(file_in.find("/") + 1);
   file_in = file_in.substr(0, file_in.find("."));

  string out_file = param.out_path + file_in + ".root";

   TFile file_out(out_file.c_str(), "NEW");
   if (!file_out.IsOpen()) {
      std::cerr << "ROOT file could not be opend." << std::endl;
      std::cerr << "File probably exists. Prevent overwriting" << std::endl;
      exit(1);
   }
   TTree tree_out("tree", "");
   int PadAmpl[geom::nPadx][geom::nPady][n::samples];
   tree_out.Branch("PadAmpl", &PadAmpl, Form("PadAmpl[%i][%i][%i]/I", geom::nPadx, geom::nPady, n::samples));

   // define the output events number
   int Nevents;
   Nevents = interface->Scan();
   if (param.nevents > 0) {
      Nevents = std::min(Nevents, param.nevents);
   }
   std::cout << "Doing conversion" << "\n[                     ]\r[" << std::flush;

   for (auto i = 0; i < Nevents; ++i) {
      if (Nevents/20 > 0) {
         if (i % (Nevents/20) == 0)
            std::cout << "#" << std::flush;
         interface->GetEvent(i, PadAmpl);
         tree_out.Fill();
      }
   }

   tree_out.Write();
   file_out.Write();
   file_out.Close();
   std::cout << "\nConvertion done" << std::endl;

   return 0;
}