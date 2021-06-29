# HAT TPC DAQ reader

## Compiling
```bash
mkdir build; cd build;
cmake ../
make
```

## Converter
Converts the AQS files to ROOT
```bash
./app/Converter -i ~/DATA/R2019_06_16-19_45_58-000.root -o ./
```

Number of events could be limited with
```bash
./app/Converter -i ~/DATA/R2019_06_16-19_45_58-000.root -n 10 -o ./
```

## EventMonitor
```bash
./app/Monitor -i ~/DATA/R2021_06_10-16_30_21-000.aqs
```