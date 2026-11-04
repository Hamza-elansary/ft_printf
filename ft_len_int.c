/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_len_int.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 20:40:16 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 08:31:21 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "libftprintf.h"

unsigned int	ft_len_int(unsigned int nbr)
{
	unsigned int	count;
	unsigned int	save_nbr;

	count = 1;
	save_nbr = nbr;
	while (save_nbr >= 10)
	{
		save_nbr /= 10;
		count *= 10;
	}
	return (count);
}
