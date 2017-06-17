/*
 * idevicelocation - Set the location on iOS devices.
 *
 * Copyright (c) 2017 Jon Gabilondo, All Rights Reserved.
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

char *udid = NULL;

static void print_usage(int argc, char **argv)
{
	char *name = NULL;

	name = strrchr(argv[0], '/');
    printf("Set Geo Location in iOS devices.\n\n");
	printf("Usage: %s [OPTIONS] LATITUDE LONGITUDE\n\n", (name ? name + 1 : argv[0]));
    printf(" The following OPTIONS are accepted:\n");
	printf
		("  -u, --udid UDID\tTarget specific device by its 40-digit device UDID.\n"
         "  -h, --help\t\tprints usage information\n"
		 "  -d, --debug\t\tenable communication debugging\n" "\n");
	printf("Homepage: <https://github.com/JonGabilondoAngulo>\n");
}

static void parse_opts(int argc, char **argv)
{
	static struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"udid", required_argument, NULL, 'u'},
        {"debug", no_argument, NULL, 'd'},
		{NULL, 0, NULL, 0}
	};
	int c;

	while (1) {
        c = getopt_long(argc, argv, "hdu:", longopts, (int *) 0);
		if (c == -1) {
			break;
		}
        switch (c) {
            case 'h':
                print_usage(argc, argv);
                exit(EXIT_SUCCESS);
            case 'u':
                if (strlen(optarg) != 40) {
                    printf("%s: invalid UDID specified (length != 40)\n", argv[0]);
                    print_usage(argc, argv);
                    exit(2);
                }
                udid = strdup(optarg);
                break;
            case 'd':
                idevice_set_debug_level(1);
                break;
            default:
                print_usage(argc, argv);
                exit(2);
        }
	}
    
    if ((argc - optind) < 2) {
        printf("ERROR: You need to specify LATITUDE and LONGITUDE\n\n");
        print_usage(argc, argv);
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv)
{
	idevice_t phone = NULL;
    lockdownd_client_t client = NULL;
    service_client_t service_client = NULL;
    lockdownd_service_descriptor_t service = NULL;
    char *lat = NULL;
    char *lng = NULL;
	int res = 0;
    
	parse_opts(argc, argv);
    
    lat = (argv+optind)[0];
    lng = (argv+optind+1)[0];
    
	if (IDEVICE_E_SUCCESS != idevice_new(&phone, udid)) {
		fprintf(stderr, "No iOS device found, is it plugged in?\n");
		return EXIT_FAILURE;
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
    }
    
    service_error_t se = service_client_new(phone, service, &service_client);
    if (se) {
        printf("Could not crate Service Client.\n");
    } else {
        
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
            printf("Could not send data to Service Client.\n");
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
	return res;
}
