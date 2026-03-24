CC		= gcc
CFLAGS	= -Wall -Wextra -Werror -Wno-unused-result -O2 -I./inc
NAME	= main.exe

SRC_DIR	= src
OBJ_DIR	= obj
INC_DIR	= inc

SRC		= $(SRC_DIR)/main.c \
		  $(SRC_DIR)/terminal.c \
		  $(SRC_DIR)/terminal_keys.c \
		  $(SRC_DIR)/line_list.c \
		  $(SRC_DIR)/line_ops.c \
		  $(SRC_DIR)/render.c \
		  $(SRC_DIR)/render_utils.c \
		  $(SRC_DIR)/undo.c \
		  $(SRC_DIR)/undo_exec.c \
		  $(SRC_DIR)/selection.c \
		  $(SRC_DIR)/selection_utils.c \
		  $(SRC_DIR)/search.c \
		  $(SRC_DIR)/search_navigate.c \
		  $(SRC_DIR)/browser.c \
		  $(SRC_DIR)/browser_render.c \
		  $(SRC_DIR)/file_io.c \
		  $(SRC_DIR)/utf8.c \
		  $(SRC_DIR)/utf8_words.c \
		  $(SRC_DIR)/events.c \
		  $(SRC_DIR)/events_clipboard.c \
		  $(SRC_DIR)/events_movement.c \
		  $(SRC_DIR)/events_delete.c \
		  $(SRC_DIR)/prompt.c

OBJ		= $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
