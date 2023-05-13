#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

void client_chat(const char *ip, const char *port) {
	struct sockaddr_in server_addr;
	int client_fd;
	char buffer[1024];

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

	struct pollfd fds[2];
	fds[0].fd = client_fd;
	fds[0].events = POLLIN;

	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;

	while (1)
	{
		int num_of_hotfds = poll(fds, 2, -1);

		if (num_of_hotfds < 0)
		{
			perror("poll");
			exit(1);
		}

		if (fds[0].revents & POLLIN)
		{
			int recvv = recv(client_fd, buffer, sizeof(buffer), 0);

			if (recvv < 0)
			{
				perror("recv");
				exit(1);
			}

			else if (recvv == 0)
			{
				printf("Connection closed by server\n");
				break;
			}

			printf("Server: %s\n", buffer);
		}

		if (fds[1].revents & POLLIN)
		{
			fgets(buffer, sizeof(buffer), stdin);

			buffer[strlen(buffer) - 1] = '\0';

			if (send(client_fd, buffer, strlen(buffer), 0) < 0)
			{
				perror("send");
				exit(1);
			}
		}

		if (fds[0].revents & (POLLHUP | POLLERR))
		{
			printf("Connection closed by server\n");
			break;
		}

		memset(buffer, 0, sizeof(buffer));
	}

	close(client_fd);
}

void server_chat(const char *port) {
	struct sockaddr_in server_addr, client_addr;
	int server_fd, client_fd, reuse = 1;
	socklen_t client_len = sizeof(client_addr);
	char buffer[1024];

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0)
	{
		perror("socket");
		exit(1);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		perror("setsockopt");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	if (listen(server_fd, 5) < 0)
	{
		perror("listen");
		exit(1);
	}

	while (1)
	{
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

		if (client_fd < 0)
		{
			perror("accept");
			exit(1);
		}

		struct pollfd fds[2];
		fds[0].fd = client_fd;
		fds[0].events = POLLIN;

		fds[1].fd = STDIN_FILENO;
		fds[1].events = POLLIN;

		while (1)
		{
			int num_of_hotfds = poll(fds, 2, -1);

			if (num_of_hotfds < 0)
			{
				perror("poll");
				exit(1);
			}

			if (fds[0].revents & POLLIN)
			{
				int num_of_bytes = recv(client_fd, buffer, sizeof(buffer), 0);

				if (num_of_bytes < 0)
				{
					perror("recv");
					exit(1);
				}

				else if (num_of_bytes == 0)
				{
					printf("Connection closed by client\n");
					break;
				}

				printf("Client: %s\n", buffer);
			}

			if (fds[1].revents & POLLIN)
			{
				fgets(buffer, sizeof(buffer), stdin);

				buffer[strlen(buffer) - 1] = '\0';

				if (send(client_fd, buffer, strlen(buffer), 0) < 0)
				{
					perror("send");
					exit(1);
				}
			}

			if (fds[0].revents & POLLHUP)
			{
				printf("Connection closed by client\n");
				break;
			}

			memset(buffer, 0, sizeof(buffer));
		}

		close(client_fd);
	}

	close(server_fd);
}
