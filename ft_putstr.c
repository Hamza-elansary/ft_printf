/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_putstr.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:36:17 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 16:20:38 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "ft_printf.h"

int	ft_putstr(char *str)
{
	size_t		i;
	int			sing;

	i = 0;
	if (!str)
	{
		str = "(null)";
	}
	while (str[i])
	{
		sing = ft_putchar(str[i]);
		if (sing == -1)
			return (-1);
		i++;
	}
	return (i);
}
