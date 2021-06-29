#include "T2KConstants.h"
#include "DAQ.h"

#include <fstream>
#include <string>
#include <iostream>

using namespace std;

void DAQ::loadDAQ()
{
    int det, fec;

    ifstream det2fec((loc::daq + "detector2fec.txt").c_str());
    while (!det2fec.eof())
    {
        det2fec >> det >> fec >> ws;
        detector2fec[det]=fec;
        fec2detector[fec]=det;
    }
    det2fec.close();

    int daq;
    ifstream ffec2daq((loc::daq + "fec2daq.txt").c_str());
    while (!ffec2daq.eof())
    {
        ffec2daq >> fec >> daq >> ws;
        fec2daq[fec]=daq;
        daq2fec[daq]=fec;
    }
    ffec2daq.close();

}
