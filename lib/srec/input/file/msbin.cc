//
// srecord - manipulate eprom load files
// Copyright (C) 2009 David Kozub <zub@linux.fjfi.cvut.cz>
// Copyright (C) 2009 Peter Miller
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//

#include <lib/srec/input/file/msbin.h>
#include <limits>
#include <algorithm>
#include <string.h>

srec_input_file_msbin::~srec_input_file_msbin()
{
    if (first_record_read)
    {
        // Check if data from file header were correct
        if (lowest_address != image_start)
        {
            warning
            (
                "image address header field is wrong (header = 0x%08lx, "
                    "actual = 0x%08lx)",
                (unsigned long)image_start,
                (unsigned long)lowest_address);
        }
        if (highest_address - lowest_address + 1 != image_length)
        {
            warning
            (
                "image length header field is wrong (header = 0x%08lx, "
                    "actual = 0x%08lx)",
                (unsigned long)image_length,
                (unsigned long)(highest_address - lowest_address + 1)
            );
        }
    }
}


srec_input_file_msbin::srec_input_file_msbin(const std::string &a_file_name) :
    srec_input_file(a_file_name),
    header_read(false),
    first_record_read(false),
    execution_start_record_seen(false),
    last_record_warning(false),
    address(0),
    remaining(0),
    record_checksum(0),
    running_checksum(0)
{
}


srec_input::pointer
srec_input_file_msbin::create(const std::string &a_file_name)
{
    return pointer(new srec_input_file_msbin(a_file_name));
}


uint32_t srec_input_file_msbin::read_qword_le()
{
    unsigned char c[sizeof(uint32_t)];

    for (size_t i = 0; i < sizeof(c); ++i)
    {
        int j = get_char();
        if (j < 0)
            fatal_error("short input file");

        assert(j <= std::numeric_limits<unsigned char>::max());
        c[i] = (unsigned char)j;
    }

    return srec_record::decode_little_endian(c, sizeof(c));
}


void srec_input_file_msbin::read_file_header()
{
    // Optional magic
    static const unsigned char Magic[7] =
        { 'B', '0', '0', '0', 'F', 'F', '\n' };

    // +1 so that buff can be reused for two qwords in case the magic is missing
    unsigned char buff[sizeof(Magic) + 1];
    for (size_t i = 0; i < sizeof(Magic); ++i)
    {
        int j = get_char();
        if (j < 0)
            fatal_error("short input file");

        assert(j <= std::numeric_limits<unsigned char>::max());
        buff[i] = j;
    }

    BOOST_STATIC_ASSERT(sizeof(buff) >= sizeof(Magic));
    if (memcmp(Magic, buff, sizeof(Magic)))
    {
        // Ok, there's no magic in the header. But it's optional anyway.

        // Fill up to two qwords
        BOOST_STATIC_ASSERT(sizeof(buff) == 2 * sizeof(uint32_t));
        int j = get_char();
        if (j < 0)
            fatal_error("short input file");
        buff[sizeof(buff) - 1] = j;

        // Read first qword
        image_start = srec_record::decode_little_endian(buff, sizeof(uint32_t));

        // Read second qword
        image_length =
            srec_record::decode_little_endian
            (
                buff + sizeof(uint32_t),
                sizeof(uint32_t)
            );
    }
    else
    {
        image_start = read_qword_le();
        image_length = read_qword_le();
    }

    // What shall we do with the (useless) file header?
    // Throw it away?
    //warning("image_start = %08x", (unsigned int)image_start);
    //warning("image_length = %08x", (unsigned int)image_length);
}


uint32_t
srec_input_file_msbin::checksum(const unsigned char *data, size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; ++i)
        sum += data[i];

    return sum;
}


bool
srec_input_file_msbin::read(srec_record &record)
{
    // Read the file header if we haven't read it yet.
    if (!header_read)
    {
        read_file_header();
        header_read = true;
    }

    if (remaining == 0)
    {
        // No remaining data from any previous record. => Try to read
        // next record header, if present.

        if (peek_char() < 0)
        {
            // Check if we have seen the execution start address record.
            if (!execution_start_record_seen)
                warning("input file is missing the execution start record");

            return false; // end of file
        }

        if (execution_start_record_seen && !last_record_warning)
        {
            warning
            (
                "the execution start record is not the last record; "
                "reading further records"
            );
            last_record_warning = true;
        }

        // Read header of the next record
        address = read_qword_le();
        remaining = read_qword_le();
        record_checksum = read_qword_le();

        // Reset running checksum
        running_checksum = 0;

        // Bookkeeping for tracking the address range - but ignore
        // the execution start address record, as it has a special
        // format!
        if (address != 0)
        {
            if (!first_record_read)
            {
                lowest_address = address;
                highest_address = address + remaining - 1;
                first_record_read = true;
            }
            else
            {
                lowest_address = std::min(lowest_address, address);
                highest_address =
                    std::max(highest_address, address + remaining - 1);
            }
        }
    }

    if (address == 0)
    {
        // This is a special record specifying the execution start address.
        if (record_checksum != 0 && use_checksums())
        {
            fatal_error
            (
                "checksum of the execution start record is not 0, as "
                    "required by specification (0x%02X != 0x00)",
                record_checksum
            );
        }

        record =
            srec_record
            (
                srec_record::type_execution_start_address,
                remaining,
                0,
                0
            );

        // This should be the last record - but if it was not, we try to read
        // further and produce a warning.
        remaining = 0;
        execution_start_record_seen = true;
        return true;
    }

    // Data record
    // Read (part) of the record
    unsigned char data[srec_record::max_data_length];
    const size_t to_read =
        std::min(remaining, (uint32_t)srec_record::max_data_length);

    int c = get_char();
    if (c < 0)
    {
        fatal_error("short input file");
        return false;
    }

    size_t read = 0;
    while (read < to_read)
    {
        assert(c <= std::numeric_limits<unsigned char>::max());
        data[read++] = c;
        if (read >= to_read)
            break;
        c = get_char();
        if (c < 0)
        {
            fatal_error("short input file");
            return false;
        }
    }

    record = srec_record(srec_record::type_data, address, data, read);
    address += read;
    assert(remaining >= read);
    remaining -= read;
    running_checksum += checksum(data, read);

    if (remaining == 0)
    {
        // All data in a record was read. => We can verify the checksum
        // now.
        if (running_checksum != record_checksum && use_checksums())
        {
            fatal_error
            (
                "wrong record checksum (0x%08X != 0x%08X)",
                running_checksum,
                record_checksum
            );
        }
    }

    return true;
}


const char *
srec_input_file_msbin::mode()
    const
{
    return "rb";
}


const char *
srec_input_file_msbin::get_file_format_name()
    const
{
    return "Windows CE Binary Image Data Format";
}