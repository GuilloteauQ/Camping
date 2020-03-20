PROG = make_bin_folder expe_run

all : $(PROG)

CC        =  mpicc
CFLAGS    =  -g -Wall -O2
CPPFLAGS  =  -DDEBUG
LDFLAGS   =  -g -lm -O2
SRC_FOLDER = src
TARGET_FOLDER = bin

make_bin_folder:
	mkdir -p bin > /dev/null

expe_run: $(TARGET_FOLDER)/main.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET_FOLDER)/%.o: $(SRC_FOLDER)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean :
	@rm -f *.o $(PROG) *.pgm
	@rm -rf $(TARGET_FOLDER)
