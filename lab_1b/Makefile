# Nathan Tsai                                                                                                                                                                                                                                
# nwtsai@gmail.com                                                                                                                                                                                                                          
# 304575323                                                                                                                                                                                                                                   

default: client server

client: lab1b-client.c 
	@gcc -lmcrypt -o lab1b-client -g lab1b-client.c
server: lab1b-server.c
	@gcc -lmcrypt -o lab1b-server -g lab1b-server.c
clean:
	@rm -f lab1b-client lab1b-server *.tar.gz *.txt
dist:
	@tar -czf lab1b-304575323.tar.gz Makefile README lab1b-client.c lab1b-server.c my.key
