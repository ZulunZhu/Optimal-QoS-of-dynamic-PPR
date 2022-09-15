./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset twitter --query_size 20 --timewin 767330 --with_idx

for ((j=80; j<=400; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset twitter --query_size $j --timewin 767330 --with_idx
done


./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset twitter --query_size 20 --timewin 248117 
for ((j=80; j<=400; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset twitter --query_size $j --timewin 248117
done

