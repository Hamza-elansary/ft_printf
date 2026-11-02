/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_parsing.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hael-ans <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 17:01:36 by hael-ans          #+#    #+#             */
/*   Updated: 2026/11/01 20:55:01 by hael-ans         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "libdeflate.h"

int	ft_parsing(const char *str)
{
	size_t	index;

	index = 0;
	while (str[index])
	{
		if (str[index] == '%' && str[index+1] == 'c')
			ft_putchar(65);
		else if (str[index] == '%' && str[index+1] == 's')
			return (2);	
		else if (str[index] == '%' && str[index+1] == 'p')
			return (3);
		else if (str[index] == '%' && str[index+1] == 'd')
			return (4);
		else if (str[index] == '%' && str[index+1] == 'i')
			return (5);
		else if (str[index] == '%' && str[index+1] == 'u')
			return (6);
		else if (str[index] == '%' && str[index+1] == 'X')
			return (7);
		else if (str[index] == '%' && str[index+1] == 'x')
			return (8);
		else if (str[index] == '%' && str[index+1] == '%')
			return (9);ls
				
		else
			write(1, &str[index], 1);
		index++;
		}
	return 0;
}

int	main()
{
	printf("%d",ft_parsing("ABC %d DEF %s"));
}
