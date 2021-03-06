#define _XOPEN_SOURCE_EXTENDED

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>

#include <ncurses.h>

#include <linebreak.h>

#include "uds.h"

#define LANGUAGE "en"
#define LOCAL_COMMAND_MARKER ":"

#define MESSAGE_INTERVAL 1000

#define INPUT_TIMEOUT 15

#define INPUT_LINE_HEIGHT  1
#define TOP_MARGIN    1
#define BOTTOM_MARGIN 1
#define LEFT_MARGIN   1
#define RIGHT_MARGIN  1

#define OUTPUT_WIDTH  (COLS)
#define OUTPUT_HEIGHT (LINES - INPUT_HEIGHT)
#define INPUT_WIDTH   (COLS)
#define INPUT_HEIGHT  (INPUT_LINE_HEIGHT + 2)
#define INPUT_Y       (OUTPUT_HEIGHT + 1)

typedef struct server_console_s {
  GMainContext *mc;
  GMainLoop    *loop;
  const gchar  *server_socket_name;
  uds_t         uds;
  WINDOW       *input_window;
  WINDOW       *output_window;
  GString      *input;
  GString      *saved_input;
  GPtrArray    *history;
  GString      *output;
  GString      *line_broken_output;
  gchar        *line_breaks;
  gssize        input_index;
  gsize         input_scroll;
  gsize         output_scroll;
  gsize         history_index;
  gsize         line_count;
} server_console_t;

static server_console_t server_console;

static void sc_move_cursor_home(server_console_t *server_console);

static void sc_get_dimensions(int *cols, int *rows) {
  int startx;
  int starty;
  int endx;
  int endy;

  getbegyx(stdscr, starty, startx);
  getmaxyx(stdscr, endy, endx);

  *cols = endx - startx;
  *rows = endy - starty;
}

static void sc_refresh_input_window(server_console_t *server_console) {
  box(server_console->input_window, 0, 0);
  wrefresh(server_console->input_window);
}

static void sc_refresh_output_window(server_console_t *server_console) {
  box(server_console->output_window, 0, 0);
  wrefresh(server_console->output_window);
}

static void sc_clear_input_window(server_console_t *server_console) {
  werase(server_console->input_window);
  sc_refresh_input_window(server_console);
}

static void sc_clear_output_window(server_console_t *server_console) {
  werase(server_console->output_window);
  sc_refresh_output_window(server_console);
}

static void sc_clear_input(server_console_t *server_console) {
  server_console->input_index = 0;
  server_console->input_scroll = 0;
  g_string_erase(server_console->input, 0, -1);
  sc_move_cursor_home(server_console);
  sc_clear_input_window(server_console);
  refresh();
}

static void sc_get_cursor_position(server_console_t *server_console, int *x,
                                                                     int *y) {
  int cursx;
  int cursy;

  getyx(stdscr, cursy, cursx);

  *x = cursx;
  *y = cursy;
}

static void sc_set_cursor_position(server_console_t *server_console, int x,
                                                                     int y) {
  move(y, x);
}

static void sc_update_cursor(server_console_t *server_console) {
  int cury;
  int curx;
  int newy = -1;
  int newx = -1;

  getyx(stdscr, cury, curx);

  if (curx < LEFT_MARGIN)
    newx = LEFT_MARGIN;

  if (curx > (INPUT_WIDTH - LEFT_MARGIN))
    newx = (INPUT_WIDTH - LEFT_MARGIN);

  if (cury != INPUT_Y)
    newy = INPUT_Y;

  if ((newx != -1) || (newy != -1)) {
    if (newx == -1)
      newx = curx;
    if (newy == -1)
      newy = cury;

    move(newy, newx);
  }
}

static gssize sc_get_input_index(server_console_t *server_console) {
  int    x;
  int    y;
  gssize pos;

  sc_get_cursor_position(server_console, &x, &y);

  if (x <= 0)
    return 0;

  if (x == (server_console->input->len + 1))
    return -1;

  return x - 1;
}

static void sc_refresh(server_console_t *server_console) {
  sc_refresh_input_window(server_console);
  sc_refresh_output_window(server_console);
  sc_update_cursor(server_console);
  refresh();
}

static void sc_get_output_dimensions(server_console_t *server_console,
                                     int *cols, int *rows) {
  int startx;
  int starty;
  int endx;
  int endy;

  getbegyx(server_console->output_window, starty, startx);
  getmaxyx(server_console->output_window, endy, endx);

  *cols = (endx - startx) - (LEFT_MARGIN + RIGHT_MARGIN);
  *rows = (endy - starty) - (TOP_MARGIN - BOTTOM_MARGIN);
}

