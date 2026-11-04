/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libftprintf.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 10:46:28 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 10:47:16 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#ifndef LIBFTPRINTF_H
# define LIBFTPRINTF_H

# include <unistd.h>
# include <stdlib.h>
# include <stdarg.h>

int					ft_printf(const char *str, ...);
int					ft_parsing(int format, va_list args);

int					ft_putchar(int c);
int					ft_putstr(char *str);
int					ft_putnbr(long nb);
int					ft_putunsigned(unsigned int nbr);
int					ft_puthex(unsigned long nb);
int					ft_puthexx(unsigned long nb);
int					ft_putpoint(void *nb);

unsigned int		ft_len_int(unsigned int nbr);
unsigned long long	ft_len_hex(unsigned long long hex);

#endif
