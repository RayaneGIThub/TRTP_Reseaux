#include "packet_interface.h"

/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
    const ptypes_t type;
    const uint8_t tr;
    const uint8_t window;
    const uint8_t seqnum;
    const uint16_t length;
    const uint32_t timestamp;
    const uint32_t crc1;
    const uint32_t crc2;
    const char *data;
};

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
    pkt_t *var = (pkt_t *)malloc(sizeof(pkt_t));
    return var;
}

void pkt_del(pkt_t *pkt)
{
    free(pkt);
}



/*
 * Décode des données reçues et crée une nouvelle structure pkt.
 * Le paquet reçu est en network byte-order.
 * La fonction vérifie que:
 * - Le CRC32 du header recu est le même que celui decode a la fin
 *   du header (en considerant le champ TR a 0)
 * - S'il est present, le CRC32 du payload recu est le meme que celui
 *   decode a la fin du payload
 * - Le type du paquet est valide
 * - La longueur du paquet et le champ TR sont valides et coherents
 *   avec le nombre d'octets recus.
 *
 * @data: L'ensemble d'octets constituant le paquet reçu
 * @len: Le nombre de bytes reçus
 * @pkt: Une struct pkt valide
 * @post: pkt est la représentation du paquet reçu
 *
 * @return: Un code indiquant si l'opération a réussi ou représentant
 *         l'erreur rencontrée.
 */
 
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt){


    char a = data[0] & 11000000;
    a = a >> 6;
    if (a== 0){
        *pkt.type = PTYPE_FEC;
    }
    if (a == 1){
        *pkt.type =PTYPE_DATA;
    }
    if (a == 2){
        *pkt.type =PTYPE_ACK;
    }
    if (a == 3){
        *pkt.type =PTYPE_NACK;
    }

    char b = data[0] & 00100000;
    b = b >> 5;
    *pkt.tr = b;

    char c = data[0] & 00011111;
    *pkt.tr = c;

    if (*pkt.type == PTYPE_DATA || *pkt.type == PTYPE_FEC){

        char d[2];
        d[0] = data[1];
        d[1] = data[2];
        uint16_t* ptrU16 = (uint16_t*)d;
        *pkt.length = *ptrU16;

        char e = data[3];
        *pkt.seqnum = e;

        char f[4];
        f[0] = data[4];
        f[1] = data[5];
        f[2] = data[6];
        f[3] = data[7];
        uint32_t* ptrU32 = (uint32_t*)f;
        *pkt.timestamp = *ptrU32;

        char g[4];
        g[0] = data[8];
        g[1] = data[9];
        g[2] = data[10];
        g[3] = data[11];
        uint32_t* ptrU33 = (uint32_t*)g;
        *pkt.crc1 = *ptrU33;

        if(*pkt.type == PTYPE_DATA && *pkt.tr == 0){

            char values[*pkt.length];
            for(int i =0; i<*pkt.length ; i++){
                values[i] = data[12+i];
            }
            *pkt.data = values;

            char h[4];
            h[0] = data[12+*pkt.length];
            h[1] = data[13+*pkt.length];
            h[2] = data[14+*pkt.length];
            h[3] = data[15+*pkt.length];
            uint32_t* ptrU34 = (uint32_t*)h;
            *pkt.crc2 = *ptrU34;
        }
        else if( *pkt.type == PTYPE_FEC){

            char values[512];
            for(int i =0; i<512 ; i++){
                values[i] = data[12+i];
            }
            *pkt.data = values;

            char h[4];
            h[0] = data[12+512];
            h[1] = data[13+512];
            h[2] = data[14+512];
            h[3] = data[15+512];
            uint32_t* ptrU34 = (uint32_t*)h;
            *pkt.crc2 = *ptrU34;

            }
        }
    }

    else{

        char e = data[1];
        *pkt.seqnum = e;

        char f[4];
        f[0] = data[2];
        f[1] = data[3];
        f[2] = data[4];
        f[3] = data[5];
        uint32_t* ptrU32 = (uint32_t*)f;
        *pkt.timestamp = *ptrU32;

        char g[4];
        g[0] = data[6];
        g[1] = data[7];
        g[2] = data[8];
        g[3] = data[9];
        uint32_t* ptrU33 = (uint32_t*)g;
        *pkt.crc1 = *ptrU33;

    }

}

/*
 * Encode une struct pkt dans un buffer, prêt à être envoyé sur le réseau
 * (c-à-d en network byte-order), incluant le CRC32 du header et
 * eventuellement le CRC32 du payload si celui-ci est non nul.
 * La fonction pkt doit calculer les champs CRC elle-même, car
 * ils ne sont pas nécessairements présents dans pkt.
 *
 * @pkt: La structure à encoder
 * @buf: Le buffer dans lequel la structure sera encodée
 * @len: La taille disponible dans le buffer
 * @len-POST: Le nombre de d'octets écrit dans le buffer
 * @return: Un code indiquant si l'opération a réussi ou E_NOMEM si
 *         le buffer est trop petit.
 */
 
pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    char a;
    if (*pkt.type == PTYPE_FEC){
        a = 0b00000000;
    }
    if (*pkt.type == PTYPE_DATA){
        a = 0b00000001;
    }
    if (*pkt.type == PTYPE_ACK){
        a = 0b00000010;
    }
    if (*pkt.type == PTYPE_NACK){
        a = 0b00000011;
    }
    a = a << 6;
    char b  ;
    if (*pkt.tr == 0 ){
        b = 0b00000000;
    }
    else {
        b = 0b00000001;
    }
    b = b << 5;
    *pkt.tr = b;

    char c = *pkt.window;
    char first = a|b|c;
    if (*pkt.type == PTYPE_DATA || *pkt.type == PTYPE_FEC){
        char f[4];
        f[0] = first;
        f[1] = *pkt.length[0];
        f[2] = *pkt.length[1];
        f[3] = *pkt.seqnum;
        uint32_t* ptrU32 = (uint32_t*)f;
        uint32_t* first_32 = *ptrU32;

    }
    else {
        char f[4];
        f[0] = first;
        f[1] = *pkt.seqnum;
        f[2] = *pkt.timestamp[1];
        f[3] = *pkt.timestamp[0];
        uint32_t* ptrU32 = (uint32_t*)f;
        uint32_t* first_32 = *ptrU32;
    }
    
}
ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    pkt->type = type;
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
    pkt->tr = tr;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    pkt->window = window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    pkt->length = length;
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
    pkt->crc1 = crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2 = crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length)
{
    pkt.data = data;
}


ssize_t predict_header_length(const pkt_t *pkt)
{
    ssize_t len = sizeof(pkt.type) + sizeof(pkt.tr) + sizeof(pkt.window) + sizeof(pkt.length) +sizeof(pkt.seqnum) +sizeof(pkt.timestamp);
    return len;
}
