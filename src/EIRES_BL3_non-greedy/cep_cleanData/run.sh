for i in `seq 1 10`;
do
    ./cep_cleanData_VLDB.out $i
    cp CleanedOutput CleanedOutput_2m_"$i"_TTL.dat
done
