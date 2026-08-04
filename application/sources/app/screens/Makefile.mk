CFLAGS		+= -I./sources/app/screens
CPPFLAGS	+= -I./sources/app/screens

VPATH += sources/app/screens

# CPP source files 
# Screen
SOURCES_CPP += sources/app/screens/scr_idle.cpp
SOURCES_CPP += sources/app/screens/scr_startup.cpp
SOURCES_CPP += sources/app/screens/scr_title.cpp
SOURCES_CPP += sources/app/screens/scr_menu_game.cpp
SOURCES_CPP += sources/app/screens/scr_game_setting.cpp
SOURCES_CPP += sources/app/screens/scr_dungeon_game.cpp
SOURCES_CPP += sources/app/screens/scr_dungeon_how_to_play.cpp
SOURCES_CPP += sources/app/screens/scr_game_over.cpp
SOURCES_CPP += sources/app/screens/scr_leaderboard.cpp
SOURCES_CPP += sources/app/screens/scr_charts_game.cpp
SOURCES_CPP += sources/app/screens/screens_bitmap.cpp

# Dungeon game 
# SOURCES_CPP += sources/app/screens/ar_game_dungeon_control.cpp
# SOURCES_CPP += sources/app/screens/ar_game_arrow.cpp
# SOURCES_CPP += sources/app/screens/ar_game_bang.cpp
# SOURCES_CPP += sources/app/screens/ar_game_border.cpp
# SOURCES_CPP += sources/app/screens/ar_game_meteoroid.cpp