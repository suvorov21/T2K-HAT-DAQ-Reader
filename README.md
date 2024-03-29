# HAT TPC DAQ reader

The tool contains input/output interfaces for HAT DAQ files. 
Any conversion can be done as well as UI visualisation (EventDisplay).

### Input
The supported inputs formats are:
1. AQS
2. ROOT file with 3D array: `[32][36][511]`
3. ROOT file with TRawEvent. The class is defined in [hat_event](https://gitlab.com/t2k-beamtest/hat_event) package.
4. Midas `.mid.lz4` format

### Output:
Supported output formats
1. ROOT file with 3D array: `[32][36][511]`. 
Requires `--array` flag and `--card 1` flag with a particular FEM card number to store
2. ROOT file with TRawEvent (default option)
3. Text ASCII format


## Compiling
```bash
git submodule update --init --recursive
mkdir build; cd build;
cmake ../
make
```

## Converter
Converts the files from any [input](#Input) format to any [output](#Output). 
The input format is chosen automatically based on the file type. 

The CLI interface is the following:
```bash
input_file {-i,--input}: Input file name (expected: 1 value)
output_path {-o,--output}: Output path (expected: 1 value)
verbose {-v,--verbose}: Verbosity level (expected: 1 value)
tracker {-s,--silicon}: Add silicon tracker info (expected: 1 value)
nEventsFile {-n,--nEventsFile}: Number of events to process (expected: 1 value)
text {--text}: Convert to text file (trigger)
array {--array}: Convert to 3D array (expected: 1 value)
card {-c,--card}: Specify the particular card that will be converted. (expected: 1 value)
help {-h,--help}: Print usage (trigger)
Command Line Args: { "--help" }
```

Example:
```bash
./app/Converter -i ~/DATA/R2019_06_16-19_45_58-000.aqs -o ./
```

Number of events could be limited with
```bash
./app/Converter -i ~/DATA/R2019_06_16-19_45_58-000.aqs -n 10 -o ./
```

The ASCII data from silicon tracker can be embedded with
```bash
./app/Converter -i ~/DATA/R2019_06_16-19_45_58-000.aqs -n 10 -o ./ -s tracker_analysis_output.dat
```

The routine can be automated with python script
```bash
python3 ./script/converter.py -e build/app/Converter -i /input_dir/aqs -o /output/ROOT/
```

WARNING! The converter does NOT allow output file overwriting, in order to 
prevent data loss. If there are old files, please, delete them manually 
or choose a different output location.

## EventMonitor

The event monitor could run over any of the input file formats. 
The interface will be chosen automatically.

```bash
./app/Monitor -i ~/DATA/R2021_06_10-16_30_21-000.aqs
```

![](resources/ed.gif)

The verbosity could be done with `-v1` flag. 
So far two level of verbosity are available.