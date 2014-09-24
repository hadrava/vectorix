rm marbles_*_test
for i in marbles_*; do
	echo testing: $i;
	./main $i > ${i}_test
done
