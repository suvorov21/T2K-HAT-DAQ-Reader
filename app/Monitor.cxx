#include "EventDisplay.hxx"
#include "platform_spec.h"

void help()
{
  printf("monitor <options>\n");
  printf("   -h                   : print usage\n");
  printf("   -i <input_file>      : input file name with a path\n");
  printf("   -v <int>             : verbosity level\n");
  exit(1);
}

int main(int argc, char **argv) {
   std::string name;
   int verbose = 0;
   for (;;) {
    int c = getopt(argc, argv, "i:v:");
    if (c < 0) break;
    switch (c) {
      case 'i' :name          = optarg;       break;
      case 'v' :verbose       = atoi(optarg); break;

      default : help();
    }
  }
  if (argc == 1 || name.empty())
    help();

   TApplication theApp("App", &argc,argv);
   new EventDisplay(gClient->GetRoot(), 1000, 1000, name, verbose);
   theApp.Run();
   return 0;
}