static gsize get_char_count(void) {
  return (INPUT_WIDTH - (LEFT_MARGIN + RIGHT_MARGIN)) - 1;
}

static gsize sc_get_max_scroll(server_console_t *server_console) {
  gsize char_count = get_char_count();

  if (server_console->input->len > char_count)
    return server_console->input->len - char_count;

  return 0;
}

static void sc_display_input(server_console_t *server_console) {
  static GString *buf;

  gsize  char_count = get_char_count();
  gsize  input_length;
  gchar *input_line;

  if (!buf)
    buf = g_string_new("");
  else
    g_string_erase(buf, 0, -1);

  input_line = server_console->input->str + server_console->input_scroll;
  input_length = MIN(
    char_count + 1,
    server_console->input->len - server_console->input_scroll
  );

  g_string_insert_len(buf, 0, input_line, input_length);

  sc_clear_input_window(server_console);
  mvwprintw(
    server_console->input_window,
    TOP_MARGIN,
    LEFT_MARGIN,
    "%s",
    buf->str
  );

  sc_refresh_input_window(server_console);
  refresh();
}

static void sc_display_output(server_console_t *server_console) {
  static GString *buf = NULL;

  int     cx;
  int     cy;
  int     cols;
  int     rows;
  int     first_line_index = 0;
  gchar  *line;
  gchar **lines;

  if (!buf)
    buf = g_string_new("");

  sc_clear_output_window(server_console);
  sc_get_output_dimensions(server_console, &cols, &rows);

  sc_get_cursor_position(server_console, &cx, &cy);

  rows -= TOP_MARGIN + BOTTOM_MARGIN;

  first_line_index  = server_console->line_count - 1;
  first_line_index -= (rows - 1);
  first_line_index -= server_console->output_scroll;
  first_line_index  = MAX(first_line_index, 0);

  line = server_console->line_broken_output->str;

  for (int i = 0; i < first_line_index; i++) {
    gchar *next_line = g_utf8_strchr(line, -1, '\n');

    if (!next_line)
      break;

    next_line = g_utf8_next_char(next_line);

    if (!next_line)
      break;

    line = next_line;
  }

  for (int i = 0; i < rows; i++) {
    gchar *next_line;

    if (!line)
      break;

    next_line = g_utf8_strchr(line, -1, '\n');

    if (next_line) {
      next_line = g_utf8_next_char(next_line);

      if (next_line) {
        g_string_erase(buf, 0, -1);
        g_string_insert_len(buf, 0, line, next_line - line);
      }
      else {
        g_string_assign(buf, line);
      }
    }
    else {
      g_string_assign(buf, line);
    }

print_line:

    mvwprintw(
      server_console->output_window,
      TOP_MARGIN + i,
      LEFT_MARGIN,
      "%s\n",
      buf->str
    );

    line = next_line;

    if (!line)
      break;
  }

refresh:

  sc_refresh_output_window(server_console);
  sc_set_cursor_position(server_console, cx, cy);
  refresh();
}

static void sc_format_output(server_console_t *server_console, gsize offset) {
  int       cols;
  int       rows;
  gsize     col;
  gchar    *os;
  gchar    *ss;
  gchar    *cs;
  gchar    *last_line_break;
  gboolean  ended_on_linebreak;

  if (!server_console->line_breaks) {
    server_console->line_breaks = g_malloc0_n(
      server_console->output->len, sizeof(gchar)
    );
  }
  else {
    server_console->line_breaks = g_realloc_n(
      server_console->line_breaks, server_console->output->len, sizeof(gchar)
    );
    memset(
      server_console->line_breaks + offset,
      0,
      server_console->output->len - offset
    );
  }

  set_linebreaks_utf8(
    server_console->output->str + offset,
    server_console->output->len - offset,
    LANGUAGE,
    server_console->line_breaks + offset
  );

  sc_get_output_dimensions(server_console, &cols, &rows);

  /*
  if (cols)
    cols--;
  */

  os = server_console->output->str;
  ss = server_console->output->str + offset;
  cs = ss;
  col = 0;
  last_line_break = NULL;

  while ((cs - os) < server_console->output->len) {
    glong lb_index = g_utf8_pointer_to_offset(os, cs);
    char lb = server_console->line_breaks[lb_index];

    ended_on_linebreak = false;

    if (lb == LINEBREAK_MUSTBREAK) {
      if (cs > ss) {
        g_string_insert_len(
          server_console->line_broken_output, -1, ss, (cs - ss)
        );
      }

      g_string_append_unichar(server_console->line_broken_output, '\n');
      server_console->line_count++;
      ended_on_linebreak = true;

      last_line_break = NULL;
      cs = g_utf8_next_char(cs);
      ss = cs;
      col = 0;

      continue;
    }

    if (lb == LINEBREAK_ALLOWBREAK)
      last_line_break = cs;

    col++;

    if (col == cols) {
      if (last_line_break) {
        cs = last_line_break;
        last_line_break = NULL;
      }

      g_string_insert_len(
        server_console->line_broken_output, -1, ss, (cs - ss) + 1
      );

      g_string_append_unichar(server_console->line_broken_output, '\n');
      server_console->line_count++;
      ended_on_linebreak = true;

      ss = g_utf8_next_char(cs);

      col = 0;
    }

    cs = g_utf8_next_char(cs);
  }

  if (!ended_on_linebreak)
    server_console->line_count++;
}

