# Nathan Tsai
# nwtsai@gmail.com
# 304575323

lab0: lab0.c
	@gcc -o lab0 -g lab0.c
check: runTests
	@if [[ -a errors.txt ]]; then \
	  echo "Failed at least one test case."; \
	  cat errors.txt; \
	  rm -f errors.txt; \
	else \
	  echo "Passed all test cases."; \
	fi 
clean:
	@rm -f lab0 *.tar.gz *.txt
dist:
	@tar -czf lab0-304575323.tar.gz Makefile README lab0.c backtrace.png breakpoint.png

# Run All Tests

runTests: runNormalTest runInvalidOPTest runOpenTest runCreatTest runCatchTest runComparisonTest

runNormalTest:
	@echo "Test if the program runs correctly when given normal parameters." > input.txt; \
	./lab0 --input=input.txt --output=output.txt; \
	if [[ $$? -ne 0 ]]; then \
	echo "Error: Program did not run correctly." >> errors.txt; \
	fi; \
	rm -f input.txt output.txt;

runInvalidOPTest:
	@echo | ./lab0 --randomOperation &> /dev/null; \
	if [[ $$? -ne 1 ]]; then \
	echo "Error: Program did not catch an invalid option." >> errors.txt; \
	fi;

runOpenTest:
	@echo "Test if unreadable file throws an error." > input.txt; \
	chmod u-r input.txt; \
	echo | ./lab0 --input=input.txt &> /dev/null;\
	if [[ $$? -ne 2 ]]; then \
	echo "Error: File without read permissions could not be read, but program didn't exit correctly." >> errors.txt; \
	fi; \
	rm -f input.txt;

runCreatTest:
	@echo "Test if unwritable file throws an error." > input.txt; \
	touch output.txt; \
	chmod u-w output.txt; \
	./lab0 --input=input.txt --output=output.txt &> /dev/null; \
	if [[ $$? -ne 3 ]]; then \
	echo "Error: File without write permissions could not be created, but program didn't exit correctly." >> errors.txt; \
	fi; \
	rm -f input.txt output.txt;

runCatchTest:
	@echo | ./lab0 --segfault --catch &> /dev/null; \
	if [[ $$? -ne 4 ]]; then \
	echo "Error: Could not catch the segmentation fault." >> errors.txt; \
	fi;

runComparisonTest:
	@echo "Test if the contents of the output file match the input file." > input.txt; \
	./lab0 --input=input.txt --output=output.txt &> /dev/null; \
	chmod u+w output.txt; \
	cmp input.txt output.txt; \
	if [[ $$? -ne 0 ]]; then \
	echo "Error: The output file does not match the input file." >> errors.txt; \
	fi; \
	rm -f input.txt output.txt;
