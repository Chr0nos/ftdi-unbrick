/*
** FT232H Unbrick by Sebastien Nicolet
** This is a Linux command line tool to fix bricked ft232h chips, by reseting
** the EEPROM to factory settings.
** this will only works if the issue is that you miss-configured the eeprom or
** overridden the ProductId and/or VendorId.
** This comme without warranty of any form: be sure of what you do.
** I decline any responsability of any kind for any damage on your device.
** This is under MIT license.
**
** In my case i got a kernel error saying the usb bus does not have suffisant
** power, so i overrided it with:
** echo "1" > /sys/bus/usb/devices/3-11/bConfigurationValue
** to allow more power (it took 510ma instead of 500 of the limit)
** then i ran the tool and unplugged/plugged the device.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ftdi.h>
#include <unistd.h>
#include <string.h>

static void     show_eeprom(struct ftdi_context *ftdi, const size_t eeprom_size);

struct s_device_location {
    int     vendor_id;
    int     product_id;
};

static void     catch_error(struct ftdi_context *ftdi,
    const char *step, int error)
{
    if (!error)
        return ;
    dprintf(STDERR_FILENO, "error: %s: %s\n",
        step, ftdi_get_error_string(ftdi));
    ftdi_usb_close(ftdi);
    ftdi_free(ftdi);
    exit(EXIT_FAILURE);
}

static void dumpmem(const void *addr, const unsigned int len)
{
    char            hex[3 * 16 + 1];
    char            ascii[17];
    char            c;
	unsigned int    i;
    unsigned int    offset = 0;
    unsigned int    i16;

	for (i = 0; i < len;) {
		i16 = i % 16;
        c = ((char*)(size_t)addr)[i];
		sprintf(hex + (3 * i16), " %02x", c);
		ascii[i16] = (c < ' ' || c > '~') ? '.' : c;
		if (++i == len || i16 == 15) {
			ascii[i16 + 1] = '\0';
			for (; i16 != 15; ++i16)
				strcat(hex, "   ");
			printf("%04x:%s  %s\n", offset, hex, ascii);
			offset = i;
		}
	}
}

static void     reset_eeprom(struct ftdi_context *ftdi)
{
    printf("%s", "before:\n");
    show_eeprom(ftdi, 0xff);
    puts("resting eeprom...");
    catch_error(ftdi, "erase eeprom", ftdi_erase_eeprom(ftdi));
    printf("%s", "after:\n");
    show_eeprom(ftdi, 0xff);
}

static void     show_eeprom(struct ftdi_context *ftdi, const size_t eeprom_size)
{
    unsigned char       *eeprom;
    int                 eeprom_fd;

    eeprom = malloc(eeprom_size);
    if (!eeprom)
    {
        dprintf(STDERR_FILENO, "%s", "error: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
        eeprom_fd = ftdi_read_eeprom(ftdi);
    catch_error(ftdi, "eeprom read", eeprom_fd);
    catch_error(ftdi, "get buffer",
        ftdi_get_eeprom_buf(ftdi, eeprom, (int)eeprom_size));
    dumpmem(eeprom, (unsigned int)eeprom_size);
    free(eeprom);
}


static void    show_chip_id(struct ftdi_context *ftdi)
{
    unsigned int chipid;

    printf("ftdi_read_chipid: %d\n", ftdi_read_chipid(ftdi, &chipid));
    printf("FTDI chipid: %X\n", chipid);
}

static int      ft_usb_context(struct ftdi_context *ftdi,
    const int vendor, const int product)
{
    printf("looking for usb device: %04x:%04x\n", vendor, product);
    if (ftdi_usb_open(ftdi, vendor, product) < 0)
    {
        dprintf(STDERR_FILENO, "unable to open ftdi device: %s\n",
            ftdi_get_error_string(ftdi));
        return (EXIT_FAILURE);
    }
    show_chip_id(ftdi);
    reset_eeprom(ftdi);
    ftdi_usb_close(ftdi);
    return (EXIT_SUCCESS);
}

int main(int ac, char **av)
{
    struct s_device_location    current_usb_dev;
    struct ftdi_context         *ftdi;
    struct ftdi_version_info    version;

    if ((ac < 2) || (sscanf(av[1], "%x:%x",
        &current_usb_dev.vendor_id, &current_usb_dev.product_id)) < 2) {
        puts("usage: [vendor_id:product_id]");
        return (EXIT_FAILURE);
    }
    if ((ftdi = ftdi_new()) == 0)
   {
        fprintf(stderr, "ftdi_new failed\n");
        return (EXIT_FAILURE);
    }
    version = ftdi_get_library_version();
    printf("Initialized libftdi %s "
        "(major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
        version.version_str, version.major, version.minor, version.micro,
        version.snapshot_str);
    ft_usb_context(ftdi, current_usb_dev.vendor_id, current_usb_dev.product_id);
    ftdi_free(ftdi);
    return (EXIT_SUCCESS);
}
