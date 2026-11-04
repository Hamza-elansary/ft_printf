/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_len_hex.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:15:30 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/03 20:14:01 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "libftprintf.h"

unsigned long long	ft_len_hex(unsigned long long hex)
{
	unsigned long long	count;
	unsigned long long	save_hex;

	count = 1;
	save_hex = hex;
	while (save_hex >= 16)
	{
		save_hex /= 16;
		count *= 16;
	}
	return (count);
}
