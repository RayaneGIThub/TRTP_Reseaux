#include "packet_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <zlib.h>

/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
    ptypes_t type;
    uint8_t tr;
    uint8_t window;
    uint8_t seqnum;
    uint16_t length;
    uint32_t timestamp;
    uint32_t crc1;
    uint32_t crc2;
    char *payload;
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

    char type = data[0] & 0b11000000;
    type = type >> 6;
    ptypes_t type2;
    if (type == 0){
        pkt_set_type(pkt,PTYPE_FEC);
        type2 = PTYPE_FEC;
    }
    if (type == 1){
        pkt_set_type(pkt,PTYPE_DATA);
        type2 = PTYPE_DATA;
    }
    if (type == 2){
        pkt_set_type(pkt,PTYPE_ACK);
        type2 = PTYPE_ACK;
    }
    if (type == 3){
        pkt_set_type(pkt,PTYPE_NACK);
        type2 = PTYPE_NACK;
    }

    char tr = data[0] & 0b00100000;
    tr = tr >> 5;
    pkt_set_tr(pkt,tr);

    char window = data[0] & 0b00011111;
    pkt_set_window(pkt,window);

    if ( pkt_get_type(pkt) == PTYPE_DATA || pkt_get_type(pkt) == PTYPE_FEC){


        uint16_t length = ntohs(*(uint16_t*)(data+1));
        pkt_set_length(pkt, length);

        if (length>512){
            return E_LENGTH;
        }

        pkt_set_seqnum(pkt,(uint8_t) data[3]);

        uint32_t ptrU32 = *(uint32_t*)(data+4);
        pkt_set_timestamp(pkt, ptrU32);

        uint32_t crc1= ntohl(*(uint32_t*)(data+8));
        pkt_set_crc1(pkt, crc1);


        char buff[8];
        memcpy(buff,data,8);
        if (tr == 1){
            buff[0]  -= 0b00100000;
        }
        uint32_t crc_1 = crc32(0L,Z_NULL,0);
        crc_1 = crc32(crc_1,(unsigned char *)buff,8);

        if (crc_1 != crc1){
            return E_CRC;
        }

        if( length>0 && tr == 0){

            char values[pkt_get_length(pkt)];
            for(int i =0; i< pkt_get_length(pkt); i++){
                values[i] = data[12+i];
            }
            pkt_set_payload(pkt,values,pkt_get_length(pkt));

            
            uint32_t crc2 = ntohl(*(uint32_t*)(data+12+length));
            pkt_set_crc2(pkt, crc2);


            char buff2[length];
            memcpy(buff2,values,length);
            uint32_t crc_2 = crc32(0L,Z_NULL,0);
            crc_2 = crc32(crc_2,(unsigned char *)buff2,length);

            if (crc_2 != crc2){
                return E_CRC;
            }



            size_t real_len;

            if (type2 == PTYPE_DATA){
                real_len = length;
            }
            else{
                real_len = 512;
            }

            if (len != 16+real_len){
                return E_LENGTH;
            }

        }

        else if( pkt->type == PTYPE_FEC){

            char values[512];
            for(int i =0; i<512 ; i++){
                values[i] = data[12+i];
            }
            pkt_set_payload(pkt, values, pkt_get_length(pkt));


            uint32_t crc2 = ntohl(*(uint32_t*)(data+12+512));
            pkt_set_crc2(pkt, crc2);

            char buff2[512];
            memcpy(buff2,values,512);
            uint32_t crc_2 = crc32(0L,Z_NULL,0);
            crc_2 = crc32(crc_2,(unsigned char *)buff2,512);

            if (crc_2 != crc2){
                return E_CRC;
            }

        }
    }

    else{

        char e = data[1];
        pkt_set_seqnum(pkt,e);

        uint32_t ptrU32 = *(uint32_t*)(data+2);
        pkt_set_timestamp(pkt, ptrU32);

        uint32_t crc1 = *(uint32_t*)(data+6);
        pkt_set_crc1(pkt, crc1);

        uint32_t crc_1 = crc32(0L,Z_NULL,0);
        crc_1 = crc32(crc_1,(unsigned char *)data,6);

        if (crc_1 != crc1){
            return E_CRC;
        }
        
    }
    
    return PKT_OK;
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
    if ((size_t)predict_header_length(pkt)> *len){
        return E_NOMEM;
    }
    *len = predict_header_length(pkt);
    size_t counter = 0;
    char a;
    if (pkt->type == PTYPE_FEC){
        a = 0b00000000;
    }
    if (pkt->type == PTYPE_DATA){
        a = 0b00000001;
    }
    if (pkt->type == PTYPE_ACK){
        a = 0b00000010;
    }
    if (pkt->type == PTYPE_NACK){
        a = 0b00000011;
    }
    a = a << 6;
    char b  ;
    if (pkt->tr == 0 ){
        b = 0b00000000;
    }
    else {
        b = 0b00000001;
    }
    b = b << 5;
    char c = pkt->window;
    uint8_t first = a|b|c;
    *buf = first ;
    counter+=sizeof(first);




    uint8_t pkt_seqnum = 0;
    uint32_t pkt_timestamp = 0;
    pkt_seqnum = pkt->seqnum;
    pkt_timestamp = pkt->timestamp;

    if (pkt->type == PTYPE_DATA || pkt->type == PTYPE_FEC){
        uint16_t pkt_length = 0;
        uint32_t crc_1 = 0;
        pkt_length = pkt->length;
        *((uint16_t*)(buf+counter)) = htons(pkt_length);
        counter+= sizeof(pkt_length);
        *((uint8_t*)(buf+counter)) = pkt_seqnum;
        counter+= sizeof(pkt_seqnum);
        *((uint32_t*)(buf+counter)) = pkt_timestamp;
        counter+= sizeof(pkt_timestamp);

        char memory_crc32[counter];
        memcpy(memory_crc32,buf,counter);
        if (pkt->tr == 1){
            memory_crc32[0]  -= 0b00100000;
        }
        crc_1 = crc32(0L,Z_NULL,0);
        crc_1 = crc32(crc_1, (unsigned char *)memory_crc32,counter);
        *((uint32_t*)(buf+counter)) = htonl(crc_1);
        counter += sizeof(crc_1);

    }
    else {
        *((uint8_t*)(buf+counter)) = pkt_seqnum;
        counter+= sizeof(pkt_seqnum);
        *((uint32_t*)(buf+counter)) = pkt_timestamp;
        counter+= sizeof(pkt_timestamp);
        uint32_t crc_1_nack_ack  = 0;
        crc_1_nack_ack = crc32(0L,Z_NULL,0);
        crc_1_nack_ack= crc32(crc_1_nack_ack, (unsigned char *)buf,counter);
        *((uint32_t*)(buf+counter)) = htonl(crc_1_nack_ack);
        counter += sizeof(crc_1_nack_ack);
    }

    if (pkt->type == PTYPE_DATA && pkt->tr == 0 && pkt->length!=0){
        memcpy(buf+counter,pkt->payload,pkt->length);

        uint32_t crc_2 = 0;
        crc_2 = crc32(0L,Z_NULL,0);
        crc_2 = crc32(crc_2, (unsigned char*)(buf+counter),pkt->length);
        counter+=pkt->length;
        *((uint32_t*)(buf+counter)) = htonl(crc_2);
        counter+= sizeof(crc_2);

    }
    else if (pkt->type == PTYPE_FEC && pkt->tr == 0 && pkt->length !=0){
        
        memcpy(buf+counter,pkt->payload,512);

        uint32_t crc_2 = 0;
        crc_2 = crc32(0L,Z_NULL,0);
        crc_2 = crc32(crc_2, (unsigned char*)(buf+counter),512);
        counter+=512;
        *((uint32_t*)(buf+counter)) = htonl(crc_2);
        counter+= sizeof(crc_2);

    }
    return PKT_OK;
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
    pkt->payload = memcpy(malloc(length), data, length);
    return PKT_OK;
}


ssize_t predict_header_length(const pkt_t *pkt)
{
    if (pkt_get_type(pkt) == PTYPE_FEC){
        return 528;
    }
    else if (pkt_get_type(pkt) > 1){
        return 10;
    }
    else if (pkt_get_tr(pkt) == 0 && pkt_get_length(pkt) != 0 ){
        return 16 + pkt_get_length(pkt);

    }
    return 12;
}
