#!/bin/bash

cd saxpy || echo "fail to find saxy"
make clean && make && ./cudaSaxpy
cd ..

cd scan || echo "fail to find scan"
make clean && make && ./checker.pl
cd ..

cd render || echo "fail to find render"
make clean && make && ./checker.py
cd ..
