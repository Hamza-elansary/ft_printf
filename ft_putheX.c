/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_putheX.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:20:25 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 16:18:50 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "ft_printf.h"

int	ft_puthexx(unsigned long nb)
{
	char		*hex;
	long		count;
	long		save_hex;

	int (index), (len), (sine);
	len = 0;
	hex = "0123456789ABCDEF";
	count = ft_len_hex(nb);
	while (count > 0)
	{
		save_hex = nb;
		index = (save_hex / count) % 16;
		sine = ft_putchar(hex[index]);
		if (sine == -1)
			return (-1);
		len += sine;
		count /= 16;
	}
	return (len);
}
