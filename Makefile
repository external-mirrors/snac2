PREFIX?=/usr/local
PREFIX_MAN=$(PREFIX)/man
CFLAGS?=-g -Wall -Wextra -pedantic

all: snac

snac: snac.o main.o data.o http.o httpd.o webfinger.o \
    activitypub.o html.o utils.o format.o upgrade.o mastoapi.o
	$(CC) $(CFLAGS) -L$(PREFIX)/lib *.o -lcurl -lcrypto $(LDFLAGS) -pthread -o $@

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(PREFIX)/include -c $<

clean:
	rm -rf *.o *.core snac makefile.depend

dep:
	$(CC) -I$(PREFIX)/include -MM *.c > makefile.depend

install:
	mkdir -p -m 755 $(PREFIX)/bin
	install -m 755 snac $(PREFIX)/bin/snac
	mkdir -p -m 755 $(PREFIX_MAN)/man1
	install -m 644 doc/snac.1 $(PREFIX_MAN)/man1/snac.1
	mkdir -p -m 755 $(PREFIX_MAN)/man5
	install -m 644 doc/snac.5 $(PREFIX_MAN)/man5/snac.5
	mkdir -p -m 755 $(PREFIX_MAN)/man8
	install -m 644 doc/snac.8 $(PREFIX_MAN)/man8/snac.8

uninstall:
	rm $(PREFIX)/bin/snac
	rm $(PREFIX_MAN)/man1/snac.1
	rm $(PREFIX_MAN)/man5/snac.5
	rm $(PREFIX_MAN)/man8/snac.8

activitypub.o: activitypub.c xs.h xs_json.h xs_curl.h xs_mime.h \
 xs_openssl.h xs_regex.h xs_time.h xs_set.h xs_match.h snac.h \
 http_codes.h
data.o: data.c xs.h xs_hex.h xs_io.h xs_json.h xs_openssl.h xs_glob.h \
 xs_set.h xs_time.h xs_regex.h xs_match.h xs_unicode.h xs_random.h snac.h \
 http_codes.h
format.o: format.c xs.h xs_regex.h xs_mime.h xs_html.h xs_json.h \
 xs_time.h snac.h http_codes.h
html.o: html.c xs.h xs_io.h xs_json.h xs_regex.h xs_set.h xs_openssl.h \
 xs_time.h xs_mime.h xs_match.h xs_html.h xs_curl.h snac.h http_codes.h
http.o: http.c xs.h xs_io.h xs_openssl.h xs_curl.h xs_time.h xs_json.h \
 snac.h http_codes.h
httpd.o: httpd.c xs.h xs_io.h xs_json.h xs_socket.h xs_unix_socket.h \
 xs_httpd.h xs_mime.h xs_time.h xs_openssl.h xs_fcgi.h xs_html.h snac.h \
 http_codes.h
main.o: main.c xs.h xs_io.h xs_json.h xs_time.h xs_openssl.h snac.h \
 http_codes.h
mastoapi.o: mastoapi.c xs.h xs_hex.h xs_openssl.h xs_json.h xs_io.h \
 xs_time.h xs_glob.h xs_set.h xs_random.h xs_url.h xs_mime.h xs_match.h \
 snac.h http_codes.h
snac.o: snac.c xs.h xs_hex.h xs_io.h xs_unicode_tbl.h xs_unicode.h \
 xs_json.h xs_curl.h xs_openssl.h xs_socket.h xs_unix_socket.h xs_url.h \
 xs_httpd.h xs_mime.h xs_regex.h xs_set.h xs_time.h xs_glob.h xs_random.h \
 xs_match.h xs_fcgi.h xs_html.h snac.h http_codes.h
upgrade.o: upgrade.c xs.h xs_io.h xs_json.h xs_glob.h snac.h http_codes.h
utils.o: utils.c xs.h xs_io.h xs_json.h xs_time.h xs_openssl.h \
 xs_random.h xs_glob.h xs_curl.h xs_regex.h snac.h http_codes.h
webfinger.o: webfinger.c xs.h xs_json.h xs_curl.h xs_mime.h snac.h \
 http_codes.h
