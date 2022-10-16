cmake .

make

rate=(0.125 0.25 0.5 1 2 4 8)

# ## Agenda
# for rt in "${rate[@]}"
# do
#     ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset twitter --lambdaq 0.01 --rate ${rt} --timewin 10000 --with_idx
# done

### Fora free
# for rt in "${rate[@]}"
# do
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset twitter --lambdaq 0.01 --rate 1 --timewin 10000
# done

### Fora+ index



./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset twitter --lambdaq 0.01 --rate 0.125 --timewin 10000 --with_idx
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset twitter --lambdaq 0.01 --rate 0.25 --timewin 10000 --with_idx

### Resacc

# for rt in "${rate[@]}"
# do
./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset twitter --lambdaq 0.01 --rate 1 --timewin 10000
# done
