
# ./agenda build --prefix ./data/ --dataset webstanford --epsilon 0.5 --beta 1 --alter_idx
# ./agenda build --prefix ./data/ --dataset webstanford --epsilon 0.5 --beta 5 --baton
# ./agenda build --prefix ./data/ --dataset webstanford --epsilon 0.5 --beta 1

# ./agenda generate-ss-query --prefix ./data/ --dataset webstanford --query_size 1000
# ./agenda gen-update --prefix ./data/ --dataset webstanford --query_size 1000

# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ./data/ --dataset webstanford --query_size 80 --timewin  4 --with_idx


# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 20 --timewin  4 --with_idx

# for ((j=80; j<=320; j=j+80))
# do
#     echo $j
# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size $j --timewin  4 --with_idx
# done

# ./agenda dynamic-ss --algo lazyup --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 380 --timewin  4 --with_idx





./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 20 --timewin 428 --with_idx

for ((j=80; j<=320; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size $j --timewin 428 --with_idx
done

./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 380 --timewin 428 --with_idx

./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 20 --timewin 12 
for ((j=80; j<=320; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size $j --timewin 12 
done
./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 380 --timewin 12

./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 20 --timewin 49 --with_idx
for ((j=80; j<=320; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size $j --timewin 49 --with_idx
done

./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset webstanford --query_size 380 --timewin 49 --with_idx

# ./agenda build --prefix ../data/ --dataset dblp --epsilon 0.5 --beta 1 --alter_idx
# ./agenda build --prefix ../data/ --dataset dblp --epsilon 0.5 --beta 5 --baton
# ./agenda build --prefix ../data/ --dataset dblp --epsilon 0.5 --beta 1

# ./agenda build --prefix ../data/ --dataset livejournal --epsilon 0.5 --beta 1 --alter_idx
# ./agenda build --prefix ../data/ --dataset livejournal --epsilon 0.5 --beta 5 --baton
# ./agenda build --prefix ../data/ --dataset livejournal --epsilon 0.5 --beta 1

# ./agenda build --prefix ../data/ --dataset orkutt --epsilon 0.5 --beta 1 --alter_idx
# ./agenda build --prefix ../data/ --dataset orkutt --epsilon 0.5 --beta 5 --baton
# ./agenda build --prefix ../data/ --dataset orkutt --epsilon 0.5 --beta 1

# ./agenda build --prefix ../data/ --dataset twitter --epsilon 0.5 --beta 1 --alter_idx
# ./agenda build --prefix ../data/ --dataset twitter --epsilon 0.5 --beta 5 --baton
# ./agenda build --prefix ../data/ --dataset twitter --epsilon 0.5 --beta 1