# HAT TPC DAQ reader

The tool with

1. AQS --> ROOT converter
2. Event display and monitoring


## Compiling
```bash
mkdir build; cd build;
cmake ../
make
```

## Converter
Converts the AQS files to ROOT
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

WARNING! The converter does NOT allow output file overwriting, in order to prevent data loss. If there are old file, please, delete them manually or choose a different output location.

## EventMonitor

The event monitor could run over both AQS and ROOT files. The interface will be chosen automatically.

```bash
./app/Monitor -i ~/DATA/R2021_06_10-16_30_21-000.aqs
```

The verbosity could be done with `-v1` flag. So far only one level of verbosity is available.