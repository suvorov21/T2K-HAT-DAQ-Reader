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

   InterfaceBase interface;

   TString file_name = TString(param.inp_file);
   InterfaceROOT* interface_root = NULL;
   InterfaceAQS* interface_aqs = NULL;

   if (file_name.EndsWith(".root")) {
      param.use_root = true;
      interface_root = new InterfaceROOT();
   } else if (file_name.EndsWith(".aqs")) {
      interface_aqs = new InterfaceAQS();
      // work_interface = static_cast<InterfaceAQS*>(work_interface);
      param.use_aqs = true;
   } else {
      std:cerr << "ERROR in converter. Unknown file type." << std::endl;
      exit(1);
   }

   if (param.use_root)
      interface_root->Initialise(file_name);
   else if (param.use_aqs)
      interface_aqs->Initialise(file_name);

   // extract the file name from the input
   std::string file_in = param.inp_file;
   while (file_in.find("/") != string::npos)
      file_in = file_in.substr(file_in.find("/") + 1);
   file_in = file_in.substr(0, file_in.find("."));

  string out_file = param.out_path + file_in + ".root";

   TFile file_out(out_file.c_str(), "RECREATE");
   TTree tree_out("tree", "");
   int PadAmpl[geom::nPadx][geom::nPady][n::samples];
   tree_out.Branch("PadAmpl", &PadAmpl, Form("PadAmpl[%i][%i][%i]/I", geom::nPadx, geom::nPady, n::samples));

   int Nevents;
   if (param.use_root)
      Nevents = interface_root->Scan();
   else if (param.use_aqs)
      Nevents = interface_aqs->Scan();
   if (param.nevents > 0) {
      Nevents = std::min(Nevents, param.nevents);
   }
   for (auto i = 0; i < Nevents; ++i) {
      if (param.use_root)
         interface_root->GetEvent(i, PadAmpl);
      else if (param.use_aqs)
         interface_aqs->GetEvent(i, PadAmpl);

      tree_out.Fill();
   }

   tree_out.Write();
   file_out.Write();
   file_out.Close();
   std::cout << "Convertion done" << std::endl;

   return 0;
}