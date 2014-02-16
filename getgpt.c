#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MMCDEV 		"/dev/block/mmcblk0"

/*
 * Size Of
 */
#define BLOCKSIZE		(512)
#define MBRSIZE			(BLOCKSIZE)	// 512
#define GPTHEADERSIZE		(BLOCKSIZE)	// 512
#define GPTENTRYSIZE		(BLOCKSIZE/4)	// 128
#define MAXENTRYNUM		(128)

/*
 * GUID Partition Table Scheme
 * 
 *     --------------------------------------------
 *     | LBA0    -    Protective MBR
 *     --------------------------------------------
 *     | LBA1    -    Primary GPT Header
 *     --------------------------------------------
 *     | LBA2    -    Entry 001~004
 *     | LBA3    -    Entry 005~008
 *     | LBA4    -    Entry 009~012
 *     | ... ... 
 *     | LBA33   -    Entry 125~128    (total 128)
 *     --------------------------------------------
 *     | LBA34  
 *     | ... ... 
 */

enum {
	ENUM_BLKADDR_MBR		= 0,
	ENUM_BLKADDR_PRIGPT_HEAD	= 1,
	ENUM_BLKADDR_PRIGPT_ENTRY_START	= 2,
	ENUM_BLKADDR_PRIGPT_ENTRY_END	= 33
};

struct gpt_header {
	char sig_name[8];
	
};

int check_if_efi(char* gptheader)
{
	int i;
	char efi_sig[8] = {0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54}; // "EFI PART"

	for(i=0; i<8; i++){
		if( gptheader[i] != efi_sig[i] ){
			printf("This is not a EFI partition.\n");
			return 1;
		}
	}
	
	return 0;
}

int parse_gpt_part(char* gpttable)
{
	int i, j;
	unsigned int lba_start, lba_end, psize;
	char part_name[80];

	printf("\t     START         END       TOTAL                 NAME\n");
	printf("\t--------------------------------------------------------------\n");

	for( j=0; j<MAXENTRYNUM; j++ ){
		/* Check if this is the last partition */
		for(i=0; i<32; i++)
			if( gpttable[i+j*GPTENTRYSIZE] != 0 )
				break;
		if(i==32)
			break;

		/* parse the part name */
		for(i=0; i<72; i++){
			part_name[i] = gpttable[56+i*2+j*GPTENTRYSIZE]&0xFF;
		}

		/* Check the start block address */
		lba_start &= 0x00000000;
		for( i=4; i<8; i++ ){
			lba_start |= ((gpttable[32+8-1-i+j*GPTENTRYSIZE]&0xFF)<<((8-1-i)*2*4));
		}

		/* Check the end block address */
		lba_end &= 0x00000000;
		for( i=4; i<8; i++ ){
			lba_end |= ((gpttable[32+8+8-1-i+j*GPTENTRYSIZE]&0xFF)<<((8-1-i)*2*4));
		}

		/* caluate the partition size of MB */
		psize = (lba_end - lba_start+1)/2/1024;

		/* Show partition info */
		printf("\t0x%08X, 0x%08X, %10d - %10dM - (%s)\n", lba_start, lba_end, (lba_end-lba_start+1), psize, part_name);
	}
	
	printf("\t--------------------------------------------------------------\n");
	return 0;
}

/*
 * Main Entry
 */
int main(int argc, char **args)
{
	int fd_mmc;
	char *buffer;

	/* open the original block device */
	fd_mmc = open(MMCDEV, O_RDWR);
	if( fd_mmc < 0 ){
		printf("Open the block dev(%s) error\n", MMCDEV);
		return -1;
	}

	/* alloc memory for read the MBR + Primary_GPT */
	int bufsize = BLOCKSIZE*( ENUM_BLKADDR_PRIGPT_ENTRY_END+1 );
	buffer = malloc( bufsize );
	if( buffer == NULL ){
		printf("Alloc memmory error\n");
		return -1;
	}
	memset( buffer, 0, bufsize );

	/* read back the whole MBR + Primary_GPT */
	if( bufsize != read( fd_mmc, buffer, bufsize ) ){
		printf("Read error.\n");
	}

	/* check if the partition is a efi partition? */
	if( check_if_efi( &buffer[ENUM_BLKADDR_PRIGPT_HEAD*BLOCKSIZE] ) == 0 ){
		/* parse the partition table... */
		parse_gpt_part( &buffer[ENUM_BLKADDR_PRIGPT_ENTRY_START*BLOCKSIZE] );
	}

	/* clean */
	free(buffer);
        return 0;
}

