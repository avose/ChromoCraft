CC=   gcc
OPTS= -std=gnu99

# Debug options
GCFLAGS= -g -Wall
GLIBS=   -lm -lpthread -lGL -lGLU -lX11 -lopenal -lalut

# Release options
CFLAGS= -O3 -Wall
LIBS=   -lm -lpthread -lGL -lGLU -lX11 -lopenal -lalut

all: chromo_craft

debug: CFLAGS = $(GCFLAGS)
debug: LIBS   = $(GLIBS)
debug: clean all

# Obj deps

util.o: util.c util.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c util.c

random.o: random.c random.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c random.c

vector.o: vector.c vector.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c vector.c

path.o: path.c path.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c path.c

color.o: color.c color.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c color.c

effect.o: effect.c effect.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c effect.c

special.o: special.c special.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c special.c

gem.o: gem.c gem.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c gem.c

tower.o: tower.c tower.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c tower.c

enemy.o: enemy.c enemy.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c enemy.c

bag.o: bag.c bag.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c bag.c

player.o: player.c player.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c player.c

game_event.o: game_event.c game_event.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c game_event.c

# I/O deps

io_bitmap.o: io_bitmap.c io_bitmap.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c io_bitmap.c

# GUI deps

gui_button.o: gui_button.c gui_button.h gui.h types.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c gui_button.c

gui_bag.o: gui_bag.c gui_bag.h gui.h types.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c gui_bag.c

gui_stats.o: gui_stats.c gui_stats.h gui.h types.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c gui_stats.c

gui_gameframe.o: gui_gameframe.c gui_gameframe.h gui.h types.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c gui_gameframe.c

gui_game_event.o: gui_game_event.c gui_game_event.h types.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c gui_game_event.c

gui.o: gui.c gui.h types.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c gui.c

# Main

chromo_craft.o: chromo_craft.c chromo_craft.h Makefile
	${CC} ${OPTS} ${CFLAGS} -c chromo_craft.c

# Link

chromo_craft: util.o random.o vector.o path.o color.o effect.o special.o gem.o tower.o enemy.o bag.o player.o io_bitmap.o gui_button.o gui_game_event.o gui_bag.o gui_stats.o gui_gameframe.o gui.o chromo_craft.o game_event.o Makefile
	${CC} ${OPTS} -o chromo_craft util.o random.o vector.o path.o color.o effect.o special.o gem.o tower.o enemy.o bag.o player.o io_bitmap.o gui_button.o gui_game_event.o gui_bag.o gui_stats.o gui_gameframe.o gui.o chromo_craft.o game_event.o ${LIBS}

# Maintenance

clean:
	rm -f chromo_craft *.o

strip: clean
	rm -f *~ \#*
