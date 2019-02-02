
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

/* predefined enums*/
enum permitEnum {no, yes};
/* predefined structs */
struct msgbox
{
	long mtype;
	char text[10];
} typedef msgbox;

struct song_node
{
	size_t	songSize;
	uint32_t nameLength;
	char* name;
	uint16_t station;
	pthread_t* thread_p;
} typedef song_node;
struct node
{
	void*	pointer;
	struct node* next;
} typedef node;

struct mymsg {
	long type;
	char* text;
};
struct welcome_msg
{
	uint8_t replyType;
	uint16_t numStations;
	uint32_t multicastGroup;
	uint16_t portNumber;
};
struct asksong_msg
{
	uint8_t replyType;
	uint16_t station_number;
};
struct announce_msg
{
	uint8_t replyType;
	uint8_t songNameSize;
	char	text[100];
} typedef announce_msg;
struct upsong_msg
{
	uint8_t		replyType;
	uint32_t	songSize;
	uint8_t		songNameSize;
	char		songName[100];
} typedef upsong_msg;
struct permit_msg
{
	uint8_t replyType;
	uint8_t permit_value;
};
struct invalid_msg
{
	uint8_t replyType;
	uint8_t replySize;
	char	text[100];
} typedef invalid_msg;
struct newstations_msg
{
	uint8_t replyType;
	uint16_t station_number;
} typedef newstations_msg;


struct client_node
{
	int fileDescriptor;
	int clientId;
	struct client_node* next;
	struct client_node* prev;
} typedef client_node;

void cascadeClient(int fd, int* id, client_node** node)
{
	client_node* head = *node;
	client_node*  temp = (client_node*)malloc(sizeof(client_node));
	temp->next = head;
	temp->prev = 0;
	temp->fileDescriptor = fd;
	
	if (id > 0)
	{
		temp->clientId		= *id;
	}
	else
	{
		temp->clientId = -1;
	}
	if (head)
	{
		head->prev = temp;
	}
	(*node) = temp;
	
	return;
}
