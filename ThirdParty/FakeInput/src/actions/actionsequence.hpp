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

#ifndef FI_ACTIONSEQUENCE_HPP
#define FI_ACTIONSEQUENCE_HPP

#include "action.hpp"

#include "keyaction.hpp"
#include "mouseaction.hpp"
#include "commandaction.hpp"
#include "waitaction.hpp"

#include <list>

namespace FakeInput
{
    /** Represents a sequence of actions. */
    class ActionSequence : public Action
    {
        typedef std::list<Action*> ActionList;

    public:
        /** ActionSequence constructor. */
        ActionSequence();

        /** ActionSequence copy-constructor. */
        ActionSequence(const ActionSequence& anotherSequence);

        /** ActionSequence destructor. */
        virtual ~ActionSequence();

        /** Creates new copy of this action sequence.
         *
         * @returns
         *     New copy of this action sequence allocated on heap.
         */
        Action* clone() const;

        /** Adds another sequence to the end of this sequence.
         *
         * So all actions from another sequence will be performed
         * after all action before it in this sequence
         *
         * @param anotherSequence
         *     Another sequence to join
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& join(ActionSequence& anotherSequence);

        /** Adds key press action to the end of this sequence.
         *
         * @param key
         *     The key to press
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& press(Key key);

        /** Adds key press action to the end of this sequence.
         *
         * @param keyType
         *     The keyType of key to press
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& press(KeyType keyType);

        /** Adds mouse press action to the end of this sequence.
         *
         * @param button
         *     The mouse button to press
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& press(MouseButton button);

        /** Adds key release action to the end of this sequence.
         *
         * @param key
         *     The key to release
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& release(Key key);

        /** Adds key release action to the end of this sequence.
         *
         * @param keyType
         *     The keyType of key to release
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& release(KeyType keyType);

        /** Adds mouse release action to the end of this sequence.
         *
         * @param button
         *     The mouse button to release
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& release(MouseButton button);

        /** Adds mouse relative motion action to the end of this sequence.
         *
         * @param dx
         *     Amount of pixels to move on X axis
         * @param dy
         *     Amount of pixels to move on Y axis
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& moveMouse(int dx, int dy);

        /** Adds mouse absolute motion action to the end of this sequence.
         *
         * @param x
         *     Where to move on X axis
         * @param y
         *     Where to move on Y axis
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& moveMouseTo(int x, int y);

        /** Adds mouse wheel up action to the end of this sequence.
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& wheelUp();

        /** Adds mouse wheel down action to the end of this sequence.
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& wheelDown();

        /** Adds command run action to the end of this sequence.
         *
         * @param cmd
         *     Command to run
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& runCommand(const std::string& cmd);

        /** Adds wait action to the end of this sequence.
         *
         * @param milisec
         *     Time to wait in milisecods
         *
         * @returns
         *     The referece to this action
         */
        ActionSequence& wait(unsigned int milisec);
        
        /** Performs all actions in sequence. */
        void send() const;

    private:
        /** List of actions in this sequence. */
        ActionList actions_;
    };
}

#endif
