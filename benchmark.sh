#!/bin/bash

cmake-build-release/src/collisions \
  -loadfactors=1.0,0.75,0.5,0.25 \
  -datasets=books_200M_uint32.ds:4,books_200M_uint64.ds:8,fb_200M_uint64.ds:8,osm_cellids_200M_uint64.ds:8,wiki_ts_200M_uint64.ds:8 \
  -datapath=data/ \
  -outfile=results/collision.csv

cmake-build-release/src/throughput \
  -loadfactors=1.0,0.75,0.5,0.25 \
  -datasets=books_200M_uint32.ds:4,books_200M_uint64.ds:8,fb_200M_uint64.ds:8,osm_cellids_200M_uint64.ds:8,wiki_ts_200M_uint64.ds:8 \
  -datapath=data/ \
  -outfile=results/throughput.csv
