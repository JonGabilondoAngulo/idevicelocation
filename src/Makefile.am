AM_CFLAGS =			\
	$(GLOBAL_CFLAGS)	\
	$(libimobiledevice_CFLAGS)	\
	$(libglib2_CFLAGS)	\
	$(libplist_CFLAGS)	\
	$(libzip_CFLAGS)

AM_LDFLAGS =			\
	$(libimobiledevice_LIBS)	\
	$(libglib2_LIBS)	\
	$(libplist_LIBS)	\
	$(libzip_LIBS)

bin_PROGRAMS = idevicelocation

idevicelocation_SOURCES = idevicelocation.c
idevicelocation_CFLAGS = $(AM_CFLAGS)
idevicelocation_LDFLAGS = $(AM_LDFLAGS)

