/*************************************************************************

 hv4d: main program

 ---------------------------------------------------------------------

                        Copyright (c) 2011, 2017
                Andreia P. Guerreiro <apg@dei.uc.pt>
             
                       Copyright (c) 2010
                  Carlos M. Fonseca <cmfonsec@dei.uc.pt>
             Manuel Lopez-Ibanez <manuel.lopez-ibanez@ulb.ac.be>
                    Luis Paquete <paquete@dei.uc.pt>
                Andreia P. Guerreiro <apg@dei.uc.pt>

 This program is free software (software libre); you can redistribute
 it and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, you can obtain a copy of the GNU
 General Public License at:
                 http://www.gnu.org/copyleft/gpl.html
 or by writing to:
           Free Software Foundation, Inc., 59 Temple Place,
                 Suite 330, Boston, MA 02111-1307 USA


*************************************************************************/
#include "io.h"
#include "hv4d.h"
#include "timer.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h> // for isspace()

#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()

#ifdef __USE_GNU
extern char *program_invocation_short_name;
#else
char *program_invocation_short_name;
#endif

static const char * const stdin_name = "<stdin>";
static int verbose_flag = 1;
static bool union_flag = false;
static char *suffix = NULL;

static void usage(void)
{
    printf("\n"
           "Usage: %s [OPTIONS] [FILE...]\n\n", program_invocation_short_name);

    printf(
"Calculate the hypervolume of each input set of each FILE. \n"
"With no FILE, or when FILE is -, read standard input.\n\n"

"Options:\n"
" -h, --help          print this summary and exit.                          \n"
"     --version       print version number and exit.                        \n"
" -v, --verbose       print some information (time, coordinate-wise maximum \n"
"                     and minimum, etc)                                     \n"
" -q, --quiet         print only the hypervolume (as opposed to --verbose). \n"
" -u, --union         treat all input sets within a FILE as a single set.   \n"
" -r, --reference=POINT use POINT as the reference point. POINT must be within\n"
"                     quotes, e.g., \"10 10 10\". If no reference point is  \n"
"                     given, it is taken as the coordinate-wise maximum of  \n"
"                     all input points.                                     \n"
/*
"                     given, it is taken as max + 0.1 * (max - min) for each\n"
"                     coordinate from the union of all input points.        \n"
*/
" -s, --suffix=STRING Create an output file for each input file by appending\n"
"                     this suffix. This is ignored when reading from stdin. \n"
"                     If missing, output is sent to stdout.                 \n"
"\n");

}

