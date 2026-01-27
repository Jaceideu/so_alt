CFILES := main.c process_1.c process_2.c process_3.c -lrt


main: $(CFILES)
	gcc -o main $(CFILES)