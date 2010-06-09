/* $Id$
 *
 * EHS is a library for embedding HTTP(S) support into a C++ application
 *
 * Copyright (C) 2004 Zachary J. Hansen
 *
 * Code cleanup, new features and bugfixes: Copyright (C) 2010 Fritz Elfert
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License version 2.1 as published by the Free Software Foundation;
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    This can be found in the 'COPYING' file.
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <termios.h>
#include <unistd.h>
#include <string>

// A small helper class for providing
// non-blocking keyboard input.
class kbdio {
    public:
        kbdio() : st(termios()), stsave(termios()) {
            tcgetattr(0, &stsave);
            memcpy(&st, &stsave, sizeof(st));
            st.c_lflag &= ~ICANON;
            st.c_cc[VMIN] = st.c_cc[VTIME] = 0;
            tcsetattr(0, TCSANOW, &st);
        }
        ~kbdio() {
            tcsetattr(0, TCSANOW, &stsave);
        }
        bool qpressed() {
            char c;
            bool ret = false;
            while (1 == read(0, &c, 1)) {
                ret |= ('q' == c);
            }
            return ret;
        }
    private:
        struct termios st;
        struct termios stsave;
};

std::string basename(const std::string & s)
{
    std::string ret(s);
    size_t pos = ret.rfind("/");
    if (pos != ret.npos)
        ret.erase(0, pos);
    return ret;
}

#endif
