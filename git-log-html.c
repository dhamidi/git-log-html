/*
 * Copyright 2014 Dario Hamidi <dario.hamidi@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef COMMAND_SEQUENCE_BUFFER
#  define COMMAND_SEQUENCE_BUFFER 12
#endif

void parse(FILE* in, FILE* out);
int  peek(FILE* in);
void advance(FILE* in, int by);
void parse_colour_code(FILE* in, FILE* out);
void warn(const char* fmt, ...);
void output_css_class_for(const char* str, FILE* out);
void html_encode(int c, FILE* out);

static const char* css_classes[] = {
   [1] = "bold",
   [2] = "faint",
   [3] = "italic",
   [4] = "single-underline",
   [5] = "blink-slow",
   [6] = "blink-rapid",
   [7] = "image-negative",
   [8] = "conceal",
   [9] = "crossed-out",
   [10] = "default-font",
   [11] = "alternate-font-1",
   [12] = "alternate-font-2",
   [13] = "alternate-font-3",
   [14] = "alternate-font-4",
   [15] = "alternate-font-5",
   [16] = "alternate-font-6",
   [17] = "alternate-font-7",
   [18] = "alternate-font-8",
   [19] = "alternate-font-9",
   [20] = "fraktur",
   [21] = "double-underline",
   [22] = "normal-colour",
   [23] = "no-italic no-fraktur",
   [24] = "no-underline",
   [25] = "no-blink",
   /* [26] is reserved */
   [27] = "image-positive",
   [28] = "no-conceal",
   [29] = "no-crossed-out",
   [30] = "foreground-0",
   [31] = "foreground-1",
   [32] = "foreground-2",
   [33] = "foreground-3",
   [34] = "foreground-4",
   [35] = "foreground-5",
   [36] = "foreground-6",
   [37] = "foreground-7",
   /* [38] is for xterm and too complex */
   [38] = NULL,
   [39] = "default-foreground",
   [40] = "foreground-0",
   [41] = "foreground-1",
   [42] = "foreground-2",
   [43] = "foreground-3",
   [44] = "foreground-4",
   [45] = "foreground-5",
   [46] = "foreground-6",
   [47] = "foreground-7",
   [48] = NULL,
   /* [48] is for xterm and too complex */
   [49] = "default-background"
};

static const char* html_entities[256] = {
   ['&'] = "&amp;",
   ['"'] = "&quot;",
   ['<'] = "&lt;",
   ['>'] = "&gt;",
   ['\''] = "&quot;",
};

static const int css_class_count = sizeof(css_classes) / sizeof(char*);

static const char* git_log_command = "git log --graph --color=always --abbrev-commit --decorate --date=relative --format=format:'%C(bold blue)%h%C(reset) - %C(bold green)(%ar)%C(reset) %C(white)%s%C(reset) %C(dim white)- %an%C(reset)%C(bold yellow)%d%C(reset)' --all";

int main(int argc, char** argv) {
   FILE* input   = NULL;

   if (argc < 2) {

      input = popen(git_log_command, "r");

      if (input == NULL) {
         warn("failed to execute git command:\n---\n%s\n", git_log_command);
         return EXIT_FAILURE;
      }

   } else {

      if ( strcmp(argv[1], "-") == 0 ) {
         input = stdin;
      } else {
         input = fopen(argv[1], "r");

         if (input == NULL) {
            perror("git-log-html: fopen");
            return EXIT_FAILURE;
         }
      }
   }

   fputs("<pre>", stdout);
   parse(input, stdout);
   fputs("</pre>", stdout);

   fflush(stdout);

   return EXIT_SUCCESS;
}

void parse(FILE* in, FILE* out) {
   int c = 0;

   while ( (c = fgetc(in)) != -1 ) {

      if (c == '\033') {        /* ESC */
         if (peek(in) == '[') {
            advance(in, 1);

            parse_colour_code(in, out);
         } else {
            fputc('\033', out);
         }
      } else {
         html_encode(c, out);
      }

   }
}

int peek(FILE* in) {
   int c = fgetc(in);
   ungetc(c, in);
   return c;
}

void advance(FILE* in, int by) {
   while ( by --> 0) { fgetc(in); }
}

void warn(const char* fmt, ...) {
   va_list ap;

   va_start(ap, fmt);
   fputs("git-log-html: ", stderr);
   vfprintf(stderr, fmt, ap);
   va_end(ap);
}

void parse_colour_code(FILE* in, FILE* out) {
   char command_sequence[COMMAND_SEQUENCE_BUFFER + 1]; /* trailing 0 byte */
   int i = 0;
   int c = 0;
   char* tok = NULL;

   memset(command_sequence, 0, COMMAND_SEQUENCE_BUFFER + 1);

   while (i < COMMAND_SEQUENCE_BUFFER && c != 'm') {
      c = fgetc(in);
      command_sequence[i++] = c;
   }

   if (c != 'm') {
      warn("parse_colour_code: ignoring unfinished command sequence: %s\n", command_sequence);
   } else {

      if ( strcmp(command_sequence, "m") == 0 ) {
         fputs("</span>", out);
      } else {
         command_sequence[i - 1] = 0;

         fputs("<span class=\"", out);

         for ( (tok = strtok(command_sequence, ";")); tok; tok = strtok(NULL, ";") ) {
            output_css_class_for(tok, out);
         }

         fputs("\">", out);
      }

   }
}

void output_css_class_for(const char* sgr, FILE* out) {
   int ansi_code = atoi(sgr);

   if ( ansi_code >= css_class_count || ansi_code < 1 ) {
      warn("output_css_class_for: undefined SGR %d\n", ansi_code);
   } else {
      fprintf(out, " %s", css_classes[ansi_code]);
   }
}

void html_encode(int c, FILE* out) {
   const char* encoded = NULL;

   if (c >= 0 && c <= 255) {
      encoded = html_entities[c];
      if (encoded) fputs(encoded, out);
      else fputc(c, out);
   } else {
      warn("html_encode: byte out of range: %d\n", c);
   }

}
