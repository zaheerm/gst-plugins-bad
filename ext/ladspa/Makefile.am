plugin_LTLIBRARIES = libgstladspa.la

libgstladspa_la_SOURCES = gstsignalprocessor.c gstladspa.c
libgstladspa_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_CONTROLLER_CFLAGS) $(GST_CFLAGS) $(LRDF_CFLAGS)
libgstladspa_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) -lgstaudio-$(GST_MAJORMINOR) $(LIBM) $(LRDF_LIBS)
libgstladspa_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstladspa_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gstsignalprocessor.h gstladspa.h
