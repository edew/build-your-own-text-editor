#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void die(const char *s) {
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
		die("tcsetattr");
	}
}

/**
 * By default terminals open in 'canonical mode'. Keyboard input is only sent
 * to your program when the user presses Enter.
 * We want to process each keypress as it happens and for this we need to
 * enable 'raw mode'.
 *
 * There is not a single switch that can be flipped to enable raw mode. We
 * have to change a number of different flags.
 */
void enableRawMode() {
	// Read the current terminal attributes.
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
		die("tcgetattr");
	}

	atexit(disableRawMode);

	// We will modify 'raw' and use 'orig_termios' to reset the
	// terminal when our program exits.
	struct termios raw = orig_termios;

	/**
	 * ECHO causes each keypres to the printed to the terminal. We want
	 * to complete control over output so we disable it.
	 *
	 * ICANON causes keypresses to be buffered until enter is pressed. We
	 * want to process each keypress one-by-one so we disable it.
	 *
	 * ISIG causes interrupt (ctrl-c), quit, suspend, and so on signals to
	 * affect the program. We want to handle these combinations ourselves
	 * so we disable it.
	 *
	 * IXON causes the software flow control characters XON and XOFF to
	 * pause and resume terminal output. We want to handle these
	 * combinations ourselves so we disable it.
	 *
	 * IEXTEN causes ctrl-v followed by [some other character] to send 
	 * [some other character] literally. We want to handle ctrl-v
	 * ourselves so we disable it.
	 *
	 * ICRNL causes input carriage returns to be translated to newlines.
	 * We want to handle the user's actual input so we disable it.
	 *
	 * OPOST causes program output to be transformed in some cases. For
	 * example '\n' characters are translated to '\r\n'. We want to
	 * exactly control the output of our program so we disable it.
	 *
	 * BRKINT, INPCK, ISTRIP, and CS8 probably don't have any effect
	 * in most terminal emulators these days. Disabling them is
	 * considered to be the proper way to enter raw mode though, so we do it
	 * for that reason.
	 *
	 * ECHO is a bitflag defined as 00000000000000000000000000001000 so
	 * when we flip the bits with ~ and AND (&) the result we effectively
	 * set the 4th bit of c_lflag to 0 and leave the other bits unchanged.
	 *
	 * The same logic applies for the other flags.
	 *
	 * CS8 is a bitmask with multiple bits so we set it using a bitwise
	 * OR (|) instead.
	 *
	 * VMIN sets the minimum number of bytes to be read before read()
	 * will return.
	 *
	 * VTIME sets the maximu amount of time that read() will wait for input
	 * before returning. It is specified in 10ths of a second so 1 = 100ms.
	 */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	// Write the modified terminal attributes. TCSAFLUSH means that all
	// pending output to stdin will be flushed before changes are made.
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		die("tcsetattr");
	}
}

int main() {
	enableRawMode();

	while (1) {
		char c = '\0';
		read(STDIN_FILENO, &c, 1);

		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {
			printf("%d ('%c')\r\n", c, c);
		}

		if (c == 'q') {
			break;
		}
	}

	return 0;
}

