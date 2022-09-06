//
// Created by SERGEY SUVOROV on 29/08/2022.
//

#include "InterfaceAqs.hxx"

#include <unordered_map>

//******************************************************************************
bool InterfaceAQS::Initialise(const std::string& file_namme, int verbose) {
//******************************************************************************
    _verbose = verbose;
    std::cout << "Initialise AQS interface" << std::endl;
    _fsrc = fopen(file_namme.c_str(), "rb");
    _daq.loadDAQ();
    cout << "...DAQ loaded successfully" << endl;

    _t2k.loadMapping();
    cout << "...Mapping loaded succesfully." << endl;
    _firstEv = -1;

    if (_fsrc == nullptr) {
        std::cerr << "Input file could not be read" << std::endl;
        std::cerr << "File: " << _fsrc << std::endl;
    }

    return true;
}

//******************************************************************************
uint64_t InterfaceAQS::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************
    // Reset _eventPos vector
    // Scan the file
    DatumContext_Init(&_dc, _sample_index_offset_zs);
    unsigned short datum;
    int err;
    bool done = true;
    int prevEvnum = -1;
    int evnum;
    if (_verbose > 0 || refresh)
        std::cout << "\nScanning the file..." << std::endl;
    if (refresh) {
        _eventPos.clear();
        fseek(_fsrc, 0, SEEK_SET);
        lastRead = 0;
    }
    else {
        fseek(_fsrc, _eventPos[start].first, SEEK_SET);
    }

    _fea.TotalFileByteRead = lastRead;
    while (done) {
        // Read one short word
        if (fread(&datum, sizeof(unsigned short), 1, _fsrc) != 1) {
            done = false;
        }
        else {
            _fea.TotalFileByteRead += sizeof(unsigned short);
            // Interpret datum
            if ((err = Datum_Decode(&_dc, datum)) < 0) {
                printf("%d Datum_Decode: %s\n", err, &_dc.ErrorString[0]);
                done = true;
            }
            else {
                if (_dc.isItemComplete) {
                    if (_dc.ItemType == IT_START_OF_EVENT) {
                        evnum = (int)_dc.EventNumber;
                        if (_firstEv < 0) {
                            if (_verbose > 1)
                                std::cout << "First event id  " << evnum << "  at  " << _fea.TotalFileByteRead - 6*sizeof(unsigned short) << std::endl;
                            _eventPos.emplace_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum);
                            _firstEv = evnum;
                            prevEvnum = evnum;
                            lastRead = _fea.TotalFileByteRead - 6*sizeof(unsigned short);
                        }
                        else if (evnum != prevEvnum) {
                            if (_fea.TotalFileByteRead - 6*sizeof(unsigned short) != _eventPos[_eventPos.size()-1].first) {
                                _eventPos.emplace_back(_fea.TotalFileByteRead - 6*sizeof(unsigned short), evnum);
                                if (_verbose > 1) {
                                    std::cout << "Event " << evnum << " at " << _fea.TotalFileByteRead - 6 * sizeof(unsigned short)
                                              << std::endl;
                                    std::cout << "time lsb:msb:mid : " << _dc.EventTimeStampLsb << " " << _dc.EventTimeStampMsb << " "
                                              << _dc.EventTimeStampMid << std::endl;
                                }
                                prevEvnum = evnum;
                                lastRead = _fea.TotalFileByteRead - 6*sizeof(unsigned short);
                            }
                            else {
                                prevEvnum = evnum;
                            }
                        }
                    }
                    else if (_dc.ItemType == IT_ADC_SAMPLE) {}
                    else if (_dc.ItemType == IT_DATA_FRAME) {}
                    else if (_dc.ItemType == IT_END_OF_FRAME) {}
                    else if (_dc.ItemType == IT_MONITORING_FRAME) {}
                    else if (_dc.ItemType == IT_CONFIGURATION_FRAME) {}
                    else if (_dc.ItemType == IT_SHORT_MESSAGE) {}
                    else if (_dc.ItemType == IT_LONG_MESSAGE) {}
                    else if (_dc.ItemType == IT_TIME_BIN_INDEX) {}
                    else if (_dc.ItemType == IT_CHANNEL_HIT_HEADER) {}
                    else if (_dc.ItemType == IT_DATA_FRAME) {}
                    else if (_dc.ItemType == IT_NULL_DATUM) {}
                    else if (_dc.ItemType == IT_CHANNEL_HIT_COUNT) {}
                    else if (_dc.ItemType == IT_LAST_CELL_READ) {}
                    else if (_dc.ItemType == IT_END_OF_EVENT) {}
                    else if (_dc.ItemType == IT_PED_HISTO_MD) {}
                    else if (_dc.ItemType == IT_UNKNOWN) {}
                    else if (_dc.ItemType == IT_CHAN_PED_CORRECTION) {}
                    else if (_dc.ItemType == IT_CHAN_ZERO_SUPPRESS_THRESHOLD) {}
                    else
                    {
                        cerr << "Interface.cxx: Unknown Item Type : " << _dc.ItemType << endl;
                        cerr << "Program will exit to prevent the storing of the corrupted data" << endl;
                        exit(1);
                    }
                }
            }
        }
    }
    if (refresh || _verbose > 0) {
        cout << "Scan done." << std::endl;
        cout << _eventPos.size() << " events in the file." << std::endl;
    }

    Nevents_run = prevEvnum;
    return _eventPos.size();
}

