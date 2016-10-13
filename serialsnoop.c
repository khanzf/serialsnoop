// Crappy experimental serial spy code
// Written by Farhan Khan (khanzf@gmail.com)
// Written to spy on my XTS3000
// Um...lets go with a BSD license...?

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <termios.h>
#include <sys/time.h>

#define BUFSIZE 10

void hexdump(unsigned char *data, int size) {
	size_t i;
	for (i = 0; i < size; ++i)
		printf("%02x ", data[i]);
	putchar('\n');
}

void terminalsettings(int fd) {
	struct termios newio;
	if( tcgetattr( fd, &newio ) < 0 ) {
		perror("Error getting terminal settings");
		exit(-1);
	}    

	/* Set some default settings */
	newio.c_iflag |= IGNBRK;
	newio.c_iflag &= ~BRKINT;
	newio.c_iflag &= ~ICRNL;
	newio.c_oflag = 0;
	newio.c_lflag = 0;
	newio.c_cc[VTIME] = 0;
	newio.c_cc[VMIN] = 1;

	/* Set our baud rate */
	cfsetospeed( &newio, B9600 );
	cfsetispeed( &newio, B9600 );

	/* Character size = 8 */
	newio.c_cflag &= ~CSIZE;
	newio.c_cflag |= CS8;

	/* One stop bit */
	newio.c_cflag &= ~CSTOPB;

	/* Parity = none */
	newio.c_iflag &= ~IGNPAR;
	newio.c_cflag &= ~( PARODD | PARENB );
	newio.c_iflag |= IGNPAR;

	/* No flow control */
	newio.c_iflag &= ~( IXON | IXOFF | IXANY );

	/* Set our serial port settings */
	if( tcsetattr( fd, TCSANOW, &newio ) < 0 ) {
		printf("Error setting terminal setting");
		exit(-1);
	}
}

int opencom(char *interfacestr) {
	int fdinterface;

	fdinterface = open(interfacestr, O_RDONLY); //O_RDWR | O_NONBLOCK);
	if (fdinterface == -1) {
		perror("Failed to open interface");
		exit(-1);
	}
	return fdinterface;
}

int readinterface(int fdinterface, char *buffer) {
	int size;
	size = read(fdinterface, buffer, BUFSIZE);
	if (size == -1) {
		perror("Failed to read interface");
		exit(-1);
	}
	return size;
}

int main(int argc, char **argv) {
	int fdone;
	int fdtwo;

	int maxfd;

	fd_set serialfds, defaultfds;
	struct timeval timer;

	struct timeval start, end;

	char data[BUFSIZE];
	int size;

	if (argc != 3) {
		fprintf(stderr, "Run as %s [serial device 1] [serial device 2]\n", argv[0]);
		fprintf(stderr, "Example: %s /dev/ttyUSB0 /dev/ttyUSB1\n", argv[0]);
		return 0;
	}

	fdone = opencom(argv[1]);
	fdtwo = opencom(argv[2]);

	terminalsettings(fdone);
	terminalsettings(fdtwo);

	if (fdone > fdtwo)
		maxfd = fdone;
	else
		maxfd = fdtwo;

	FD_ZERO(&defaultfds);
	FD_SET(fdone, &defaultfds);
	FD_SET(fdtwo, &defaultfds);


	gettimeofday(&start, NULL);
	while(1) {
		memset(data, 0, BUFSIZE);
		serialfds = defaultfds;

		timer.tv_sec = 60;
		timer.tv_usec = 0;

		select(maxfd+1, &serialfds, NULL, NULL, &timer);
		gettimeofday(&end, NULL);

		printf("%ld ", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));

		if (FD_ISSET(fdone, &serialfds)) {
			size = readinterface(fdone, data);
			printf("%s -> %s: %02X\n", argv[1], argv[2], (unsigned char)( data[0] & 0xFF) );
		}
		else if (FD_ISSET(fdtwo, &serialfds)) {
			size = readinterface(fdtwo, data);
			printf("%s -> %s: %02X\n", argv[2], argv[1], (unsigned char)( data[0] & 0xFF) );
		}
		else
			continue;

		fflush(NULL); // Force it to print to the terminal.
	}

	return 0;
}
