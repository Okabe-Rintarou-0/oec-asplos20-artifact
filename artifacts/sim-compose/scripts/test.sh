#!/bin/bash

if [ ! -d "../logs/001" ]
then
  mkdir "../logs/001"
fi
if [ ! -d "../logs/002" ]
then
  mkdir "../logs/002"
fi
if [ ! -d "../logs/003" ]
then
  mkdir "../logs/003"
fi
if [ ! -d "../logs/004" ]
then
  mkdir "../logs/004"
fi
if [ ! -d "../logs/006" ]
then
  mkdir "../logs/006"
fi
if [ ! -d "../logs/008" ]
then
  mkdir "../logs/008"
fi
if [ ! -d "../logs/012" ]
then
  mkdir "../logs/012"
fi
if [ ! -d "../logs/016" ]
then
  mkdir "../logs/016"
fi
if [ ! -d "../logs/024" ]
then
  mkdir "../logs/024"
fi
if [ ! -d "../logs/032" ]
then
  mkdir "../logs/032"
fi
if [ ! -d "../logs/048" ]
then
  mkdir "../logs/048"
fi
if [ ! -d "../logs/064" ]
then
  mkdir "../logs/064"
fi
if [ ! -d "../logs/096" ]
then
  mkdir "../logs/096"
fi
if [ ! -d "../logs/128" ]
then
  mkdir "../logs/128"
fi
if [ ! -d "../logs/192" ]
then
  mkdir "../logs/192"
fi
if [ ! -d "../logs/256" ]
then
  mkdir "../logs/256"
fi
if [ ! -d "../logs/384" ]
then
  mkdir "../logs/384"
fi

baseline=$( ps -u $(whoami) | sed 1d | wc -l )
./prep_cs_scenarios.sh
cd ../build/
CC=$HOME/sw/gcc-8.3.0-install/bin/gcc CXX=$HOME/sw/gcc-8.3.0-install/bin/g++ \
 LD_LIBRARY_PATH=$HOME/sw/gcc-8.3.0-install/lib64/ cmake ../source/
make
for orbit in planet
do
  for gnd_count in 010
  do
    # equatorial ground stations
    gnd_config=eq
    ./sim-compose ../logs/$orbit-$gnd_config-$gnd_count/ \
                   ../data/$orbit-$gnd_config-$gnd_count/dt/ \
                   ../data/$orbit-$gnd_config-$gnd_count/sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/sensor/ \
                   ../data/$orbit-$gnd_config-$gnd_count/gnd/ \
                   ../data/$orbit-$gnd_config-$gnd_count/tx-sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/tx-gnd/ \
                   ../data/$orbit-$gnd_config-$gnd_count/rx-sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/rx-gnd/
    # north/south polar ground stations
    gnd_config=ns
    ./sim-compose ../logs/$orbit-$gnd_config-$gnd_count/ \
                   ../data/$orbit-$gnd_config-$gnd_count/dt/ \
                   ../data/$orbit-$gnd_config-$gnd_count/sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/sensor/ \
                   ../data/$orbit-$gnd_config-$gnd_count/gnd/ \
                   ../data/$orbit-$gnd_config-$gnd_count/tx-sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/tx-gnd/ \
                   ../data/$orbit-$gnd_config-$gnd_count/rx-sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/rx-gnd/
    # university ground stations
    gnd_config=un
    ./sim-compose ../logs/$orbit-$gnd_config-$gnd_count/ \
                   ../data/$orbit-$gnd_config-$gnd_count/dt/ \
                   ../data/$orbit-$gnd_config-$gnd_count/sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/sensor/ \
                   ../data/$orbit-$gnd_config-$gnd_count/gnd/ \
                   ../data/$orbit-$gnd_config-$gnd_count/tx-sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/tx-gnd/ \
                   ../data/$orbit-$gnd_config-$gnd_count/rx-sat/ \
                   ../data/$orbit-$gnd_config-$gnd_count/rx-gnd/
    # if necessary, wait for cores to free
    while (( $(( $( ps -u $(whoami) | sed 1d | wc -l ) - $baseline )) > $(nproc) ))
    do
      sleep 1000
    done
  done
done

