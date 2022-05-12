Make:
	gcc --std=c99 -pthread assignment-4.c -o assignment-4
Test:
	assignment-4 < input1.txt > output1.txt
	assignment-4 < input2.txt > output2.txt
	assignment-4 < input3.txt > output3.txt
