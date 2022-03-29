#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

#define MAX_LENGTH 1024

int listing(const char* dirPath, off_t size, int permExecute) {
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	char fullPath[MAX_LENGTH];
	struct stat statbuf;
	
	dir = opendir(dirPath);
	if(dir == NULL) {
		printf("ERROR\n");
		perror("Reason");
		return -1;
	}
	printf("SUCCESS\n");
	while((entry = readdir(dir)) != NULL) {
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			snprintf(fullPath, MAX_LENGTH, "%s/%s", dirPath, entry->d_name);
			if(lstat(fullPath, &statbuf) == 0) {
				if(S_ISREG(statbuf.st_mode) && size != -1 && permExecute == 0) {
					if(statbuf.st_size > size) {
						printf("%s\n", fullPath);
					}
				} else if(permExecute == 1 && size == -1) {
					if(statbuf.st_mode & S_IXUSR) {
						printf("%s\n", fullPath);
					}
				} else if(size == -1 && permExecute == 0) {
					printf("%s\n", fullPath);
				}
			}
		}
	}
	closedir(dir);
	return 0;
}

int recursiveListing(const char* dirPath, int success, off_t size, int permExecute) {
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	char fullPath[MAX_LENGTH];
	struct stat statbuf;
	
	dir = opendir(dirPath);
	if(dir == NULL) {
		printf("ERROR\n");
		perror("Reason");
		return -1;
	}
	if(success == 0) {
		printf("SUCCESS\n");
		success += 1;
	}
	while((entry = readdir(dir)) != NULL) {
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			snprintf(fullPath, MAX_LENGTH, "%s/%s", dirPath, entry->d_name);
			if(lstat(fullPath, &statbuf) == 0) {
				if(size == -1 && permExecute == 0) {
					printf("%s\n", fullPath);
					if(S_ISDIR(statbuf.st_mode)) {
						recursiveListing(fullPath, success, size, permExecute);
					}
				} else if(size != -1 && permExecute == 0) {
					if(S_ISREG(statbuf.st_mode)) {
						if(statbuf.st_size > size) {
							printf("%s\n", fullPath);
						}
					} else if(S_ISDIR(statbuf.st_mode)) {
						recursiveListing(fullPath, success, size, permExecute);
					}
				} else if(size == -1 && permExecute == 1) {
					if(statbuf.st_mode & S_IXUSR) {
						printf("%s\n", fullPath);
					}
					if(S_ISDIR(statbuf.st_mode)) {
						recursiveListing(fullPath, success, size, permExecute);
					}
				}
			}
		}
	}
	closedir(dir);
	return 0;
}

