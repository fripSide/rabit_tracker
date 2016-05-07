nrep=0
rc=254
while [ $rc -eq 254 ];
do
export rabit_num_trial=$nrep
echo 'hellp'
echo 'shell'
echo `test/basic.rabit 3  rabit_num_trial=$nrep`;
test/basic.rabit 3
rc=$?;
nrep=$((nrep+1));
done
