/* ************************************************************************** */
/*                                                                            */
/*   main.c - ft_printf test suite (42 School, Linux)                         */
/*                                                                            */
/*   Build : cc -Wall -Wextra -Werror -g main.c -L. -lftprintf -o test        */
/*   Run   : ./test [-v] [--no-ranges] [--skip-write] [--malformed]           */
/*                                                                            */
/*   How it works                                                             */
/*   - Every test runs in its own forked child process, so a segfault or an   */
/*     infinite loop in ft_printf is reported as CRASH / TIMEOUT instead of   */
/*     killing the whole suite.                                               */
/*   - In the child, file descriptor 1 is redirected (dup2) to an unlinked    */
/*     temporary file. printf() and then ft_printf() are called with the      */
/*     exact same arguments, each into a fresh capture.                       */
/*   - The captured BYTES (length + memcmp, never strcmp) and the return      */
/*     values are compared, so "\0" produced by %c is handled correctly.      */
/*                                                                            */
/* ************************************************************************** */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
** Some tests intentionally use an empty format string, which GCC would flag
** with -Wformat-zero-length (an error under -Werror). The behaviour is valid.
*/
#if defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

/*
** Declared here (instead of including ft_printf.h) so this file does not
** depend on the content of your header. The format attribute lets the
** compiler check the test arguments for ft_printf exactly like for printf.
*/
#if defined(__GNUC__)
int	ft_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
#else
int	ft_printf(const char *format, ...);
#endif

#define TEST_TIMEOUT 5

/* ------------------------------------------------------------------------- */
/*  Global state                                                             */
/* ------------------------------------------------------------------------- */

typedef struct s_cap
{
	unsigned char	*data;
	size_t			len;
}	t_cap;

typedef int	(*t_wf_fn)(void);

static int	g_total;
static int	g_passed;
static int	g_failed;
static int	g_verbose;
static int	g_quiet;
static int	g_ranges = 1;
static int	g_color;
static int	g_tmp_fd = -1;
static int	g_saved_fd = -1;

/* Volatile so the compiler cannot constant-fold NULL into a format check. */
static char *volatile	g_null_str = NULL;
static void *volatile	g_null_ptr = NULL;

static int		g_static_int = 42;
static char		g_static_buf[16] = "static";

struct s_point
{
	int	x;
	int	y;
};

/* ------------------------------------------------------------------------- */
/*  Small helpers                                                            */
/* ------------------------------------------------------------------------- */

static const char	*col_green(void)
{
	return (g_color ? "\033[32m" : "");
}

static const char	*col_red(void)
{
	return (g_color ? "\033[31m" : "");
}

static const char	*col_yellow(void)
{
	return (g_color ? "\033[33m" : "");
}

static const char	*col_reset(void)
{
	return (g_color ? "\033[0m" : "");
}

static void	die(const char *msg)
{
	perror(msg);
	_exit(2);
}

static void	section(const char *title)
{
	printf("\n[%s]\n", title);
}

static void	print_escaped(const unsigned char *d, size_t n)
{
	size_t	i;
	size_t	limit;

	limit = (n > 120) ? 120 : n;
	putchar('"');
	i = 0;
	while (i < limit)
	{
		if (d[i] == '\n')
			fputs("\\n", stdout);
		else if (d[i] == '\t')
			fputs("\\t", stdout);
		else if (d[i] == '\\')
			fputs("\\\\", stdout);
		else if (d[i] == '"')
			fputs("\\\"", stdout);
		else if (d[i] == '\0')
			fputs("\\0", stdout);
		else if (isprint(d[i]))
			putchar(d[i]);
		else
			printf("\\x%02X", d[i]);
		i++;
	}
	putchar('"');
	if (n > limit)
		printf("... (+%zu bytes)", n - limit);
}

/* Builds a heap string of n characters: ABC...XYZABC... (caller frees). */
static char	*make_str(size_t n)
{
	char	*s;
	size_t	i;

	s = malloc(n + 1);
	if (!s)
		return (NULL);
	i = 0;
	while (i < n)
	{
		s[i] = (char)('A' + (i % 26));
		i++;
	}
	s[n] = '\0';
	return (s);
}

/* ------------------------------------------------------------------------- */
/*  Output capture (fd based, byte exact)                                    */
/* ------------------------------------------------------------------------- */

static void	cap_begin(void)
{
	char	path[] = "/tmp/ft_printf_test_XXXXXX";

	fflush(stdout);
	if (g_tmp_fd < 0)
	{
		g_tmp_fd = mkstemp(path);
		if (g_tmp_fd < 0)
			die("mkstemp");
		unlink(path);
	}
	if (ftruncate(g_tmp_fd, 0) < 0 || lseek(g_tmp_fd, 0, SEEK_SET) < 0)
		die("reset capture file");
	g_saved_fd = dup(STDOUT_FILENO);
	if (g_saved_fd < 0 || dup2(g_tmp_fd, STDOUT_FILENO) < 0)
		die("redirect stdout");
}

static t_cap	cap_end(void)
{
	t_cap	cap;
	off_t	size;
	size_t	got;
	ssize_t	n;

	fflush(stdout);
	if (dup2(g_saved_fd, STDOUT_FILENO) < 0)
		die("restore stdout");
	close(g_saved_fd);
	g_saved_fd = -1;
	size = lseek(g_tmp_fd, 0, SEEK_CUR);
	if (size < 0 || lseek(g_tmp_fd, 0, SEEK_SET) < 0)
		die("lseek");
	cap.data = malloc((size_t)size + 1);
	if (!cap.data)
		die("malloc");
	got = 0;
	while (got < (size_t)size)
	{
		n = read(g_tmp_fd, cap.data + got, (size_t)size - got);
		if (n <= 0)
			die("read");
		got += (size_t)n;
	}
	cap.data[got] = '\0';
	cap.len = got;
	return (cap);
}

