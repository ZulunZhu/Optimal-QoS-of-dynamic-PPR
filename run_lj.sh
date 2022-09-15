./agenda dynamic-ss --algo layzyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size 20 --timewin 1036 --with_idx

for ((j=80; j<=400; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo layzyup --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size $j --timewin 1036 --with_idx
done

# ./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size 20 --timewin 27281 --with_idx

# for ((j=80; j<=400; j=j+80))
# do
#     echo $j
# ./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size $j --timewin 27281 --with_idx
# done


# ./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size 20 --timewin 8680 
# for ((j=80; j<=400; j=j+80))
# do
#     echo $j
# ./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size $j --timewin 8680 
# done

# ./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size 20 --timewin 1174 --with_idx
# for ((j=80; j<=400; j=j+80))
# do
#     echo $j
# ./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset livejournal --query_size $j --timewin 1174 --with_idx
# done