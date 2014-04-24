# Description

`git-log-html` takes the colorized output of `git log` and replaces ANSI
colour codes with HTML `span` elements.  The span elements have classes
corresponding to the colour codes.  See [default.css](./default.css) for
definitions of the most common classes.

# Invocation

    git-log-html
    # runs
    # git log --graph --abbrev-commit --decorate --date=relative \
    # --format=format:'%C(bold blue)%h%C(reset) \
    # - %C(bold green)(%ar)%C(reset) \
    # %C(white)%s%C(reset) \
    # %C(dim white)- %an%C(reset) \
    # %C(bold yellow)%d%C(reset)' --all

    git-log-html -
    # colourizes stdin

    git-log-html FILE
    # colourizes the contents of FILE

# Installation

    make
    sudo cp git-log-html /usr/local/bin/git-log-html

# Example

In order to get an impression of how the output looks, run `make test`
and then open `test.html` in a browser.

# Author

Dario Hamidi `<dario.hamidi@gmail.com>`
