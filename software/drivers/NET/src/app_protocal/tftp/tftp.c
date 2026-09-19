

/* don't use BSD socket now */
#include "el_udp.h"

#define TFTP_MAX_FILE_SIZE (1 << 25)U

#define TFTP_OPCODE_READ_REQ 1
#define TFTP_OPCODE_WRITE_REQ 2
#define TFTP_OPCODE_DATA 3
#define TFTP_OPCODE_ACK 4
#define TFTP_OPCODE_ERROR 5

/* tftp request mode string(file format?) */
static const char * tftp_req_mode_str[] = {
    #define TFTP_REQ_MODE_STR(x) #x,
    [0] = TFTP_REQ_MODE_STR(netascii)
    [1] = TFTP_REQ_MODE_STR(octet)
    [2] = TFTP_REQ_MODE_STR(mail)
    [3] = NULL
    #undef TFTP_REQ_MODE_STR
};

typedef struct{
    uint16_t opcode;
}tftp_hdr_t;

typedef struct
{
    tftp_hdr_t hdr;
    const char * file_name;
    char dummy;
    const char * mode;
    char dummy2;
}tftp_req_t;

typedef struct{
    tftp_hdr_t hdr;
    uint16_t block;
    char data[];
}tftp_data_t;

typedef struct{
    tftp_hdr_t hdr;
    uint16_t block;
}tftp_ack_t;

typedef enum{
    TFTP_ERR_CODE_NOT_DEFINED = 0,
    TFTP_ERR_CODE_FILE_NOT_FOUND = 1,
    TFTP_ERR_CODE_ACCESS_VIOLATION = 2,
    TFTP_ERR_CODE_DISK_FULL = 3,
    TFTP_ERR_CODE_ILLEGAL_OPERATION = 4,
    TFTP_ERR_CODE_UNKNOWN_TID = 5,
    TFTP_ERR_CODE_FILE_EXISTS = 6,
    TFTP_ERR_CODE_NO_SUCH_USER = 7
}tftp_err_code_t;

typedef struct{
    tftp_hdr_t hdr;
    tftp_err_code_t err_code;
    const char * err_msg;
    char dummy;
}tftp_err_t;

udp_t * u_s = NULL;
net_err_t tftp_server_init(void)
{
    u_s = udp_create();
    if( u_s == NULL )
        goto _error;

    /* tftp server usually bind to 0.0.0.0 */
    endpoint_t p = {.ip4addr.ipv = 0, .port = TFTP_PORT};
    if(NET_ERR_NOK == udp_bind(u_s, &p))
        goto _error;
    return NET_ERR_OK;
_error:
    if( u_s ) udp_destroy(u_s);
    return NET_ERR_NOK;
}

net_err_t tftp_server_start(void)
{
    ip4addr_t from;
    uint16_t port, len;
    static char server_buffer[600];
    tftp_hdr_t * hdr;
    while(1)
    {    
        if(NET_ERR_OK != udp_recvfrom(u_s, server_buffer, 600, &from, &port, &len))
            break;
        hdr = (tftp_hdr_t *)server_buffer;
        /* parse request */
        switch(_htons(hdr->opcode))
        {
        case TFTP_OPCODE_READ_REQ: 
            break;
        case TFTP_OPCODE_WRITE_REQ: 
            break;
        case TFTP_OPCODE_DATA: 
            break;
        case TFTP_OPCODE_ACK: 
            break;
        case TFTP_OPCODE_ERROR: 
            break;
        default:
            /* not such code err return */
            break;
        }
    }
    return NET_ERR_NOK;
}

/* tftp put file request */
void tftp_put()
{

}

/* tftp get file request */
void tftp_get()
{

}

/*  */
void tftp_error()
{

}