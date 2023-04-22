CURRENT_DIR = $(shell pwd)
DIR_INC = ./include
DIR_SRC = ./source
DIR_OBJ = ./obj

SRC = $(wildcard $(DIR_SRC)/*.cc)
OBJ = $(patsubst %.cc, ${DIR_OBJ}/%.o, $(notdir $(SRC)))

CC = g++
CFLAGS = -g -Wall -I$(DIR_INC) -std=c++11
TARGET = np_simple
TARGET2= np_single_proc

.PHONY: all
all:$(TARGET) $(TARGET2)

${TARGET}:$(filter-out ${DIR_OBJ}/$(TARGET2).o, $(OBJ))
	$(CC) -o $@ $^

${TARGET2}:$(filter-out ${DIR_OBJ}/$(TARGET).o, $(OBJ))
	$(CC) -o $@ $^

$(DIR_OBJ)/%.o: ${DIR_SRC}/%.cc ${DIR_INC}/%.h 
	mkdir -p $(DIR_OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY:clean
clean:
	rm -rf ${DIR_OBJ}/*.o -f $(TARGET) $(TARGET2)
