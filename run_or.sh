./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset orkutt --query_size 20 --timewin 54812 --with_idx

for ((j=80; j<=400; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo fora --epsilon 0.5 --prefix ../data/ --dataset orkutt --query_size $j --timewin 54812 --with_idx
done


./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset orkutt --query_size 20 --timewin 17010 
for ((j=80; j<=400; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo resacc --epsilon 0.5 --prefix ../data/ --dataset orkutt --query_size $j --timewin 17010 
done

./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset orkutt --query_size 20 --timewin 1823 --with_idx
for ((j=80; j<=400; j=j+80))
do
    echo $j
./agenda dynamic-ss --algo fora --alter_idx --epsilon 0.5 --prefix ../data/ --dataset orkutt --query_size $j --timewin 1823 --with_idx
done