//******************************************************************************
TRawEvent* InterfaceAQS::GetEvent(long int id) {
//******************************************************************************
    unsigned short datum;
    int err;
    bool done = true;
    if (_verbose > 1)
        std::cout << "\nGetting event #" << id << "  at pos  " << _eventPos[id].first << "  with id  " << _eventPos[id].second << std::endl;;

    fseek(_fsrc, _eventPos[id].first, SEEK_SET);

    // clean the padAmpl
    auto event = new TRawEvent(id);
    int eventNumber = -1;
    std::unordered_map<int32_t, TRawHit*> hitMap;

    while (done) {
        if (fread(&datum, sizeof(unsigned short), 1, _fsrc) != 1) {
            done = false;
            if (ferror(_fsrc))
                cout << "\nERROR" << endl;
            if (feof(_fsrc))
                cout << "\nreach EOF" << endl;
        } else {

            _fea.TotalFileByteRead += sizeof(unsigned short);
            // Interpret datum
            if ((err = Datum_Decode(&_dc, datum)) < 0) {
                printf("%d Datum_Decode: %s\n", err, &_dc.ErrorString[0]);
                done = true;
            }

            // Decode
            if (_dc.isItemComplete) {

                if (_dc.ItemType == IT_START_OF_EVENT) {
                    if (_verbose > 1)
                        std::cout << "Found event with id " << (int)_dc.EventNumber << std::endl;
                    event->SetTime(_dc.EventTimeStampMid,
                                   _dc.EventTimeStampMsb,
                                   _dc.EventTimeStampLsb);

                    if ((int)_dc.EventNumber == _eventPos[id].second)
                        eventNumber = (int)_dc.EventNumber;
                } else if (eventNumber == _eventPos[id].second && _dc.ItemType == IT_ADC_SAMPLE) {
                    if (_dc.ChannelIndex != 15 && _dc.ChannelIndex != 28 && _dc.ChannelIndex != 53 && _dc.ChannelIndex != 66 && _dc.ChannelIndex > 2 && _dc.ChannelIndex < 79) {

                        if (_verbose > 1) {
                            std::cout << "card\t" << _dc.CardIndex << "\t" << _dc.ChipIndex << "\t" << _dc.ChannelIndex << std::endl;
                        }
                        auto chHash = HashChannel(_dc.CardIndex, _dc.ChipIndex, _dc.ChannelIndex);
                        auto hitIt = hitMap.find(chHash);


                        int a = (int)_dc.AbsoluteSampleIndex;
                        int b = (int)_dc.AdcSample;

                        if (hitIt != hitMap.end()) {
                            hitMap[chHash]->SetADCunit(a, b);
                        } else {
                            auto hitCandidate = new TRawHit(_dc.CardIndex,
                                                            _dc.ChipIndex,
                                                            _dc.ChannelIndex);
                            hitCandidate->ResetWF();
                            hitCandidate->SetADCunit(a, b);
                            hitMap[chHash] = hitCandidate;
                        }
                    }
                }
                else if (_dc.ItemType == IT_END_OF_EVENT) {
                    // go to the next event
                    done = false;
                }

            } // end of if (_dc.isItemComplete)
        } // end of second loop inside while
    } // end of while(done) loop

    event->Reserve(hitMap.size());
    for (auto& hit : hitMap) {
        hit.second->ShrinkWF();
        event->AddHit(hit.second);
    }
    return event;
}

int32_t InterfaceAQS::HashChannel(const int card, const int chip, const int channel) {
    return card*16*80 + chip*80 + channel;
}

