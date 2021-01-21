NAME=ft232h-fix
CC=clang
INC=-I /usr/include/libftdi1
LINK=-l ftdi1
CFLAGS=-Wall -Werror -Wextra -Weverything -Wno-reserved-id-macro -Wno-padded

OBJS=ft232h_fix.o

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(INC) $(LINK) -o $(NAME) $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(INC) -o $@ $(CFLAGS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re:	fclean $(NAME)
