#include "stdio.h"
#include "stdlib.h"
#include "string.h"

struct compress_mib_header {
	unsigned char signature[6];
	unsigned short compRate;
	unsigned int compLen;
};

struct hw_param_header {
	unsigned char signature[4];  // Tag + version
	unsigned int len;
};

struct mib_entry {
	unsigned short id;
	unsigned short len;
	unsigned char data[];
};

struct decompressed_data {
	unsigned int size;
	unsigned char *header_start;
	unsigned char *data_start;
	unsigned char *data_end;
};

int Decode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput)	{
    unsigned int ui, uj, uc;
	int  i, j, k, r, c;
	unsigned int  flags;
	unsigned int ulPos=0;
	unsigned int ulExpLen=0;
    char *text_buf;

	if ((text_buf = (char *) malloc(4113)) == 0) {
		return 0;
	}
	
	for (i = 0; i < 4078; i++)
		text_buf[i] = ' ';
	r = 4078;
	flags = 0;
	while(1) {
		if (((flags >>= 1) & 0x100) == 0) {
			c = ucInput[ulPos++];
			if (ulPos>inLen)
				break;
			flags = c | 0xff00;	
		}
		if (flags & 1) {
			c = ucInput[ulPos++];
			if ( ulPos > inLen )
				break;
			ucOutput[ulExpLen++] = c;
			text_buf[r++] = c;
			r &= 4095;
		} else {
			i = ucInput[ulPos++];
			if ( ulPos > inLen ) break;
			j = ucInput[ulPos++];
			if ( ulPos > inLen ) break;
			
			i |= ((j & 0xf0) << 4);
			j = (j & 0x0f) + 2;
			for (k = 0; k <= j; k++) {
				c = text_buf[(i + k) & 4095];
				ucOutput[ulExpLen++] = c;
				text_buf[r++] = c;
				r &= 4095;
			}
		}
	}

	free(text_buf);
	return ulExpLen;
}

unsigned char checksum(char *input, int len) {
	unsigned char c = 0;

    for (int j = 0; j < len; j++)
    {
        c += input[j];
    }

	return c;
}

