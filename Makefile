CC      = gcc
SRC     = main.c
TARGET  = mandelbrot

PKG_LIBS := raylib lua
CFLAGS   := -Wall -Wextra -O3 $(shell pkg-config --cflags $(PKG_LIBS) 2>/dev/null)
LIBS     := $(shell pkg-config --libs $(PKG_LIBS) 2>/dev/null) -lm -lpthread

ifdef LUA_PKG
CFLAGS := -Wall -Wextra -O3 $(shell pkg-config --cflags raylib $(LUA_PKG) 2>/dev/null)
LIBS   := $(shell pkg-config --libs raylib $(LUA_PKG) 2>/dev/null) -lm -lpthread
endif

.PHONY: build run-caso

# ------------------------------------------------------------------------------
# Geracao do executavel
# ------------------------------------------------------------------------------
build: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# ------------------------------------------------------------------------------
# Caso de estudo:
#  parametros: offsetX offsetY zoom power [maxIter]
# ------------------------------------------------------------------------------
run-caso: build
	./$(TARGET) -0.65 -0.5 9.0 2.0 400
