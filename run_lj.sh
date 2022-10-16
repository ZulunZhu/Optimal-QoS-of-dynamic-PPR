# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size 20 --timewin  1036 --with_idx

# for ((j=80; j<=320; j=j+80))
# do
#     echo $j
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size $j --timewin  1036 --with_idx
# done
cmake .

make

rate=(0.125 0.25 0.5 1 2 4 8)
# ./agenda test_linear --algo lazyup --lambdaq 0.1 --lambdau 0.3 --timewin 1000 --prefix ../data/ --dataset livejournal --with_idx

# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate 0.125 --timewin 1000 --with_idx
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate 0.25 --timewin 1000 --with_idx
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate 0.5 --timewin 1000 --with_idx
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate 2 --timewin 1000 --with_idx
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate 4 --timewin 1000 --with_idx
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate 8 --timewin 1000 --with_idx


### Fora
for rt in "${rate[@]}"
do
    ./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate ${rt} --timewin 1000
done

### Fora+

for rt in "${rate[@]}"
do
    ./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate ${rt} --timewin 1000 --with_idx
done

### Resacc

for rt in "${rate[@]}"
do
    ./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset livejournal --lambdaq 0.1 --rate ${rt} --timewin 1000
done