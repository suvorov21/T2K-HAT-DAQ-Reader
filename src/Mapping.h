#ifndef Mapping_h
#define Mapping_h

#include "T2KConstants.h"

class Mapping
{
    public :
        // Constructors
        void loadMapping();

        // Getters
        int ichip(int card, int chip, int bin){return m_ichip[card][chip][bin];}
        int jchip(int card, int chip, int bin){return m_jchip[card][chip][bin];}
        int connector(int card, int chip, int ichip, int jchip){return m_connector[card][chip][ichip][jchip];}
        int i(int card, int chip, int bin);
        int j(int card, int chip, int bin);

        // Other

    private :
        int m_ichip[n::cards][n::chips][n::bins];
        int m_jchip[n::cards][n::chips][n::bins];
        int m_connector[n::cards][n::chips][geom::padOnchipx][geom::padOnchipy];
};

#endif
