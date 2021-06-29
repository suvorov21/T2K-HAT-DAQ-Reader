#ifndef DAQ_h
#define DAQ_h

#include "T2KConstants.h"

class DAQ
{
    public :
        // Constructors
        void loadDAQ();

        // Getters
        int DAQchannel(int detector)
        {
          int res;
          res = fec2daq[detector2fec[detector]];
          if (res==-99 && detector==63){res=19;}
          return(res);
        }
        int connector(int daqchannel){return(fec2detector[daq2fec[daqchannel]]);}

        // Other

    private :
        int detector2fec[n::bins];
        int fec2daq[n::bins];
        int daq2fec[n::bins];
        int fec2detector[n::bins];
};

#endif
