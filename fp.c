/* FP.C ==================================================================== */
// @desc	Application to fingerprint a system. In write mode (default),
//          reads MAC address of primary network adapter and serial number
//          of primary disk drive and writes to a fingerprint file. In read
//          mode, retrieves the MAC and HDD serial and compares against the 
//          fingerprint file.
//
//          Full functionality including HDD serial requires root access. 
//          Appropriate error checking is provided if this is not available.
//
// @author	Jon Stoecker <jws527@gmail.com>
// @date	4/10/2012
//
// Tested with GCC 4.6.1 in Ubuntu Linux 11.10 / kernel version 3.0.0-17
/* ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/hdreg.h>
#include <errno.h>
#include <fcntl.h>

#define FLAG 1
#define MAC_LENGTH 6 
#define MODE_WRITE 0
#define MODE_READ 1
#define true 0
#define false -1

/*
 *  Options: change read flag, output filename, devices, test mode etc.
 */
#define FPRNT_FLAG "-r"				// The flag that sets read mode
#define OUT_FILE "default.fprint"	// Stores fingerprint
#define DEVICE "eth0"				// Network device to retrieve MAC from 
#define DRIVE "/dev/sda"            // Hard drive to retrieve serial from
#define TEST                        // If active, ouputs test data

/*
 *	Definitions	
 */
typedef struct fingerprintStruct {
	int mac_addr[MAC_LENGTH];
	char* hdd_serial;
} fingerprintType;

typedef int modeType;
typedef int bool;

void get_mac_addr(fingerprintType*);
bool get_hdd_ser(fingerprintType*);
bool compare_file(fingerprintType*, FILE*, bool);
FILE* parameter_check(modeType*, int, char**);
void display_usage(char**);

/*
 *  Main	
 */
int main(int argc, char** argv)
{
	modeType mode;
	bool hdd_access;
    FILE* fpFile = parameter_check(&mode, argc, argv);

	fingerprintType fprnt;
	get_mac_addr(&fprnt);
    hdd_access = get_hdd_ser(&fprnt);

#ifdef TEST
	int i;
	printf("Test: Sys MAC Addr: ");
	for(i = 0; i < MAC_LENGTH; ++i) {
		printf("%.2X ", (unsigned char)fprnt.mac_addr[i]);
	}
	printf("\n");
#endif	

	// Print to file (WRITE mode) or display results (READ mode)
	if(mode == MODE_WRITE) {
		fwrite(fprnt.mac_addr, 1, sizeof(fprnt.mac_addr), fpFile);
        if (hdd_access == true) {
   		    fwrite(fprnt.hdd_serial, 1,
                   strlen(fprnt.hdd_serial),fpFile);
        }
	} else {
		bool result = compare_file(&fprnt, fpFile, hdd_access);
		switch(result) {
			case true: printf("The fingerprint matches.\n"); break;
			case false: printf("The fingerprint DOES NOT match.\n"); break;
		}
	}

	fclose(fpFile);

	return 0;
}

/*
 *	Retrieves primary MAC address from the system
 */
void get_mac_addr(fingerprintType* fprnt)
{
	int i;
	struct ifreq ifreq_bfr;
	i = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&ifreq_bfr, 0x00, sizeof(ifreq_bfr));
	strcpy(ifreq_bfr.ifr_name, DEVICE);
	ioctl(i, SIOCGIFHWADDR, &ifreq_bfr);
	for(i = 0; i < MAC_LENGTH; ++i)
	{
		fprnt->mac_addr[i] = ifreq_bfr.ifr_hwaddr.sa_data[i];
	}
}

/*
 *	Retrieves serial number for primary hard disk drive 
 */
