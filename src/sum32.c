/* Copyright(c) 1986 Association of Universities for Research in Astronomy
 * Inc.
 */

/* SUM32 -- accumulate the 32 bit and 16 bit 1's complement checksums
 * for a file.  Reports the checksums and their complements as well as
 * the size of the file.
 *
 * Usage:
 *
 *	sum32 [-v] [-c] [-p] [-i <ascii>] [<file>|<number>]
 *
 * Command line flags:
 *	-v                verbose mode
 *	-c                report only the ascii coded complement
 *	-p                permute the bytes for FITS keyword alignment
 *	-i                invert the transformation given an ascii checksum
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

#define	SZ_PATHNAME	161
#define	RECORD		2880
#define	BLOCK		10

#define	ERR		0
#define	OK		1

static int checkfile (FILE *fp, unsigned short *sum16, unsigned int *sum32);
static void print_usage (void);

void checksum (unsigned char *buf, int length,
               unsigned short *sum16, unsigned int *sum32);

void char_encode (unsigned int value, char *ascii,
                  int nbytes, int permute);


int
main (int argc, char *argv[])
{
	unsigned short	sum16;
	unsigned int	sum32, tmp16;
	register DIR	*dir;
	int	size, len, iarg, i;
	int	verbose=0, code=0, inverse=0, got_name=0, num_mode=0, permute=0;
	char    name[SZ_PATHNAME], ascii[SZ_PATHNAME];
	FILE	*fp;


  	if (argc <= 1) {
  	    print_usage ();
  	    exit (-1);
  	}

	for (iarg=1; iarg < argc; iarg++) {
	    len = strlen (argv[iarg]);

	    if (argv[iarg][0] == '-' && len == 2) {
		if (argv[iarg][1] == 'v') {
		    verbose++;
		    continue;

		} else if (argv[iarg][1] == 'c') {
		    code++;
		    continue;

		} else if (argv[iarg][1] == 'p') {
		    permute++;
		    continue;

		} else if (argv[iarg][1] == 'i') {
		    if (++iarg >= argc) {
			print_usage ();
			exit (-1);
		    } else {
			strncpy (ascii, argv[iarg], SZ_PATHNAME);
			inverse++;
			continue;
		    }

		} else {
		    printf ("unknown command line flag `%s'\n", argv[iarg]);
		    print_usage ();
		    exit (-1);
		}

	    } else if (! got_name) {
		strncpy (name, argv[iarg], SZ_PATHNAME);
		got_name++;
		continue;

	    } else {
		printf ("too many arguments: `%s'\n", argv[iarg]);
		print_usage ();
		exit (-1);
	    }
	}

	if (inverse) {
	    sum16 = 0;
	    sum32 = 0;
	    size = strlen (ascii);

	    if (permute)
		for (i=0; i < size; i++)
		    name[i] = ascii[(i+1)%size] - 0x30;
	    else
		for (i=0; i < size; i++)
		    name[i] = ascii[i] - 0x30;

	    checksum ((unsigned char *)name, size, &sum16, &sum32);

	} else if (! got_name) {
	    size = checkfile (stdin, &sum16, &sum32);

	} else if ((dir = opendir (name)) != NULL) {
	    /* silently exit if a directory is presented
	     * should likely handle other special files similarly
	     */
	    closedir (dir);
	    exit (-1);

	} else if ((fp = fopen (name, "r")) != NULL) {
	    size = checkfile (fp, &sum16, &sum32);
	    fclose (fp);

	} else {
	    sum32 = atoi (name);

	    tmp16 = sum32 / 0x10000;
	    tmp16 += sum32 % 0x10000;
	    while (tmp16>>16)
		tmp16 = (tmp16 & 0xFFFF) + (tmp16>>16);
	    sum16 = tmp16;

	    num_mode++;
	}

	if (verbose) {
	    char_encode (sum16, ascii, 2, permute);
	    printf ("\nchecksum16:  %05u = %8s\n", sum16, ascii);
	    char_encode (~sum16, ascii, 2, permute);
	    printf ("complement:  %05u = %8s\n", ~sum16 & 0xFFFF, ascii);

	    char_encode (sum32, ascii, 4, permute);
	    printf ("\nchecksum32:  %010u = %16s\n", sum32, ascii);
	    char_encode (~sum32, ascii, 4, permute);
	    printf ("complement:  %010u = %16s\n", ~sum32, ascii);

	    if (! num_mode)
		printf ("\n file size:  %d bytes\n\n", size);

	} else {
	    if (code) {
		char_encode (~sum32, ascii, 4, permute);
		printf ("%16s\n", ascii);
	    } else if (~sum32) {
		printf ("%010u\n", sum32);
	    } else
		printf ("sum_zeroed\n");
	}

	exit (0);
}


static int
checkfile (
    FILE *fp,
    unsigned short *sum16,
    unsigned int *sum32
)
{
	char	record[BLOCK*RECORD];
	int	size, recsize;

	*sum16 = 0;
	*sum32 = 0;
	size = 0;

	while (! feof (fp))
	    if ((recsize = fread (record, sizeof(char), BLOCK*RECORD, fp))) {
		checksum ((unsigned char *)record, recsize, sum16, sum32);
		size += recsize;
	    }

	return (size);
}


static void
print_usage (void)
{
	printf ("usage: sum32 [-v] [-c] [-p] [-i <ascii>] [<file>|<number>]\n");
}
