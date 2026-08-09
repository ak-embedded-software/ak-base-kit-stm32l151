# Automatically generated on Fri Feb 19 22:58:03 2010
#
# Do not edit, modify UserConf.mk instead!
#

PLATFORM=XNUT_100
HWDEF+=-D$(PLATFORM)
MCU_ATMEGA2561=atmega2561
MCU_ATMEGA128=atmega128
MCU_ATMEGA103=atmega103
MCU=$(MCU_ATMEGA128)
HWDEF+=-D__HARVARD_ARCH__
CRUROM=crurom


include $(top_appdir)/UserConf.mk