bool get_hdd_ser(fingerprintType* fprnt)
{
    static struct hd_driveid hd;
    int fd, i;

    if ((fd = open("/dev/sda", O_RDONLY | O_NONBLOCK)) < 0) {
        printf("error: cannot access hard drive (do you have su/root?)\n");
        printf("Accuracy is reduced without HDD serial access.\n");
        return false;
    }
    if (!ioctl(fd, HDIO_GET_IDENTITY, &hd)) {
        i = strlen(hd.serial_no);
        fprnt->hdd_serial = (char*)malloc(i * sizeof(char)); 
        for (i = 0; i < strlen(hd.serial_no); ++i) {
            fprnt->hdd_serial[i] = hd.serial_no[i];
        }
        fprnt->hdd_serial[i-1] = '\0';
#ifdef TEST
        printf("Test: HDD serial: %s\n", fprnt->hdd_serial);
#endif
    } else if (errno == -ENOMSG) {
        printf("No serial number available\n");
    } else {
            perror("ERROR: HDIO_GET_IDENTITY");
        exit(1);
    }
    return true;
}


/*
 *	Compares two files (fingerprints), byte by byte
 */
bool compare_file(fingerprintType* fprnt, FILE* fpFile, bool hdd_access)
{
	bool result = true;
	int i, fileSize, serialSize;

	// First determine file size
	fseek(fpFile, 0, SEEK_END);
	fileSize = ftell(fpFile);
    serialSize = fileSize - (MAC_LENGTH * sizeof(int));
    char* str_buffer = (char*)malloc(fileSize * sizeof(char));
	rewind(fpFile);
#ifdef TEST
    printf("Test: File size: %i bytes\n", fileSize);
#endif

	// Allocate a buffer to contain the fingerprint file data
	// and read file into buffer
	int* buffer = (int*)malloc(fileSize * sizeof(int));
	fread(buffer, sizeof(int), fileSize, fpFile);
    if (hdd_access == true) {
        fseek(fpFile, MAC_LENGTH * sizeof(int), SEEK_SET);
        fread(str_buffer, sizeof(char), serialSize, fpFile);
        rewind(fpFile);
    }

#ifdef TEST
	printf("Test: fingerprint MAC: ");
	for (i = 0; i < MAC_LENGTH; ++i) {
		printf("%.2X ", (unsigned char)buffer[i]);
	}
    printf("\n");
    if (hdd_access == true) {
        printf("\nTest: %s\n", str_buffer);
    }
#endif

	// Compare fingerprint file to data from fingerprint check
	for (i = 0; i < MAC_LENGTH; ++i) {
		if (buffer[i] != fprnt->mac_addr[i]) {
			printf("Found mismatching MAC values at %i.\n", i+1);
			result = false;
		}
	}

    // Compare HDD serial data to fingerprint data
    if (hdd_access == true) {
	    for (i = 0; i < serialSize; ++i) {
		    if (str_buffer[i] != fprnt->hdd_serial[i]) {
			    printf("Found mismatching HDD serial values at %i.\n", i+1);
			    result = false;
		    }
	    }
    } 

    free(buffer);
    free(str_buffer);

	return result;	
}

/*
 *	Initialization function for reading/writing	
 */
FILE* parameter_check(modeType* mode, int argc, char** argv)
{
	FILE* fpFile;
    int fd;

	// Check for appropriate number of parameters.
	if (argc > 2) {
		display_usage(argv);
		exit(1);
	}

	// Open file for read/write with appropriate error checking
	if (argc == 2 && !strcmp(argv[FLAG], FPRNT_FLAG)) {
		fpFile = fopen(OUT_FILE, "r");
		if (!fpFile) {
			printf("error: system not fingerprinted\n");
			exit(1);
		}
		*mode = MODE_READ;
	} else if (argc == 2 && strcmp(argv[FLAG], FPRNT_FLAG)) {
		printf("error: invalid option flag\n");
		display_usage(argv);
		exit(1);
	} else {
		if (fopen(OUT_FILE, "r")) {
			char c;
			printf("System already fingerprinted. Overwrite? (y/n): ");
			scanf("%c", &c);

			while (c != 'y' && c != 'n') {
				printf("Invalid selection. Please enter (y/n): ");
				scanf("%c", &c);
			}
			if (c == 'n') exit(0);
		}
		fpFile = fopen(OUT_FILE, "w");
		*mode = MODE_WRITE;
	}

	return fpFile;	
}

/*
 *	Message prompt displaying usage syntax
 */
void display_usage(char** argv)
{
	printf("Usage: %s <option flag>\n", argv[0]);
	printf("Flags: -r read existing fingerprint\n");
}
