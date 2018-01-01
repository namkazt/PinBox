/**
 * This file is part of the FakeInput library (https://github.com/uiii/FakeInput)
 *
 * Copyright (C) 2011 by Richard Jedlicka <uiii.dev@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef FI_WAITACTION_HPP
#define FI_WAITACTION_HPP

#include "action.hpp"

namespace FakeInput
{
    /** Represents a wait action.
     *
     * Waits specified amount of time in miliseconds.
     */
    class Wait : public Action
    {
    public:
        /** Wait action constructor.
         *
         * @param milisec
         *     Amount of time to wait in miliseconds.
         */
        Wait(unsigned int milisec);

        /** Creates new copy of this action.
         *
         * @returns
         *     New copy of this action allocated on heap.
         */
        Action* clone() const;

        /** Performs this action. */
        void send() const;

    private:
        unsigned int milisec_;
    };
}

#endif
