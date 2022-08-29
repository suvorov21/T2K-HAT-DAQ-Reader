//
// Created by SERGEY SUVOROV on 29/08/2022.
//

#include <bitset>

#include "InterfaceMidas.hxx"

//******************************************************************************
bool InterfaceMidas::Initialise(const std::string& file_name, int verbose) {
//******************************************************************************
    if (file_name == "")
        return false;
    _reader = TMNewReader(file_name.c_str());
    _verbose = verbose;
    if (_reader->fError) {
        std::cout << "Cannot open input file " <<  file_name << std::endl;
        delete _reader;
        return false;
    }
    std::cout << "Opened " <<  file_name << std::endl;
    _filename = file_name;
    return true;
}

//******************************************************************************
uint64_t InterfaceMidas::Scan(int start, bool refresh, int& Nevents_run) {
//******************************************************************************

    unsigned int events_number = 0;
    if (start < _currentEventIndex){
        delete _reader;
        _reader = TMNewReader(_filename.c_str());
        _currentEventIndex = 0;
    }
//    uint64_t number = 0;
    while (true) {
        _currentEvent = TMReadEvent(_reader);

        if (!_currentEvent) {
            // EOF
            delete _currentEvent;
            break;
        }

        if (_currentEvent->error) {
            // broken event
            printf("event with error, bye!\n");
            delete _currentEvent;
            break;
        }
        _currentEventIndex++;
        events_number++;
    }
    Nevents_run = events_number; /// TODO Multiple files non-supported
    return events_number;
}

