/*
 * idevicelocation - Manage apps on iOS devices.
 *
 * Copyright (C) 2010-2015 Martin Szulecki <m.szulecki@libimobiledevice.org>
 * Copyright (C) 2010-2014 Nikias Bassen <nikias@gmx.li>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more profile.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#define _GNU_SOURCE 1
#define __USE_GNU 1
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/service.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/notification_proxy.h>

#include <plist/plist.h>

#define swap32(value) (((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) | ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24) )

char *lat = NULL;
char *lng = NULL;
char *udid = NULL;
char *options = NULL;

enum cmd_mode {
    CMD_NONE = 0,
    CMD_LIST_APPS,
    CMD_INSTALL,
    CMD_UNINSTALL,
    CMD_UPGRADE,
    CMD_LIST_ARCHIVES,
    CMD_ARCHIVE,
    CMD_RESTORE,
    CMD_REMOVE_ARCHIVE
};

int cmd = CMD_NONE;

char *last_status = NULL;
int wait_for_command_complete = 0;
int notification_expected = 0;
int is_device_connected = 0;
int command_completed = 0;
int err_occurred = 0;

static void print_usage(int argc, char **argv)
{
	char *name = NULL;

	name = strrchr(argv[0], '/');
	printf("Usage: %s OPTIONS\n", (name ? name + 1 : argv[0]));
	printf("Set Geo Location in iOS devices.\n\n");
	printf
		("  -u, --udid UDID\tTarget specific device by its 40-digit device UDID.\n"
         "  -h, --help\t\tprints usage information\n"
         "  -l, --lat\t\tlatitude\n"
         "  -n, --lng\t\tlongitude\n"
		 "  -d, --debug\t\tenable communication debugging\n" "\n");
	printf("Homepage: <http://libimobiledevice.org>\n");
}

static void parse_opts(int argc, char **argv)
{
	static struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"udid", required_argument, NULL, 'u'},
        {"debug", no_argument, NULL, 'd'},
        {"lat", required_argument, NULL, 'l'},
        {"lng", required_argument, NULL, 'n'},
		{NULL, 0, NULL, 0}
	};
	int c;

	while (1) {
        c = getopt_long(argc, argv, "hdu:l:n:", longopts, (int *) 0);
		if (c == -1) {
			break;
		}
        
		switch (c) {
            case 'l':
                lat = strdup(optarg);
                break;
            case 'n':
                lng = strdup(optarg);
                break;
            case 'h':
                print_usage(argc, argv);
                exit(0);
		case 'd':
			idevice_set_debug_level(1);
			break;
		default:
			print_usage(argc, argv);
			exit(2);
		}
	}

//    if (cmd == CMD_NONE) {
//        printf("ERROR: No mode/command was supplied.\n");
//    }
    
//    if (cmd == CMD_NONE || optind <= 1 || (argc - optind > 0)) {
//        print_usage(argc, argv);
//        exit(2);
//    }
}


int main(int argc, char **argv)
{
	idevice_t phone = NULL;
    lockdownd_client_t client = NULL;
    service_client_t service_client = NULL;
    lockdownd_service_descriptor_t service = NULL;
	int res = 0;
    
	parse_opts(argc, argv);

	argc -= optind;
	argv += optind;

	if (IDEVICE_E_SUCCESS != idevice_new(&phone, udid)) {
		fprintf(stderr, "No iOS device found, is it plugged in?\n");
		return -1;
	}

	if (
        LOCKDOWN_E_SUCCESS !=
        lockdownd_client_new_with_handshake(phone, &client, "idevicelocation")
        ) {
		fprintf(stderr, "Could not connect to lockdownd. Exiting.\n");
		goto leave_cleanup;
	}
    
    if ((lockdownd_start_service(client, "com.apple.dt.simulatelocation",  &service) != LOCKDOWN_E_SUCCESS) || !service) {
             fprintf(stderr,  "Could not start com.apple.dt.simulatelocation!\n");
             goto leave_cleanup;
    } else {
        printf("SUCCESS: Service opened com.apple.dt.simulatelocation.\n");
    }
    
    service_error_t se = service_client_new(phone, service, &service_client);
    if (se) {
        printf("ERROR: Could not crate Service Client.\n");
    } else {
        printf("SUCCESS: Service Client created.\n");
        
        uint32_t start = 0;
        uint32_t stop = swap32(1);
        uint32_t sent = 0;
        uint32_t lat_len = swap32(strlen(lat));
        uint32_t lng_len = swap32(strlen(lng));
        service_error_t e;
        
        e = service_send(service_client, (void*)&start, sizeof(start), &sent);
        e = service_send(service_client, (void*)&lat_len, sizeof(lat_len), &sent);
        e = service_send(service_client, lat, strlen(lat), &sent);
        e = service_send(service_client, (void*)&lng_len, sizeof(lng_len), &sent);
        e = service_send(service_client, lng, strlen(lng), &sent);
        e = service_send(service_client, (void*)&stop, sizeof(stop), &sent);
        
        if (e) {
            printf("ERROR: Could not send data to Service Client.\n");
        }
    }
    
    if (service_client) {
        service_client_free(service_client);
    }
    service_client = NULL;
    if (service) {
        lockdownd_service_descriptor_free(service);
    }
    service = NULL;
    
	setbuf(stdout, NULL);

	if (last_status) {
		free(last_status);
		last_status = NULL;
	}
	notification_expected = 0;
    
//    else {
//        printf
//            ("ERROR: no command selected?! This should not be reached!\n");
//        res = -2;
//        goto leave_cleanup;
//    }
    
	if (client) {
		/* not needed anymore */
		lockdownd_client_free(client);
		client = NULL;
	}
    
leave_cleanup:
	if (client) {
		lockdownd_client_free(client);
	}
	idevice_free(phone);

	if (udid) {
		free(udid);
	}
	if (options) {
		free(options);
	}

	if (err_occurred && !res) {
		res = 128;
	}

	return res;
}