static void sc_reformat_output(server_console_t *server_console) {
  g_string_erase(server_console->line_broken_output, 0, -1);
  server_console->line_count = 0;
  sc_format_output(server_console, 0);
}

static void sc_add_output_manual(server_console_t *server_console,
                                 const gchar *str,
                                 const gsize len) {
  gsize starting_length = server_console->output->len;

  if (!str)
    return;

  if (!len)
    return;

  if (!g_utf8_validate(str, len, NULL))
    g_string_append(server_console->output, "<Invalid UTF-8 data>\n");
  else
    g_string_append(server_console->output, str);

  sc_format_output(server_console, starting_length);
  sc_display_output(server_console);
}

static void sc_add_output(server_console_t *server_console, GString *data) {
  sc_add_output_manual(server_console, data->str, data->len);
}

static void handle_data(uds_t *uds, uds_peer_t *peer) {
  sc_add_output(&server_console, uds->input);
}

static void handle_exception(uds_t *uds) {
  sc_add_output(&server_console, g_string_prepend(uds->exception,
    "Network Exception: "
  ));
}

static void cleanup(void) {
  uds_free(&server_console.uds);
  clear();
  refresh();
  endwin();
}

static gboolean task_send_data(gpointer user_data) {
  static guint counter = 1;
  static GString *s = NULL;

  server_console_t *server_console = (server_console_t *)user_data;
  uds_t *uds = &server_console->uds;
  uds_peer_t *server = uds_get_peer(uds, server_console->server_socket_name);

  if (!s)
    s = g_string_new("");

  if (!server) {
    g_printerr("No server peer!\n");
    return G_SOURCE_REMOVE;
  }

  g_string_printf(s, "Client message %u", counter);
  uds_peer_sendto(server, s->str);

  counter++;

  return G_SOURCE_CONTINUE;
}

static void sc_scroll_input_right(server_console_t *server_console) {
  server_console->input_scroll = MAX(server_console->input_scroll - 1, 0);
}

static void sc_scroll_input_left(server_console_t *server_console) {
  server_console->input_scroll = MIN(
    server_console->input_scroll + 1, sc_get_max_scroll(server_console)
  );
}

static void sc_move_cursor_left(server_console_t *server_console) {
  int x;
  int y;

  sc_get_cursor_position(server_console, &x, &y);

  if (x <= LEFT_MARGIN)
    sc_scroll_input_right(server_console);

  x = MAX(x - 1, LEFT_MARGIN);

  sc_set_cursor_position(server_console, x, y);

  if (server_console->input_index == -1)
    server_console->input_index = server_console->input->len - 1;
  else if (server_console->input_index > 0)
    server_console->input_index--;

  sc_display_input(server_console);
}

static void sc_move_cursor_right(server_console_t *server_console) {
  int   x;
  int   y;
  gsize ms = sc_get_max_scroll(server_console);
  gsize char_count = get_char_count();

  sc_get_cursor_position(server_console, &x, &y);

  if (x > char_count)
    sc_scroll_input_left(server_console);

  x = MIN(x + 1, MIN(
    server_console->input->len + LEFT_MARGIN,
    char_count + LEFT_MARGIN
  ));

  sc_set_cursor_position(server_console, x, y);

  if (server_console->input_index == server_console->input->len - 1)
    server_console->input_index = -1;
  else if (server_console->input_index < server_console->input->len - 1)
    server_console->input_index++;

  sc_display_input(server_console);
}

static void sc_move_cursor_home(server_console_t *server_console) {
  int x;
  int y;

  sc_get_cursor_position(server_console, &x, &y);
  sc_set_cursor_position(server_console, LEFT_MARGIN, y);
  server_console->input_index = 0;
  server_console->input_scroll = 0;
  sc_display_input(server_console);
}

