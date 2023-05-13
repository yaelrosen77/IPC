#if !defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE)
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <openssl/evp.h>

#define FILE_SIZE 104857600
#define CHUNK_SIZE 65536
#define UDP_CHUNK_SIZE 1280
#define UDS_NAME "/tmp/uds_sock"
#define POLL_TIMEOUT 2000

char* md5hash(char *data) {
	EVP_MD_CTX *mdctx;
	unsigned char *md5_digest = NULL;
	char *checksumString = NULL;
	unsigned int md5_digest_len = EVP_MD_size(EVP_md5());
	mdctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
	EVP_DigestUpdate(mdctx, data, FILE_SIZE);
	md5_digest = (unsigned char*)OPENSSL_malloc(md5_digest_len);
	EVP_DigestFinal_ex(mdctx, md5_digest, &md5_digest_len);
	EVP_MD_CTX_free(mdctx);
	checksumString = (char *)calloc((md5_digest_len * 2 + 1), sizeof(int8_t));
	if (checksumString == NULL)
	{
		OPENSSL_free(md5_digest);
		return NULL;
	}
	for (uint32_t i = 0; i < md5_digest_len; i++)
		sprintf(checksumString + (i * 2), "%02x", md5_digest[i]);
	OPENSSL_free(md5_digest);
	return checksumString;
}

