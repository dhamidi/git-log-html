CFLAGS=-std=c99 -Wall -pedantic

git-log-html: git-log-html.c

test: test.html

test.html: git-log-html test-input
	./git-log-html test-input > test.body.html
	cat test.header.html test.body.html test.footer.html > $@

clean:
	-rm test.html test.body.html git-log-html
