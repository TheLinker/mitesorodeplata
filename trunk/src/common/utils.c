#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include "utils.h"

//! Byte swap unsigned short
uint16_t swap_uint16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

//! Byte swap signed short
int16_t swap_int16(int16_t val) {
    return (val << 8) | ((val >> 8) & 0xFF);
}

//! Byte swap unsigned int
uint32_t swap_uint32(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

//! Byte swap int
int32_t swap_int32(int32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

uint32_t unicode_strlen(uint16_t *str)
{
    return u_strlen((UChar *) str);
}

uint16_t *unicode_utf8_to_utf16(const uint8_t *src_utf8, const int32_t src_utf8size, int32_t *dest_utf16size) {
    UErrorCode error = U_ZERO_ERROR;

    uint16_t utf16_buff[(src_utf8size + 1) * 2];
    uint16_t *utf16 = NULL;

    u_strFromUTF8(utf16_buff, (src_utf8size + 1) * 2, dest_utf16size, (const char *)src_utf8, src_utf8size, &error);

    if (error != U_ZERO_ERROR) {
        return NULL;
    }

    utf16 = malloc(*dest_utf16size);
    memcpy(utf16, utf16_buff, *dest_utf16size);

    return utf16;
}

int unicode_utf8_to_utf16_inbuffer(const uint8_t *src_utf8, const int32_t src_utf8size, uint16_t *dest_utf16, int32_t *dest_utf16size) {
    UErrorCode error = U_ZERO_ERROR;

    u_strFromUTF8(dest_utf16, (src_utf8size + 1) * 2, dest_utf16size, (const char *)src_utf8, src_utf8size, &error);

    if (error != U_ZERO_ERROR) {
        return 0;
    }

    return 1;
}

uint8_t *unicode_utf16_to_utf8(const uint16_t *src_utf16, const int32_t src_utf16size, int32_t *dest_utf8size) {
    UErrorCode error = U_ZERO_ERROR;

    uint8_t utf8_buff[src_utf16size + 1];
    uint8_t *utf8 = NULL;

    u_strToUTF8((char *)utf8_buff, src_utf16size + 1, dest_utf8size, src_utf16, src_utf16size, &error);

    if (error != U_ZERO_ERROR) {
        return NULL;
    }

    utf8 = malloc(*dest_utf8size);
    memcpy(utf8, utf8_buff, *dest_utf8size);

    return utf8;
}

int unicode_utf16_to_utf8_inbuffer(const uint16_t *src_utf16, const int32_t src_utf16size, uint8_t* dest_utf8, int32_t *dest_utf8size) {
    UErrorCode error = U_ZERO_ERROR;

    u_strToUTF8((char *)dest_utf8, src_utf16size + 1, dest_utf8size, src_utf16, src_utf16size, &error);

    if (error != U_ZERO_ERROR) {
        return 0;
    }

    return 1;
}

uint8_t** string_split2( uint8_t *str, uint8_t delimitor ){
    uint8_t **array;
    int elementsAmount = 0;

    {
        int cont;
        for(cont=0; str[cont] != '\0'; cont++ ){
            if( str[cont] == delimitor)    elementsAmount++;
        }
        elementsAmount++;
    }

    {
        array = calloc(elementsAmount+1, sizeof(uint8_t*) );
    }

    {
        int pibot, length, cont, elementIndex;
        for(cont=0, pibot=0, length=0, elementIndex=0; elementIndex != elementsAmount ; cont++ ){
            if( str[cont] == delimitor || str[cont] == '\0' ){
                array[elementIndex] = malloc(length + 1);
                if( length > 0 ){
                    strncpy((char *)array[elementIndex], (char *)&str[pibot], length);
                    array[elementIndex][length] = '\0';
                } else {
                    array[elementIndex][0] = '\0';
                }
                pibot = pibot + length + 1;
                length = 0;
                elementIndex++;
            } else {
                length++;
            }
        }
        array[elementIndex] = NULL;
    }
    return array;
}

void string_split3( uint8_t *str, uint8_t delimitor, uint8_t **result ){
    int pibot, length, cont, elementIndex = 0;

    for(cont=0, pibot=0, length=0; str[cont] != '\0' ; cont++ ){

        if( str[cont] == delimitor ){
            if( length > 0 ){
                memcpy(result[elementIndex], &str[pibot], length);
                result[elementIndex][length] = '\0';
            } else {
                result[elementIndex][0] = '\0';
            }
            pibot = pibot + length + 1;
            length = 0;
            elementIndex++;
        } else {
            length++;
        }
    }

    memcpy(result[elementIndex], &str[pibot], length);

    result[elementIndex + 1] = NULL;
}

uint8_t** string_split4( const uint8_t *str, int8_t *delimitor ){
    uint8_t *tmp_str = calloc(strlen((char *)str) + 1, 1);
    memcpy(tmp_str, str, strlen((char *)str));

    uint8_t  *pch;
    uint8_t **array;
    int elementsAmount = 1;

    array = calloc(elementsAmount, sizeof(uint8_t*) );

    pch =(uint8_t *) strtok ((char *)tmp_str, (char *)delimitor);
    while( pch ) {
printf("split4: pch: %s(%d)", pch, strlen(pch));
        array[elementsAmount] = calloc(strlen((char *)pch) + 1, 1);
        memcpy(array[elementsAmount], pch, strlen((char *)pch));
        array[elementsAmount][strlen(pch)] = '\0';
        elementsAmount++;
        void *tmp = realloc(array, elementsAmount * sizeof(uint8_t*) + 1);
        if (!tmp)
            exit(-1);
        array = tmp;
        pch =(uint8_t *) strtok (NULL, (char *)delimitor);
    }

    free(tmp_str);

    array[elementsAmount] = NULL;
    return array;
}

