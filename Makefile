NAME = libftprintf.a
CC = cc
FLAGE = -Wall -Wextra -Werror
SRC = ft_len_int.c  ft_printf.c   ft_puthex.c \
      ft_putnbr.c    ft_putstr.c \
      ft_len_hex.c  ft_parsing.c  ft_putchar.c  ft_putheX.c \
      ft_putpoint.c  ft_putunsigned.c 

OBJ = $(SRC:.c=.o)

all : $(NAME)
$(NAME) : $(OBJ)
	ar rcs $(NAME) $(OBJ)
%.o: %.c
	$(CC) $(FLAGE) -c $< -o $@

clean :
	rm -f $(OBJ)
fclean: clean
	rm -f $(NAME)
re: fclean all

.PHONY: all clean fclean re
