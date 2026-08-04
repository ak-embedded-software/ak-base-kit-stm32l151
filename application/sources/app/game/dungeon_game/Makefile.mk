CFLAGS		+= -I./sources/app/game/dungeon_game
CPPFLAGS	+= -I./sources/app/game/dungeon_game

VPATH += sources/app/game/dungeon_game

# CPP source files 
# Dungeon game 
SOURCES_CPP += sources/app/game/dungeon_game/dungeon_control.cpp
SOURCES_CPP += sources/app/game/dungeon_game/dungeon_runtime.cpp
SOURCES_CPP += sources/app/game/dungeon_game/dungeon_action.cpp
SOURCES_CPP += sources/app/game/dungeon_game/dungeon_effect.cpp
SOURCES_CPP += sources/app/game/dungeon_game/dungeon_lane.cpp
SOURCES_CPP += sources/app/game/dungeon_game/dungeon_state.cpp