/* ------------------------------------------------------------------------- */
/*  Verdict + process supervision                                            */
/* ------------------------------------------------------------------------- */

/* Runs in the child: prints PASS/FAIL, returns 1 when everything matches. */
static int	verdict(const char *name, const t_cap *exp, int r_exp,
				const t_cap *got, int r_got)
{
	int		same_bytes;
	int		same_ret;
	size_t	i;
	size_t	min;

	same_bytes = (exp->len == got->len
			&& (exp->len == 0 || memcmp(exp->data, got->data, exp->len) == 0));
	same_ret = (r_exp == r_got);
	if (same_bytes && same_ret)
	{
		if (!g_quiet || g_verbose)
		{
			printf("%s[PASS]%s %s", col_green(), col_reset(), name);
			if (g_verbose)
				printf("  (ret=%d, %zu bytes)", r_exp, exp->len);
			printf("\n");
		}
		fflush(stdout);
		return (1);
	}
	printf("%s[FAIL]%s %s\n", col_red(), col_reset(), name);
	printf("       printf   : ret=%d, %zu bytes: ", r_exp, exp->len);
	print_escaped(exp->data, exp->len);
	printf("\n       ft_printf: ret=%d, %zu bytes: ", r_got, got->len);
	print_escaped(got->data, got->len);
	printf("\n");
	if (!same_ret)
		printf("       -> return value differs\n");
	if (!same_bytes)
	{
		min = (exp->len < got->len) ? exp->len : got->len;
		i = 0;
		while (i < min && exp->data[i] == got->data[i])
			i++;
		printf("       -> output differs at byte %zu\n", i);
	}
	fflush(stdout);
	return (0);
}