void client_performance(const char *ip, const char *port, const char *param1, const char *param2) {
	struct sockaddr_in server_addr;
	int client_fd;

	char buffer[1024] = {0};

	char *data = (char *)malloc(FILE_SIZE * sizeof(char));
	char mode = -1;

	if (data == NULL)
	{
		perror("malloc");
		exit(1);
	}

	if (strcmp(param1, "ipv4") == 0)
	{
		if (strcmp(param2, "tcp") == 0)
			mode = 0;

		else if (strcmp(param2, "udp") == 0)
			mode = 1;
	}

	if (strcmp(param1, "ipv6") == 0)
	{
		if (strcmp(param2, "tcp") == 0)
			mode = 2;
			
		else if (strcmp(param2, "udp") == 0)
			mode = 3;
	}

	else if (strcmp(param1, "uds") == 0)
	{
		if (strcmp(param2, "stream") == 0)
			mode = 4;
			
		else if (strcmp(param2, "dgram") == 0)
			mode = 5;
	}

	else if (strcmp(param1, "mmap") == 0)
		mode = 6;

	else if (strcmp(param1, "pipe") == 0)
		mode = 7;

	if (mode == -1)
	{
		printf("Invalid protocol\n");
		exit(1);
	}

	srand(time(NULL));

	printf("Generating random data...\n");

	for (int i = 0; i < FILE_SIZE; i++)
		data[i] = rand() % 256;

	printf("Data generated\n");
	printf("Calculating MD5 hash...\n");

	char *md5 = md5hash(data);

	if (md5 == NULL)
	{
		perror("malloc");
		free(data);
		exit(1);
	}

	printf("MD5 hash calculated\n");

	strcpy(buffer, param1);
	strcat(buffer, " ");
	strcat(buffer, param2);
	strcat(buffer, " ");
	strcat(buffer, md5);

	free(md5);

	client_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (client_fd < 0)
	{
		perror("socket");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = inet_addr(ip);

	if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect");
		exit(1);
	}

	printf("Connected to server\n");
	printf("Sending data...\n");

	if (send(client_fd, buffer, (strlen(buffer) + 1), 0) < 0)
	{
		perror("send");
		exit(1);
	}

	printf("Data sent\n");
	printf("Receiving data...\n");

	if (recv(client_fd, buffer, sizeof(buffer), 0) < 0)
	{
		perror("recv");
		exit(1);
	}

	printf("Data received\n");

	struct timeval start, end;

	switch(mode)
	{
		case 0:
		{
			printf("IPv4 TCP\n");

			struct sockaddr_in server_addr2;
			int client_fd2;

			client_fd2 = socket(AF_INET, SOCK_STREAM, 0);

			if (client_fd2 < 0)
			{
				perror("socket");
				exit(1);
			}

			memset(&server_addr2, 0, sizeof(server_addr2));

			server_addr2.sin_family = AF_INET;
			server_addr2.sin_port = htons(atoi(port) + 1);
			server_addr2.sin_addr.s_addr = server_addr.sin_addr.s_addr;

			if (connect(client_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
			{
				perror("connect");
				exit(1);
			}

			printf("Connected to server\n");

			int bytes_sent = 0;

			gettimeofday(&start, NULL);

			while (bytes_sent < FILE_SIZE)
			{
				int chunk_size = (FILE_SIZE - bytes_sent < CHUNK_SIZE) ? FILE_SIZE - bytes_sent : CHUNK_SIZE;
				int bytes = send(client_fd2, data + bytes_sent, chunk_size, 0);
				gettimeofday(&end, NULL);

				if (bytes < 0)
				{
					perror("send");
					exit(1);
				}

				bytes_sent += bytes;
			}

			printf("Sent %d bytes\n", bytes_sent);

			close(client_fd2);

			break;
		}

		case 1:
		{
			printf("IPv4 UDP\n");

			struct sockaddr_in server_addr2;
			int client_fd2;

			client_fd2 = socket(AF_INET, SOCK_DGRAM, 0);

			if (client_fd2 < 0)
			{
				perror("socket");
				exit(1);
			}

			memset(&server_addr2, 0, sizeof(server_addr2));

			server_addr2.sin_family = AF_INET;
			server_addr2.sin_port = htons(atoi(port) + 1);
			server_addr2.sin_addr.s_addr = server_addr.sin_addr.s_addr;

			int bytes_sent = 0;

			gettimeofday(&start, NULL);

			while (bytes_sent < FILE_SIZE)
			{
				int chunk_size = (FILE_SIZE - bytes_sent < UDP_CHUNK_SIZE) ? FILE_SIZE - bytes_sent : UDP_CHUNK_SIZE;
				int bytes = sendto(client_fd2, data + bytes_sent, chunk_size, 0, (struct sockaddr *)&server_addr2, sizeof(server_addr2));
				gettimeofday(&end, NULL);

				if (bytes < 0)
				{
					perror("send");
					exit(1);
				}

				bytes_sent += bytes;
			}

			printf("Sent %d bytes\n", bytes_sent);

			close(client_fd2);

			break;
		}

		case 2:
		{
			printf("IPv6 TCP\n");

			struct sockaddr_in6 server_addr2;
			int client_fd2;

			client_fd2 = socket(AF_INET6, SOCK_STREAM, 0);

			if (client_fd2 < 0)
			{
				perror("socket");
				exit(1);
			}

			memset(&server_addr2, 0, sizeof(server_addr2));

			server_addr2.sin6_family = AF_INET6;
			server_addr2.sin6_port = htons(atoi(port) + 1);
			
			if (inet_pton(AF_INET6, "::1", &server_addr2.sin6_addr) < 0)
			{
				perror("inet_pton");
				exit(1);
			}

			if (connect(client_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
			{
				perror("connect");
				exit(1);
			}

			printf("Connected to server\n");

			int bytes_sent = 0;

			gettimeofday(&start, NULL);

			while (bytes_sent < FILE_SIZE)
			{
				int chunk_size = (FILE_SIZE - bytes_sent < CHUNK_SIZE) ? FILE_SIZE - bytes_sent : CHUNK_SIZE;
				int bytes = send(client_fd2, data + bytes_sent, chunk_size, 0);
				gettimeofday(&end, NULL);

				if (bytes < 0)
				{
					perror("send");
					exit(1);
				}

				bytes_sent += bytes;
			}

			printf("Sent %d bytes\n", bytes_sent);

			close(client_fd2);

			break;
		}

		case 3:
		{
			printf("IPv6 UDP\n");

			struct sockaddr_in6 server_addr2;
			int client_fd2;

			client_fd2 = socket(AF_INET6, SOCK_DGRAM, 0);

			if (client_fd2 < 0)
			{
				perror("socket");
				exit(1);
			}

			memset(&server_addr2, 0, sizeof(server_addr2));

			server_addr2.sin6_family = AF_INET6;
			server_addr2.sin6_port = htons(atoi(port) + 1);
			
			if (inet_pton(AF_INET6, "::1", &server_addr2.sin6_addr) < 0)
			{
				perror("inet_pton");
				exit(1);
			}

			int bytes_sent = 0;

			gettimeofday(&start, NULL);

			while (bytes_sent < FILE_SIZE)
			{
				int chunk_size = (FILE_SIZE - bytes_sent < UDP_CHUNK_SIZE) ? FILE_SIZE - bytes_sent : UDP_CHUNK_SIZE;
				int bytes = sendto(client_fd2, data + bytes_sent, chunk_size, 0, (struct sockaddr *)&server_addr2, sizeof(server_addr2));
				gettimeofday(&end, NULL);

				if (bytes < 0)
				{
					perror("send");
					exit(1);
				}

				bytes_sent += bytes;
			}

			printf("Sent %d bytes\n", bytes_sent);

			close(client_fd2);

			break;
		}

		case 4:
		{
			printf("UDS Stream\n");

			struct sockaddr_un server_addr2;
			int client_fd2;

			client_fd2 = socket(AF_UNIX, SOCK_STREAM, 0);

			if (client_fd2 < 0)
			{
				perror("socket");
				exit(1);
			}

			memset(&server_addr2, 0, sizeof(server_addr2));

			server_addr2.sun_family = AF_UNIX;
			strcpy(server_addr2.sun_path, UDS_NAME);

			if (connect(client_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
			{
				perror("connect");
				exit(1);
			}

			printf("Connected to server\n");

			int bytes_sent = 0;

			gettimeofday(&start, NULL);

			while (bytes_sent < FILE_SIZE)
			{
				int chunk_size = (FILE_SIZE - bytes_sent < CHUNK_SIZE) ? FILE_SIZE - bytes_sent : CHUNK_SIZE;
				int bytes = send(client_fd2, data + bytes_sent, chunk_size, 0);
				gettimeofday(&end, NULL);

				if (bytes < 0)
				{
					perror("send");
					exit(1);
				}

				bytes_sent += bytes;
			}

			printf("Sent %d bytes\n", bytes_sent);

			close(client_fd2);

			break;
		}

		case 5:
		{
			printf("UDS Datagram\n");

			struct sockaddr_un server_addr2;
			int client_fd2;

			client_fd2 = socket(AF_UNIX, SOCK_DGRAM, 0);

			if (client_fd2 < 0)
			{
				perror("socket");
				exit(1);
			}

			memset(&server_addr2, 0, sizeof(server_addr2));

			server_addr2.sun_family = AF_UNIX;
			strcpy(server_addr2.sun_path, UDS_NAME);

			int bytes_sent = 0;

			gettimeofday(&start, NULL);

			while (bytes_sent < FILE_SIZE)
			{
				int chunk_size = (FILE_SIZE - bytes_sent < CHUNK_SIZE) ? FILE_SIZE - bytes_sent : CHUNK_SIZE;
				int bytes = sendto(client_fd2, data + bytes_sent, chunk_size, 0, (struct sockaddr *)&server_addr2, sizeof(server_addr2));
				gettimeofday(&end, NULL);

				if (bytes < 0)
				{
					perror("send");
					exit(1);
				}

				bytes_sent += bytes;
			}

			printf("Sent %d bytes\n", bytes_sent);

			close(client_fd2);

			break;
		}

		case 6:
		{
			printf("MMAP\n");

			char *ptr = MAP_FAILED;

			int fd = open(param2, O_RDWR | O_CREAT, 0666);

			if (fd < 0)
			{
				perror("open");
				exit(1);
			}

			ftruncate(fd, FILE_SIZE);

			ptr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

			if (ptr == MAP_FAILED)
			{
				perror("mmap");
				exit(1);
			}

			send(client_fd, &fd, sizeof(int), 0);

			gettimeofday(&start, NULL);
			memcpy(ptr, data, FILE_SIZE);
			gettimeofday(&end, NULL);

			munmap(ptr, FILE_SIZE);
			close(fd);
			break;
		}

		case 7:
		{
			unlink(param2);
			printf("FIFO\n");

			if (mkfifo(param2, 0666) == -1)
			{
				perror("mkfifo");
				exit(1);
			}

			int fd = open(param2, O_WRONLY, 0666);

			if (fd < 0)
			{
				perror("open");
				exit(1);
			}

			send(client_fd, &fd, sizeof(int), 0);

			int bytes_sent = 0;

			gettimeofday(&start, NULL);
			while (bytes_sent < FILE_SIZE)
			{
				int chunk_size = (FILE_SIZE - bytes_sent < CHUNK_SIZE) ? FILE_SIZE - bytes_sent : CHUNK_SIZE;
				int bytes = write(fd, data + bytes_sent, chunk_size);
				gettimeofday(&end, NULL);

				bytes_sent += bytes;
			}

			gettimeofday(&end, NULL);
			close(fd);

			break;
		}

		default:
			break;
	}

	free(data);

	float time_taken = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

	printf("Time taken: %f ms\n", time_taken);
	printf("Throughput: %f MB/s\n", (FILE_SIZE / time_taken * 1000.0 / 1024.0 / 1024.0));

	close(client_fd);
}

void server_performance(const char *port, bool quiet) {
	struct sockaddr_in server_addr, client_addr;

	int client_fd, bytes_received = 0, reuse = 1;
	int clientlen = sizeof(client_addr);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0)
	{
		perror("socket");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = INADDR_ANY;

	char *data = malloc(FILE_SIZE * sizeof(char));

	if (data == NULL)
	{
		perror("malloc");
		exit(1);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
	{
		perror("setsockopt");
		exit(1);
	}

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	if (listen(server_fd, 1) < 0)
	{
		perror("listen");
		exit(1);
	}

	if (!quiet)
		printf("Listening on port %s\n", port);

	while (1)
	{
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&clientlen);

		if (client_fd < 0)
		{
			perror("accept");
			exit(1);
		}

		if (!quiet)
			printf("Accepted connection\n");

		
		char buffer[256];

		bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);

		if (bytes_received < 0)
		{
			perror("recv");
			exit(1);
		}

		if (!quiet)
			printf("%s\n", buffer);

		char param1[128], param2[128], param3[128];
		char *token = strtok(buffer, " ");

		strcpy(param1, token);
		token = strtok(NULL, " ");
		strcpy(param2, token);
		token = strtok(NULL, " ");
		strcpy(param3, token);

		int mode = -1;

		if (strcmp(param1, "ipv4") == 0)
		{
			if (strcmp(param2, "tcp") == 0)
				mode = 0;

			else if (strcmp(param2, "udp") == 0)
				mode = 1;
		}

		if (strcmp(param1, "ipv6") == 0)
		{
			if (strcmp(param2, "tcp") == 0)
				mode = 2;
				
			else if (strcmp(param2, "udp") == 0)
				mode = 3;
		}

		else if (strcmp(param1, "uds") == 0)
		{
			if (strcmp(param2, "stream") == 0)
				mode = 4;
				
			else if (strcmp(param2, "dgram") == 0)
				mode = 5;
		}

		else if (strcmp(param1, "mmap") == 0)
			mode = 6;

		else if (strcmp(param1, "pipe") == 0)
			mode = 7;

		if (mode == -1)
		{
			printf("Invalid protocol\n");
			close(client_fd);
			continue;
		}

		struct timeval start, end;

		int bytes_received2 = 0;

		switch(mode)
		{
			case 0:
			{
				struct sockaddr_in server_addr2, client_addr2;
				int client_fd2;
				int clientlen2 = sizeof(client_addr2);
				int server_fd2 = socket(AF_INET, SOCK_STREAM, 0);

				if (server_fd2 < 0)
				{
					perror("socket");
					exit(1);
				}

				memset(&server_addr2, 0, sizeof(server_addr2));
				memset(&client_addr2, 0, sizeof(client_addr2));

				server_addr2.sin_family = AF_INET;
				server_addr2.sin_port = htons(atoi(port) + 1);
				server_addr2.sin_addr.s_addr = INADDR_ANY;

				if (setsockopt(server_fd2, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
				{
					perror("setsockopt");
					exit(1);
				}

				if (bind(server_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
				{
					perror("bind");
					exit(1);
				}

				if (listen(server_fd2, 1) < 0)
				{
					perror("listen");
					exit(1);
				}

				send(client_fd, "ACK", 3, 0);

				client_fd2 = accept(server_fd2, (struct sockaddr *)&client_addr2, (socklen_t *)&clientlen2);

				if (client_fd2 < 0)
				{
					perror("accept");
					exit(1);
				}

				close(server_fd2);

				if (!quiet)
					printf("Accepted connection\n");

				gettimeofday(&start, NULL);

				while (bytes_received2 < FILE_SIZE)
				{
					int chunk_size = FILE_SIZE - bytes_received2 < CHUNK_SIZE ? FILE_SIZE - bytes_received2 : CHUNK_SIZE;

					int bytes = recv(client_fd2, data + bytes_received2, chunk_size, 0);
					gettimeofday(&end, NULL);

					if (bytes < 0)
					{
						perror("recv");
						exit(1);
					}

					bytes_received2 += bytes;
				}

				close(client_fd2);

				break;
			}

			case 1:
			{
				struct sockaddr_in server_addr2, client_addr2;
				int clientlen2 = sizeof(client_addr2);

				int server_fd2 = socket(AF_INET, SOCK_DGRAM, 0);

				if (server_fd2 < 0)
				{
					perror("socket");
					exit(1);
				}

				memset(&server_addr2, 0, sizeof(server_addr2));
				memset(&client_addr2, 0, sizeof(client_addr2));

				server_addr2.sin_family = AF_INET;
				server_addr2.sin_port = htons(atoi(port) + 1);
				server_addr2.sin_addr.s_addr = INADDR_ANY;

				if (setsockopt(server_fd2, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
				{
					perror("setsockopt");
					exit(1);
				}

				if (bind(server_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
				{
					perror("bind");
					exit(1);
				}

				if (!quiet)
					printf("Waiting for the client to send data\n");

				
				struct pollfd fds[1];

				fds[0].fd = server_fd2;
				fds[0].events = POLLIN;

				send(client_fd, "ACK", 3, 0);

				gettimeofday(&start, NULL);

				while (bytes_received2 < FILE_SIZE)
				{
					int ret = poll(fds, 1, 1000);

					if (ret < 0)
					{
						perror("poll");
						exit(1);
					}

					else if (ret == 0)
					{
						if (!quiet)
							printf("Timeout\n");

						break;
					}

					if (fds[0].revents & POLLIN)
					{
						int chunk_size = FILE_SIZE - bytes_received2 < UDP_CHUNK_SIZE ? FILE_SIZE - bytes_received2 : UDP_CHUNK_SIZE;

						int bytes = recvfrom(server_fd2, data + bytes_received2, chunk_size, 0, (struct sockaddr *)&client_addr2, (socklen_t *)&clientlen2);
						gettimeofday(&end, NULL);

						if (bytes < 0)
						{
							perror("recvfrom");
							exit(1);
						}

						bytes_received2 += bytes;
					}
				}

				close(server_fd2);

				break;
			}

			case 2:
			{
				struct sockaddr_in6 server_addr2, client_addr2;
				int client_fd2;
				int clientlen2 = sizeof(client_addr2);
				int server_fd2 = socket(AF_INET6, SOCK_STREAM, 0);

				if (server_fd2 < 0)
				{
					perror("socket");
					exit(1);
				}

				memset(&server_addr2, 0, sizeof(server_addr2));
				memset(&client_addr2, 0, sizeof(client_addr2));

				server_addr2.sin6_family = AF_INET6;
				server_addr2.sin6_port = htons(atoi(port) + 1);
				server_addr2.sin6_addr = in6addr_any;

				if (setsockopt(server_fd2, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
				{
					perror("setsockopt");
					exit(1);
				}

				if (bind(server_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
				{
					perror("bind");
					exit(1);
				}

				if (listen(server_fd2, 1) < 0)
				{
					perror("listen");
					exit(1);
				}

				send(client_fd, "ACK", 3, 0);

				client_fd2 = accept(server_fd2, (struct sockaddr *)&client_addr2, (socklen_t *)&clientlen2);

				if (client_fd2 < 0)
				{
					perror("accept");
					exit(1);
				}

				close(server_fd2);

				if (!quiet)
					printf("Accepted connection\n");

				gettimeofday(&start, NULL);

				while (bytes_received2 < FILE_SIZE)
				{
					int chunk_size = FILE_SIZE - bytes_received2 < CHUNK_SIZE ? FILE_SIZE - bytes_received2 : CHUNK_SIZE;

					int bytes = recv(client_fd2, data + bytes_received2, chunk_size, 0);
					gettimeofday(&end, NULL);

					if (bytes < 0)
					{
						perror("recv");
						exit(1);
					}

					bytes_received2 += bytes;
				}

				close(client_fd2);

				break;
			}

			case 3:
			{
				struct sockaddr_in6 server_addr2, client_addr2;
				int clientlen2 = sizeof(client_addr2);

				int server_fd2 = socket(AF_INET6, SOCK_DGRAM, 0);

				if (server_fd2 < 0)
				{
					perror("socket");
					exit(1);
				}

				memset(&server_addr2, 0, sizeof(server_addr2));
				memset(&client_addr2, 0, sizeof(client_addr2));

				server_addr2.sin6_family = AF_INET6;
				server_addr2.sin6_port = htons(atoi(port) + 1);
				server_addr2.sin6_addr = in6addr_any;

				if (setsockopt(server_fd2, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
				{
					perror("setsockopt");
					exit(1);
				}

				if (bind(server_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
				{
					perror("bind");
					exit(1);
				}

				if (!quiet)
					printf("Waiting for the client to send data\n");

				
				struct pollfd fds[1];

				fds[0].fd = server_fd2;
				fds[0].events = POLLIN;

				send(client_fd, "ACK", 3, 0);

				gettimeofday(&start, NULL);

				while (bytes_received2 < FILE_SIZE)
				{
					int ret = poll(fds, 1, 1000);

					if (ret < 0)
					{
						perror("poll");
						exit(1);
					}

					else if (ret == 0)
					{
						if (!quiet)
							printf("Timeout\n");

						break;
					}

					if (fds[0].revents & POLLIN)
					{
						int chunk_size = FILE_SIZE - bytes_received2 < UDP_CHUNK_SIZE ? FILE_SIZE - bytes_received2 : UDP_CHUNK_SIZE;

						int bytes = recvfrom(server_fd2, data + bytes_received2, chunk_size, 0, (struct sockaddr *)&client_addr2, (socklen_t *)&clientlen2);
						gettimeofday(&end, NULL);

						if (bytes < 0)
						{
							perror("recvfrom");
							exit(1);
						}

						bytes_received2 += bytes;
					}
				}

				close(server_fd2);

				break;
			}

			case 4:
			{
				struct sockaddr_un server_addr2, client_addr2;
				int client_fd2;

				int server_fd2 = socket(AF_UNIX, SOCK_STREAM, 0);

				if (server_fd2 < 0)
				{
					perror("socket");
					exit(1);
				}

				memset(&server_addr2, 0, sizeof(server_addr2));
				memset(&client_addr2, 0, sizeof(client_addr2));

				server_addr2.sun_family = AF_UNIX;
				strcpy(server_addr2.sun_path, UDS_NAME);
				unlink(UDS_NAME);

				int len = sizeof(server_addr2.sun_family) + strlen(server_addr2.sun_path);

				if (bind(server_fd2, (struct sockaddr *)&server_addr2, len) < 0)
				{
					perror("bind");
					exit(1);
				}

				if (listen(server_fd2, 1) < 0)
				{
					perror("listen");
					exit(1);
				}

				send(client_fd, "ACK", 3, 0);

				client_fd2 = accept(server_fd2, (struct sockaddr *)&client_addr2, (socklen_t *)&len);

				if (client_fd2 < 0)
				{
					perror("accept");
					exit(1);
				}

				close(server_fd2);

				if (!quiet)
					printf("Accepted connection\n");

				gettimeofday(&start, NULL);

				while (bytes_received2 < FILE_SIZE)
				{
					int chunk_size = FILE_SIZE - bytes_received2 < CHUNK_SIZE ? FILE_SIZE - bytes_received2 : CHUNK_SIZE;

					int bytes = recv(client_fd2, data + bytes_received2, chunk_size, 0);
					gettimeofday(&end, NULL);

					if (bytes < 0)
					{
						perror("recv");
						exit(1);
					}

					bytes_received2 += bytes;
				}

				close(client_fd2);

				break;
			}

			case 5:
			{
				struct sockaddr_un server_addr2, client_addr2;

				int server_fd2 = socket(AF_UNIX, SOCK_DGRAM, 0);

				if (server_fd2 < 0)
				{
					perror("socket");
					exit(1);
				}

				memset(&server_addr2, 0, sizeof(server_addr2));
				memset(&client_addr2, 0, sizeof(client_addr2));

				server_addr2.sun_family = AF_UNIX;
				strcpy(server_addr2.sun_path, UDS_NAME);
				unlink(UDS_NAME);

				int len = sizeof(server_addr2.sun_family) + strlen(server_addr2.sun_path);
				int len2 = 0;

				if (bind(server_fd2, (struct sockaddr *)&server_addr2, len) < 0)
				{
					perror("bind");
					exit(1);
				}

				send(client_fd, "ACK", 3, 0);

				struct pollfd fds[1];

				fds[0].fd = server_fd2;
				fds[0].events = POLLIN;

				gettimeofday(&start, NULL);

				while (bytes_received2 < FILE_SIZE)
				{
					int ret = poll(fds, 1, 1000);

					if (ret < 0)
					{
						perror("poll");
						exit(1);
					}

					else if (ret == 0)
					{
						if (!quiet)
							printf("Timeout\n");

						break;
					}

					if (fds[0].revents & POLLIN)
					{
						int chunk_size = FILE_SIZE - bytes_received2 < CHUNK_SIZE ? FILE_SIZE - bytes_received2 : CHUNK_SIZE;

						int bytes = recvfrom(server_fd2, data + bytes_received2, chunk_size, 0, (struct sockaddr *)&client_addr2, (socklen_t *)&len2);
						gettimeofday(&end, NULL);

						if (bytes < 0)
						{
							perror("recvfrom");
							exit(1);
						}

						bytes_received2 += bytes;
					}
				}

				close(server_fd2);
				
				break;
			}

			case 6:
			{
				int tmp = 0;
				send(client_fd, "ACK", 3, 0);
				recv(client_fd, &tmp, sizeof(tmp), 0);

				sleep(1);

				int fd = open(param2, O_RDONLY);
				if (fd < 0)
				{
					perror("open");
					exit(1);
				}

				char *mmap_data = mmap(NULL, FILE_SIZE, PROT_READ, MAP_SHARED, fd, 0);

				if (mmap_data == MAP_FAILED)
				{
					perror("mmap");
					exit(1);
				}

				gettimeofday(&start, NULL);
				memcpy(data, mmap_data, FILE_SIZE);
				gettimeofday(&end, NULL);

				munmap(mmap_data, FILE_SIZE);

				close(fd);

				unlink(param2);

				bytes_received2 = FILE_SIZE;

				break;
			}

			case 7:
			{
				send(client_fd, "ACK", 3, 0);
				sleep(1);

                if (!quiet)
				    printf("Waiting for file...\n");

				int fd = open(param2, O_RDONLY);

				if (fd < 0)
				{
					perror("open");
					exit(1);
				}

				gettimeofday(&start, NULL);

				while (bytes_received2 < FILE_SIZE)
				{
					int chunk_size = FILE_SIZE - bytes_received2 < CHUNK_SIZE ? FILE_SIZE - bytes_received2 : CHUNK_SIZE;

					int bytes = read(fd, data + bytes_received2, chunk_size);
					gettimeofday(&end, NULL);

					if (bytes < 0)
					{
						perror("read");
						exit(1);
					}

					bytes_received2 += bytes;
				}

				close(fd);

				unlink(param2);

				break;
			}

			default:
				break;
		}

		float time_taken = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

		char *md5sum = md5hash(data);

		if (!quiet)
		{
			printf("Received %d bytes\n", bytes_received2);
			printf("Time taken: %f ms\n", time_taken);
			printf("Throughput: %f MB/s\n", (bytes_received2 / time_taken) * 1000.0 / (1024.0 * 1024.0));

			if (strcmp(md5sum, param3) != 0)
			{
				printf("MD5 mismatch\n");
			}

			else
			{
				printf("MD5 OK\n");
			}
		}

		else
		{
			if (strcmp(md5sum, param3) == 0)
			{
				if (mode < 6)
				{
					printf("%s_%s,%d\n", param1, param2, (int)time_taken);
				}

				else
				{
					printf("%s,%d\n", param1, (int)time_taken);
				}
			}

			else
			{
				printf("error: not all bytes received: %d/%d bytes (%0.2f%%), ", bytes_received2, FILE_SIZE, (float)bytes_received2 / FILE_SIZE * 100.0);

				if (mode < 6)
				{
					printf("%s_%s,%d\n", param1, param2, (int)time_taken);
				}

				else
				{
					printf("%s,%d\n", param1, (int)time_taken);
				}
			}
		}

		free(md5sum);

		close(client_fd);
	}

	close(server_fd);

}