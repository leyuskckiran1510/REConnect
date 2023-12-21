CC = gcc
CFLAGS = -ggdb3 -Wall -Wextra -pedantic -I./include  -lpthread -fsanitize=address

SRCDIR = src
INCDIR = include
BUILDDIR = build
BINDIR = bin

CLIENT_SRC = $(wildcard $(SRCDIR)/client/*.c)
SERVER_SRC = $(wildcard $(SRCDIR)/server/*.c)
OTHER_SRC = $(wildcard $(SRCDIR)/*.c)

CLIENT_OBJ = $(patsubst $(SRCDIR)/client/%.c, $(BUILDDIR)/client/%.o, $(CLIENT_SRC))
SERVER_OBJ = $(patsubst $(SRCDIR)/server/%.c, $(BUILDDIR)/server/%.o, $(SERVER_SRC))
OTHER_OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(OTHER_SRC))

CLIENT_EXECUTABLE = $(BINDIR)/client
SERVER_EXECUTABLE = $(BINDIR)/server

all:  $(CLIENT_EXECUTABLE) $(SERVER_EXECUTABLE)

$(CLIENT_EXECUTABLE):$(OTHER_OBJ) $(CLIENT_OBJ) 
	$(CC)  $^ -o $@ $(CFLAGS)

$(SERVER_EXECUTABLE):$(OTHER_OBJ) $(SERVER_OBJ)
	$(CC)  $^ -o $@ $(CFLAGS)

$(BUILDDIR)/client/%.o: $(SRCDIR)/client/%.c
	$(CC)  -c $< -o $@ $(CFLAGS)

$(BUILDDIR)/server/%.o: $(SRCDIR)/server/%.c
	$(CC)  -c $< -o $@ $(CFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC)  -c $< -o $@ $(CFLAGS)

run-client:
	@$(CLIENT_EXECUTABLE)

run-server:
	@$(SERVER_EXECUTABLE)

run-s:run-server
run-c:run-client
	
clean:
	rm -f $(BUILDDIR)/client/*.o $(BUILDDIR)/server/*.o $(BINDIR)/*


.PHONY: all clean run-client run-server
