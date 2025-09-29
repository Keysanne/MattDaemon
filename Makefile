NAME 		= MattDaemon
CC 			= g++
CFLAGS 		=-g

CFILES =	srcs/main.cpp \
			srcs/Logger.cpp \
			srcs/server.cpp

OFILES = 	$(CFILES:.cpp=.o)

all: $(NAME)

$(NAME): $(OFILES)
	$(CC) $(CFLAGS) -Iincludes $(OFILES) -o $(NAME)

.cpp.o:
	$(CC) $(CFLAGS) -Iincludes -c $< -o ${<:.cpp=.o}

clean:
	rm -f $(OFILES)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY:  all clean fclean re
