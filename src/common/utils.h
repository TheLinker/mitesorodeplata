/*
 * utils.h
 *
 *  Created on: 28/06/2011
 *      Author: fviale
 */

#ifndef UTILS_H_
#define UTILS_H_

    #include <stddef.h>
    #include <stdint.h>

    /******************/
    /*    Bit Operation */
    /******************/

    //! Byte swap unsigned short
    uint16_t swap_uint16(uint16_t val);

    //! Byte swap signed short
    int16_t swap_int16(int16_t val);

    //! Byte swap unsigned int
    uint32_t swap_uint32(uint32_t val);

    //! Byte swap int
    int32_t swap_int32(int32_t val);


    /***********************************************************/
    /*    Unicode Conversion  include -licui18n -licuuc -licudata*/
    /***********************************************************/
    uint32_t unicode_strlen(uint16_t *str);

    uint16_t *unicode_utf8_to_utf16(const uint8_t *src_utf8, const int32_t src_utf8size, int32_t *dest_utf16size);

    int unicode_utf8_to_utf16_inbuffer(const uint8_t *src_utf8, const int32_t src_utf8size, uint16_t *dest_utf16, int32_t *dest_utf16size);

    uint8_t *unicode_utf16_to_utf8(const uint16_t *src_utf16, const int32_t src_utf16size, int32_t *dest_utf8size);

    int unicode_utf16_to_utf8_inbuffer(const uint16_t *src_utf16, const int32_t src_utf16size, uint8_t* dest_utf8, int32_t *dest_utf8size);

    uint8_t** string_split2( uint8_t *str, uint8_t delimitor );

    void string_split3( uint8_t *str, uint8_t delimitor, uint8_t **result );

#endif /* UTILS_H_ */
