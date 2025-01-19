
CFLAGS = -std=c99 -Wall -pedantic -Isrc
PREFIX ?= /usr/local

main: sexp_prettify readme_update

all: sexp_prettify sexp_prettify_cpp_cli sexp_prettify_kicad_cli sexp_prettify_kicad_original_cli

src/sexp_prettify.o: src/sexp_prettify.c
	$(CC) $(CFLAGS) -c -o $@ $^

sexp_prettify: sexp_prettify_cli.c src/sexp_prettify.o
	$(CC) $(CFLAGS) -o $@ $^

sexp_prettify_cpp_cli: sexp_prettify_cpp_cli.cpp src/sexp_prettify.o src/sexp_prettify.h
	$(CXX) $(CFLAGS) -o $@ $^

sexp_prettify_kicad_cli: sexp_prettify_kicad_cli.cpp
	$(CXX) $(CFLAGS) -o $@ $^

sexp_prettify_kicad_original_cli: sexp_prettify_kicad_original_cli.cpp
	$(CXX) $(CFLAGS) -o $@ $^

# Dev Note: $ is used by both make and AWK. Must escape $ for use in AWK within makefile.
.PHONY: readme_update
readme_update:
	# Library Version (From clib package metadata)
	jq -r '.version' clib.json | xargs -I{} sed -i 's|<version>.*</version>|<version>{}</version>|' README.md
	jq -r '.version' clib.json | xargs -I{} sed -i 's|<versionBadge>.*</versionBadge>|<versionBadge>![Version {}](https://img.shields.io/badge/version-{}-blue.svg)</versionBadge>|' README.md

.PHONY: install
install: sexp_prettify_cli
	install sexp_prettify_cli $(PREFIX)/bin/sexp_prettify

.PHONY: uninstall
uninstall: sexp_prettify_cli
	rm -f $(PREFIX)/bin/sexp_prettify

.PHONY: clean
clean:
	rm *.o  || true
	rm sexp_prettify_cli || true
	rm sexp_prettify_cpp_cli || true
	rm sexp_prettify_kicad_cli || true
	rm sexp_prettify_kicad_original_cli || true

.PHONY: cicd
cicd: all check time

.PHONY: check
check: all
	./test_all.sh

.PHONY: time
time: all
	./time_test_all.sh

.PHONY: format
format:
	# pip install clang-format
	clang-format -i *.c
	clang-format -i *.h
