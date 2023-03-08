#!/bin/bash

# prog1
cd prog1_mandelbrot_threads/ || (echo "prog1_mandelbrot_threads not found" && exit)
make
rm output_view*
touch output_view1
touch output_view2
## Q1,Q2, using multithread to run view 1
for threadNum in {2..8} 12 16; do
    echo "threadNum is ${threadNum}"
    ./mandelbrot -t "${threadNum}" -v 1 >> output_view1
    ./mandelbrot -t "${threadNum}" -v 2 >> output_view2
done
python plot.py
cd ..

cd prog2_vecintrin/ || (echo "prog2_vecintrin not found" && exit)
for ((VECTOR_WIDTH=2;VECTOR_WIDTH<=16;VECTOR_WIDTH+=2)); do
    make clean && make MYCFLAGS="-DVECTOR_WIDTH=$VECTOR_WIDTH"
    ./myexp -s 10000
done
cd ..

cd prog3_mandelbrot_ispc || (echo "prog3_mandelbrot_ispc not found" && exit)
make
echo "run program of view 1"
./mandelbrot_ispc -t
echo "run program of view 2"
./mandelbrot_ispc -t -v 2
cd ..

cd prog4_sqrt || (echo "prog4_sqrt not found" && exit)
make && ./sqrt
cd ..

cd prog5_saxpy || (echo "prog5_saxpy not found" && exit)
make && ./saxpy
cd ..

cd prog6_kmeans || (echo "prog6_kmeans not found" && exit)
make && ./kmeans
cd ..