static void version(void)
{
    printf("%s version " VERSION
#ifdef ARCH
           " (optimised for " ARCH ")"
#endif
           "\n\n", program_invocation_short_name);

    printf(
"Copyright (c) 2011, 2017"
"\nAndreia P. Guerreiro <apg@dei.uc.pt>\n"
"\nCopyright (c) 2010"
"\nCarlos M. Fonseca <cmfonsec@dei.uc.pt>"
"\nManuel Lopez-Ibanez <manuel.lopez-ibanez@ulb.ac.be>"
"\nLuis Paquete <paquete@dei.uc.pt>"
"\nAndreia P. Guerreiro <apg@dei.uc.pt>\n"
"\n"
"This is free software, and you are welcome to redistribute it under certain\n"
"conditions.  See the GNU General Public License for details. There is NO   \n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"\n"        );
}

static inline void
vector_printf (const double *vector, int size)
{
    int k;
    for (k = 0; k < size; k++)
        printf (" %f", vector[k]);
}

static double *
read_reference(char * str, int *nobj)
{
    double * reference;
    char * endp;
    char * cursor;

    int k = 0, size = 10;

    reference = malloc(size * sizeof(double));
    endp = str;

    do {
        cursor = endp;
        if (k == size) {
            size += 10;
            reference = realloc(reference, size * sizeof(double));
        }
        reference[k] = strtod(cursor, &endp);
        k++;
    } while (cursor != endp);

    // not end of string: error
    while (*cursor != '\0') {
        if (!isspace(*cursor)) return NULL;
        cursor++;
    }

    // no number: error
    if (k == 1) return NULL;

    *nobj = k-1;
    return reference;
}

static inline void
handle_read_data_error (int err, const char *filename)
{
    switch (err) {
    case 0: /* No error */
        break;

    case READ_INPUT_FILE_EMPTY:
        errprintf ("%s: no input data.", filename);
        exit (EXIT_FAILURE);

    case READ_INPUT_WRONG_INITIAL_DIM:
        errprintf ("check the argument of -r, --reference.\n");
    default:
        exit (EXIT_FAILURE);
    }
}

static void
data_range (double **maximum, double **minimum,
            const double *data, int nobj, int rows)
{
    int n, k = 0;
    int r = 0;

    if (*maximum == NULL) {
        *maximum = malloc (nobj*sizeof(double));
        for (k = 0; k < nobj; k++)
            (*maximum)[k] = data[k];
        r = 1;
    }

    if (*minimum == NULL) {
        *minimum = malloc (nobj*sizeof(double));
        for (k = 0; k < nobj; k++)
            (*minimum)[k] = data[k];
        r = 1;
    }

    for (; r < rows; r++) {
        for (n = 0; n < nobj; n++, k++) {
            if ((*maximum)[n] < data[k])
                (*maximum)[n] = data[k];
            if ((*minimum)[n] > data[k])
                (*minimum)[n] = data[k];
        }
    }
}

static void
file_range (const char *filename, double **maximum_p, double **minimum_p,
            int *dim_p)
{
    double *data = NULL;
    int *cumsizes = NULL;
    int nruns = 0;
    int dim = *dim_p;
    double *maximum = *maximum_p;
    double *minimum = *minimum_p;

    handle_read_data_error (
        read_data (filename, &data, &dim, &cumsizes, &nruns), filename);

    data_range (&maximum, &minimum, data, dim, cumsizes[nruns-1]);

    *dim_p = dim;
    *maximum_p = maximum;
    *minimum_p = minimum;

    free (data);
    free (cumsizes);
}

/*
   FILENAME: input filename. If NULL, read stdin.

   REFERENCE: reference point. If NULL, use MAXIMUM.

   MAXIMUM: maximum objective vector. If NULL, caculate it from the
   input file.

   NOBJ_P: pointer to number of objectives. If NULL, calculate it from
   input file.

*/

static void
hv_file (const char *filename, double *reference,
         double *maximum, double *minimum, int *nobj_p)
{
    double *data = NULL;
    int *cumsizes = NULL;
    int cumsize;
    int nruns = 0;
    int n;
    int nobj = *nobj_p;
    char *outfilename = NULL;
    FILE *outfile = stdout;
    bool setmax = false;
    bool setref = false;

    int err = read_data (filename, &data, &nobj, &cumsizes, &nruns);
    if (!filename) filename = stdin_name;
    handle_read_data_error (err, filename);

    if (filename != stdin_name && suffix) {
        int outfilename_len = strlen(filename) + strlen(suffix) + 1;

        outfilename = malloc (sizeof(char) * outfilename_len);
        strcpy (outfilename, filename);
        strcat (outfilename, suffix);

        outfile = fopen (outfilename, "w");
        if (outfile == NULL) {
            errprintf ("%s: %s\n", outfilename, strerror(errno));
            exit (EXIT_FAILURE);
        }
    }

    if (union_flag) {
        cumsizes[0] = cumsizes[nruns - 1];
        nruns = 1;
    }

    if (verbose_flag >= 2)
        printf("# file: %s\n", filename);

    if (maximum == NULL) {
        setmax = true;
        data_range (&maximum, &minimum, data, nobj, cumsizes[nruns-1]);
        if (verbose_flag >= 2) {
            printf ("# maximum:");
            vector_printf (maximum, nobj);
            printf ("\n");
            printf ("# minimum:");
            vector_printf (minimum, nobj);
            printf ("\n");
        }
    }

    if (reference != NULL) {
        for (n = 0; n < nobj; n++) {
            if (reference[n] <= maximum[n]) {
                warnprintf ("%s: some points do not strictly dominate "
                            "the reference point and they will be discarded",
                            filename);
                break;
            }
        }
    } else {
        setref = true;
        reference = malloc(nobj * sizeof(double));
//         double v;
//         char str[20];
        for (n = 0; n < nobj; n++) {
            /* default reference point is: *//*
            sprintf(str, "%f", maximum[n]);
//             v = (double)(index(str, '.')-&str[0]);
            v= strlen(str);*/
//             printf("v: %d\n", v);
//             v -= 16;
            reference[n] = maximum[n] + maximum[n] * pow(10.0,-15);
//             printf("maximum = %16.15g, m = %16.15g\n", maximum[n], reference[n]);
            /* however, a better reference point is:
            reference[n] = maximum[n] + 0.1 * (maximum[n] - minimum[n]);
            so that extreme points have some influence. */
        }
    }

    if (verbose_flag >= 2) {
        printf ("# reference:");
        vector_printf (reference, nobj);
        printf ("\n");
    }

    for (n = 0, cumsize = 0; n < nruns; cumsize = cumsizes[n], n++) {
        double time_elapsed_cpu;
        double volume;

        if (verbose_flag >= 2)
            fprintf (outfile, "# Data set %d:\n", n + 1);


        Timer_start ();
        volume = hv4d (&data[nobj * cumsize],
                            cumsizes[n] - cumsize, reference);
        if (volume <= 0.0) {
            warnprintf ("%s: none of the points strongly dominates the reference point",
                            filename);
//             errprintf ("none of the points strictly dominates the reference point\n");
//             exit (EXIT_FAILURE);
        }

        time_elapsed_cpu = Timer_elapsed_virtual ();

        fprintf (outfile, "%-16.15g\n", volume);



        if (verbose_flag >= 2) {
            fprintf (outfile, "# Time computing hypervolume (cpu): %f seconds\n", time_elapsed_cpu);
        }

    }

    if (outfilename) {
        if (verbose_flag)
            fprintf (stderr, "# %s -> %s\n", filename, outfilename);
        fclose (outfile);
        free (outfilename);
    }
    free (data);
    free (cumsizes);
    if (setmax){
        free (maximum);
        free (minimum);
    }
    if (setref) free(reference);
    *nobj_p = nobj;
}

int main(int argc, char *argv[])
{
    double *reference = NULL;
    int nobj = 0;
    int numfiles, k;

    int opt; /* it's actually going to hold a char.  */
    int longopt_index;

    /* see the man page for getopt_long for an explanation of these fields.  */
    static struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"reference",  required_argument, NULL, 'r'},
        {"union",      no_argument,       NULL, 'u'},
        {"suffix",     required_argument, NULL, 's'},
#ifdef EXPERIMENTAL
        {"reorder",    no_argument,       NULL, 'R'},
#endif

        {NULL, 0, NULL, 0} /* marks end of list */
    };

