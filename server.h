typedef struct Client_list{
	int data;
	struct Client_list* prev;
	struct Client_list* link;
	char IP[16];
	char name[31];
}Clients;

Clients *newNode(int sockfd, char* ip){
	Clients *list = (Clients *)malloc(sizeof(Clients));
	list->data = sockfd;
	list->link = NULL;
	list->prev = NULL;
	strncpy(list->IP, ip, 16);
	strncpy(list->name, "NULL", 5);
	return list;
}

