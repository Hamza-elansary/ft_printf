> **Project:** 42 / 1337 `ft_printf`  
> **Goal:** Understand how `printf()` works and build your own implementation without copying a ready-made solution.

---

## 1. Variadic Functions

**Priority: Must learn first**

Variadic functions can receive a fixed number of named parameters followed by a variable number of additional arguments. `ft_printf` uses this mechanism to receive values described by its format string.

### Research questions

1. What is a variadic function in C?
2. What does `...` mean in a function declaration?
3. What is `va_list`?
4. What do `va_start`, `va_arg`, `va_copy`, and `va_end` do?
5. What are the default argument promotions?
6. Why are `char` and `short` promoted to `int` when passed through `...`?
7. Why should you not assume that `va_list` can be copied using ordinary assignment?
8. What happens if `va_arg` requests a type that does not match the actual argument type?

### References

- [Linux man page — stdarg(3)](https://man7.org/linux/man-pages/man3/stdarg.3.html)
- [cppreference — Variadic arguments](https://en.cppreference.com/w/c/variadic)
- [POSIX — stdarg.h](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/stdarg.h.html)

**Important:** The correct macro name is `va_copy`, not `var_copy`.

---

## 2. Understand the Original `printf`

**Priority: Must learn**

Before implementing a function that imitates `printf`, study the behavior of the original function.

### Research questions

- What is a format string?
- How does `printf` interpret conversion specifiers?
- What is the return value of `printf`?
- How are null string pointers handled on your target system?
- What is the difference between signed and unsigned conversions?
- How are integers represented in hexadecimal?
- How does `%p` display a pointer?
- What happens when the format string contains an invalid or incomplete conversion?

### Conversion specifiers

| Specifier | Meaning |
|:--|:--|
| `%c` | Character |
| `%s` | String |
| `%p` | Pointer address |
| `%d` | Signed decimal integer |
| `%i` | Signed integer |
| `%u` | Unsigned decimal integer |
| `%x` | Lowercase hexadecimal |
| `%X` | Uppercase hexadecimal |
| `%%` | Percent character |

### References

- [Linux man page — printf(3)](https://man7.org/linux/man-pages/man3/printf.3.html)
- [cppreference — Formatted output](https://en.cppreference.com/w/c/io/fprintf)

---

## 3. `write()` — Output and System Calls

**Priority: Important**

In the project, `write()` is the main authorized output function. Understand what it does instead of treating it as a simple printing command.

### Research questions

1. What is a system call?
2. What is a file descriptor?
3. What are `STDIN_FILENO`, `STDOUT_FILENO`, and `STDERR_FILENO`?
4. What does the return value of `write()` represent?
5. What is a partial write?
6. How can `write()` fail?
7. What is the difference between writing to a file and writing to a terminal?

### References

- [Linux man page — write(2)](https://man7.org/linux/man-pages/man2/write.2.html)
- [GNU libc — I/O primitives](https://www.gnu.org/software/libc/manual/html_node/I_002fO-Primitives.html)

---

## 4. Number Representation — Decimal and Hexadecimal

**Priority: Important**

This knowledge is especially useful for `%d`, `%i`, `%u`, `%x`, `%X`, and `%p`.

### Research questions

- What is base 10?
- What is base 16?
- How do you convert a decimal integer to hexadecimal?
- What is the difference between signed and unsigned integers?
- What are `INT_MIN`, `INT_MAX`, and `UINT_MAX`?
- What is integer overflow?
- What is the difference between `int`, `unsigned int`, `long`, and `unsigned long`?
- Why is `INT_MIN` a special case when converting a negative number to a positive magnitude?

### References

- [cppreference — Integer types](https://en.cppreference.com/w/c/types/integer)
- [cppreference — C conversions](https://en.cppreference.com/w/c/language/conversion)

---

## 5. Parsing Algorithms — Reading the Format String

**Priority: Must learn**

Parsing is the central idea behind `ft_printf`. Your function must inspect the format string and decide what each character means.

### Research questions

1. What is parsing?
2. What is string traversal?
3. How can a program distinguish normal characters from `%`?
4. What should happen after the parser finds `%`?
5. How can a parser identify a conversion specifier?
6. What is a finite state machine?
7. How can a `switch` statement help organize conversion handling?

### Suggested topics to search

- `String Parsing in C`
- `Finite State Machine in C`
- `Format String Parsing`
- `Character Classification in C`

### References

- [Introduction to parsing](https://www.geeksforgeeks.org/compiler-design/what-is-parsing/)
- [printf(3) — Format string syntax](https://man7.org/linux/man-pages/man3/printf.3.html)

---

## 6. Function Pointers and Conversion Handlers

**Priority: Useful, optional for the mandatory part**

Function pointers can help organize conversion handlers. For example, each conversion can be associated with a function responsible for printing that type.

### Research questions

- What is a function pointer?
- How do you declare and call one?
- What is an array of function pointers?
- What is a dispatch table?
- When does this approach make code easier to maintain?
- Is it necessary for the mandatory part of `ft_printf`?

### Suggested topics to search

- `Function Pointers in C`
- `Array of Function Pointers in C`
- `Dispatch Table in C`

This is an optional design technique, not a requirement for the basic project.

---

## 7. Testing and Debugging

**Priority: Very important**

Do not rely only on visual output. Compare your function with the original `printf`, including return values and edge cases.

### Suggested testers

1. [Tripouille/printfTester](https://github.com/Tripouille/printfTester)
2. [PLEB42/printf-god](https://github.com/PLEB42/printf-god)

Use external testers after writing your own tests. First try to predict the expected output and return value yourself.

### Testing checklist

#### Basic conversions

- [ ] `%c`
- [ ] `%s`
- [ ] `%p`
- [ ] `%d`
- [ ] `%i`
- [ ] `%u`
- [ ] `%x`
- [ ] `%X`
- [ ] `%%`

#### String edge cases

- [ ] Empty string
- [ ] Null string pointer
- [ ] Single-character string
- [ ] Long string
- [ ] Multiple string conversions

#### Integer edge cases

- [ ] `0`
- [ ] `-1`
- [ ] `INT_MAX`
- [ ] `INT_MIN`
- [ ] `UINT_MAX`
- [ ] Large hexadecimal values

#### Format-string cases

- [ ] Empty format string
- [ ] Format string without conversions
- [ ] Consecutive `%%`
- [ ] Multiple conversions
- [ ] Trailing `%`

#### Return-value cases

- [ ] Only normal characters
- [ ] Only conversions
- [ ] Mixed normal characters and conversions
- [ ] Empty output
- [ ] Compare the return value with the original `printf`

> **Note:** Some edge cases, such as a trailing `%` or a null string pointer, can depend on the specification or implementation. Check the actual project subject and the reference behavior required by your evaluation. Do not assume every invalid input has portable, defined behavior.

---

## 8. Recommended Research Order

Follow this order to build the concepts progressively.

| Step | Topic | Priority |
|:--:|:--|:--|
| 01 | Variadic functions | Required |
| 02 | `va_list` and its macros | Required |
| 03 | `printf` format syntax | Required |
| 04 | String parsing | Required |
| 05 | `write()` and file descriptors | Required |
| 06 | Decimal and hexadecimal conversion | Required |
| 07 | Signed and unsigned integers | Required |
| 08 | Pointer representation | Required |
| 09 | Function pointers | Optional |
| 10 | Finite state machines | Useful |
| 11 | Memory and error handling | Important |
| 12 | Testing and edge cases | Required |

---

## 9. Architecture — Think Before Implementation

A useful conceptual model for `ft_printf` is:

1. **Main function:** coordinates the process and manages the variadic argument list.
2. **Parser:** traverses the format string.
3. **Normal-character handling:** writes characters that are not conversion specifiers.
4. **Conversion handling:** identifies the requested conversion.
5. **Conversion handlers:** print characters, strings, integers, pointers, or hexadecimal values.
6. **Counter:** tracks the number of characters successfully written.
7. **Return:** returns the count according to the required `printf` behavior.

Do not feel forced to use a particular architecture or a large number of files. Choose a design you can explain and test.

---

## 10. Five Core References to Keep

### Linux man pages

- [printf(3)](https://man7.org/linux/man-pages/man3/printf.3.html)
- [stdarg(3)](https://man7.org/linux/man-pages/man3/stdarg.3.html)
- [write(2)](https://man7.org/linux/man-pages/man2/write.2.html)

### cppreference

- [Variadic arguments](https://en.cppreference.com/w/c/variadic)
- [Formatted output](https://en.cppreference.com/w/c/io/fprintf)
- [Integer types](https://en.cppreference.com/w/c/types/integer)

### GitHub testers

- [printfTester](https://github.com/Tripouille/printfTester)
- [printf-god](https://github.com/PLEB42/printf-god)

---

## Final Learning Goal

Before implementing `ft_printf`, try to answer this question:

**How does a C variadic function retrieve arguments of different types from a function call?**

Do not stop at memorizing `va_start` and `va_arg`. Understand the C rules, the default argument promotions, and the role of the platform ABI at a conceptual level.

The goal is to understand the path from:

`format string` → `conversion specifier` → `argument retrieval` → `value formatting` → `write()` → `return value`

Build your implementation step by step, test each behavior, and make sure you can explain why each part exists.
"""
