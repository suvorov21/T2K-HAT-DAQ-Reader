#include "EventDisplay.hxx"
#include "fdecoder.h"
#include "datum_decoder.h"
#include "platform_spec.h"
#include "frame.h"
#include "Interface.hxx"

void help()
{
  printf("monitor <options>\n");
  printf("   -h                   : print usage\n");
  printf("   -i <input_file>      : input file name with a path\n");
  exit(1);
}

int main(int argc, char **argv) {
   TString name;
   for (;;) {
    int c = getopt(argc, argv, "i:o:p:n:ht");
    if (c < 0) break;
    switch (c) {
      case 'i' :name          = optarg;       break;

      case '?' : help();
    }
  }
  if (argc == 1)
    help();

   TApplication theApp("App", &argc,argv);
   new EventDisplay(gClient->GetRoot(), 1000, 1000, name);
   theApp.Run();
   return 0;
}
