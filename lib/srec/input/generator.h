//
//      srecord - The "srecord" program.
//      Copyright (C) 2007 Peter Miller
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 3 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program. If not, see
//      <http://www.gnu.org/licenses/>.
//

#ifndef LIB_SREC_INPUT_GENERATOR_H
#define LIB_SREC_INPUT_GENERATOR_H

#include <lib/srec/input.h>
#include <lib/interval.h>

/**
  * The srec_input_generator class is used to represent the state of
  * generating data from whole cloth.
  */
class srec_input_generator:
    public srec_input
{
public:
    /**
      * The destructor.
      */
    virtual ~srec_input_generator();

    /**
      * The constructor.
      *
      * @param range
      *     The data range over which data is to be generated.
      */
    srec_input_generator(const interval &range);

    /**
      * The create class method may be used to create new instances of
      * input data generators.
      *
      * @param cmdln
      *     The command line arguments, for deciding what to generate.
      */
    static srec_input *create(srec_arglex *cmdln);

protected:
    // See base class for documentation
    int read(srec_record &);

    // See base class for documentation
    void disable_checksum_validation();

    /**
      * The generate_data method is used to manufacture data for a
      * specific address.
      *
      * @param address
      *     The address to generate data for.
      * @returns
      *     one byte of data
      */
    virtual unsigned char generate_data(unsigned long address) = 0;

private:
    /**
      * The range instance variable is used to remember the address
      * range over which we are to generate data.  It shrinks as the
      * data is generated.
      */
    interval range;

    /**
      * The default constructor.
      */
    srec_input_generator();

    /**
      * The copy constructor.
      */
    srec_input_generator(const srec_input_generator &);

    /**
      * The assignment operator.
      */
    srec_input_generator &operator=(const srec_input_generator &);
};

// vim:ts=8:sw=4:et
#endif // LIB_SREC_INPUT_GENERATOR_H