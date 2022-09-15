## Tested Environment
- Ubuntu 16
- C++ 11
- GCC 5.4
- Boost
- cmake

## Install the library 
### Install some essential library
```sh
$ sudo apt  install libopenblas-dev liblapack-dev libarpack2-dev libsuperlu-dev libboost-all-dev
```
### Install armadillo
```sh
$ git clone https://gitlab.com/conradsnicta/armadillo-code.git
$ cd armadillo-code/
$ cmake .
$ sudo make install
```
### Install ensmallen
```sh
$ git clone https://github.com/mlpack/ensmallen.git
$ cd ensmallen
$ mkdir build
$ cd build
$ cmake ..
$ sudo make install
```

## Compile
```sh
$ cmake .
$ make
```

# For temporary test of optimal response time measurements
```sh
$ bash run_webs.sh
```

## Parameters
```sh
./agenda action_name --algo <algorithm> [options]
```
- action:
    - query: static SSPPR query
    - build: build index, for FORA only
    - dynamic-ss: dynamic SSPPR  query
- algo: which algorithm you prefer to run
    - baton: Baton
    - fora: FORA
    - partup: Agenda with partial update
    - lazyup: Agenda with lazy update
    - resacc: ResAcc
- options
    - --prefix \<prefix\>
    - --epsilon \<epsilon\>
    - --dataset \<dataset\>
    - --query_size \<queries count\>
    - --update_size \<updates count\>
    - --with_idx
    - --beta: controls the trade-off between random walk and forward push



## Data
The example data format is in `./data/webstanford/` folder. The data for DBLP, Pokec, Orkut, LiveJournal, Twitter are not included here for size limitation reason. You can find them online.

## Generate workloads
Generate query files for the graph data. Each line contains a node id.

```sh
$ ./agenda generate-ss-query --prefix <data-folder> --dataset <graph-name> --query_size <query count>
```

```sh
$ ./agenda gen-update --prefix <data-folder> --dataset <graph-name> --query_size <query count>
```

- Example:

```sh
$ ./agenda generate-ss-query --prefix ./data/ --dataset webstanford --query_size 1000
```

## Indexing
Construct index files for the graph data using a single core.

- Example

For FORA index:
```sh
$ ./agenda build --prefix ./data/ --dataset webstanford --epsilon 0.5
```
For Baton and Agenda index:
```sh
$ ./agenda build --prefix ./data/ --dataset webstanford --epsilon 0.5 --baton
```

## Query
Process queries.

```sh
$ ./agenda <query-type> --algo <algo-name> --prefix <data-folder> --dataset <graph-name> --result_dir <output-folder> --epsilon <relative error> --query_size <query count> --update_size<update count> [--with-idx --exact]
```

- Example:

For SSPPR query on static graphs

```sh
// FORA
$ ./agenda query --algo fora --prefix ./data/ --dataset webstanford --epsilon 0.5 --query_size 200

// FORA+
$ ./agenda query --algo fora --prefix ./data/ --dataset webstanford --epsilon 0.5 --query_size 200 --with_idx

// Agenda 
$ ./agenda query --algo genda --prefix ./data/ --dataset webstanford --epsilon 0.5 --query_size 200 --with_idx
```

For SSPPR query on dynamic graphs:
```sh
// FORA
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ./data/ --dataset webstanford --query_size 200 --update_size 200  --with_idx

// FORA+
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ./data/ --dataset webstanford --query_size 200 --update_size 200 

// Baton
./agenda dynamic-ss --algo baton --epsilon 0.5 --prefix ./data/ --dataset webstanford --query_size 200 --update_size 200 --with_idx

// ResAcc
$ ./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ./data/ --dataset webstanford --query_size 200 --update_size 200 --with_idx

// Agenda
$ ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ./data/ --dataset webstanford --query_size 200 --update_size 200 --with_idx


```


