#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void client_chat(const char *ip, const char *port);
void server_chat(const char *port);

void client_performance(const char *ip, const char *port, const char *param1, const char *param2);
void server_performance(const char *port, bool quiet);

void print_usage(char *name) {
	printf("Part A usage:\n");
	printf("Client: %s -c IP PORT\n", name);
	printf("Server: %s -s PORT\n", name);

	printf("Part B usage:\n");
	printf("Client: %s -c IP PORT -p PARAM1 PARAM2\n", name);
	printf("Server: %s -s PORT -p [-q]\n", name);
}

int main(int argc, char *argv[]) {
	if (argc < 2)
	{
		print_usage(argv[0]);
		return 1;
	}

	else if (strcmp(argv[1],"-c") == 0)
	{
		if (argc < 4)
		{
			print_usage(argv[0]);
			return 1;
		}

		switch(argc)
		{
			case 4:
			{
				client_chat(argv[2], argv[3]);
				break;
			}

			case 7:
			{
				if (strcmp(argv[4],"-p") == 0)
				{
					client_performance(argv[2], argv[3], argv[5], argv[6]);
					break;
				}

				print_usage(argv[0]);
				return 1;
			}

			default:
			{
				print_usage(argv[0]);
				return 1;
			}
		}
	}

	else if (strcmp(argv[1],"-s") == 0)
	{
		if (argc < 3)
		{
			print_usage(argv[0]);
			return 1;
		}

		switch (argc)
		{
			case 3:
			{
				server_chat(argv[2]);
				break;
			}

			case 4:
			{
				if (strcmp(argv[3],"-p") == 0)
				{
					server_performance(argv[2], false);
					break;
				}

				print_usage(argv[0]);
				return 1;
			}

			case 5:
			{
				if (strcmp(argv[3],"-p") == 0 && strcmp(argv[4],"-q") == 0)
				{
					server_performance(argv[2], true);
					break;
				}

				print_usage(argv[0]);
				return 1;
			}

			default:
			{
				print_usage(argv[0]);
				return 1;
			}
		}
	}

	else
	{
		print_usage(argv[0]);
		return 1;
	}

	return 0;
}
