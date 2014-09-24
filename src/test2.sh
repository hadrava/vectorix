./test.sh
for i in marbles_*test; do
	echo testing: $i;
	./main $i > ${i}_test
done
