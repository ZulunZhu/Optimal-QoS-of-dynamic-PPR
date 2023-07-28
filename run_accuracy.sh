cmake .

make

rate=(0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9)
## Agenda
# for rt in "${rate[@]}"
# do
#     rm /home/zulun/Project/data/dblp/lazyup.txt
#     ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset dblp --lambdaq 10 --rate 1 --timewin 10 --with_idx --shuffle --esf ${rt} --check_size 10
#     ./agenda calc-acc --prefix ../data/ --dataset dblp --esf ${rt}
# done
## Fora
for rt in "${rate[@]}"
do
    rm /home/zulun/Project/data/dblp/fora.txt
    ./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset dblp --lambdaq 10 --rate 4 --timewin 10 --with_idx --shuffle --esf ${rt} --check_size 10
    ./agenda calc-acc --prefix ../data/ --dataset dblp --esf ${rt}
done