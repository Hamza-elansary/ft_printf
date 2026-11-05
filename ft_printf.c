/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 22:58:50 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 16:18:05 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "ft_printf.h"

static int	ft_check_format(const char *str, va_list args, int *len)
{
	int	sine;

	sine = ft_parsing(str[1], args);
	if (sine == -1)
		return (-1);
	*len += sine;
	return (2);
}

int	ft_printf(const char *str, ...)
{
	va_list	args;
	int		len;
	int		i;
	int		ret;

	len = 0;
	i = 0;
	va_start(args, str);
	while (str[i])
	{
		if (str[i] == '%')
			ret = ft_check_format(&str[i], args, &len);
		else
		{
			ret = ft_putchar(str[i]);
			if (ret != -1)
				len += ret;
		}
		if (ret == -1)
			return (va_end(args), -1);
		i += ret;
	}
	va_end(args);
	return (len);
}