static void sc_move_cursor_to_end(server_console_t *server_console) { int   x;
  int   y;
  gsize char_count = get_char_count();

  sc_get_cursor_position(server_console, &x, &y);
  x = MIN(server_console->input->len + LEFT_MARGIN, char_count + LEFT_MARGIN);
  sc_set_cursor_position(server_console, x, y);
  server_console->input_index = -1;
  server_console->input_scroll = sc_get_max_scroll(server_console);
  sc_display_input(server_console);
}

static void sc_show_history_previous(server_console_t *server_console) {
  if (server_console->history_index == server_console->history->len)
    return;

  if (server_console->history_index == 0)
    g_string_assign(server_console->saved_input, server_console->input->str);

  server_console->history_index++;

  g_string_assign(server_console->input, g_ptr_array_index(
    server_console->history,
    (server_console->history->len - 1) - (server_console->history_index - 1)
  ));

  sc_move_cursor_to_end(server_console);
}

static void sc_show_history_next(server_console_t *server_console) {
  if (server_console->history_index == 0)
    return;

  server_console->history_index--;

  if (server_console->history_index == 0) {
    g_string_assign(server_console->input, server_console->saved_input->str);
  }
  else {
    g_string_assign(server_console->input, g_ptr_array_index(
      server_console->history,
      (server_console->history->len - 1) - (server_console->history_index - 1)
    ));
  }

  sc_move_cursor_to_end(server_console);
}

static void sc_delete_next_char(server_console_t *server_console) {
  if (server_console->input_index < 0)
    return;

  if (!server_console->input->len)
    return;

  if (server_console->input_index >= server_console->input->len)
    return;

  g_string_erase(server_console->input, server_console->input_index, 1);
  sc_display_input(server_console);
}

static void sc_delete_previous_char(server_console_t *server_console) {
  sc_move_cursor_left(server_console);
  sc_delete_next_char(server_console);
}

static void sc_scroll_output_up(server_console_t *server_console) {
  int cols;
  int rows;

  sc_get_output_dimensions(server_console, &cols, &rows);

  rows -= (TOP_MARGIN + BOTTOM_MARGIN);

  if (server_console->output_scroll < server_console->line_count - rows) {
    server_console->output_scroll++;
    sc_display_output(server_console);
  }
}

static void sc_scroll_output_down(server_console_t *server_console) {
  if (server_console->output_scroll > 0) {
    server_console->output_scroll--;
    sc_display_output(server_console);
  }
}

static void sc_dump_output(server_console_t *server_console) {
  GError *error = NULL;
  gboolean success;

  success = g_file_set_contents(
    "output.log",
    server_console->output->str,
    server_console->output->len,
    &error
  );

  if (!success) {
    g_printerr("Error dumping output: %s\n", error->message);
    exit(EXIT_FAILURE);
  }
}

static void sc_handle_local_command(server_console_t *server_console) {
  if (g_strcmp0(server_console->input->str, ":q") == 0)
    exit(EXIT_SUCCESS);
  if (g_strcmp0(server_console->input->str, ":quit") == 0)
    exit(EXIT_SUCCESS);
  if (g_strcmp0(server_console->input->str, ":dumpout") == 0)
    sc_dump_output(server_console);
}

static void sc_handle_command(server_console_t *server_console) {
  static GString *buf = NULL;

  uds_peer_t *server;

  if (!server_console->input->len)
    return;

  if (strncmp(server_console->input->str, LOCAL_COMMAND_MARKER, 1) == 0) {
    sc_handle_local_command(server_console);
    server_console->history_index = 0;
    return;
  }

  server = uds_get_peer(
    &server_console->uds, server_console->server_socket_name
  );

  if (!server) {
    g_printerr("No server!\n");
    return;
  }

  if (!buf)
    buf = g_string_new("");

  g_string_assign(buf, server_console->input->str);
  g_string_append_unichar(buf, '\n');

  uds_peer_sendto(server, server_console->input->str);
  g_ptr_array_add(server_console->history,
    g_strdup(server_console->input->str)
  );
  sc_add_output(server_console, buf);

  server_console->history_index = 0;
}

static void sc_handle_key(server_console_t *server_console, wint_t key) {
  if (key == '\n') {
    sc_handle_command(server_console);
    sc_clear_input(server_console);

    return;
  }

  if (key == 127) {
    sc_delete_previous_char(server_console);
    return;
  }

  g_string_insert_unichar(
    server_console->input, server_console->input_index, key
  );

  sc_move_cursor_right(server_console);
  sc_display_input(server_console);
}

