/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_parsing.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 23:43:20 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 16:17:40 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "ft_printf.h"

int	ft_parsing(int format, va_list args)
{
	int	sing;

	sing = 0;
	if (format == 'c')
		sing = ft_putchar(va_arg(args, int));
	else if (format == 's')
		sing = ft_putstr(va_arg(args, char *));
	else if (format == 'd' || format == 'i')
		sing = ft_putnbr(va_arg(args, int));
	else if (format == 'u')
		sing = ft_putunsigned(va_arg(args, unsigned int));
	else if (format == 'p')
		sing = ft_putpoint(va_arg(args, void *));
	else if (format == 'x')
		sing = ft_puthex(va_arg(args, unsigned int));
	else if (format == 'X')
		sing = ft_puthexx(va_arg(args, unsigned int));
	else if (format == '%')
		sing = ft_putchar('%');
	return (sing);
}
