#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

//#pragma link C++ main  T2KMonitor;
#pragma link C++ class Mapping+;
#pragma link C++ class DAQ+;
#pragma link C++ class EventDisplay+;

// Stand-alone Handlers defined in Handlers.h and implemented in main program
#pragma link C++ function T2KConstants;
#pragma link C++ function Globals;

#endif
