
# ./agenda build --prefix ./data/ --dataset dblp --epsilon 0.5 --beta 1 --alter_idx
# ./agenda build --prefix ./data/ --dataset dblp --epsilon 0.5 --beta 5 --baton
# ./agenda build --prefix ./data/ --dataset dblp --epsilon 0.5 --beta 1

cmake .

make

rate=(0.125 0.25 0.5 1 2 4 8)
## Agenda
for rt in "${rate[@]}"
do
    ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset dblp --lambdaq 10 --rate ${rt} --timewin 10 --with_idx
done

### Fora
for rt in "${rate[@]}"
do
    ./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset dblp --lambdaq 10 --rate ${rt} --timewin 10
done

## Fora+

for rt in "${rate[@]}"
do
    ./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset dblp --lambdaq 10 --rate ${rt} --timewin 10 --with_idx
done

### Resacc

for rt in "${rate[@]}"
do
    ./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset dblp --lambdaq 10 --rate ${rt} --timewin 10
done