TRawEvent* InterfaceMidas::GetEvent(long id) {
    TMEvent* midas_event = GoToEvent(id);
    if (midas_event == nullptr){
        std::cerr << "Cannot go to event id " << id << std::endl;
        exit(1);
    }
    midas_event->FindAllBanks();
    if (midas_event->error){
//        std::cerr << "Error with banks of event " << id << std::endl;
        return nullptr;
    }


    /// Get event number
    auto bank_count = midas_event->FindBank("COUN");
    unsigned int event_number = 0;
    event_number = GetUIntFromBank(midas_event->GetBankData(bank_count));

    if (event_number != id){std::cout << "Error " << id << "\t" << "\t" << std::bitset<32>(id) << "\t" << std::bitset<32>(event_number)
                                      << "\t" << std::bitset<8>(midas_event->GetBankData(bank_count)[3]) << std::bitset<8>(midas_event->GetBankData(bank_count)[2]) << std::bitset<8>(midas_event->GetBankData(bank_count)[1]) << std::bitset<8>(midas_event->GetBankData(bank_count)[0]) << std::endl;}

    /// Define TRawEvent
    auto event = new TRawEvent(event_number);

    /// Get timing of the event
    auto bank_tmsb = midas_event->FindBank("TMSB");
    auto tmsb = GetUShortFromBank(midas_event->GetBankData(bank_tmsb));
    auto bank_tmid = midas_event->FindBank("TMID");
    auto tmid = GetUShortFromBank(midas_event->GetBankData(bank_tmid));
    auto bank_tlsb = midas_event->FindBank("TLSB");
    auto tlsb = GetUShortFromBank(midas_event->GetBankData(bank_tlsb));
    event->SetTime(tmid,tmsb,tlsb);
//    std::cout << "-> Times: " << tmsb << "\t" << tmid << "\t" << tlsb << std::endl;

    /// Get number of waveforms
    auto bank_nwav = midas_event->FindBank("NWAV");
    auto waveformsNumber = GetUShortFromBank(midas_event->GetBankData(bank_nwav));
    if (waveformsNumber <= 0){
        std::cout << "No waveform in event " << id << std::endl;
        return nullptr;
    }
//    std::cout << "-> Number of waveforms: " << waveformsNumber << std::endl;

    /// Common to both versions
    auto bank_wave = midas_event->FindBank("WAVE");
    auto bank_nadc = midas_event->FindBank("NADC");

    unsigned short version = 0;
    /// version 1
    auto bank_femc = midas_event->FindBank("FEMC");
    auto bank_chip = midas_event->FindBank("CHIP");
    auto bank_chan = midas_event->FindBank("CHAN");
    auto bank_tbin = midas_event->FindBank("TBIN");
    if (bank_femc){
        version = 1;
        std::cout << "Using version 1" << std::endl;
    }
    /// version 2
    auto bank_chid = midas_event->FindBank("CHID");
    auto bank_tmin = midas_event->FindBank("TMIN");
    auto bank_tmax = midas_event->FindBank("TMAX");
    if (bank_chid and version == 0) {
        version = 2;
        std::cout << "Using version 2" << std::endl;
    }

//    event->Reserve(waveformsNumber);
    TRawHit* hit;

    std::vector<unsigned short> femc_vector, chip_vector, chan_vector, chid_vector, nadc_vector, tbin_vector, tmin_vector, wave_vector;

    nadc_vector = GetUCharVectorFromBank(midas_event->GetBankData(bank_nadc), waveformsNumber);
    unsigned int total_size_wave = 0;
    for (unsigned int i =0; i < waveformsNumber; i++) total_size_wave+=nadc_vector[i];
    std::cout <<"Total number of ADC counts: " << total_size_wave << std::endl;
    std::cout <<"Number waves: " << waveformsNumber << std::endl;
    wave_vector = GetUShortVectorFromBank(midas_event->GetBankData(bank_wave), total_size_wave);

    std::cout << "here" << std::endl;
    if (version == 1){
        femc_vector = GetUCharVectorFromBank(midas_event->GetBankData(bank_femc), waveformsNumber);
        chip_vector = GetUCharVectorFromBank(midas_event->GetBankData(bank_chip), waveformsNumber);
        chan_vector = GetUCharVectorFromBank(midas_event->GetBankData(bank_chan), waveformsNumber);
        tbin_vector = GetUShortVectorFromBank(midas_event->GetBankData(bank_tbin), waveformsNumber);
    }
    else if (version == 2) {
        chid_vector = GetUShortVectorFromBank(midas_event->GetBankData(bank_chid), waveformsNumber);
        tmin_vector = GetUShortVectorFromBank(midas_event->GetBankData(bank_tmin), waveformsNumber); // we use the number of adc instead of the tmax
        femc_vector.resize(waveformsNumber);
        chip_vector.resize(waveformsNumber);
        chan_vector.resize(waveformsNumber);
        for (unsigned int i = 0; i < waveformsNumber; i++) {
            femc_vector[i] = ExtractFemCard(chid_vector[i]);
            chip_vector[i] = ExtractChipId(chid_vector[i]);
            chan_vector[i] = ExtractChanId(chid_vector[i]);
        }
    }
    else {
        std::cerr << "Version " << version << " not implemented!";
        exit(1);
    }
    unsigned int wave_counter = 0;
    unsigned int counter_adc = 0;
    for (int i =0; i < waveformsNumber; i++) {

        if (chan_vector[i] == 15 or chan_vector[i] == 28 or chan_vector[i] == 53 or chan_vector[i] == 66 or chan_vector[i] <= 2 or chan_vector[i] >= 79)
        {
            std::cout << "Cannot process this channel: " << chan_vector[i] << std::endl;
            continue;
        }
        if (version == 1){
            hit = new TRawHit(femc_vector[i], chip_vector[i], chan_vector[i]);
            std::vector<unsigned int> adc_vector;
            adc_vector.resize(540);
            adc_vector.clear();
            hit->ResetWF();
            for (int jtbin = 0; jtbin < nadc_vector[i]; jtbin++){
                hit->SetADCunit(tbin_vector[jtbin], wave_vector[jtbin]);
            }
        }
        else if (version == 2){
            std::cout << femc_vector[i] << "\t" << chip_vector[i] << "\t" << chan_vector[i] << std::endl;
            hit = new TRawHit(femc_vector[i], chip_vector[i], chan_vector[i]);
            std::vector<unsigned int> adc_vector;
            adc_vector.resize(540);
            adc_vector.clear();
            hit->ResetWF();
            for (int jtbin = 0; jtbin < nadc_vector[i]; jtbin++){
                //std::cout << counter_adc+jtbin << "\t" << tmin_vector[i]+jtbin << "\t" << wave_vector[counter_adc+jtbin] << std::endl;
                hit->SetADCunit(tmin_vector[i]+jtbin, wave_vector[counter_adc+jtbin]);
            }
            counter_adc+= nadc_vector[i];
        }
        hit->ShrinkWF();
        event->AddHit(hit);
    }
    return event;

}

