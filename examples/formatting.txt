Messages in `snac` allow a special subset of Markdown, that includes *emphasized*, **strong**, `monospaced`, ~~strikethrough~~ and __underlined__ styles by surrounding text with one asterisk, two asterisks, backquotes, two tildes and two underlines, respectively.

Line breaks are respected and output as you write them.


Prepending a greater-than symbol in a line makes it a quote:

> This is quoted text
>
> All angle-prepended lines are grouped in the same blockquote

It also allows preformatted text using three backquotes in a single line:

```
  /* this is preformatted text */

  struct node {
      struct node *prev;
      struct node *next;
  };

```

- One level bullet lists
- are also supported,
- by starting a line with a hyphen or asterisk followed by a space.

URLs like https://en.wikipedia.org/wiki/Main_Page are made clickable, https://comam.es/what-is-snac.

Links can also be written in [standard Markdown style](https://comam.es/what-is-snac).

Some emojis: X-D <3 :beer: :shrug: :shrug2:

Image URLs written in standard Markdown style for images ![susie, snac's girl](https://comam.es/snac-doc/susie64.png) are converted to ActivityPub attachments.

Three minus symbols in a line make a separator:

---

Headings can be defined using one, two, or three hash symbols in the beginning of a line, followed by a blank:

# header level 1
## header level 2
### header level 3

But please take note that every ActivityPub implementation out there have its own rules for filtering out these formatting styles, so you can only guess what other people will really see.

These acrobatics are better documented in the `snac(5)` man page.
