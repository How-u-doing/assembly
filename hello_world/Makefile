.PHONY: clean

hello:
	as hello.s -o hello.o
	# objdump -dr hello.o
	ld hello.o -o hello
	# objdump -d hello
	# objdump --full-contents hello

clean:
	rm *.o hello
