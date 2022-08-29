//
// Created by SERGEY SUVOROV on 29/08/2022.
//

#ifndef DAQ_READER_SRC_INTERFACEMIDAS_HXX_
#define DAQ_READER_SRC_INTERFACEMIDAS_HXX_

#include "InterfaceBase.hxx"

/// Midas file reader

class InterfaceMidas : public InterfaceBase {
 public:
    explicit InterfaceMidas(){_currentEvent = new TMEvent();};
    ~InterfaceMidas() override = default;
    bool Initialise(const std::string &file_name, int verbose) override;
    uint64_t Scan(int start, bool refresh, int &Nevents_run) override;
    TRawEvent *GetEvent(long int id) override;
    void GetTrackerEvent(long int id, Float_t pos[8]) override {
        throw std::logic_error("No tracker info in TRawEvent");
    }

 private:
    TMEvent* GoToEvent(long int id);
    bool IsValid(TMEvent*);
    unsigned int GetUIntFromBank(char*);
    unsigned short GetUShortFromBank(char*);
    std::vector<unsigned short> GetUShortVectorFromBank(char*, unsigned int = 0);
    std::vector<unsigned short> GetUCharVectorFromBank(char*, unsigned int = 0);
    unsigned short ExtractFemCard(unsigned short data);
    unsigned short ExtractChipId(unsigned short data);
    unsigned short ExtractChanId(unsigned short data);
//    std::vector<unsigned short> GetFemCardVectorFromBank(char*, unsigned int = 0);
//    std::vector<unsigned short> GetChipVectorFromBank(char*, unsigned int = 0);
//    std::vector<unsigned short> GetChanVectorFromBank(char*, unsigned int = 0);

 private:
    std::string _filename;
    TMReaderInterface* _reader;
    unsigned int _currentEventIndex{0};
    TMEvent* _currentEvent{};
//    TTree *_tree_in;
    TRawEvent *_event;

};


#endif //DAQ_READER_SRC_INTERFACEMIDAS_HXX_
