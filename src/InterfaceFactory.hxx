//
// Created by SERGEY SUVOROV on 29/08/2022.
//

#ifndef DAQ_READER_SRC_INTERFACEFACTORY_HXX_
#define DAQ_READER_SRC_INTERFACEFACTORY_HXX_

#include "InterfaceBase.hxx"
#include "InterfaceRoot.hxx"
#include "InterfaceMidas.hxx"
#include "InterfaceAqs.hxx"

/// Factory returns proper input interface
class InterfaceFactory {
 public:
    static std::shared_ptr<InterfaceBase> get(const TString &file_name) {
        if (file_name.EndsWith(".aqs")) {
            return std::make_shared<InterfaceAQS>();
        }
        if (file_name.EndsWith(".mid.lz4")) {
            return std::make_shared<InterfaceMidas>();
        }

        if (file_name.EndsWith(".root")) {
            TFile file(file_name);
            if (file.Get<TTree>("EventTree")) {
                return std::make_shared<InterfaceRawEvent>();
            } else if (file.Get<TTree>("tree")) {
                return std::make_shared<InterfaceROOT>();
            } else {
                std::cerr << "ERROR in converter. Unknown ROOT file type." << std::endl;
            }
        };

        std::cerr << "ERROR in converter. Unknown file type." << std::endl;
        return nullptr;
    }
};

#endif //DAQ_READER_SRC_INTERFACEFACTORY_HXX_
