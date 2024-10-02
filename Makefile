# Definindo o compilador
CC = gcc

# Definindo as flags de compilação
CFLAGS = -Wall -g

# Nome do exec
TARGET = cache_simulator

# Arquivos fonte
SOURCES = cache_simulator.c cache.c

# Arquivos obj
OBJECTS = $(SOURCES:.c=.o)

# construir o exe
$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) -lm 
# -lm para linkar a biblioteca math.h

# Regra para compilar os arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os arquivos objeto e o exec
clean:
	rm -f $(OBJECTS) $(TARGET)

# Regras especiais
.PHONY: clean
