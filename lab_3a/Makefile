# # # # # # # # # # # # # # # # # # # # #
# Nathan Tsai       # Regan Hsu         #
# nwtsai@gmail.com  # hsuregan@ucla.edu #
# 304575323         # 604296090         #
# # # # # # # # # # # # # # # # # # # # #

FLAGS = -Wall -Wextra -g

default: lab3a.c ext2_fs.h
	@gcc $(FLAGS) -o lab3a lab3a.c
dist:
	@tar -czf lab3a-304575323.tar.gz lab3a.c ext2_fs.h Makefile README
clean:
	@rm -f lab3a *.tar.gz