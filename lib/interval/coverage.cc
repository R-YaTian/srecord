//
//      srecord - Manipulate EPROM load files
//      Copyright (C) 2008 Peter Miller
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

#include <lib/interval.h>


long long
interval::coverage()
    const
{
    long long total = 0;
    for (size_t j = 0; j < length; j+= 2)
    {
        long long lo = data[j];
        long long hi = (data[j + 1] == 0 ? (1LL << 32) : data[j + 1]);
        total += (hi - lo);
    }
    return total;
}