int main(int argc, char ** argv) {
    struct compress_mib_header h;
	struct hw_param_header h_dec;
    unsigned char buf[sizeof(struct compress_mib_header)];
	FILE *fp;
	char *ifname;
	unsigned int offset;
	unsigned int key_id, key_offset, key_size;
	unsigned char *input;
	int matches, c;
	struct decompressed_data decompressed_data;
	int large_header;
	int h_len;
	unsigned int random_key = 0;
	struct mib_entry current_mib_entry;

	if (argc != 4) {
		printf("Wrong number of parameters!\n");
		printf("Usage: ./extract_and_decode_mib [header type: large/small] [offset] [input filename]\n");
		printf("If checksum fails or header size does not match, try changing the header type.\n");
		exit(1);
	}
	
	ifname = argv[3];
	
	matches = sscanf(argv[2], "0x%x", &offset);

	if (matches =! 1 ) {
		printf("Offset should be hex (i.e. 0x1337)\n");
		exit(1);
	};

	if (!strcmp("large", argv[1])){
		printf("Using large data header.\n");
		large_header = 1;
	} else if (!strcmp("small", argv[1])) {
		printf("Using small data header.\n");
		large_header = 0;
	} else {
		printf("Wrong header type\n");
		exit(1);
	}

	printf("Reading %s @ offset: 0x%x\n", ifname, offset);

    fp = fopen(ifname, "rb");
    fseek(fp, offset, SEEK_SET);
	// Read the size of the biggest header
	fread(&buf, 1, sizeof(struct compress_mib_header), fp);

	if (!memcmp(buf, "COMP", 4)) {
		if (!memcmp(buf+4, "HS", 2))
			printf("Found compressed hw mib!\n");
		if (!memcmp(buf+4, "DS", 2))
			printf("Found compressed ds mib!\n");
		if (!memcmp(buf+4, "CS", 2))
			printf("Found compressed cs mib!\n");

		memcpy(h.signature, buf, 6);
		h.compRate = buf[7] | buf[6] << 8;
		h.compLen = buf[11] | buf[10] << 8 | buf[9] << 16 | buf[8] << 24;

		printf("signature: %s\n", h.signature);
		printf("comp_rate: %d\n", h.compRate);
		printf("comp_len: %d\n", h.compLen);

		input = (unsigned char *) malloc(h.compLen);
		decompressed_data.header_start = (char *) malloc(h.compRate * h.compLen);
		
		fread(input, 1, h.compLen, fp);
		
		decompressed_data.size = Decode(input, h.compLen, decompressed_data.header_start);

		if (memcmp(decompressed_data.header_start, "6G", 2) && memcmp(decompressed_data.header_start, "H6", 2) && memcmp(decompressed_data.header_start, "6g", 2)) {
			printf("Invalid signature: %c%c\n", *decompressed_data.header_start, *(decompressed_data.header_start+1));
			exit(1);
		}

		if (large_header) {
			h_dec.len = decompressed_data.header_start[7] | decompressed_data.header_start[6] << 8 | decompressed_data.header_start[5] << 16 | decompressed_data.header_start[4] << 24;
			h_len = 8;
		} else {
   			h_dec.len = decompressed_data.header_start[5] | decompressed_data.header_start[4] << 8;
			h_len = 6;
		}

		memcpy(h_dec.signature, decompressed_data.header_start, 4);

		if (decompressed_data.size != h_dec.len + h_len) {
			printf("Invalid header length value.\n");
			exit(1);
		}

		decompressed_data.data_start = decompressed_data.header_start + h_len;
		decompressed_data.data_end = decompressed_data.data_start + h_dec.len - 1; // Last one is for checksum
		c = checksum(decompressed_data.data_start, h_dec.len);

		if (c) {
			printf("Invalid checksum: %d\n", c);
			exit(1);
		} else {
			printf("Checksum OK!\n");
		}


		printf("\n-----------------------------------------------------------------------\n");
		printf("Decompressed mib data has the form '[short id] [short size] [data]'.\n");
		printf("Use id from libapmib.so mib tables to locate desired value.\n");
		printf("i.e: HW_RANDOM_KEY is located at hwmib_table, has id 0x3EE4 and size 4.\n");
		printf("So to get its value please enter 0x3ee4'.\n");

		matches = 0;
		matches = scanf("0x%x", &key_id);

		if (matches =! 1 ) {
			printf("key offset should be hex (i.e. 0x1337)\n");
			exit(1);
		};

		unsigned char *ddp = decompressed_data.data_start;

		while (ddp < decompressed_data.data_end) {
			current_mib_entry.id = ddp[1] | ddp[0] << 8;
			current_mib_entry.len = ddp[3] | ddp[2] << 8;

			if (current_mib_entry.id == key_id) {
				printf("\nFound HW_RANDOM_KEY!\n");
				
				for (int i = current_mib_entry.len - 1; i >= 0; i--) {
					random_key |= ddp[i + 4] << ((current_mib_entry.len - 1 - i) * 8);
				}
				
				printf("HW_RANDOM_KEY=%u\n", random_key);
				
				break;
			}

			unsigned int current_mib_entry_size = sizeof(struct mib_entry) + current_mib_entry.len;

			ddp += current_mib_entry_size;
		}

		free(decompressed_data.header_start);
		free(input);
	} else if (memcmp(buf, "6G", 2) && memcmp(buf, "H6", 2) && memcmp(buf, "6g", 2)) {
		printf("Invalid signature: %c%c\n", *buf, *(buf+1));
		exit(1);
	} else {
		printf("Found uncompressed mib!\n");
		printf("Signature: %c%c\n", *buf, *(buf+1));

		memcpy(h_dec.signature, buf, 4);
   		if (large_header) {
			h_dec.len = buf[7] | buf[6] << 8 | buf[5] << 16 | buf[4] << 24;
			h_len = 8;
		} else {
   			h_dec.len = buf[5] | buf[4] << 8;
			h_len = 6;
		}

		input = (char *) malloc(h_dec.len);

		fseek(fp, offset + h_len, SEEK_SET);
		fread(input, 1, h_dec.len, fp);
		
		c = checksum(input, h_dec.len);

		if (c) {
			printf("Invalid checksum: %d\n", c);
			exit(1);
		} else {
			printf("Checksum OK!\n");
		}

		printf("\n-----------------------------------------------------------------------\n");
		printf("Uncompressed mib data is concatenated one entry after the other'.\n");
		printf("Use the offset from libapmib.so mib tables to locate desired value.\n");
		printf("i.e: HW_RANDOM_KEY is located at hwmib_table, has offset 0x1393 and size 4.\n");
		printf("So to get its value please enter 0x1393 and then 4'.\n");

		matches = 0;
		matches = scanf("0x%x", &key_offset);

		if (matches =! 1 ) {
			printf("key offset should be hex (i.e. 0x1337)\n");
			exit(1);
		};

		matches = 0;
		matches = scanf("%u", &key_size);

		if (matches =! 1 ) {
			printf("key offset size be dec (i.e. 4)\n");
			exit(1);
		};

		for (int j = key_size - 1; j >= 0; j--) {
			random_key |= input[key_offset + j] << ((key_size - 1 - j) * 8);
		}
		
		printf("HW_RANDOM_KEY=%u\n", random_key);
		
		free(input);
	}
}