AUTOMAKE_OPTIONS = subdir-objects
DEFS = -DLOCALEDIR=\"$(datadir)/locale\" @DEFS@ @MATH_OPTIMIZER_FLAGS@ @MMX_CFLAGS@ @SSE_CFLAGS@ -fno-strict-aliasing -funroll-loops
DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/intl -I$(top_srcdir)

noinst_LIBRARIES	=	libpsx.a
libpsx_a_SOURCES 	= 	psx.cpp irq.cpp timer.cpp dma.cpp frontio.cpp cpu.cpp gte.cpp dis.cpp cdc.cpp spu.cpp gpu.cpp mdec.cpp
libpsx_a_SOURCES	+=	input/gamepad.cpp

if WANT_DEBUGGER
libpsx_a_SOURCES	+=	debug.cpp
endif
