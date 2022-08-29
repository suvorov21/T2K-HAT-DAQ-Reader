//
// Created by SERGEY SUVOROV on 29/08/2022.
//

#ifndef DAQ_READER_SRC_INTERFACEAQS_HXX_
#define DAQ_READER_SRC_INTERFACEAQS_HXX_

#include "InterfaceBase.hxx"

/// AQS file reader
class InterfaceAQS : public InterfaceBase {
 public:
    explicit InterfaceAQS() = default;;
    ~InterfaceAQS() override = default;
    bool Initialise(const std::string &file_name, int verbose) override;
    uint64_t Scan(int start, bool refresh, int &Nevents_run) override;
    TRawEvent *GetEvent(long int id) override;
    void GetTrackerEvent(long int id, Float_t pos[8]) override {
        throw std::logic_error("No tracker info in AQS");
    }

 private:
    Features _fea;
    std::vector<std::pair<long int, int> > _eventPos;
    __int64 lastRead;
    DatumContext _dc;
    FILE* _fsrc{nullptr};
    int _sample_index_offset_zs;
    DAQ _daq;
    Mapping _t2k;

    int _firstEv;
};

#endif //DAQ_READER_SRC_INTERFACEAQS_HXX_
