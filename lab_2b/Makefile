# Nathan Tsai
# nwtsai@gmail.com
# 304575323

export PATH := /usr/local/cs/bin/:$(PATH)

default: lab2_list.o SortedList.o 
	@gcc -pthread -g -o lab2_list lab2_list.c SortedList.c
tests: lab2_list
	@rm -f lab2b_list.csv
	@-./lab2_list --sync=m --iterations=1000 --threads=1  >> lab2b_list.csv
	@-./lab2_list --sync=m --iterations=1000 --threads=2  >> lab2b_list.csv
	@-./lab2_list --sync=m --iterations=1000 --threads=4  >> lab2b_list.csv
	@-./lab2_list --sync=m --iterations=1000 --threads=8  >> lab2b_list.csv
	@-./lab2_list --sync=m --iterations=1000 --threads=12 >> lab2b_list.csv
	@-./lab2_list --sync=m --iterations=1000 --threads=16 >> lab2b_list.csv
	@-./lab2_list --sync=m --iterations=1000 --threads=24 >> lab2b_list.csv
	@-./lab2_list --sync=s --iterations=1000 --threads=1  >> lab2b_list.csv
	@-./lab2_list --sync=s --iterations=1000 --threads=2  >> lab2b_list.csv
	@-./lab2_list --sync=s --iterations=1000 --threads=4  >> lab2b_list.csv
	@-./lab2_list --sync=s --iterations=1000 --threads=8  >> lab2b_list.csv
	@-./lab2_list --sync=s --iterations=1000 --threads=12 >> lab2b_list.csv
	@-./lab2_list --sync=s --iterations=1000 --threads=16 >> lab2b_list.csv
	@-./lab2_list --sync=s --iterations=1000 --threads=24 >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=1  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=2  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=4  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=8  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=16 --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=1  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=2  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=4  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=8  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=16 --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=1  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=2  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=4  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=8  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=16 --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=1  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=2  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=4  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=8  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=16 --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=1  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=2  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=4  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=8  --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=16 --yield=id >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=10  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=20  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=40  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=80  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=10  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=20  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=40  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=80  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=10  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=20  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=40  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=80  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=10  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=20  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=40  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=80  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=10  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=20  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=40  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=80  --yield=id --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=10  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=20  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=40  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=1  --iterations=80  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=10  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=20  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=40  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=4  --iterations=80  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=10  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=20  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=40  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=8  --iterations=80  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=10  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=20  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=40  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=12 --iterations=80  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=10  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=20  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=40  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4 --threads=16 --iterations=80  --yield=id --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=1  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=2  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=4  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=8  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=12 --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=1  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=2  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=4  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=8  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=12 --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=1  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=2  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=4  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=8  --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=12 --iterations=1000 --sync=m >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=1  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=2  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=4  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=8  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=4  --threads=12 --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=1  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=2  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=4  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=8  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=8  --threads=12 --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=1  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=2  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=4  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=8  --iterations=1000 --sync=s >> lab2b_list.csv
	@-./lab2_list --lists=16 --threads=12 --iterations=1000 --sync=s >> lab2b_list.csv
profile:
	@gcc -o profile.out -pthread -g -L /u/cs/ugrad/tsain/lib/ -lprofiler lab2_list.o SortedList.o
	-rm -f ./raw.gperf
	rm -f profile.out
	LD_PRELOAD=/u/cs/ugrad/tsain/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=worker ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf
graphs: lab2b_list.gp
	@gnuplot ./lab2b_list.gp
dist: clean default tests graphs
	@tar -czf lab2b-304575323.tar.gz lab2_list.c Makefile README *.png SortedList.c SortedList.h lab2b_list.csv lab2b_list.gp profile.out
clean:
	@rm -f lab2_list *.csv *.png *.o *.tar.gz
