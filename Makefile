# /* ******************* Makefile ******************* *
#  *    Computer Communications and Networks (IPK)    *
#  *            Lucie Svobodova, xsvobo1x             *
#  *                 		FIT BUT                   		*
#  *                   2021/2022                      *
#  * ************************************************ */

CC = gcc
CFLAGS = -std=gnu99 -pedantic -Wall -Wextra -Werror

hinfosvc: hinfosvc.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o hinfosvc