int parseValidation(const char* filePath, int parse, int find) {
	int version = 0, no_of_sections = 0, type = 0, size;
	char name[17], magic[2];
	int fd = open(filePath, O_RDONLY);
	if(fd == -1)
	{
		printf("ERROR\n");
		perror("Reason");
		return -1;
	}
	if(find != 1) {
		lseek(fd, 0, SEEK_SET);
		if(read(fd, magic, 1) != 1) {
			printf("ERROR\n");
			perror("Reason");
			close(fd);
			return -2;
		}
		magic[1] = '\0';
		if(strcmp(magic, "6") != 0) {
			printf("ERROR\n");
			printf("wrong magic\n");
			close(fd);
			return -3;
		}
		lseek(fd, 3, SEEK_SET);
		if(read(fd, &version, 2) != 2) {
			printf("ERROR\n");
			perror("Reason");
			close(fd);
			return -4;
		}
		if(version < 121 || version > 198) {
			printf("ERROR\n");
			printf("wrong version\n");
			close(fd);
			return -5;
		}
		if(read(fd, &no_of_sections, 1) != 1) {
			printf("ERROR\n");
			perror("Reason");
			close(fd);
			return -6;
		}
		if(no_of_sections < 5 || no_of_sections > 16) {
			printf("ERROR\n");
			printf("wrong sect_nr\n");
			close(fd);
			return -7;
		}
		lseek(fd, 22, SEEK_SET);
		int current = 22;
		for(int i = 1; i <= no_of_sections; i++) {
			if(read(fd, &type, 2) != 2) {
				printf("ERROR\n");
				perror("Reason");
				close(fd);
				return -8;
			}
			if(type != 59 && type != 66 && type != 76 && type != 40) {
				printf("ERROR\n");
				printf("wrong sect_types\n");
				close(fd);
				return -9;
			}
			current += 26;
			lseek(fd, current, SEEK_SET);
		}
		if(parse == 1) {
			printf("SUCCESS\n");
			printf("version=%d\n", version);
			printf("nr_sections=%d\n", no_of_sections);
			lseek(fd, 6, SEEK_SET);
			current = 6;
			for(int i = 1; i <= no_of_sections; i++) {
				read(fd, name, 16);
				name[16] = '\0';
				read(fd, &type, 2);
				current += 22;
				lseek(fd, current, SEEK_SET);
				read(fd, &size, 4);
				printf("section%d: %s %d %d\n", i, name, type, size);
				current += 4;
			}
		}
	} else if(find == 1) {
		lseek(fd, 0, SEEK_SET);
		if(read(fd, magic, 1) != 1) {
			close(fd);
			return -2;
		}
		magic[1] = '\0';
		if(strcmp(magic, "6") != 0) {
			close(fd);
			return -3;
		}
		lseek(fd, 3, SEEK_SET);
		if(read(fd, &version, 2) != 2) {
			close(fd);
			return -4;
		}
		if(version < 121 || version > 198) {
			close(fd);
			return -5;
		}
		if(read(fd, &no_of_sections, 1) != 1) {
			close(fd);
			return -6;
		}
		if(no_of_sections < 5 || no_of_sections > 16) {
			close(fd);
			return -7;
		}
		lseek(fd, 22, SEEK_SET);
		int current = 22;
		for(int i = 1; i <= no_of_sections; i++) {
			if(read(fd, &type, 2) != 2) {
				close(fd);
				return -8;
			}
			if(type != 59 && type != 66 && type != 76 && type != 40) {
				close(fd);
				return -9;
			}
			current += 26;
			lseek(fd, current, SEEK_SET);
		}
		int offset, size, currentLine = 1, sections = 0;
		lseek(fd, 5, SEEK_SET);
		read(fd, &no_of_sections, 1);
		current = 6;
		for(int i = 1; i <= no_of_sections; i++) {
			current += 18;
			read(fd, &offset, 4);
			read(fd, &size, 4);
			current += 8;
			lseek(fd, offset, SEEK_SET);
			struct stat statbuf;
			char *buffer;
			if(lstat(filePath, &statbuf) == 0) {
				buffer = (char*)malloc((statbuf.st_size + 1) * sizeof(char));
				if(read(fd, buffer, statbuf.st_size) == -1) {
					printf("ERROR\n");
					perror("Reason");
					free(buffer);
					close(fd);
					return -3;
				}
				buffer[statbuf.st_size] = '\0';
			}
			for(int i = 1; i <= statbuf.st_size; i++) {
				if(buffer[i - 1] == '\n') {
					currentLine += 1;
				}
				if(currentLine == 15) {
					break;
				}
			}
			free(buffer);
			if(currentLine == 14) {
				sections += 1;
			}
			if(sections == 2) {
				break;
			}
			lseek(fd, current, SEEK_SET);
		}
		if(sections != 2) {
			close(fd);
			return -10;
		}
	}
	close(fd);
	return 0;
}

