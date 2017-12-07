target = micro-lisp

source = $(target).c

$(target): $(source)
	clang -Wall -Wextra -o $@ $?

clean:
	rm -fv $(target)

vi:
	vi $(source)

