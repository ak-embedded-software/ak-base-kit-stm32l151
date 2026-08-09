CFLAGS		+= -I./sources/app/game/game_loneblade
CPPFLAGS	+= -I./sources/app/game/game_loneblade

VPATH += sources/app/game/game_loneblade

# Register game engine source files
SOURCES_CPP += sources/app/game/game_loneblade/game_loneblade.cpp
SOURCES_CPP += sources/app/game/game_loneblade/game_player.cpp
SOURCES_CPP += sources/app/game/game_loneblade/game_monster.cpp
SOURCES_CPP += sources/app/game/game_loneblade/game_combat.cpp
SOURCES_CPP += sources/app/game/game_loneblade/game_arrow.cpp
SOURCES_CPP += sources/app/game/game_loneblade/game_ult_wave.cpp
SOURCES_CPP += sources/app/game/game_loneblade/game_item.cpp
SOURCES_CPP += sources/app/game/game_loneblade/game_boss.cpp
