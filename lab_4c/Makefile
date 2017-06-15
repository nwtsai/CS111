# # # # # # # # # # #
# Nathan Tsai       #
# nwtsai@gmail.com  #
# 304575323         #
# # # # # # # # # # #

files = Makefile lab4c_tcp.c lab4c_tls.c README

default: 
	@gcc -std=gnu11 -pthread -O2 -lm -lmraa -o lab4c_tcp lab4c_tcp.c
	@gcc -std=gnu11 -pthread -O2 -lm -lmraa -lssl -lcrypto -o lab4c_tls lab4c_tls.c 
dist:
	@tar -czf lab4c-304575323.tar.gz $(files)
clean:
	@rm -f lab4c_tcp lab4c_tls lab4c-304575323.tar.gz *.txt *.log