int extractProcess(const char* filePath, int section, int line) {
	int offset, size, currentSection = 1, currentLine = 1, no_of_sections = 0;
	int fd = open(filePath, O_RDONLY);
	if(fd == -1)
	{
		printf("ERROR\n");
		perror("Reason");
		return -1;
	}
	lseek(fd, 5, SEEK_SET);
	read(fd, &no_of_sections, 1);
	if(no_of_sections < section) {
		printf("ERROR\n");
		printf("invalid section\n");
		close(fd);
        	return -2;
	}
	int current = 6;
	while(currentSection != section) {
		current += 26;
		lseek(fd, current, SEEK_SET);
		currentSection += 1;
	}
	current += 18;
	lseek(fd, current, SEEK_SET);
	read(fd, &offset, 4);
	read(fd, &size, 4);
	lseek(fd, offset, SEEK_SET);
	struct stat statbuf;
	char *buffer;
	if(lstat(filePath, &statbuf) == 0) {
		buffer = (char*)malloc(statbuf.st_size * sizeof(char));
		if(read(fd, buffer, statbuf.st_size) == -1) {
			printf("ERROR\n");
			perror("Reason");
			free(buffer);
			close(fd);
			return -3;
		}
		buffer[statbuf.st_size] = '\0';
	}
	int *lineLength = (int*)calloc(MAX_LENGTH, sizeof(int));
	for(int i = 1; i <= size; i++) {
		lineLength[currentLine - 1] += 1;
		if(buffer[i - 1] == '\n') {
			currentLine += 1;
		}
	}
	if(currentLine < line) {
		printf("ERROR\n");
		printf("invalid line\n");
		free(lineLength);
		free(buffer);
		close(fd);
        	return -4;
	}
	int searchedLine = currentLine - line + 1;
	current = 0;
	for(int i = 1; i < searchedLine; i++) {
		current += lineLength[i - 1];
	}
	char *lineResult = (char*)malloc((lineLength[searchedLine - 1] + 1) * sizeof(char));
	for(int i = lineLength[searchedLine - 1]; i > 0; i--) {
		lineResult[i - 1] = buffer[current];
		current += 1;
	}
	lineResult[lineLength[searchedLine - 1]] = '\0';
	printf("SUCCESS");
	if(searchedLine == currentLine) {
		printf("\n");
	}
	printf("%s\n", lineResult);
	free(lineResult);
	free(lineLength);
	free(buffer);
	close(fd);
	return 0;
}

int findAllFiles(const char* dirPath, int success) {
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	char fullPath[MAX_LENGTH];
	struct stat statbuf;
	
	dir = opendir(dirPath);
	if(dir == NULL) {
		printf("ERROR\n");
		perror("Reason");
		return -1;
	}
	if(success == 0) {
		printf("SUCCESS\n");
		success += 1;
	}
	while((entry = readdir(dir)) != NULL) {
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			snprintf(fullPath, MAX_LENGTH, "%s/%s", dirPath, entry->d_name);
			if(lstat(fullPath, &statbuf) == 0) {
				if(S_ISDIR(statbuf.st_mode)) {
					findAllFiles(fullPath, success);
				} else if(S_ISREG(statbuf.st_mode)) {
					if(parseValidation(fullPath, 0, 1) == 0) {
						printf("%s\n", fullPath);
					}
				}
			}
		}
	}
	closedir(dir);
	return 0;
}