#ifndef __USE_GNU
    program_invocation_short_name = argv[0];
#endif

    while (0 < (opt = getopt_long (argc, argv, "hVvquRr:s:",
                                   long_options, &longopt_index))) {
        switch (opt) {
        case 'r': // --reference
            reference = read_reference (optarg, &nobj);
            if (reference == NULL) {
                errprintf ("invalid reference point '%s",
                           optarg);
                exit (EXIT_FAILURE);
            }
            break;

        case 'u': // --union
            union_flag = true;
            break;

        case 's': // --suffix
            suffix = optarg;
            break;

        case 'V': // --version
            version();
            exit(EXIT_SUCCESS);

        case 'q': // --quiet
            verbose_flag = 0;
            break;

        case 'v': // --verbose
            verbose_flag = 2;
            break;

        case '?':
            // getopt prints an error message right here
            fprintf (stderr, "Try `%s --help' for more information.\n",
                     program_invocation_short_name);
            exit(EXIT_FAILURE);
        case 'h':
            usage();
            exit(EXIT_SUCCESS);

        default: // should never happen
            abort();
        }
    }

    numfiles = argc - optind;

    if (numfiles < 1) /* Read stdin.  */
        hv_file (NULL, reference, NULL, NULL, &nobj);

    else if (numfiles == 1) {
        hv_file (argv[optind], reference, NULL, NULL, &nobj);
    }
    else {
        double *maximum = NULL;
        double *minimum = NULL;
        if (reference == NULL) {
            /* Calculate the maximum among all input files to use as
               reference point.  */
            for (k = 0; k < numfiles; k++)
                file_range (argv[optind + k], &maximum, &minimum, &nobj);

            if (verbose_flag >= 2) {
                printf ("# maximum:");
                vector_printf (maximum, nobj);
                printf ("\n");
                printf ("# minimum:");
                vector_printf (minimum, nobj);
                printf ("\n");
            }
        }
        for (k = 0; k < numfiles; k++)
            hv_file (argv[optind + k], reference, maximum, minimum, &nobj);
        
        free (maximum);
        free (minimum);
    }
    if (reference != NULL) free (reference);
    return EXIT_SUCCESS;
}
