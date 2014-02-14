#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MMCDEV 		"/dev/block/mmcblk0"
#define BLOCKSIZE	(512)
#define GPTTABLESIZE	(128)

#define MBR_OFFSET		(0)
#define GPTHEADER_OFFSET	(1)

#define MBRDAT				"/data/partition_mbr.dat"
#define PRIMARYGPTHEADERDAT		"/data/partition_gptheader.dat"
#define GPTTABLEDAT			"/data/partition_gpttable.dat"

int main(int argc, char **args)
{
	int fd_mmc;
	char buffer[BLOCKSIZE];
	int i, j;
	unsigned int lba_start, lba_end, psize;
	char part_name[80];

	fd_mmc = open(MMCDEV, O_RDWR);
	if( fd_mmc < 0 ){
		return -1;
	}

	if(0){
		printf("===== MBR =====\n");
		memset( buffer, 0, BLOCKSIZE);
		//seek( fd_mmc, MBR_OFFSET, SEEK_SET );
		read( fd_mmc, buffer, BLOCKSIZE );
		for( i=0; i<BLOCKSIZE; i++ ){
			printf("%02X", buffer[i]&0xFF);
			printf(" ");
			if( (i!=0) && (i%16==15) ){
				printf("\n");
			}
		}
		printf("\n");
	}

	if(0){
		printf("===== GPT HEADER =====\n");
		memset( buffer, 0, BLOCKSIZE);
		//seek( fd_mmc, GPTHEADER_OFFSET, SEEK_SET );
		read(fd_mmc, buffer, BLOCKSIZE );
		for( i=0; i<BLOCKSIZE; i++ ){
			printf("%02X", buffer[i]&0xFF);
			printf(" ");
			if( (i!=0) && (i%16==15) ){
				printf("\n");
			}
		}
		printf("\n");
	}

	//printf("===== GPT TABLE =====\n");
	printf("---------------------------------------------------------------\n");
	printf(" START_LBA,    END_LBA,        LBA -    SIZE(MB) - NAME\n");
	printf("---------------------------------------------------------------\n");
	/* skip 2 block */
	read( fd_mmc, buffer, BLOCKSIZE );
	read( fd_mmc, buffer, BLOCKSIZE );
	/* skip 2 block */
	while(1){
		/* Read ont partition info, which size is 128 Bytes */
		memset( buffer, 0, BLOCKSIZE);
		read(fd_mmc, buffer, GPTTABLESIZE );

	#if 0
		/* Show the original data */
		for( i=0; i<GPTTABLESIZE; i++ ){
			printf("%02X", buffer[i]&0xFF);
			printf(" ");
			if( (i!=0) && (i%16==15) ){
				printf("\n");
			}
		}
	#endif

		/* Check if this is the last partition */
		for(i=0; i<32; i++){
			if( buffer[i] != 0 ){
				break;
			}
		}
		if(i==32){
			break;
		}

		/* Check the partition Name */
		memset( part_name, 0, sizeof(part_name));
		for(i=0; i<72; i++){
			part_name[i] = buffer[56+i*2]&0xFF;
		}

		/* Check the start block address */
		lba_start &= 0x00000000;
		for( i=4; i<8; i++ ){
			//printf("%02X ", buffer[32+8-1-i]&0xFF);
			lba_start |= ((buffer[32+8-1-i]&0xFF)<<((8-1-i)*2*4));
		}

		/* Check the   end block address */
		lba_end &= 0x00000000;
		for( i=4; i<8; i++ ){
			//printf("%02X ", buffer[32+8+8-1-i]&0xFF);
			lba_end |= ((buffer[32+8+8-1-i]&0xFF)<<((8-1-i)*2*4));
		}

		/* caluate the partition size of MB */
		psize = (lba_end - lba_start+1)/2/1024;

		/* Show partition info */
		printf("0x%08X, 0x%08X, %10d - %10dM - (%s)\n", lba_start, lba_end, (lba_end-lba_start+1), psize, part_name);
	}

        return 0;
}

