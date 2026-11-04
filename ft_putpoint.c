/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_putpoint.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 20:26:54 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/04 09:23:53 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "libftprintf.h"
#include <stdlib.h>
#include <stdio.h>

int	ft_putpoint(void *nb)
{
	unsigned long long int	nbr;
	int			len;
	int			sine;

	sine = 0;
	len = 0;
	if (!nb)
		return (ft_putstr("(nil)"));
	nbr = (unsigned long long )nb;
	sine = ft_putstr("0x");
	if (sine == -1)
		return (-1);
	len = ft_puthex(nbr);
	if (len == -1)
		return (-1);
	return (len +2);
}