TMEvent* InterfaceMidas::GoToEvent(long id) {

    // if the index is too far away, we need to move to the right location
    if (id < _currentEventIndex){
        delete _reader;
        _reader = TMNewReader(_filename.c_str());
        _currentEvent = TMReadEvent(_reader);
        if (!IsValid(_currentEvent)) {
            // EOF
            _currentEvent = nullptr;
            std::cerr << "End of file was reached during initialization" << std::endl;
            return nullptr;
        }
        _currentEventIndex = 0;
    }

    /// TODO there might be a better way to go to an event, but the midasio lib doesn't allow this with a user-friendly method
//    TMEvent *e = nullptr;
    while (id > _currentEventIndex){
        _currentEvent = TMReadEvent(_reader); // read but drops the event
        if (!IsValid(_currentEvent)) {
            // EOF
            std::cerr << "End of file was reached while " << id << " was requested!" << std::endl;
            return nullptr;
        }
        _currentEventIndex++;
    }
    // if the current index is the right one, nothing to do
    if (id == _currentEventIndex){
        return _currentEvent;
    }
    else {
        std::cerr << "Event id " << _currentEventIndex << " was found while " << id << " was requested!" << std::endl;
        return nullptr;
    }
    return nullptr;
}


bool InterfaceMidas::IsValid(TMEvent* event) {
    if (!event) {
        printf("nulltptr event, bye!\n");
        return false;
    }

    if (event->error) {
        // broken event
        printf("event with error, bye!\n");
        delete event;
        return false;
    }
    return true;
}

unsigned int InterfaceMidas::GetUIntFromBank(char * data) {
    unsigned int value = 0;
    value = (unsigned int)(
        ((unsigned char)data[3] << 24) +
            ((unsigned char)data[2] << 16) +
            ((unsigned char)data[1] << 8) +
            ((unsigned char)data[0] << 0));
    return value;
}

unsigned short InterfaceMidas::GetUShortFromBank(char * data) {
    unsigned short value = 0;
    value = (unsigned short)(((unsigned char)data[1] << 8) +
        ((unsigned char)data[0] << 0));
    return value;
}

std::vector<unsigned short> InterfaceMidas::GetUShortVectorFromBank(char * data, unsigned int nvalues) {
    std::vector<unsigned short> vector;
    vector.resize(nvalues);
    for (int i = 0; i < nvalues; i++){
        vector[i] = (unsigned short)(((unsigned char)data[2*i+1] << 8) +
            ((unsigned char)data[2*i] << 0));
    }
    return vector;
}

unsigned short InterfaceMidas::ExtractFemCard(unsigned short data) {
    auto bits = std::bitset<16>(data);
    return (unsigned short) ((bits.test(11) << 0) +
        (bits.test(12) << 1) +
        (bits.test(13) << 2));
}

unsigned short InterfaceMidas::ExtractChipId(unsigned short data) {
    auto bits = std::bitset<16>(data);
    return (unsigned short) ((bits.test(7) << 0) +
        (bits.test(8) << 1) +
        (bits.test(9) << 2) +
        (bits.test(10) << 3));
}

unsigned short InterfaceMidas::ExtractChanId(unsigned short data) {
    auto bits = std::bitset<16>(data);
    return (unsigned short) ((bits.test(0) << 0) +
        (bits.test(1) << 1) +
        (bits.test(2) << 2) +
        (bits.test(3) << 3) +
        (bits.test(4) << 4) +
        (bits.test(5) << 5) +
        (bits.test(6) << 6));
}

std::vector<unsigned short> InterfaceMidas::GetUCharVectorFromBank(char * data, unsigned int nvalues) {
    std::vector<unsigned short> vector;
    vector.resize(nvalues);
    for (int i = 0; i < nvalues; i++){
        vector[i] = (unsigned short)((unsigned char)data[i] << 0);
    }
    return vector;
}
