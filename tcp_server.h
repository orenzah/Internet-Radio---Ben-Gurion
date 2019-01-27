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
	
} typedef client_node;