/* Runs in the parent: waits for the child and updates the counters. */
static void	reap(const char *name, pid_t pid)
{
	int	status;

	g_total++;
	if (pid < 0 || waitpid(pid, &status, 0) < 0)
	{
		printf("%s[ERROR]%s %s (fork/wait failed)\n",
			col_red(), col_reset(), name);
		g_failed++;
		return ;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		g_passed++;
	else if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
		g_failed++;
	else if (WIFSIGNALED(status))
	{
		printf("%s[%s]%s %s (signal %d: %s)\n", col_red(),
			WTERMSIG(status) == SIGALRM ? "TIMEOUT" : "CRASH", col_reset(),
			name, WTERMSIG(status), strsignal(WTERMSIG(status)));
		g_failed++;
	}
	else
	{
		printf("%s[ERROR]%s %s (harness error, exit code %d)\n",
			col_red(), col_reset(), name, WEXITSTATUS(status));
		g_failed++;
	}
}

/*
** TEST(name, format, args...)
** Calls printf and ft_printf with IDENTICAL arguments inside a child process
** and compares captured bytes and return values.
*/
#define TEST(name, ...) \
	do { \
		pid_t	pid_; \
		fflush(stdout); \
		pid_ = fork(); \
		if (pid_ == 0) \
		{ \
			t_cap	a_; \
			t_cap	b_; \
			int		r1_; \
			int		r2_; \
			int		ok_; \
			alarm(TEST_TIMEOUT); \
			cap_begin(); \
			r1_ = printf(__VA_ARGS__); \
			a_ = cap_end(); \
			cap_begin(); \
			r2_ = ft_printf(__VA_ARGS__); \
			b_ = cap_end(); \
			ok_ = verdict(name, &a_, r1_, &b_, r2_); \
			free(a_.data); \
			free(b_.data); \
			_exit(ok_ ? 0 : 1); \
		} \
		reap(name, pid_); \
	} while (0)

/* ------------------------------------------------------------------------- */
/*  %c                                                                       */
/* ------------------------------------------------------------------------- */

static void	test_char(void)
{
	char	label[64];
	int		i;

	section("CHARACTER TESTS");
	TEST("Lowercase 'a'", "%c", 'a');
	TEST("Lowercase 'z'", "%c", 'z');
	TEST("Uppercase 'A'", "%c", 'A');
	TEST("Uppercase 'Z'", "%c", 'Z');
	TEST("Digit '0'", "%c", '0');
	TEST("Digit '9'", "%c", '9');
	TEST("Space", "%c", ' ');
	TEST("Exclamation mark", "%c", '!');
	TEST("Tilde", "%c", '~');
	TEST("Newline", "%c", '\n');
	TEST("Tab", "%c", '\t');
	TEST("Backslash", "%c", '\\');
	TEST("Percent character as argument", "%c", '%');
	TEST("Single quote", "%c", '\'');
	TEST("Double quote", "%c", '"');
	TEST("Null character", "%c", '\0');
	TEST("DEL (127)", "%c", 127);
	TEST("Byte 128", "%c", 128);
	TEST("Byte 255", "%c", 255);
	TEST("Text around char", "before %c after", 'X');
	TEST("Char in brackets", "[%c]", 'x');
	TEST("Two chars", "%c%c", '4', '2');
	TEST("Five chars (Hello)", "%c%c%c%c%c", 'H', 'e', 'l', 'l', 'o');
	TEST("Chars separated by spaces", "%c %c %c", 'a', 'b', 'c');
	TEST("Same char repeated", "%c%c%c%c", 'z', 'z', 'z', 'z');
	TEST("Three null characters", "%c%c%c", 0, 0, 0);
	TEST("Null char between letters", "a%cb", '\0');
	TEST("Null char then text", "%cabc", '\0');
	TEST("Text then null char", "abc%c", '\0');
	TEST("Null chars mixed with letters", "%c%c%c%c", 0, 'A', 0, 'B');
	TEST("Char and percent", "%c%%%c", 'a', 'b');
	TEST("Char and string", "%c%s%c", 'a', "str", 'b');
	TEST("Char and decimal", "%c%d%c", 'x', 42, 'y');
	TEST("Char and unsigned", "%c%u%c", 'x', 42u, 'y');
	TEST("Char and hex", "%c%x%c", '<', 255, '>');
	TEST("Char and HEX", "%c%X%c", '<', 255, '>');
	TEST("Char and pointer", "%c%p%c", '(', (void *)&g_static_int, ')');
	TEST("Null char mixed with int", "%d%c%d", 1, '\0', 2);
	if (!g_ranges)
		return ;
	printf("  ... every byte value 0..255 (silent unless failing)\n");
	g_quiet = 1;
	i = 0;
	while (i < 256)
	{
		snprintf(label, sizeof(label), "%%c with byte %d", i);
		TEST(label, "%c", i);
		i++;
	}
	g_quiet = 0;
}

/* ------------------------------------------------------------------------- */
/*  %s                                                                       */
/* ------------------------------------------------------------------------- */

static void	test_string(void)
{
	char	*s100;
	char	*s1000;
	char	*s10000;
	char	*s100000;

	section("STRING TESTS");
	s100 = make_str(100);
	s1000 = make_str(1000);
	s10000 = make_str(10000);
	s100000 = make_str(100000);
	if (!s100 || !s1000 || !s10000 || !s100000)
	{
		printf("%s[ERROR]%s cannot allocate long strings\n",
			col_red(), col_reset());
		g_total++;
		g_failed++;
		free(s100);
		free(s1000);
		free(s10000);
		free(s100000);
		return ;
	}
	TEST("Normal string", "%s", "Hello, World!");
	TEST("Empty string", "%s", "");
	TEST("NULL string", "%s", g_null_str);
	TEST("One-character string", "%s", "a");
	TEST("Only spaces", "%s", "     ");
	TEST("String with spaces", "%s", "Hello World with several spaces");
	TEST("Leading and trailing spaces", "%s", "  padded  ");
	TEST("Digits string", "%s", "1234567890");
	TEST("Tab inside string", "%s", "tab\there");
	TEST("Newline inside string", "%s", "line1\nline2\n");
	TEST("Backslash inside string", "%s", "back\\slash");
	TEST("Quotes inside string", "%s", "say \"hi\" and 'bye'");
	TEST("Special characters", "%s", "!@#$^&*()_+-=[]{};:,.<>?/|~`");
	TEST("String containing %d (not a format)", "%s", "%d %s %x %%");
	TEST("String containing only %", "%s", "%");
	TEST("UTF-8 bytes in string", "%s", "h\xC3\xA9llo w\xC3\xB6rld \xE2\x9C\x93");
	TEST("High bytes in string", "%s", "\x80\x90\xA0\xFE\xFF");
	TEST("100 characters", "%s", s100);
	TEST("1000 characters", "%s", s1000);
	TEST("10000 characters", "%s", s10000);
	TEST("100000 characters", "%s", s100000);
	TEST("Text around string", "Hello, %s!", "42 School");
	TEST("String in brackets", "[%s]", "abc");
	TEST("Empty string in brackets", "[%s]", "");
	TEST("NULL string in brackets", "[%s]", g_null_str);
	TEST("Two strings", "%s %s", "Hello", "World");
	TEST("Three consecutive strings", "%s%s%s", "a", "b", "c");
	TEST("Strings separated by pipes", "%s|%s|%s", "one", "two", "three");
	TEST("Empty strings consecutive", "%s%s%s", "", "", "");
	TEST("Empty, NULL and normal", "%s|%s|%s", "", g_null_str, "x");
	TEST("NULL then string", "%s %s", g_null_str, "x");
	TEST("Two NULL strings", "%s%s", g_null_str, g_null_str);
	TEST("Same string ten times", "%s%s%s%s%s%s%s%s%s%s", "ab", "ab", "ab",
		"ab", "ab", "ab", "ab", "ab", "ab", "ab");
	TEST("String and char", "%s%c%s", "foo", '-', "bar");
	TEST("String and decimal", "Name: %s, Age: %d", "Alice", 30);
	TEST("String and negative decimal", "%s=%d", "temp", -15);
	TEST("String and unsigned", "%s: %u", "max", UINT_MAX);
	TEST("String and hex", "%s %x", "hex", 255);
	TEST("String and HEX", "%s %X", "HEX", 255);
	TEST("String and pointer", "%s at %p", "static", (void *)g_static_buf);
	TEST("String and percent", "%s%%", "100");
	TEST("Strings and several ints", "%s %d %s %i %s", "a", 1, "b", 2, "c");
	TEST("Ten strings 10000 chars (100 KB out)",
		"%s%s%s%s%s%s%s%s%s%s", s10000, s10000, s10000, s10000, s10000,
		s10000, s10000, s10000, s10000, s10000);
	free(s100);
	free(s1000);
	free(s10000);
	free(s100000);
}

/* ------------------------------------------------------------------------- */
/*  %p                                                                       */
/* ------------------------------------------------------------------------- */

static void	test_pointer(void)
{
	int				stack_int;
	char			stack_arr[8];
	double			stack_dbl;
	struct s_point	pt;
	int				*heap_int;
	char			*heap_str;
	void			*heap_big;

	section("POINTER TESTS (glibc prints NULL as \"(nil)\")");
	stack_int = 7;
	stack_arr[0] = 'a';
	stack_dbl = 3.14;
	pt.x = 1;
	pt.y = 2;
	heap_int = malloc(sizeof(int));
	heap_str = malloc(64);
	heap_big = malloc(100000);
	if (!heap_int || !heap_str || !heap_big)
	{
		printf("%s[ERROR]%s malloc failed\n", col_red(), col_reset());
		g_total++;
		g_failed++;
		free(heap_int);
		free(heap_str);
		free(heap_big);
		return ;
	}
	TEST("NULL pointer", "%p", g_null_ptr);
	TEST("NULL pointer literal cast", "%p", (void *)0);
	TEST("Stack int address", "%p", (void *)&stack_int);
	TEST("Stack array address", "%p", (void *)stack_arr);
	TEST("Stack double address", "%p", (void *)&stack_dbl);
	TEST("Stack struct address", "%p", (void *)&pt);
	TEST("Stack struct member address", "%p", (void *)&pt.y);
	TEST("Static int address", "%p", (void *)&g_static_int);
	TEST("Static array address", "%p", (void *)g_static_buf);
	TEST("String literal address", "%p", (void *)"literal");
	TEST("Function address", "%p", (void *)&test_pointer);
	TEST("Heap int address", "%p", (void *)heap_int);
	TEST("Heap string address", "%p", (void *)heap_str);
	TEST("Heap big block address", "%p", (void *)heap_big);
	TEST("int * converted to void *", "%p", (void *)(int *)&stack_int);
	TEST("char * converted to void *", "%p", (void *)(char *)stack_arr);
	TEST("double * converted to void *", "%p", (void *)(double *)&stack_dbl);
	TEST("struct * converted to void *", "%p", (void *)(struct s_point *)&pt);
	TEST("Address 0x1", "%p", (void *)(uintptr_t)1);
	TEST("Address 0xF", "%p", (void *)(uintptr_t)15);
	TEST("Address 0x10", "%p", (void *)(uintptr_t)16);
	TEST("Address 0xFF", "%p", (void *)(uintptr_t)255);
	TEST("Address 0x100", "%p", (void *)(uintptr_t)256);
	TEST("Address 0xDEADBEEF", "%p", (void *)(uintptr_t)0xDEADBEEFu);
	TEST("Address INT_MAX", "%p", (void *)(uintptr_t)INT_MAX);
	TEST("Address UINT_MAX", "%p", (void *)(uintptr_t)UINT_MAX);
	TEST("Address INTPTR_MAX", "%p", (void *)(uintptr_t)INTPTR_MAX);
	TEST("Address UINTPTR_MAX", "%p", (void *)(uintptr_t)UINTPTR_MAX);
	TEST("Text around pointer", "Address: %p!", (void *)&stack_int);
	TEST("Pointer in brackets", "[%p]", (void *)heap_int);
	TEST("Two pointers", "%p %p", (void *)&stack_int, (void *)heap_int);
	TEST("Three pointers", "%p|%p|%p", (void *)&stack_int,
		(void *)&g_static_int, (void *)heap_str);
	TEST("NULL and valid pointers", "%p %p %p", g_null_ptr,
		(void *)&stack_int, g_null_ptr);
	TEST("Same pointer three times", "%p %p %p", (void *)heap_int,
		(void *)heap_int, (void *)heap_int);
	TEST("Pointer and decimal", "ptr=%p val=%d", (void *)&stack_int,
		stack_int);
	TEST("Pointer and char", "%p%c%p", (void *)&stack_int, ':',
		(void *)heap_int);
	TEST("Pointer and string", "%p -> %s", (void *)g_static_buf,
		g_static_buf);
	TEST("Pointer and hex", "%p %x", (void *)(uintptr_t)255, 255);
	TEST("Pointer and percent", "%p%%", (void *)&stack_int);
	free(heap_int);
	free(heap_str);
	free(heap_big);
}

/* ------------------------------------------------------------------------- */
/*  %d / %i                                                                  */
/* ------------------------------------------------------------------------- */

static void	range_int(void)
{
	char	label[64];
	int		i;
	int		k;
	int		p;

	printf("  ... %%d / %%i -100..100 and powers of 10/2 (silent unless failing)\n");
	g_quiet = 1;
	i = -100;
	while (i <= 100)
	{
		snprintf(label, sizeof(label), "%%d with %d", i);
		TEST(label, "%d", i);
		snprintf(label, sizeof(label), "%%i with %d", i);
		TEST(label, "%i", i);
		i++;
	}
	p = 1;
	k = 0;
	while (k < 10)
	{
		snprintf(label, sizeof(label), "%%d around 10^%d", k);
		TEST(label, "%d %d %d", p - 1, p, p + 1);
		snprintf(label, sizeof(label), "%%d around -10^%d", k);
		TEST(label, "%d %d %d", -p - 1, -p, -p + 1);
		if (k < 9)
			p *= 10;
		k++;
	}
	k = 0;
	while (k < 31)
	{
		p = (int)(1u << k);
		snprintf(label, sizeof(label), "%%d around +-2^%d", k);
		TEST(label, "%d %d %d %d", p - 1, p, -p, -p + 1);
		k++;
	}
	g_quiet = 0;
}

static void	test_int(void)
{
	section("SIGNED INTEGER TESTS");
	TEST("%d zero", "%d", 0);
	TEST("%d one", "%d", 1);
	TEST("%d minus one", "%d", -1);
	TEST("%d two", "%d", 2);
	TEST("%d minus two", "%d", -2);
	TEST("%d positive", "%d", 42);
	TEST("%d negative", "%d", -42);
	TEST("%d nine", "%d", 9);
	TEST("%d ten", "%d", 10);
	TEST("%d 99", "%d", 99);
	TEST("%d 100", "%d", 100);
	TEST("%d 999", "%d", 999);
	TEST("%d 1000", "%d", 1000);
	TEST("%d 12345", "%d", 12345);
	TEST("%d -12345", "%d", -12345);
	TEST("%d 1000000", "%d", 1000000);
	TEST("%d -1000000", "%d", -1000000);
	TEST("%d INT_MAX", "%d", INT_MAX);
	TEST("%d INT_MAX - 1", "%d", INT_MAX - 1);
	TEST("%d INT_MIN", "%d", INT_MIN);
	TEST("%d INT_MIN + 1", "%d", INT_MIN + 1);
	TEST("%d char promoted ('A')", "%d", 'A');
	TEST("%d short promoted", "%d", (short)-5);
	TEST("%i zero", "%i", 0);
	TEST("%i positive", "%i", 42);
	TEST("%i negative", "%i", -42);
	TEST("%i INT_MAX", "%i", INT_MAX);
	TEST("%i INT_MIN", "%i", INT_MIN);
	TEST("%i 1000", "%i", 1000);
	TEST("Text around %d", "Value: %d units", 123);
	TEST("%d in brackets", "[%d]", -7);
	TEST("Two %d", "%d %d", 1, 2);
	TEST("Two %d no space", "%d%d", 12, 34);
	TEST("Three %d", "%d%d%d", 1, -2, 3);
	TEST("Five %d", "%d,%d,%d,%d,%d", 1, 2, 3, 4, 5);
	TEST("Repeated same %d", "%d %d %d %d", 77, 77, 77, 77);
	TEST("Two %i", "%i %i", -1, 1);
	TEST("Mixed %d and %i", "%d %i %d %i", 1, 2, 3, 4);
	TEST("Mixed %d and %i boundaries", "%d|%i|%d|%i", INT_MIN, INT_MAX, 0, -1);
	TEST("%d around zero", "%d %d %d %d %d", -2, -1, 0, 1, 2);
	TEST("%d and percent", "%d%%", 50);
	TEST("%d and string", "%d %s", 42, "answer");
	TEST("%d and char", "%d%c%d", 1, '+', 2);
	TEST("%d and unsigned", "%d %u", -1, 1u);
	TEST("%d and hex", "%d is 0x%x", 255, 255);
	TEST("%d and pointer", "%d %p", 5, (void *)&g_static_int);
	if (g_ranges)
		range_int();
}

/* ------------------------------------------------------------------------- */
/*  %u                                                                       */
/* ------------------------------------------------------------------------- */

static void	range_unsigned(void)
{
	char			label[64];
	unsigned int	i;
	unsigned int	p;
	int				k;

	printf("  ... %%u 0..200 and powers of 10/2 (silent unless failing)\n");
	g_quiet = 1;
	i = 0;
	while (i <= 200)
	{
		snprintf(label, sizeof(label), "%%u with %u", i);
		TEST(label, "%u", i);
		i++;
	}
	p = 1;
	k = 0;
	while (k < 10)
	{
		snprintf(label, sizeof(label), "%%u around 10^%d", k);
		TEST(label, "%u %u %u", p - 1, p, p + 1);
		if (k < 9)
			p *= 10;
		k++;
	}
	k = 0;
	while (k < 32)
	{
		p = 1u << k;
		snprintf(label, sizeof(label), "%%u around 2^%d", k);
		TEST(label, "%u %u %u", p - 1, p, p + 1);
		k++;
	}
	g_quiet = 0;
}

static void	test_unsigned(void)
{
	section("UNSIGNED INTEGER TESTS");
	TEST("%u zero", "%u", 0u);
	TEST("%u one", "%u", 1u);
	TEST("%u nine", "%u", 9u);
	TEST("%u ten", "%u", 10u);
	TEST("%u 99", "%u", 99u);
	TEST("%u 100", "%u", 100u);
	TEST("%u 101", "%u", 101u);
	TEST("%u 999", "%u", 999u);
	TEST("%u 1000", "%u", 1000u);
	TEST("%u 9999", "%u", 9999u);
	TEST("%u 10000", "%u", 10000u);
	TEST("%u 65535", "%u", 65535u);
	TEST("%u 65536", "%u", 65536u);
	TEST("%u 999999", "%u", 999999u);
	TEST("%u 1000000", "%u", 1000000u);
	TEST("%u 999999999", "%u", 999999999u);
	TEST("%u 1000000000", "%u", 1000000000u);
	TEST("%u INT_MAX", "%u", (unsigned int)INT_MAX);
	TEST("%u INT_MAX + 1", "%u", (unsigned int)INT_MAX + 1u);
	TEST("%u 4000000000", "%u", 4000000000u);
	TEST("%u UINT_MAX - 1", "%u", UINT_MAX - 1u);
	TEST("%u UINT_MAX", "%u", UINT_MAX);
	TEST("%u (unsigned)-1", "%u", (unsigned int)-1);
	TEST("%u (unsigned)INT_MIN", "%u", (unsigned int)INT_MIN);
	TEST("%u (unsigned)-42", "%u", (unsigned int)-42);
	TEST("Text around %u", "Count: %u items", 7u);
	TEST("%u in brackets", "[%u]", 123u);
	TEST("Two %u", "%u %u", 1u, 2u);
	TEST("Three %u no space", "%u%u%u", 1u, 22u, 333u);
	TEST("Four %u boundaries", "%u|%u|%u|%u", 0u, 1u, INT_MAX + 1u, UINT_MAX);
	TEST("Repeated same %u", "%u %u %u", 99u, 99u, 99u);
	TEST("%u and %d", "%u %d", UINT_MAX, INT_MIN);
	TEST("%u and string", "%u %s", 5u, "five");
	TEST("%u and percent", "%u%%", 100u);
	TEST("%u and char", "%u%c%u", 1u, ',', 2u);
	if (g_ranges)
		range_unsigned();
}

/* ------------------------------------------------------------------------- */
/*  %x / %X                                                                  */
/* ------------------------------------------------------------------------- */

static void	range_hex(void)
{
	char			label[64];
	unsigned int	i;
	unsigned int	p;
	int				k;

	printf("  ... %%x / %%X 0..300 and powers of 2 (silent unless failing)\n");
	g_quiet = 1;
	i = 0;
	while (i <= 300)
	{
		snprintf(label, sizeof(label), "%%x with %u", i);
		TEST(label, "%x", i);
		snprintf(label, sizeof(label), "%%X with %u", i);
		TEST(label, "%X", i);
		i++;
	}
	k = 0;
	while (k < 32)
	{
		p = 1u << k;
		snprintf(label, sizeof(label), "%%x around 2^%d", k);
		TEST(label, "%x %x %x", p - 1, p, p + 1);
		snprintf(label, sizeof(label), "%%X around 2^%d", k);
		TEST(label, "%X %X %X", p - 1, p, p + 1);
		k++;
	}
	g_quiet = 0;
}

static void	test_hex(void)
{
	section("HEXADECIMAL TESTS");
	TEST("%x zero", "%x", 0);
	TEST("%X zero", "%X", 0);
	TEST("%x one", "%x", 1);
	TEST("%X one", "%X", 1);
	TEST("%x 9 / %x 10", "%x %x", 9, 10);
	TEST("%x 15", "%x", 15);
	TEST("%X 15", "%X", 15);
	TEST("%x 16", "%x", 16);
	TEST("%X 16", "%X", 16);
	TEST("%x 17", "%x", 17);
	TEST("%x 31 32 33", "%x %x %x", 31, 32, 33);
	TEST("%x 254", "%x", 254);
	TEST("%x 255", "%x", 255);
	TEST("%X 255", "%X", 255);
	TEST("%x 256", "%x", 256);
	TEST("%X 256", "%X", 256);
	TEST("%x 257", "%x", 257);
	TEST("%x 4095", "%x", 4095);
	TEST("%x 4096", "%x", 4096);
	TEST("%x 65535", "%x", 65535);
	TEST("%x 65536", "%x", 65536);
	TEST("%x 0xABCDEF", "%x", 0xABCDEFu);
	TEST("%X 0xABCDEF", "%X", 0xABCDEFu);
	TEST("%x 0x12345678", "%x", 0x12345678u);
	TEST("%X 0x12345678", "%X", 0x12345678u);
	TEST("%x 0x0F0F0F0F", "%x", 0x0F0F0F0Fu);
	TEST("%x 0xDEADBEEF", "%x", 0xDEADBEEFu);
	TEST("%X 0xDEADBEEF", "%X", 0xDEADBEEFu);
	TEST("%x 0xCAFEBABE", "%x", 0xCAFEBABEu);
	TEST("%X 0xCAFEBABE", "%X", 0xCAFEBABEu);
	TEST("%x INT_MAX", "%x", INT_MAX);
	TEST("%X INT_MAX", "%X", INT_MAX);
	TEST("%x 0x80000000", "%x", 0x80000000u);
	TEST("%X 0x80000000", "%X", 0x80000000u);
	TEST("%x UINT_MAX (lowercase)", "%x", UINT_MAX);
	TEST("%X UINT_MAX (uppercase)", "%X", UINT_MAX);
	TEST("%x (unsigned)-1", "%x", (unsigned int)-1);
	TEST("%x (unsigned)INT_MIN", "%x", (unsigned int)INT_MIN);
	TEST("%X (unsigned)-42", "%X", (unsigned int)-42);
	TEST("Text around %x", "hex: %x!", 48879);
	TEST("Text around %X", "HEX: %X!", 48879);
	TEST("0x prefix written in format", "0x%x", 255);
	TEST("%x in brackets", "[%x]", 171);
	TEST("%x and %X same value", "%x %X", 0xABCDEFu, 0xABCDEFu);
	TEST("Three %x", "%x%x%x", 1, 171, 4095);
	TEST("Three %X", "%X-%X-%X", 1, 171, 4095);
	TEST("Repeated same %x", "%x %x %x %x", 255, 255, 255, 255);
	TEST("Mixed %x %X boundaries", "%x|%X|%x|%X", 0, UINT_MAX, INT_MAX,
		0x80000000u);
	TEST("%x and decimal", "%x = %d", 255, 255);
	TEST("%X and unsigned", "%X = %u", 255, 255u);
	TEST("%x and string", "%x %s", 16, "sixteen");
	TEST("%x and percent", "%x%%", 255);
	TEST("%x and char", "%x%c%X", 10, '-', 11);
	if (g_ranges)
		range_hex();
}

/* ------------------------------------------------------------------------- */
/*  Format string tests                                                      */
/* ------------------------------------------------------------------------- */

static void	test_format(void)
{
	section("FORMAT TESTS");
	TEST("Empty format", "");
	TEST("Text only", "Hello, World!");
	TEST("Text only with newline", "Hello, World!\n");
	TEST("Only a newline", "\n");
	TEST("Only a tab", "\t");
	TEST("Only spaces", "     ");
	TEST("Several lines", "line1\nline2\nline3\n");
	TEST("Escape sequences", "a\tb\\c\"d'e\r\x41");
	TEST("UTF-8 text", "caf\xC3\xA9 \xE2\x82\xAC \xE2\x9C\x93");
	TEST("Single %%", "%%");
	TEST("Two %%", "%%%%");
	TEST("Three %%", "%%%%%%");
	TEST("Five %%", "%%%%%%%%%%");
	TEST("Spaced %%", "%% %% %%");
	TEST("Text and %%", "100%%");
	TEST("Text around %%", "a%%b");
	TEST("%% then literal d", "%%d");
	TEST("%% then literal s", "%%s");
	TEST("%% then literal c", "%%c");
	TEST("%% then literal p", "%%p");
	TEST("%% then literal u", "%%u");
	TEST("%% then %c", "%%%c", 'a');
	TEST("%c then %%", "%c%%", 'a');
	TEST("%% then %s", "%%%s", "str");
	TEST("%% then %d", "%%%d", 42);
	TEST("%d then %%", "%d%%", 42);
	TEST("%% then %x", "%%%x", 255);
	TEST("%% then %X", "%%%X", 255);
	TEST("%% then %u", "%%%u", 42u);
	TEST("%% then %p", "%%%p", (void *)(uintptr_t)255);
	TEST("%% between conversions", "%d%%%d", 1, 2);
	TEST("%x %% %X", "%x%%%X", 171, 171);
	TEST("Percent text", "Progress: %d%% done, %d%% left", 25, 75);
	TEST("One conversion %c", "%c", 'x');
	TEST("One conversion %s", "%s", "x");
	TEST("One conversion %d", "%d", 1);
	TEST("One conversion %i", "%i", 1);
	TEST("One conversion %u", "%u", 1u);
	TEST("One conversion %x", "%x", 1);
	TEST("One conversion %X", "%X", 1);
	TEST("One conversion %p", "%p", (void *)(uintptr_t)1);
	TEST("Text before conversion", "Number: %d", 42);
	TEST("Text after conversion", "%d is the number", 42);
	TEST("Text before and after", "<<%d>>", 42);
	TEST("Text between conversions", "%d and %d", 1, 2);
	TEST("Consecutive conversions", "%c%s%d%i%u%x%X%p%%", 'a', "s", 1, 2, 3u,
		255, 255, (void *)(uintptr_t)16);
	TEST("Five consecutive %d", "%d%d%d%d%d", 1, 2, 3, 4, 5);
	TEST("Every conversion in a sentence",
		"c=%c s=%s d=%d i=%i u=%u x=%x X=%X p=%p %%", 'Q', "str", -1, -2,
		3u, 255, 255, (void *)(uintptr_t)4096);
	TEST("Text between every conversion",
		"A%cB%sC%dD%iE%uF%xG%XH%pI%%J", 'c', "s", 1, 2, 3u, 10, 11,
		(void *)(uintptr_t)12);
	TEST("Long text without conversions",
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
		"eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim "
		"ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
		"aliquip ex ea commodo consequat. Duis aute irure dolor in "
		"reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
		"pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
		"culpa qui officia deserunt mollit anim id est laborum.\n");
}

/* ------------------------------------------------------------------------- */
/*  Advanced tests                                                           */
/* ------------------------------------------------------------------------- */

static void	test_advanced(void)
{
	section("ADVANCED TESTS");
	TEST("Long format with every conversion",
		"Lorem ipsum dolor sit amet, %s consectetur adipiscing elit %d, sed "
		"do eiusmod tempor incididunt ut labore et dolore magna aliqua %c. "
		"Ut enim ad minim veniam %u, quis nostrud exercitation ullamco "
		"laboris nisi ut aliquip ex ea commodo consequat %x. Duis aute irure "
		"dolor in reprehenderit in voluptate velit esse cillum dolore eu "
		"fugiat nulla pariatur %X. Excepteur sint occaecat cupidatat non "
		"proident %p, sunt in culpa qui officia %i 100%%.\n",
		"arg", -123, 'Z', 3000000000u, 0xBEEF, 0xBEEF,
		(void *)&g_static_int, 77);
	TEST("Twenty consecutive %d",
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16, 17,
		-18, 19, -20);
	TEST("Thirty consecutive %c",
		"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1',
		'2', '3');
	TEST("Twenty consecutive %%",
		"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
	TEST("Fifteen mixed conversions",
		"%c|%s|%d|%i|%u|%x|%X|%p|%%|%c|%s|%d|%u|%x|%X",
		'a', "bc", -1, 2, 3u, 4, 5, (void *)(uintptr_t)6, 'd', "ef", -7, 8u,
		9, 10);
	TEST("Repeated arguments", "%d %d %s %s %c %c %x %x %u %u", 5, 5, "r",
		"r", 'k', 'k', 255, 255, 5u, 5u);
	TEST("All boundary values together", "%d %d %u %u %x %X %i",
		INT_MIN, INT_MAX, 0u, UINT_MAX, UINT_MAX, UINT_MAX, INT_MIN);
	TEST("Null bytes between conversions", "%c%d%c%s%c", 0, 1, 0, "x", 0);
	TEST("Null byte inside text", "start%cend", '\0');
	TEST("Null bytes only", "%c%c%c%c%c%c%c%c", 0, 0, 0, 0, 0, 0, 0, 0);
	TEST("Null byte then NULL string", "%c%s", 0, g_null_str);
	TEST("Null byte and %%", "%c%%%c", 0, 0);
	TEST("Return value: empty output", "%s", "");
	TEST("Return value: text only", "12345");
	TEST("Return value: counts null byte", "%c", 0);
	TEST("Return value: INT_MIN is 11 chars", "%d", INT_MIN);
	TEST("Return value: UINT_MAX is 10 chars", "%u", UINT_MAX);
	TEST("Return value: UINT_MAX hex is 8 chars", "%x", UINT_MAX);
}

/* ------------------------------------------------------------------------- */
/*  Write failure tests                                                      */
/* ------------------------------------------------------------------------- */

static int	wf_empty(void)
{
	return (ft_printf(""));
}

static int	wf_text(void)
{
	return (ft_printf("Hello, write failure!\n"));
}

static int	wf_char(void)
{
	return (ft_printf("%c", 'x'));
}

static int	wf_str(void)
{
	return (ft_printf("%s", "Hello"));
}

static int	wf_int(void)
{
	return (ft_printf("%d", 42));
}

static int	wf_hex(void)
{
	return (ft_printf("%x", 255));
}

static int	wf_ptr(void)
{
	return (ft_printf("%p", (void *)&g_static_int));
}

static int	wf_percent(void)
{
	return (ft_printf("%%"));
}

/*
** closed = 0 : stdout is redirected to /dev/full (every write fails: ENOSPC)
** closed = 1 : stdout (fd 1) is closed (every write fails: EBADF)
** zero   = 1 : no write is attempted, so 0 is expected instead of a
**              negative value.
*/
static void	run_wf(const char *name, t_wf_fn fn, int closed, int zero)
{
	pid_t	pid;
	int		saved;
	int		fd;
	int		ret;
	int		ok;

	fflush(stdout);
	pid = fork();
	if (pid == 0)
	{
		alarm(TEST_TIMEOUT);
		saved = dup(STDOUT_FILENO);
		if (saved < 0)
			_exit(2);
		if (closed)
			close(STDOUT_FILENO);
		else
		{
			fd = open("/dev/full", O_WRONLY);
			if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0)
				_exit(2);
			close(fd);
		}
		ret = fn();
		if (dup2(saved, STDOUT_FILENO) < 0)
			_exit(2);
		close(saved);
		ok = zero ? (ret == 0) : (ret < 0);
		if (ok)
			printf("%s[PASS]%s %s  (ret=%d)\n", col_green(), col_reset(),
				name, ret);
		else
			printf("%s[FAIL]%s %s  (ret=%d, expected %s)\n", col_red(),
				col_reset(), name, ret, zero ? "0" : "a negative value");
		fflush(stdout);
		_exit(ok ? 0 : 1);
	}
	reap(name, pid);
}

static void	test_write_failure(void)
{
	section("WRITE FAILURE TESTS (strict: negative return expected)");
	if (access("/dev/full", W_OK) != 0)
	{
		printf("%s[SKIP]%s /dev/full not available on this system\n",
			col_yellow(), col_reset());
		return ;
	}
	run_wf("Empty format, no write attempted -> 0", wf_empty, 0, 1);
	run_wf("/dev/full: plain text", wf_text, 0, 0);
	run_wf("/dev/full: %c", wf_char, 0, 0);
	run_wf("/dev/full: %s", wf_str, 0, 0);
	run_wf("/dev/full: %d", wf_int, 0, 0);
	run_wf("/dev/full: %x", wf_hex, 0, 0);
	run_wf("/dev/full: %p", wf_ptr, 0, 0);
	run_wf("/dev/full: %%", wf_percent, 0, 0);
	run_wf("Closed stdout: plain text", wf_text, 1, 0);
	run_wf("Closed stdout: %d", wf_int, 1, 0);
	run_wf("Closed stdout: %s", wf_str, 1, 0);
}

/* ------------------------------------------------------------------------- */
/*  Optional robustness check for a malformed format (--malformed)           */
/* ------------------------------------------------------------------------- */

/*
** A trailing '%' is undefined behaviour for the real printf, so there is NO
** reference result to compare against. We only verify that ft_printf neither
** crashes nor hangs (it must not read past the end of the format string).
** The format is passed through a variable, plus a dummy argument, so the
** compiler does not reject it and a naive va_arg cannot read garbage.
*/
static void	test_malformed(void)
{
	pid_t		pid;
	t_cap		cap;
	const char	*fmt;

	section("MALFORMED FORMAT (robustness only, output not compared)");
	fmt = "abc%";
	fflush(stdout);
	pid = fork();
	if (pid == 0)
	{
		alarm(TEST_TIMEOUT);
		cap_begin();
		(void)ft_printf(fmt, 0);
		cap = cap_end();
		free(cap.data);
		printf("%s[PASS]%s Trailing '%%' did not crash or hang\n",
			col_green(), col_reset());
		fflush(stdout);
		_exit(0);
	}
	reap("Trailing '%' does not crash or hang", pid);
}

/* ------------------------------------------------------------------------- */
/*  main                                                                     */
/* ------------------------------------------------------------------------- */

static void	usage(const char *prog)
{
	printf("Usage: %s [-v] [--no-ranges] [--skip-write] [--malformed]\n", prog);
	printf("  -v, --verbose  show return value and length of passing tests\n");
	printf("  --no-ranges    skip the big silent loops (faster under valgrind)\n");
	printf("  --skip-write   skip the write-failure tests\n");
	printf("  --malformed    run the trailing-%% robustness check\n");
}

int	main(int argc, char **argv)
{
	int		i;
	int		skip_write;
	int		malformed;
	double	rate;

	skip_write = 0;
	malformed = 0;
	i = 1;
	while (i < argc)
	{
		if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
			g_verbose = 1;
		else if (!strcmp(argv[i], "--no-ranges"))
			g_ranges = 0;
		else if (!strcmp(argv[i], "--skip-write"))
			skip_write = 1;
		else if (!strcmp(argv[i], "--malformed"))
			malformed = 1;
		else
		{
			usage(argv[0]);
			return (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ? 0 : 2);
		}
		i++;
	}
	g_color = isatty(STDOUT_FILENO);
	printf("========================================\n");
	printf("          FT_PRINTF TEST SUITE\n");
	printf("========================================\n");
	test_char();
	test_string();
	test_pointer();
	test_int();
	test_unsigned();
	test_hex();
	test_format();
	test_advanced();
	if (!skip_write)
		test_write_failure();
	if (malformed)
		test_malformed();
	rate = g_total ? (100.0 * g_passed) / g_total : 0.0;
	fflush(stdout);
	printf("\n========================================\n");
	printf("TOTAL TESTS : %d\n", g_total);
	printf("PASSED      : %d\n", g_passed);
	printf("FAILED      : %d\n", g_failed);
	printf("SUCCESS     : %.1f%%\n", rate);
	printf("========================================\n");
	return (g_failed ? 1 : 0);
}
