CURRENT_DIR = $(shell pwd)
DIR_INC = ./include
DIR_SRC = ./source
DIR_OBJ = ./obj

SRC = $(wildcard $(DIR_SRC)/*.cc)
OBJ = $(patsubst %.cc, ${DIR_OBJ}/%.o, $(notdir $(SRC)))
DEPENDS := $(patsubst %.cc, ${DIR_OBJ}/%.d, $(notdir $(SRC)))

CC = g++
CFLAGS = -g -Wall -I$(DIR_INC) -std=c++11
TARGET = np_simple
TARGET2 = np_single_proc
TARGET3 = np_multi_proc

.PHONY: all
all:$(TARGET) $(TARGET2) $(TARGET3) 

${TARGET}:$(filter-out ${DIR_OBJ}/$(TARGET2).o ${DIR_OBJ}/$(TARGET3).o, $(OBJ))
	$(CC) -o $@ $^

${TARGET2}:$(filter-out ${DIR_OBJ}/$(TARGET).o ${DIR_OBJ}/$(TARGET3).o, $(OBJ))
	$(CC) -o $@ $^

${TARGET3}:$(filter-out ${DIR_OBJ}/$(TARGET).o ${DIR_OBJ}/$(TARGET2).o, $(OBJ))
	$(CC) -o $@ $^

$(DIR_OBJ)/%.o: ${DIR_SRC}/%.cc ${DIR_INC}/%.h 
	mkdir -p $(DIR_OBJ)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPENDS)

.PHONY:clean
clean:
	rm -rf ${DIR_OBJ} -f $(TARGET) $(TARGET2)
