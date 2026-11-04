/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_putnbr.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 20:44:04 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 08:34:38 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "libftprintf.h"

int	ft_putnbr(long  nb)
{
	long	count;
	int		len;
	long		save_nb;

	len = 0;
	if (nb < 0)
	{
		if (ft_putchar('-') == -1)
			return (-1);
		nb = nb * -1;
		len++;
	}
	save_nb = nb;
	count = ft_len_int(save_nb);
	while (count > 0)
	{
		if (ft_putchar(save_nb / count + 48) == -1)
			return (-1);
		len++;
		save_nb = save_nb % count;
		count /= 10;
	}
	return (len);
}
