/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_putunsigned.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 09:33:51 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 09:52:42 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "libftprintf.h"
int	ft_putunsigned(unsigned int nbr)
{
	unsigned int	count;
	int		sing;

	sing = 0;
	count = ft_len_int(nbr);
	while (count > 0)
	{
		if (ft_putchar(nbr / count + 48) == -1)
			return (-1);
		nbr %= count;
		count /= 10;
		sing++;
	}
	return (sing);
}
