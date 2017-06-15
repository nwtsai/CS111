# # # # # # # # # # #
# Nathan Tsai       #
# nwtsai@gmail.com  #
# 304575323         #
# # # # # # # # # # #

FLAGS = -std=gnu11 -pthread -O2 -lm -lmraa
files = Makefile lab4b.c README check.sh

default: 
	@gcc $(FLAGS) -o lab4b lab4b.c
check: lab4b
	@chmod +x check.sh
	@-./check.sh
dist:
	@tar -czf lab4b-304575323.tar.gz $(files)
clean:
	@rm -f lab4b lab4b-304575323.tar.gz