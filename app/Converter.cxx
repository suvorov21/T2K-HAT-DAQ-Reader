#include "InterfaceFactory.hxx"
#include "Output.hxx"

#include <iostream>

#include "CmdLineParser.h"

#include "TString.h"

int main(int argc, char **argv) {
//    Param param;

    // inti CLI module
    CmdLineParser clParser;
    clParser.setIsUnixGnuMode(true);
    clParser.setIsFascist((true));

    // define CLI
    clParser.addOption("input_file", {"-i", "--input"}, "Input file name");
    clParser.addOption("output_path", {"-o", "--output"}, "Output path");
    clParser.addOption("verbose", {"-v", "--verbose"}, "Verbosity level");
    clParser.addOption("tracker", {"-s", "--silicon"}, "Add silicon tracker info");
    clParser.addOption("nEventsFile", {"-n", "--nEventsFile"}, "Number of events to process");

    clParser.addOption("array", {"--array"}, "Convert to 3D array");
    clParser.addOption("card", {"-c", "--card"}, "Specify the particular card that will be converted.");

    clParser.addTriggerOption("help", {"-h", "--help"}, "Print usage");

    // do parsing
    clParser.parseCmdLine(argc, argv);

    if (clParser.isOptionTriggered("help")) {
        std::cout << clParser.getConfigSummary();
        exit(0);
    }

    // assign results
    auto fileName = clParser.getOptionVal<std::string>("input_file", "", 0);
    auto outPath = clParser.getOptionVal<std::string>("output_path", "", 0);
    auto trackerName = clParser.getOptionVal<std::string>("tracker", "", 0);

    auto nEventsRead = clParser.getOptionVal<uint64_t>("nEventsFile", 0, 0);
    auto verbose = clParser.getOptionVal<int>("verbose", 1, 0);

    bool useArray = clParser.isOptionTriggered("array");
    auto card = clParser.getOptionVal<int>("card", 0, 0);

    // define the proper interface to read it
    std::shared_ptr<InterfaceBase> interface = InterfaceFactory::get(fileName);
    if (!interface->Initialise(fileName, verbose)) {
        std::cerr << "Interface initialisation fails. Exit" << std::endl;
        exit(1);
    }

    // Whether to read silicon tracker stuff
    auto tracker = std::make_shared<InterfaceTracker>();
    auto read_tracker = tracker->Initialise(trackerName, verbose);

    // extract the file name from the input
    TString out_file = OutputBase::getFileName(outPath, fileName);

    // Select the output format
    std::shared_ptr<OutputBase> output;
    if (useArray) {
        output = std::make_shared<OutputArray>();
        output->SetCard(card);
    } else {
        output = std::make_shared<OutputTRawEvent>();
    }
    output->Initialise(out_file, read_tracker);

    // define the output events number
    uint64_t nEventsFile;
    nEventsFile = interface->Scan(-1, true, tmp);

    if (read_tracker) {
        uint64_t N_tracker = tracker->Scan(-1, true, tmp);
        std::cout << N_tracker << " events in the tracker file" << std::endl;
        // overflow by 1 is allowed by trigger design
        if (N_tracker - 1 > nEventsFile)
            std::cerr << "Number of events in tracker is larger then in TPC" << std::endl;
    }
    if (nEventsRead > 0) {
        nEventsFile = std::min(nEventsFile, nEventsRead);
    }
    if (verbose == 1)
        std::cout << "Doing conversion" << "\n[                     ]\r[" << std::flush;

    for (long int i = 0; i < nEventsFile; ++i) {
        if (verbose > 1)
            std::cout << "Working on " << i << std::endl;
        else if (verbose == 1 && nEventsFile / 20 > 0) {
            if (i % (nEventsFile / 20) == 0)
                std::cout << "#" << std::flush;
        }
        output->AddEvent(interface->GetEvent(i));

        if (read_tracker) {
            std::vector<float> tracker_data;
            tracker->GetEvent(i, tracker_data);
            output->AddTrackerEvent(tracker_data);
        }
        output->Fill();
    } // loop over events

    output->Finilise();
    if (verbose > 0)
        std::cout << "\nConversion done" << std::endl;

    return 0;
}