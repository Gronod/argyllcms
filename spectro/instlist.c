/* 
 * Argyll Color Management System
 * USB & Serial Instrument Enumeration Utility.
 * Outputs connected instruments in structured JSON format.
 *
 * Author: Graeme W. Gill / Gordon
 *
 * Copyright 2026 Graeme W. Gill / Gordon
 * All rights reserved.
 *
 * This material is licenced under the GNU AFFERO GENERAL PUBLIC LICENSE Version 3 :-
 * see the License.txt file for licencing details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "copyright.h"
#include "aconfig.h"
#include "numlib.h"
#include "cgats.h"
#include "conv.h"
#include "xicc.h"
#include "insttypes.h"
#include "icoms.h"
#include "inst.h"

static void usage(char *diag, ...) {
	fprintf(stderr,"Enumerate Instrument Ports, Version %s\n",ARGYLL_VERSION_STR);
	if (diag != NULL) {
		va_list args;
		fprintf(stderr,"  Diagnostic: ");
		va_start(args, diag);
		vfprintf(stderr, diag, args);
		va_end(args);
		fprintf(stderr,"\n");
	}
	fprintf(stderr,"Author: Graeme W. Gill / Gordon, licensed under the AGPL Version 3\n");
	fprintf(stderr,"usage: instlist [-options]\n");
	fprintf(stderr," -h, -?, --help  Show this help information\n");
	fprintf(stderr," -v              Verbose diagnostics to stderr\n");
	fprintf(stderr," -D [level]      Print debug diagnostics to stderr\n");
	exit(1);
}

#include "ui.h"

int main(int argc, char *argv[]) {
	int fa, nfa;
	int verb = 0;
	int debug = 0;
	icompaths *icmps = NULL;
	icompath **paths = NULL;
	int i;
	int count = 0;

	/* Set executable path for logger */
	set_exe_path(argv[0]);

	/* Process command-line options */
	for (fa = 1; fa < argc; fa++) {
		nfa = fa + 1;
		if (argv[fa][0] == '-') {
			char *na = NULL;
			if (nfa < argc && argv[nfa][0] != '-')
				na = argv[nfa];

			if (strcmp(argv[fa], "-h") == 0 || strcmp(argv[fa], "-?") == 0 ||
			    strcmp(argv[fa], "--help") == 0 || strcmp(argv[fa], "-help") == 0) {
				usage(NULL);
			} else if (argv[fa][1] == 'v') {
				verb = 1;
				g_log->verb = verb;
			} else if (argv[fa][1] == 'D') {
				debug = 1;
				if (na != NULL && na[0] >= '0' && na[0] <= '9') {
					debug = atoi(na);
					fa = nfa;
				}
				g_log->debug = debug;
			} else {
				usage("Unknown option '%s'", argv[fa]);
			}
		} else {
			usage("Unexpected argument '%s'", argv[fa]);
		}
	}

	if ((icmps = new_icompaths(g_log)) == NULL) {
		printf("{\n  \"event\": \"instruments\",\n  \"devices\": []\n}\n");
		fflush(stdout);
		return 1;
	}

	paths = icmps->paths;

	printf("{\n  \"event\": \"instruments\",\n  \"devices\": [\n");

	if (paths != NULL) {
		for (i = 0; paths[i] != NULL; i++) {
			char *tname = inst_name(paths[i]->dtype);
			char *name = paths[i]->name ? paths[i]->name : "Unknown";
			
			if (count > 0) {
				printf(",\n");
			}
			
			printf("    {\n");
			printf("      \"port\": %d,\n", i + 1);
			printf("      \"name\": \"");
			/* Escape quotes and backslashes in name */
			{
				char *p;
				for (p = name; *p != '\0'; p++) {
					if (*p == '"' || *p == '\\') {
						putchar('\\');
					}
					putchar(*p);
				}
			}
			printf("\",\n");
			printf("      \"type\": \"%s\"\n", (tname != NULL && tname[0] != '\0') ? tname : "Unknown");
			printf("    }");
			count++;
		}
	}

	printf("\n  ]\n}\n");
	fflush(stdout);

	if (icmps != NULL)
		icmps->del(icmps);

	return 0;
}
