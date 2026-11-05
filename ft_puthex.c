/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_puthex.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:31:17 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 16:22:10 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "ft_printf.h"

int	ft_puthex(unsigned long nb)
{
	unsigned long long	save_hex;
	char				*hex;
	long				count;
	int					index;
	int					len;

	len = 0;
	hex = "0123456789abcdef";
	count = ft_len_hex(nb);
	while (count > 0)
	{
		save_hex = nb;
		index = (save_hex / count) % 16;
		if (ft_putchar(hex[index]) == -1)
			return (-1);
		len++;
		count /= 16;
	}
	return (len);
}
