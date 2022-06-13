#ifndef DAQ_h
#define DAQ_h

#include <iostream>

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
        int connector(int daqchannel){
          if (daqchannel< 0 or daqchannel > n::bins)
            std::cerr << "Requested channel " << daqchannel << " larger than " << n::bins << std::endl;
          return (fec2detector[daq2fec[daqchannel]]);
        }

        // Other
        void printDAQ2Fec(){
          for (unsigned int i =0; i< n::bins; i++){
            std::cout << i << "\t" << daq2fec[i] << std::endl;
          }
        }

    private :
        int detector2fec[n::bins];
        int fec2daq[n::bins];
        int daq2fec[n::bins];
        int fec2detector[n::bins];
};

#endif
