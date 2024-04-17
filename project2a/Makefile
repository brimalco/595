# Define variable CC to be the compiler we want to use
CC = clang

# Define CFLAGS for the flags we will want to use with clang
CFLAGS = -g -Wall

# Define TARGETS to be the targets to be run when calling 'make all'
TARGETS = clean penn-sh

# Define SRCS to be the source files needed to compile 'penn-sh'
# SRCS = penn-sh.c tokenizer.c 

# Define PHONY targets to prevent make from confusing the phony target with the same file names
.PHONY: clean all

# If no arguments are passed to make, it will attempt the 'penn-sh' target
default: penn-sh

# This runs the 'clean' and 'penn-sh' targets when 'make all' is run
all: $(TARGETS)

# This will construct the binary 'penn-sh'
# $^ = names of all the prerequisites, with spaces between them
# $@ = complete name of the target
# $< = name of the first prerequisite
penn-sh: penn-sh.c tokenizer.c 
	$(CC) $(CFLAGS) $^ -o $@

# $(RM) is the platform agnostic way to delete a file (here rm -f)
clean:
	$(RM) penn-sh

# tar ball generator
TARSCRS = Makefile penn-sh.c tokenizer.c tokenizer.h

tar: clean
	$(RM) project2a.tar.gz
	tar zcvf project2a.tar.gz $(TARSCRS)
