# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 20 --timewin  48 --with_idx

# for ((j=80; j<=320; j=j+80))
# do
#     echo $j
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size $j --timewin  48 --with_idx
# done

# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 380 --timewin  48 --with_idx

./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 20 --timewin 946 --with_idx

for ((j=80; j<=320; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size $j --timewin 946 --with_idx
done

./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 380 --timewin 946 --with_idx



./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 20 --timewin 254 
for ((j=80; j<=320; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size $j --timewin 254 
done

./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 380 --timewin 254



./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 20 --timewin 101 --with_idx
for ((j=80; j<=320; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size $j --timewin 101 --with_idx
done
./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset dblp --query_size 380 --timewin 101 --with_idx
