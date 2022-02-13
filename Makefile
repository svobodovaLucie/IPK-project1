# /* ******************* Makefile ******************* *
#  *        Počítačové komunikace a sítě (IPK)        *
#  *            Lucie Svobodová, xsvobo1x             *
#  *                 FIT VUT v Brně                   *
#  *                   2021/2022                      *
#  * ************************************************ */

CC = gcc
CFLAGS = -std=gnu99 -pedantic -Wall -Wextra -Werror

hinfosvc: hinfosvc.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o hinfosvc
