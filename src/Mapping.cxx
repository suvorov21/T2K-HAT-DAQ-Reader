#include "T2KConstants.h"
#include "Mapping.h"

#include <fstream>
#include <string>
#include <iostream>

using namespace std;

void Mapping::loadMapping()
{
    // Load in the correspondance betwwen pad coordinates and pin connectors
            /*
                    --> Cards
                ---  ---  ---  ---
    C          | A || C || A || C |
    h          | A || C || A || C |
    i           ---  ---  ---  ---
    p          | B || D || B || D |
    s          | B || D || B || D |
                ---  ---  ---  ---



        */

    int i, j, connector;

    ifstream A((loc::mapping + "ChipA.txt").c_str());
    while (!A.eof())
    {
        A >> j >> i >> connector >> ws;
        //int index = channel - 3;
        //if (channel >=43) index -= 6;
        int index = connector;
        m_ichip[0][0][index] = i ;
        m_jchip[0][0][index] = j ;
        m_ichip[0][1][index] = i ;
        m_jchip[0][1][index] = j ;
        m_ichip[1][0][index] = i ;
        m_jchip[1][0][index] = j ;
        m_ichip[1][1][index] = i ;
        m_jchip[1][1][index] = j ;
        // Coord to channel
        m_connector[0][0][i][j] = connector ;
        m_connector[0][1][i][j] = connector ;
        m_connector[1][0][i][j] = connector ;
        m_connector[1][1][i][j] = connector ;


    }
    A.close();

    ifstream B((loc::mapping + "ChipB.txt").c_str());
    while(!B.eof())
    {
        B >> j >> i >> connector >> ws;
        //int index = channel - 3;
        //if (channel >=43) index -= 6;
        int index = connector;
        m_ichip[0][2][index] = i ;
        m_jchip[0][2][index] = j ;
        m_ichip[0][3][index] = i ;
        m_jchip[0][3][index] = j ;
        m_ichip[1][2][index] = i ;
        m_jchip[1][2][index] = j ;
        m_ichip[1][3][index] = i ;
        m_jchip[1][3][index] = j ;
        // Coord to channel
        m_connector[0][2][i][j] = connector ;
        m_connector[0][3][i][j] = connector ;
        m_connector[1][2][i][j] = connector ;
        m_connector[1][3][i][j] = connector ;
    }
    B.close();

    ifstream C((loc::mapping + "ChipC.txt").c_str());
    while(!C.eof())
    {
        C >> j >> i >> connector >> ws;;
        //int index = channel - 3;
        //if (channel >=43) index -= 6;
        int index = connector;
        m_ichip[0][4][index] = i ;
        m_jchip[0][4][index] = j ;
        m_ichip[0][5][index] = i ;
        m_jchip[0][5][index] = j ;
        m_ichip[1][4][index] = i ;
        m_jchip[1][4][index] = j ;
        m_ichip[1][5][index] = i ;
        m_jchip[1][5][index] = j ;
        // Coord to channel
        m_connector[0][4][i][j] = connector ;
        m_connector[0][5][i][j] = connector ;
        m_connector[1][4][i][j] = connector ;
        m_connector[1][5][i][j] = connector ;
    }
    C.close();

    ifstream D((loc::mapping + "ChipD.txt").c_str());
    while(!D.eof())
    {
        D >> j >> i >> connector >> ws;
        //int index = channel - 3;
        //if (channel >=43) index -= 6;
        int index = connector;
        m_ichip[0][6][index] = i ;
        m_jchip[0][6][index] = j ;
        m_ichip[0][7][index] = i ;
        m_jchip[0][7][index] = j ;
        m_ichip[1][6][index] = i ;
        m_jchip[1][6][index] = j ;
        m_ichip[1][7][index] = i ;
        m_jchip[1][7][index] = j ;
        // Coord to channel
        m_connector[0][6][i][j] = connector ;
        m_connector[0][7][i][j] = connector ;
        m_connector[1][6][i][j] = connector ;
        m_connector[1][7][i][j] = connector ;

    }
    D.close();

    ifstream R((loc::mapping + "reverseMap.txt").c_str());
    int x, y, chip, channel;
    while(!R.eof()) {
        R >> x >> y >> chip >> channel;
        rev_map[std::make_pair(x, y)] = std::make_pair(chip, channel);
    }
}


int Mapping::i(int card, int chip, int bin)
{
    int result;
    result = m_ichip[card][chip][bin] + (1-card)*geom::padOnchipx*geom::chipOnx + (chip/geom::chipOny)*geom::padOnchipx;
    return result;
}

int Mapping::j(int card, int chip, int bin)
{
    int result;
    result=m_jchip[card][chip][bin] + (geom::chipOny-1-(chip%geom::chipOny))*geom::padOnchipy;
    return result;
}

std::pair<int, int> Mapping::getElectronics(int row, int column) {
    return rev_map[std::make_pair(row, column)];
}
