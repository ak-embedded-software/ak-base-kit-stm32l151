CFLAGS		+= -I./sources/app/screens
CPPFLAGS	+= -I./sources/app/screens

VPATH += sources/app/screens

# CPP source files
SOURCES_CPP += sources/app/screens/screens_bitmap.cpp
SOURCES_CPP += sources/app/screens/scr_idle.cpp
SOURCES_CPP += sources/app/screens/scr_qrcode.cpp
SOURCES_CPP += sources/app/screens/scr_startup.cpp
SOURCES_CPP += sources/app/screens/scr_welcome.cpp
SOURCES_CPP += sources/app/screens/scr_game_menu.cpp
SOURCES_CPP += sources/app/screens/scr_game_playing.cpp
SOURCES_CPP += sources/app/screens/scr_game_over.cpp
SOURCES_CPP += sources/app/screens/scr_game_settings.cpp
SOURCES_CPP += sources/app/screens/scr_game_win.cpp
SOURCES_CPP += sources/app/screens/scr_game_leaderboard.cpp