int main(int argc, char **argv) {
	if(argc == 2) {
		if(strcmp(argv[1], "variant") == 0) {
			printf("76660\n");
        	} else {
        		printf("Invalid! Try again!\n");
        		return -1;
        	}
	} else if(argc == 3) {
		if(strcmp(argv[1], "list") == 0 && strncmp(argv[2], "path=", 5) == 0) {
			char dirPath[MAX_LENGTH];
			for(int i = 0; i < strlen(argv[2]) - 5; i++) {
				dirPath[i] = argv[2][i + 5];
			}
			if(dirPath[strlen(argv[2]) - 6] == '/') {
				dirPath[strlen(argv[2]) - 6] = '\0';
			} else {
				dirPath[strlen(argv[2]) - 5] = '\0';
			}
			listing(dirPath, -1, 0);
		} else if(strcmp(argv[1], "parse") == 0 && strncmp(argv[2], "path=", 5) == 0) {
			char filePath[MAX_LENGTH];
			for(int i = 0; i < strlen(argv[2]) - 5; i++) {
				filePath[i] = argv[2][i + 5];
			}
			if(filePath[strlen(argv[2]) - 6] == '/') {
				filePath[strlen(argv[2]) - 6] = '\0';
			} else {
				filePath[strlen(argv[2]) - 5] = '\0';
			}
			parseValidation(filePath, 1, 0);
		} else if(strcmp(argv[1], "findall") == 0 && strncmp(argv[2], "path=", 5) == 0) {
			char dirPath[MAX_LENGTH];
			for(int i = 0; i < strlen(argv[2]) - 5; i++) {
				dirPath[i] = argv[2][i + 5];
			}
			if(dirPath[strlen(argv[2]) - 6] == '/') {
				dirPath[strlen(argv[2]) - 6] = '\0';
			} else {
				dirPath[strlen(argv[2]) - 5] = '\0';
			}
			findAllFiles(dirPath, 0);
		} else {
        		printf("Invalid! Try again!\n");
        		return -2;
        	}
	} else if(argc == 4) {
		if(strcmp(argv[1], "list") == 0) {
			if((strncmp(argv[2], "path=", 5) == 0 && strcmp(argv[3], "recursive") == 0) || (strncmp(argv[3], "path=", 5) == 0 && strcmp(argv[2], "recursive") == 0)) {
				char dirPath[MAX_LENGTH];
				if(strncmp(argv[2], "path=", 5) == 0) {
					for(int i = 0; i < strlen(argv[2]) - 5; i++) {
						dirPath[i] = argv[2][i + 5];
					}
					if(dirPath[strlen(argv[2]) - 6] == '/') {
						dirPath[strlen(argv[2]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[2]) - 5] = '\0';
					}
				} else {
					for(int i = 0; i < strlen(argv[3]) - 5; i++) {
						dirPath[i] = argv[3][i + 5];
					}
					if(dirPath[strlen(argv[3]) - 6] == '/') {
						dirPath[strlen(argv[3]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[3]) - 5] = '\0';
					}
				}
				recursiveListing(dirPath, 0, -1, 0);
			} else if((strncmp(argv[2], "path=", 5) == 0 && strncmp(argv[3], "size_greater=", 13) == 0) || (strncmp(argv[3], "path=", 5) == 0 && strncmp(argv[2], "size_greater=", 13) == 0)) {
				char dirPath[MAX_LENGTH];
				char fileSize[MAX_LENGTH];
				int breakDigit = 0;
				if(strncmp(argv[2], "path=", 5) == 0) {
					for(int i = 0; i < strlen(argv[2]) - 5; i++) {
						dirPath[i] = argv[2][i + 5];
					}
					for(int i = 0; i < strlen(argv[3]) - 13; i++) {
						if(isdigit(argv[3][i + 13]) == 0) {
							breakDigit = 1;
							break;
						}
						fileSize[i] = argv[3][i + 13];
					}
					if(breakDigit != 1) {
						fileSize[strlen(argv[3]) - 13] = '\0';
					} else {
						printf("Invalid! Try again!\n");
        					return -3;
					}
					if(dirPath[strlen(argv[2]) - 6] == '/') {
						dirPath[strlen(argv[2]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[2]) - 5] = '\0';
					}
				} else {
					for(int i = 0; i < strlen(argv[3]) - 5; i++) {
						dirPath[i] = argv[3][i + 5];
					}
					for(int i = 0; i < strlen(argv[2]) - 13; i++) {
						if(isdigit(argv[2][i + 13]) == 0) {
							breakDigit = 1;
							break;
						}
						fileSize[i] = argv[2][i + 13];
					}
					if(breakDigit != 1) {
						fileSize[strlen(argv[2]) - 13] = '\0';
					} else {
						printf("Invalid! Try again!\n");
        					return -4;
					}
					if(dirPath[strlen(argv[3]) - 6] == '/') {
						dirPath[strlen(argv[3]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[3]) - 5] = '\0';
					}
				}
				off_t size = atoi(fileSize);
				listing(dirPath, size, 0);
			} else if((strncmp(argv[2], "path=", 5) == 0 && strcmp(argv[3], "has_perm_execute") == 0) || (strncmp(argv[3], "path=", 5) == 0 && strcmp(argv[2], "has_perm_execute") == 0)) {
				char dirPath[MAX_LENGTH];
				if(strncmp(argv[2], "path=", 5) == 0) {
					for(int i = 0; i < strlen(argv[2]) - 5; i++) {
						dirPath[i] = argv[2][i + 5];
					}
					if(dirPath[strlen(argv[2]) - 6] == '/') {
						dirPath[strlen(argv[2]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[2]) - 5] = '\0';
					}
				} else {
					for(int i = 0; i < strlen(argv[3]) - 5; i++) {
						dirPath[i] = argv[3][i + 5];
					}
					if(dirPath[strlen(argv[3]) - 6] == '/') {
						dirPath[strlen(argv[3]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[3]) - 5] = '\0';
					}
				}
				listing(dirPath, -1, 1);
			} else {
        			printf("Invalid! Try again!\n");
        			return -5;
        		}	
		} else {
        		printf("Invalid! Try again!\n");
        		return -6;
        	}
	} else if (argc == 5) {
		if(strcmp(argv[1], "list") == 0) {
			if((strcmp(argv[2], "recursive") == 0 && ((strncmp(argv[3], "path=", 5) == 0 && strncmp(argv[4], "size_greater=", 13) == 0) || (strncmp(argv[4], "path=", 5) == 0 && strncmp(argv[3], "size_greater=", 13) == 0))) || (strcmp(argv[3], "recursive") == 0 && ((strncmp(argv[2], "path=", 5) == 0 && strncmp(argv[4], "size_greater=", 13) == 0) || (strncmp(argv[4], "path=", 5) == 0 && strncmp(argv[2], "size_greater=", 13) == 0))) || (strcmp(argv[4], "recursive") == 0 && ((strncmp(argv[3], "path=", 5) == 0 && strncmp(argv[2], "size_greater=", 13) == 0) || (strncmp(argv[2], "path=", 5) == 0 && strncmp(argv[3], "size_greater=", 13) == 0)))) {
				char dirPath[MAX_LENGTH];
				char fileSize[MAX_LENGTH];
				int breakDigit = 0;
				if(strncmp(argv[2], "path=", 5) == 0) {
					if(strncmp(argv[3], "size_greater=", 13) == 0) {
						for(int i = 0; i < strlen(argv[2]) - 5; i++) {
							dirPath[i] = argv[2][i + 5];
						}
						for(int i = 0; i < strlen(argv[3]) - 13; i++) {
							if(isdigit(argv[3][i + 13]) == 0) {
								breakDigit = 1;
								break;
							}
							fileSize[i] = argv[3][i + 13];
						}
						if(breakDigit != 1) {
							fileSize[strlen(argv[3]) - 13] = '\0';
						} else {
							printf("Invalid! Try again!\n");
        						return -3;
						}
						if(dirPath[strlen(argv[2]) - 6] == '/') {
							dirPath[strlen(argv[2]) - 6] = '\0';
						} else {
							dirPath[strlen(argv[2]) - 5] = '\0';
						}	
					} else {
						for(int i = 0; i < strlen(argv[2]) - 5; i++) {
							dirPath[i] = argv[2][i + 5];
						}
						for(int i = 0; i < strlen(argv[4]) - 13; i++) {
							if(isdigit(argv[4][i + 13]) == 0) {
								breakDigit = 1;
								break;
							}
							fileSize[i] = argv[4][i + 13];
						}
						if(breakDigit != 1) {
							fileSize[strlen(argv[4]) - 13] = '\0';
						} else {
							printf("Invalid! Try again!\n");
        						return -3;
						}
						if(dirPath[strlen(argv[2]) - 6] == '/') {
							dirPath[strlen(argv[2]) - 6] = '\0';
						} else {
							dirPath[strlen(argv[2]) - 5] = '\0';
						}
					}
				} else if(strncmp(argv[3], "path=", 5) == 0) {
					if(strncmp(argv[2], "size_greater=", 13) == 0) {
						for(int i = 0; i < strlen(argv[3]) - 5; i++) {
							dirPath[i] = argv[3][i + 5];
						}
						for(int i = 0; i < strlen(argv[2]) - 13; i++) {
							if(isdigit(argv[2][i + 13]) == 0) {
								breakDigit = 1;
								break;
							}
							fileSize[i] = argv[2][i + 13];
						}
						if(breakDigit != 1) {
							fileSize[strlen(argv[2]) - 13] = '\0';
						} else {
							printf("Invalid! Try again!\n");
        						return -3;
						}
						if(dirPath[strlen(argv[3]) - 6] == '/') {
							dirPath[strlen(argv[3]) - 6] = '\0';
						} else {
							dirPath[strlen(argv[3]) - 5] = '\0';
						}	
					} else {
						for(int i = 0; i < strlen(argv[3]) - 5; i++) {
							dirPath[i] = argv[3][i + 5];
						}
						for(int i = 0; i < strlen(argv[4]) - 13; i++) {
							if(isdigit(argv[4][i + 13]) == 0) {
								breakDigit = 1;
								break;
							}
							fileSize[i] = argv[4][i + 13];
						}
						if(breakDigit != 1) {
							fileSize[strlen(argv[4]) - 13] = '\0';
						} else {
							printf("Invalid! Try again!\n");
        						return -3;
						}
						if(dirPath[strlen(argv[3]) - 6] == '/') {
							dirPath[strlen(argv[3]) - 6] = '\0';
						} else {
							dirPath[strlen(argv[3]) - 5] = '\0';
						}
					}
				} else {
					if(strncmp(argv[3], "size_greater=", 13) == 0) {
						for(int i = 0; i < strlen(argv[4]) - 5; i++) {
							dirPath[i] = argv[4][i + 5];
						}
						for(int i = 0; i < strlen(argv[3]) - 13; i++) {
							if(isdigit(argv[3][i + 13]) == 0) {
								breakDigit = 1;
								break;
							}
							fileSize[i] = argv[3][i + 13];
						}
						if(breakDigit != 1) {
							fileSize[strlen(argv[3]) - 13] = '\0';
						} else {
							printf("Invalid! Try again!\n");
        						return -3;
						}
						if(dirPath[strlen(argv[4]) - 6] == '/') {
							dirPath[strlen(argv[4]) - 6] = '\0';
						} else {
							dirPath[strlen(argv[4]) - 5] = '\0';
						}	
					} else {
						for(int i = 0; i < strlen(argv[4]) - 5; i++) {
							dirPath[i] = argv[4][i + 5];
						}
						for(int i = 0; i < strlen(argv[2]) - 13; i++) {
							if(isdigit(argv[2][i + 13]) == 0) {
								breakDigit = 1;
								break;
							}
							fileSize[i] = argv[2][i + 13];
						}
						if(breakDigit != 1) {
							fileSize[strlen(argv[2]) - 13] = '\0';
						} else {
							printf("Invalid! Try again!\n");
        						return -3;
						}
						if(dirPath[strlen(argv[4]) - 6] == '/') {
							dirPath[strlen(argv[4]) - 6] = '\0';
						} else {
							dirPath[strlen(argv[4]) - 5] = '\0';
						}
					}
				}
				off_t size = atoi(fileSize);
				recursiveListing(dirPath, 0, size, 0);
			} else if((strcmp(argv[2], "recursive") == 0 && ((strncmp(argv[3], "path=", 5) == 0 && strcmp(argv[4], "has_perm_execute") == 0) || (strncmp(argv[4], "path=", 5) == 0 && strcmp(argv[3], "has_perm_execute") == 0))) || (strcmp(argv[3], "recursive") == 0 && ((strncmp(argv[2], "path=", 5) == 0 && strcmp(argv[4], "has_perm_execute") == 0) || (strncmp(argv[4], "path=", 5) == 0 && strcmp(argv[2], "has_perm_execute") == 0))) || (strcmp(argv[4], "recursive") == 0 && ((strncmp(argv[3], "path=", 5) == 0 && strcmp(argv[2], "has_perm_execute") == 0) || (strncmp(argv[2], "path=", 5) == 0 && strcmp(argv[3], "has_perm_execute") == 0)))) {
				char dirPath[MAX_LENGTH];
				if(strncmp(argv[2], "path=", 5) == 0) {
					for(int i = 0; i < strlen(argv[2]) - 5; i++) {
						dirPath[i] = argv[2][i + 5];
					}
					if(dirPath[strlen(argv[2]) - 6] == '/') {
						dirPath[strlen(argv[2]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[2]) - 5] = '\0';
					}
				} else if(strncmp(argv[3], "path=", 5) == 0){
					for(int i = 0; i < strlen(argv[3]) - 5; i++) {
						dirPath[i] = argv[3][i + 5];
					}
					if(dirPath[strlen(argv[3]) - 6] == '/') {
						dirPath[strlen(argv[3]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[3]) - 5] = '\0';
					}
				} else {
					for(int i = 0; i < strlen(argv[4]) - 5; i++) {
						dirPath[i] = argv[4][i + 5];
					}
					if(dirPath[strlen(argv[4]) - 6] == '/') {
						dirPath[strlen(argv[4]) - 6] = '\0';
					} else {
						dirPath[strlen(argv[4]) - 5] = '\0';
					}
				}
				recursiveListing(dirPath, 0, -1, 1);
			} else {
  	      		printf("Invalid! Try again!\n");
        			return -4;
        		}
		} else if(strcmp(argv[1], "extract") == 0 && strncmp(argv[2], "path=", 5) == 0 && strncmp(argv[3], "section=", 8) == 0 && strncmp(argv[4], "line=", 5) == 0) {
			char filePath[MAX_LENGTH];
			for(int i = 0; i < strlen(argv[2]) - 5; i++) {
				filePath[i] = argv[2][i + 5];
			}
			if(filePath[strlen(argv[2]) - 6] == '/') {
				filePath[strlen(argv[2]) - 6] = '\0';
			} else {
				filePath[strlen(argv[2]) - 5] = '\0';
			}
			if(parseValidation(filePath, 0, 0) != 0) {
				printf("ERROR\n");
				printf("invalid file\n");
				return -6;
			}
			int breakDigit = 0;
			char sectionNumber[MAX_LENGTH];
			for(int i = 0; i < strlen(argv[3]) - 8; i++) {
				if(isdigit(argv[3][i + 8]) == 0) {
					breakDigit = 1;
					break;
				}
				sectionNumber[i] = argv[3][i + 8];
			}
			if(breakDigit != 1) {
				sectionNumber[strlen(argv[3]) - 8] = '\0';
			} else {
				printf("ERROR\n");
				printf("invalid section\n");
        			return -7;
			}
			int sectNo = atoi(sectionNumber);
			char lineNumber[MAX_LENGTH];
			for(int i = 0; i < strlen(argv[4]) - 5; i++) {
				if(isdigit(argv[4][i + 5]) == 0) {
					breakDigit = 1;
					break;
				}
				lineNumber[i] = argv[4][i + 5];
			}
			if(breakDigit != 1) {
				lineNumber[strlen(argv[4]) - 5] = '\0';
			} else {
				printf("ERROR\n");
				printf("invalid line\n");
        			return -8;
			}
			int lnNo = atoi(lineNumber);
			extractProcess(filePath, sectNo, lnNo);
		} else {
        		printf("Invalid! Try again!\n");
        		return -9;
        	}
	}
	return 0;
}