static void sc_handle_function_key(server_console_t *server_console,
                                   wint_t key) {
  switch (key) {
    case KEY_UP:
      sc_show_history_previous(server_console);
    break;
    case KEY_DOWN:
      sc_show_history_next(server_console);
    break;
    case KEY_LEFT:
      sc_move_cursor_left(server_console);
    break;
    case KEY_RIGHT:
      sc_move_cursor_right(server_console);
    break;
    case KEY_HOME:
      sc_move_cursor_home(server_console);
    break;
    case KEY_END:
      sc_move_cursor_to_end(server_console);
    break;
    case KEY_BACKSPACE:
      sc_delete_previous_char(server_console);
    break;
    case KEY_DC:
      sc_delete_next_char(server_console);
    break;
    case KEY_PPAGE:
      sc_scroll_output_up(server_console);
    break;
    case KEY_NPAGE:
      sc_scroll_output_down(server_console);
    break;
    case KEY_ENTER:
      sc_handle_command(server_console);
    break;
    default:
    break;
  }
}

static WINDOW* add_window(int x, int y, int width, int height) {
  WINDOW *w = newwin(height, width, y, x);

  box(w, 0, 0);
  wrefresh(w);

  return w;
}

static void delete_window(WINDOW *w) {
  werase(w);
  wborder(w, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
  refresh();
  delwin(w);
  refresh();
}

static void init_curses(void) {
  initscr();
  cbreak();
  noecho();
  timeout(INPUT_TIMEOUT);
  keypad(stdscr, TRUE);
  refresh();
}

static gboolean task_resize_console(gpointer user_data) {
  server_console_t *server_console = (server_console_t *)user_data;

  sc_clear_input_window(server_console);
  sc_clear_output_window(server_console);

  delete_window(server_console->input_window);
  delete_window(server_console->output_window);

  endwin();
  initscr();
  refresh();
  clear();

  server_console->input_window = add_window(
    0, OUTPUT_HEIGHT, INPUT_WIDTH, INPUT_HEIGHT
  );
  server_console->output_window = add_window(
    0, 0, OUTPUT_WIDTH, OUTPUT_HEIGHT
  );

  wrefresh(server_console->input_window);
  wrefresh(server_console->output_window);

  sc_reformat_output(server_console);

  sc_display_input(server_console);
  sc_display_output(server_console);

  sc_refresh(server_console);

  return G_SOURCE_REMOVE;
}

static void handle_resize(int signal) {
  g_idle_add(task_resize_console, &server_console);
}

static gboolean task_read_input(gpointer user_data) {
  server_console_t *server_console = (server_console_t *)user_data;
  wint_t wch;
  int status = get_wch(&wch);

  switch (status) {
    case OK:
      sc_handle_key(server_console, wch);
    break;
    case KEY_CODE_YES:
      sc_handle_function_key(server_console, wch);
    break;
    default:
    break;
  }

  return G_SOURCE_CONTINUE;
}

static void free_history_entry(gpointer data) {
  g_free(data);
}

int main(int argc, char **argv) {
  signal(SIGWINCH, handle_resize);

  if (argc != 2) {
    g_printerr("\nUsage: %s [ socket_path ]\n\n",
      g_path_get_basename(argv[0])
    );
    exit(EXIT_FAILURE);
  }

  memset(&server_console, 0, sizeof(server_console_t));

  server_console.server_socket_name = argv[1];

  init_curses();

  uds_init(
    &server_console.uds,
    CLIENT_SOCKET_NAME,
    handle_data,
    handle_exception,
    FALSE
  );

  atexit(cleanup);

  server_console.input_window = add_window(
    0, OUTPUT_HEIGHT, INPUT_WIDTH, INPUT_HEIGHT
  );
  server_console.output_window = add_window(
    0, 0, OUTPUT_WIDTH, OUTPUT_HEIGHT
  );

  server_console.input = g_string_new("");
  server_console.saved_input = g_string_new("");
  server_console.history = g_ptr_array_new_with_free_func(free_history_entry);
  server_console.output = g_string_new("");
  server_console.line_broken_output = g_string_new("");
  server_console.line_breaks = NULL;

  server_console.input_scroll = 0;
  server_console.output_scroll = 0;
  server_console.history_index = 0;
  server_console.line_count = 0;

  wrefresh(server_console.input_window);
  wrefresh(server_console.output_window);
  sc_update_cursor(&server_console);
  refresh();

  uds_connect(&server_console.uds, server_console.server_socket_name);

  server_console.mc = g_main_context_default();
  server_console.loop = g_main_loop_new(server_console.mc, FALSE);

  g_idle_add(task_read_input, &server_console);
  // g_timeout_add(MESSAGE_INTERVAL, task_send_data, &server_console);

  g_main_loop_run(server_console.loop);

  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */

